// =============================================================================
// deadcode-api implementation — marker stubs.
//
// Every function here is intentionally minimal.  In an UNPROCESSED build (the
// SDK linked, deadcode-cli never run) the markers are no-ops and
// DCApiCheckIntegrity always reports "intact" (1), so the program behaves
// exactly as written — convenient for day-to-day development and debugging.
// At obfuscation time deadcode-cli locates these functions through the
// binary's PDB and transforms the marked regions; the marker calls themselves
// do not survive into the protected binary.
//
// Two implementation choices below exist solely to keep each marker a real,
// individually addressable symbol all the way through the optimizer and
// linker.  Both are about toolchain behavior, not about what the engine does:
//
//   * DCAPI_NOINLINE — a marker must never be inlined into its caller.  The
//     SDK's contract is that each *call site* is discoverable post-build; if
//     the compiler inlined the body, there would be no call to find.
//
//   * Distinct function bodies — each marker writes a different constant to a
//     volatile sink.  If the bodies were byte-identical, MSVC's identical-
//     COMDAT folding (/OPT:ICF, on by default for Release) would legitimately
//     merge them into one function at a single address.  The seven markers
//     would then be indistinguishable from each other, and every marked region
//     would resolve to the same kind of protection.  A per-function constant
//     gives each marker a unique body — hence a unique address — and the
//     `volatile` write keeps the optimizer from discarding the body as empty.
//
// The constants are arbitrary; the only requirement is that no two are equal.
// =============================================================================
#include "deadcode-api.h"

#include <cstdint>

#if defined(_MSC_VER)
// Also matches clang-cl, which defines _MSC_VER and accepts __declspec.
#  define DCAPI_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#  define DCAPI_NOINLINE __attribute__((noinline))
#else
#  define DCAPI_NOINLINE
#endif

namespace {

// One distinct constant per marker.  See the file header for why they must
// differ (anti-fold) and why they are written through `volatile` (anti-elide).
constexpr std::uint64_t kTagBeginVM        = 0xDCAB000100000101ULL;
constexpr std::uint64_t kTagBeginMutation  = 0xDCAB000100000102ULL;
constexpr std::uint64_t kTagBeginUltra     = 0xDCAB000100000103ULL;
constexpr std::uint64_t kTagEnd            = 0xDCAB000100000104ULL;

constexpr std::uint64_t kTagBeginCheck     = 0xDCAB000200000201ULL;
constexpr std::uint64_t kTagEndCheck       = 0xDCAB000200000202ULL;
constexpr std::uint64_t kTagCheckIntegrity = 0xDCAB000200000203ULL;

inline void emit_tag(std::uint64_t tag) {
    // `volatile` forces the store to be emitted, so the body is never folded
    // away to nothing and never collapsed into another marker's body.
    volatile std::uint64_t sink = tag;
    (void)sink;
}

}   // anonymous namespace

extern "C" {

DCAPI_NOINLINE void DCApiBeginVirtualization(const char* name) {
    emit_tag(kTagBeginVM);
    (void)name;
}

DCAPI_NOINLINE void DCApiBeginMutation(const char* name) {
    emit_tag(kTagBeginMutation);
    (void)name;
}

DCAPI_NOINLINE void DCApiBeginUltra(const char* name) {
    emit_tag(kTagBeginUltra);
    (void)name;
}

DCAPI_NOINLINE void DCApiEnd(void) {
    emit_tag(kTagEnd);
}

DCAPI_NOINLINE void DCApiBeginCheck(int check_id) {
    emit_tag(kTagBeginCheck);
    (void)check_id;
}

DCAPI_NOINLINE void DCApiEndCheck(int check_id) {
    emit_tag(kTagEndCheck);
    (void)check_id;
}

// In an unprocessed build the integrity check ALWAYS reports "intact" so that
// unobfuscated builds run normally.  deadcode-cli replaces this body so the
// protected binary recomputes the CRC32 of the marked range at runtime and
// compares it against the value captured at obfuscation time.
DCAPI_NOINLINE int DCApiCheckIntegrity(int check_id) {
    emit_tag(kTagCheckIntegrity);
    (void)check_id;
    return 1;
}

}   // extern "C"
