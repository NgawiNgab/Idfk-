// Tiny host-only library. GNU ld adds a controlled note for the probe tests.
extern "C" int ore_outline_build_id_fixture() {
    return 0;
}

extern "C" {
using FixtureFunction = int (*)();
extern FixtureFunction const ore_outline_build_id_fixture_slot = &ore_outline_build_id_fixture;
}
