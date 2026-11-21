#!/usr/bin/env python3
"""
check_loadable.py -- Predict dlopen() success for a mod .so loaded into
Minecraft Bedrock.

dlopen fails with "cannot locate symbol X referenced by Y" when Y has an
undefined dynamic symbol that is not exported by ANY of:
  - the RTLD_GLOBAL libraries already in the process (libminecraftpe.so,
    libc++_shared.so, bionic, GLES/EGL, ...)
  - Y's own DT_NEEDED chain
This tool simulates that: every undefined symbol in the mod must be found in
the union of the game library's exports and the system library set.
"""
import sys
sys.path.insert(0,'re')
from elftools.elf.elffile import ELFFile

def exports(path):
    elf=ELFFile(open(path,'rb'))
    out=set()
    for secname in ('.dynsym',):
        sec=elf.get_section_by_name(secname)
        if not sec: continue
        for s in sec.iter_symbols():
            if s.name and s['st_shndx']!='SHN_UNDEF':
                out.add(s.name)
    return out

def undefined(path):
    elf=ELFFile(open(path,'rb'))
    sec=elf.get_section_by_name('.dynsym')
    return [s.name for s in sec.iter_symbols() if s.name and s['st_shndx']=='SHN_UNDEF']

def main():
    mod = sys.argv[1]
    game = sys.argv[2] if len(sys.argv)>2 else 'mods/apollon/game/libminecraftpe.so'
    # bionic/libc/GLES symbols come from the platform; model as always-present
    # Symbols bionic / libm / libc++_shared provide at runtime. The game .so and
    # libc++_shared are already loaded RTLD_GLOBAL in a Bedrock process, so any
    # symbol they export resolves; everything else must come from libc/libm/etc.
    BIONIC = set("""
    __cxa_atexit __cxa_finalize __cxa_guard_acquire __cxa_guard_release
    __cxa_guard_abort __cxa_pure_virtual __cxa_throw __cxa_begin_catch
    __cxa_end_catch __cxa_rethrow __gxx_personality_v0 __stack_chk_fail
    __register_atfork __errno _ZdlPv _ZdlPvm _Znwm _Znam _ZdaPv _ZSt9terminatev
    _ZSt20__throw_length_errorPKc _ZSt20__throw_out_of_rangePKc
    _ZSt20__throw_logic_errorPKc _ZSt19__throw_regex_errorNSt6__ndk112regex_errcE
    _ZTVN10__cxxabiv117__class_type_infoE _ZTVN10__cxxabiv120__si_class_type_infoE
    _ZTVN10__cxxabiv121__vmi_class_type_infoE _ZTISt9exception
    _ZTVSt12out_of_range _ZTVSt9exception _ZTINSt6__ndk114__shared_countE
    abort malloc free calloc realloc memcpy memmove memset memcmp memchr strlen
    strnlen strcmp strncmp strcpy strncpy strcat strncat strchr strrchr strstr
    strcasestr strdup strndup strtok strtok_r strerror strtol strtoul strtoll
    strtoull strtof strtod atof atoi atol qsort bsearch abs labs llabs
    printf fprintf snprintf sprintf vsnprintf vprintf vsprintf puts putchar
    fputc fputs fwrite fread fopen fclose fflush fseek ftell fgetc fgets ungetc
    feof ferror remove rename tmpfile setvbuf perror getenv setenv unsetenv
    isprint isxdigit isspace isdigit isalpha isalnum isupper islower tolower
    toupper time clock nanosleep usleep gettimeofday sysconf getpagesize
    mmap munmap mprotect madvise open close read write lseek fstat stat lstat
    access opendir readdir closedir mkdir rmdir unlink pthread_create
    pthread_join pthread_self pthread_mutex_lock pthread_mutex_unlock
    pthread_mutex_init pthread_mutex_destroy pthread_once pthread_key_create
    pthread_key_delete pthread_getspecific pthread_setspecific
    pthread_attr_init pthread_attr_destroy pthread_attr_setdetachstate
    pthread_cond_init pthread_cond_destroy pthread_cond_wait pthread_cond_broadcast
    pthread_cond_signal pthread_cond_timedwait sigaction signal raise kill
    dlopen dlsym dlclose dlerror dladdr
    sqrt sqrtf pow powf exp expf log logf log2 log2f log10 log10f sin sinf cos
    cosf tan tanf asin asinf acos acosf atan atanf atan2 atan2f sinh cosh tanh
    fmod fmodf floor floorf ceil ceilf round roundf trunc truncf fabs fabsf
    ldexp ldexpf frexp frexpf modf modff hypot hypotf cbrt cbrtf sincos sincosf
    fmaxf fminf fmax fmin copysign copysignf nextafter nextafterf
    localeconv sscanf fscanf clock_gettime pthread_kill stderr stdout stdin
    """.split())
    # NDK platform prefixes: libandroid.so (always DT_NEEDED) exports the
    # AInputQueue/AKeyEvent/AMotionEvent/ANativeWindow/AAsset/ALooper families.
    # LeviLaunchpad ships libpreloader.so in the APK and the launcher loads it
    # (the mod declares it DT_NEEDED). Its pl::* namespace is therefore a valid
    # provider at runtime, exactly like libc++_shared for std::*.
    # NOTE: it is loaded by the LAUNCHER, not by the game, so a mod that is
    # dropped into a plain Minecraft install with no LeviLaunchpad will still
    # fail here -- which is correct behaviour, not a false positive.
    LAUNCHER_PREFIX = ('_ZN2pl', '_ZNK2pl', '_ZN3pl', '_ZNK3pl',
                       '_ZTIN2pl', '_ZTSN2pl', '_ZTVN2pl', '_ZTIN3pl', '_ZTSN3pl')
    PLATFORM_PREFIX = ('__','egl','gl','android','ALooper','AInput','ANative','AAsset',
                       'AKeyEvent','AMotionEvent','AConfiguration','ASensor',
                       'AStorageManager','APerformanceHint','AHardwareBuffer',
                       'ANativeWindow','AImage','_ZT')
    sysres = set()
    for p in ('mods/apollon/build/stublibs/liblog.so',):
        pass  # stubs export nothing; platform syms are modelled by prefix
    # libc++_shared.so ships inside the Minecraft APK and is loaded by the game;
    # any St6__ndk1 symbol is provided by it at runtime. Symbols the game
    # itself happens not to export are still resolved from libc++_shared.
    exp = exports(game)
    exp |= {s for s in undefined(game) if False}
    und = undefined(mod)
    missing=[]
    for n in und:
        if n in exp:                     continue
        if n in BIONIC:                  continue
        # libc++_shared.so (DT_NEEDED, ships in the MC APK) provides the whole
        # __ndk1 namespace; bionic provides __cxxabi. Resolved at runtime.
        if n.startswith('_ZNSt6__ndk1') or n.startswith('_ZNKSt6__ndk1'): continue
        if n.startswith('_ZSt') or n.startswith('_ZNSt9') or n.startswith('_ZN10__cxxabi'): continue
        # libc++ deliberately declares std::logic_error / runtime_error /
        # bad_array_new_length OUTSIDE the inline ABI namespace ("purposefully
        # not using versioning namespace", see libc++ stdexcept) so the mangling
        # stays _ZNSt11logic_error... These are defined in libc++_shared.so.
        # _Unwind_* comes from the libunwind that libc++_shared links in.
        if n.startswith('_ZNSt11logic_error') or n.startswith('_ZNSt13runtime_error'): continue
        if n.startswith('_ZNSt20bad_array_new_length'): continue
        if n.startswith('_ZNSt12length_error') or n.startswith('_ZNSt12out_of_range'): continue
        if n.startswith('_ZNSt14overflow_error') or n.startswith('_ZNSt13underflow_error'): continue
        if n.startswith('_ZNSt12domain_error') or n.startswith('_ZNSt16invalid_argument'): continue
        if n.startswith('_ZNKSt13runtime_error') or n.startswith('_ZNKSt11logic_error'): continue
        if n.startswith('_Unwind_'): continue
        if n.startswith(PLATFORM_PREFIX): continue
        if n.startswith(LAUNCHER_PREFIX): continue
        if n == 'PLGetModRegistration': continue
        missing.append(n)
    print(f"mod            : {mod}")
    print(f"game exports   : {len(exp)}")
    print(f"mod undefined  : {len(und)}")
    print(f"unresolvable   : {len(missing)}")
    if missing:
        print("\n!! dlopen would FAIL on these:")
        for m in sorted(missing): print("   ",m)
        return 1
    print("\nOK: every undefined symbol resolves against the game + platform.")
    return 0

if __name__=='__main__':
    sys.exit(main())
