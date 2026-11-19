#include "ore_outline/block_esp.hpp"

#include <algorithm>
#include <cmath>

namespace ore_outline::esp {
namespace {

std::string_view trim(std::string_view s) noexcept {
    constexpr std::string_view whitespace = " \t\r\n";
    const auto first = s.find_first_not_of(whitespace);
    if (first == s.npos) return {};
    return s.substr(first, s.find_last_not_of(whitespace) + 1 - first);
}

// Pop one token, preserving empty interior/trailing tokens for validation.
bool token(std::string_view& s, char delimiter, std::string_view& value) noexcept {
    const auto next = s.find(delimiter);
    value = trim(s.substr(0, next));
    if (next == s.npos) return false;
    s.remove_prefix(next + 1);
    return true;
}

using Clip = std::array<double, 4>;
constexpr double kNearW = 1e-6;
constexpr std::size_t kMaxFaceVertices = 4 + 7;
constexpr std::size_t kMaxHullInputs = 6 * kMaxFaceVertices;
constexpr std::array<std::array<unsigned, 4>, 6> kFaces{{
    {0, 1, 3, 2}, {4, 5, 7, 6}, {0, 1, 5, 4}, {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 3, 7, 5},
}};

Clip transform(const CameraFrame& camera, double x, double y, double z) noexcept {
    Clip p{};
    for (std::size_t i = 0; i < p.size(); ++i) {
        const auto* row = &camera.world_to_clip[i * 4];
        p[i] = row[0] * x + row[1] * y + row[2] * z + row[3];
    }
    return p;
}

double distance(const Clip& p, std::size_t plane) noexcept {
    if (plane == 0) return p[3] - kNearW;
    const auto coord = (plane - 1) / 2;
    return p[3] + ((plane & 1) != 0 ? p[coord] : -p[coord]);
}

bool finite(const auto& values) noexcept {
    return std::all_of(values.begin(), values.end(), [](auto v) { return std::isfinite(v); });
}

bool drawable(const Viewport& v) noexcept {
    return finite(std::array{v.left, v.top, v.width, v.height}) && v.left >= 0 && v.top >= 0 &&
        v.width > 0 && v.height > 0 && v.left + v.width <= 32'768 && v.top + v.height <= 32'768;
}

Point screen(const Clip& p, const Viewport& v) noexcept {
    return {v.left + v.width * (1 + std::clamp(p[0] / p[3], -1.0, 1.0)) * 0.5,
            v.top + v.height * (1 - std::clamp(p[1] / p[3], -1.0, 1.0)) * 0.5};
}

std::span<const Clip> clip_face(const std::array<Clip, 8>& corners,
        const std::array<unsigned, 4>& face, std::array<Clip, kMaxFaceVertices>& in) noexcept {
    auto count = face.size();
    for (std::size_t i = 0; i < count; ++i) in[i] = corners[face[i]];
    std::array<Clip, kMaxFaceVertices> out{};
    for (std::size_t plane = 0; plane < 7 && count; ++plane) {
        std::size_t n = 0;
        auto a = in[count - 1];
        auto da = distance(a, plane);
        for (std::size_t i = 0; i < count; ++i) {
            const auto& b = in[i];
            const auto db = distance(b, plane);
            if (!std::isfinite(da) || !std::isfinite(db)) return {};
            if ((da < 0 && db > 0) || (da > 0 && db < 0)) {
                const auto delta = da - db;
                if (!std::isfinite(delta) || n == out.size()) return {};
                auto& intersection = out[n++];
                for (std::size_t j = 0; j < a.size(); ++j) intersection[j] = std::lerp(a[j], b[j], da / delta);
            }
            if (db >= 0) {
                if (n == out.size()) return {};
                out[n++] = b;
            }
            a = b;
            da = db;
        }
        count = n;
        std::copy_n(out.begin(), count, in.begin());
    }
    return {in.data(), count};
}

double cross(Point a, Point b, Point c) noexcept {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

void fill_silhouette(const CameraFrame& camera, BlockTarget target, std::vector<Triangle>& fills) {
    std::array<Clip, 8> corners{};
    for (std::size_t i = 0; i < corners.size(); ++i) {
        corners[i] = transform(camera, target.position.x + static_cast<double>(i & 1),
                        target.position.y + static_cast<double>((i >> 1) & 1),
                        target.position.z + static_cast<double>((i >> 2) & 1));
        if (!finite(corners[i])) return;
    }
    std::array<Point, kMaxHullInputs> points{};
    std::size_t count = 0;
    for (const auto& face : kFaces) {
        std::array<Clip, kMaxFaceVertices> clipped{};
        for (const auto& p : clip_face(corners, face, clipped)) {
            if (p[3] > 0) points[count++] = screen(p, camera.viewport);
        }
    }
    auto active = std::span(points).first(count);
    std::sort(active.begin(), active.end(), [](Point a, Point b) { return a.x == b.x ? a.y < b.y : a.x < b.x; });
    const auto end = std::unique(active.begin(), active.end());
    count = static_cast<std::size_t>(end - active.begin());
    if (count < 3) return;

    // Andrew monotone chain: one fill per pixel, without double-drawing the cube faces.
    std::array<Point, 2 * kMaxHullInputs> hull{};
    std::size_t n = 0;
    const auto append = [&](Point point, std::size_t floor) {
        while (n >= floor + 2 && cross(hull[n - 2], hull[n - 1], point) <= 1e-8) --n;
        hull[n++] = point;
    };
    for (std::size_t i = 0; i < count; ++i) append(points[i], 0);
    const auto floor = n - 1;
    for (std::size_t i = count - 1; i > 0; --i) append(points[i - 1], floor);
    // n includes the repeated starting point. Drop, rather than truncate, an
    // unexpectedly complex silhouette. A nondegenerate cube clipped by seven
    // planes has at most 22 vertices (at most 13 faces).
    if (n < 4 || n - 3 > kMaxTrianglesPerTarget) return;
    for (std::size_t i = 2; i + 1 < n; ++i) {
        fills.push_back({{hull[0], hull[i - 1], hull[i]}, target.color});
    }
}

} // namespace

bool valid_identifier(std::string_view id) noexcept {
    const auto sep = id.find(':');
    if (id.size() > kMaxIdBytes || sep == id.npos || sep == 0 || sep == id.size() - 1) return false;
    const auto basic = [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-'; };
    return std::all_of(id.begin(), id.begin() + static_cast<std::ptrdiff_t>(sep), basic) &&
        std::all_of(id.begin() + static_cast<std::ptrdiff_t>(sep + 1), id.end(),
                    [&](char c) { return basic(c) || c == '/'; });
}

std::optional<std::size_t> IdentifierList::match(std::string_view id) const noexcept {
    for (std::size_t i = 0; i < std::min(size, names.size()); ++i) {
        if (names[i] == id) return i;
    }
    return {};
}

std::optional<IdentifierList> parse_identifiers(std::string_view text) {
    if (text.size() > kMaxIds * (kMaxIdBytes + 4)) return {};
    IdentifierList list{};
    if (trim(text).empty()) return list;
    bool more;
    do {
        std::string_view id;
        more = token(text, ',', id);
        if (!valid_identifier(id)) return {};
        if (list.match(id).has_value()) continue;
        if (list.size == list.names.size()) return {};
        list.names[list.size++] = id;
    } while (more);
    return list;
}

Color block_color(std::string_view identifier, std::size_t filter_index) noexcept {
    const auto category = classify(identifier).category;
    if (category == Category::AncientDebris) return {255, 172, 77, 255};
    if (category && category != Category::Coal) return color_for(*category);
    constexpr std::array colors{
        Color{244, 132, 252, 255}, Color{251, 185, 66, 255}, Color{113, 251, 162, 255}, Color{100, 182, 252, 255},
        Color{216, 106, 116, 255}, Color{160, 139, 255, 255}, Color{255, 135, 62, 255}, Color{233, 245, 141, 255},
    };
    return colors[filter_index % colors.size()];
}

Batch project(WorldKey world, std::span<const BlockTarget> targets,
              const CameraFrame& frame, DrawOptions options) {
    Batch result;
    if (!world.epoch || world != frame.world || !drawable(frame.viewport) || !finite(frame.world_to_clip) ||
        (!options.lines && !options.chams)) return result;
    targets = targets.first(std::min(targets.size(), kMaxTargets));
    for (const auto target : targets) {
        if (options.chams) fill_silhouette(frame, target, result.fills);
        if (!options.lines) continue;
        const auto c = transform(frame, target.position.x + 0.5, target.position.y + 0.5, target.position.z + 0.5);
        if (!finite(c)) continue;
        bool inside = true;
        for (std::size_t plane = 0; plane < 7; ++plane) inside = inside && distance(c, plane) >= 0;
        if (!inside) continue;
        const auto p = screen(c, frame.viewport);
        const auto& v = frame.viewport;
        result.lines.push_back({v.left + v.width * 0.5, v.top + v.height * 0.59, p.x, p.y, target.color});
    }
    return result;
}

std::optional<CameraFrame> make_fullscreen_frame(WorldKey world, const RelativeCamera& c,
        double width, double height) noexcept {
    const Viewport viewport{0, 0, width, height};
    if (!world.epoch || !drawable(viewport) || !finite(c.origin) || !finite(c.view) || !finite(c.projection)) return {};
    const auto& v = c.view;
    const auto& p = c.projection;
    const auto near = [](double a, double b) { return std::abs(a - b) < 0.005; };
    if (p[0] < 0.05f || p[0] > 16 || p[5] < 0.05f || p[5] > 16 || !near(p[11], -1) || !near(p[15], 0) ||
        p[14] >= 0 || p[10] > -0.99f || std::abs((static_cast<double>(p[5]) / p[0]) * height / width - 1) > 0.03 ||
        !near(v[3], 0) || !near(v[7], 0) || !near(v[11], 0) || !near(v[15], 1)) return {};
    for (std::size_t i : {1u, 2u, 3u, 4u, 6u, 7u, 12u, 13u}) if (!near(p[i], 0)) return {};
    for (std::size_t i = 0; i < 3; ++i) {
        if (std::abs(c.origin[i]) > 30'000'000.0f || std::abs(v[12 + i]) > 16) return {};
        for (std::size_t j = 0; j <= i; ++j) {
            double dot = 0;
            for (std::size_t k = 0; k < 3; ++k) dot += static_cast<double>(v[i * 4 + k]) * v[j * 4 + k];
            if (std::abs(dot - (i == j ? 1.0 : 0.0)) > 0.03) return {};
        }
    }
    CameraFrame frame{world, {}, viewport};
    auto& m = frame.world_to_clip;
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            for (std::size_t k = 0; k < 4; ++k) m[row * 4 + col] += static_cast<double>(p[k * 4 + row]) * v[col * 4 + k];
        }
        for (std::size_t col = 0; col < 3; ++col) m[row * 4 + 3] -= m[row * 4 + col] * c.origin[col];
    }
    return frame;
}

} // namespace ore_outline::esp
