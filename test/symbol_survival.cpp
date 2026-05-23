// =============================================================================
// symbol_survival — the SDK's most important test.
//
// The whole SDK is worthless if the toolchain removes the marker functions
// before the engine can see them: if the compiler inlines a marker, the call
// site vanishes; if the linker folds identical bodies (/OPT:ICF) the markers
// stop being distinguishable; if /OPT:REF dead-strips an unreferenced marker
// it never reaches the PDB.  This program references every marker, and the
// CTest driver (cmake/check_symbols.cmake) then asserts all seven symbols are
// present in the built library and the linked image.
//
// It is built in BOTH Debug and Release (/O2) with /Zi + /DEBUG.  Release is
// the case that matters in practice and the one most likely to expose a
// missing anti-inline / anti-fold guard, which is why CI runs both.
//
// Expected: prints one line and exits 0.  After running deadcode-cli, the
// marker calls are gone from the image and the marked ranges are transformed,
// but the printed value is unchanged (the program's observable behavior is
// preserved).
// =============================================================================
#include <cstdio>
#include <cstdint>
#include <deadcode-api.h>

static volatile std::uint32_t g_sink = 0;

static int virtualized(int a, int b) {
    DCApiBeginVirtualization("survival.vm");
    int x = a * 7 + b;
    DCApiEnd();
    return x;
}

static int mutated(int a, int b) {
    DCApiBeginMutation("survival.mut");
    int x = a ^ (b << 1);
    DCApiEnd();
    return x;
}

static int ultra(int a, int b) {
    DCApiBeginUltra("survival.ultra");
    int x = (a + b) * 3;
    DCApiEnd();
    return x;
}

static int checked() {
    DCApiBeginCheck(1);
    int magic = 0xC0DE;
    DCApiEndCheck(1);
    // Returns magic in an unprocessed build; in a protected build the engine's
    // recomputed CRC gates this.
    return DCApiCheckIntegrity(1) ? magic : 0;
}

int main() {
    g_sink ^= static_cast<std::uint32_t>(virtualized(3, 5));
    g_sink ^= static_cast<std::uint32_t>(mutated(7, 11));
    g_sink ^= static_cast<std::uint32_t>(ultra(2, 13));
    g_sink ^= static_cast<std::uint32_t>(checked());
    std::printf("symbol_survival sink=%X\n", static_cast<unsigned>(g_sink));
    return 0;
}
