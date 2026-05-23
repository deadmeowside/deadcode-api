// =============================================================================
// Example 03 — self-integrity check at startup.
//
// Wraps a critical function in an integrity-check zone, then verifies that zone
// at program startup and refuses to run if it has been altered.
//
// How the pieces fit:
//   * DCApiBeginCheck(7) / DCApiEndCheck(7) bracket the bytes that must remain
//     exactly as shipped. The engine records a CRC32 of that range at
//     obfuscation time.
//   * DCApiCheckIntegrity(7) recomputes the CRC at runtime and returns 1 if the
//     range is intact, 0 if anything in it changed — a software breakpoint, an
//     inline hook, or a static patch all alter the bytes.
//
// Calling the check at startup catches tampering before the protected logic
// runs. In a real product you would also place additional checks deeper in the
// program and call them from unrelated code paths (see docs/integrity-checks.md),
// so removing the guard is not a single-edit job.
//
// What to observe after running deadcode-cli:
//   * Clean protected binary: prints the result and exits 0 (the check passes).
//   * If the marked range is patched in the protected binary, the startup
//     check fails and the program exits before doing real work.
//   * In an UNPROCESSED build the check is a no-op that always passes, so the
//     program runs normally during development.
// =============================================================================
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <deadcode-api.h>

// The routine whose machine code must not be altered in the field. Its bytes
// are the protected range; tampering with them is what the check detects.
static std::uint64_t critical_transform(std::uint64_t seed) {
    std::uint64_t v;
    DCApiBeginCheck(7);
    v = seed * 0x100000001B3ull;
    v ^= v >> 33;
    v *= 0xFF51AFD7ED558CCDull;
    v ^= v >> 29;
    DCApiEndCheck(7);
    return v;
}

// Returns nonzero if the protected range is intact. Called before anything
// else so a tampered build stops immediately.
static int verify_startup_integrity() {
    return DCApiCheckIntegrity(7);
}

int main() {
    if (!verify_startup_integrity()) {
        std::fprintf(stderr, "integrity check failed: refusing to run\n");
        return EXIT_FAILURE;
    }

    std::uint64_t out = critical_transform(0xC0FFEEull);
    std::printf("integrity ok, transform=%016llX\n",
                static_cast<unsigned long long>(out));
    return EXIT_SUCCESS;
}
