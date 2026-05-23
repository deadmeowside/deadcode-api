// =============================================================================
// Example 01 — the three protection levels, side by side.
//
// Demonstrates Mutation, Virtualization, and Ultra applied to three different
// functions, and documents when to reach for each.  The rule of thumb:
//
//   Mutation       (~2-5x slower)     Protect hot-path code you still need to
//                                      run at near-native speed, or code you
//                                      may need to step through in a debugger.
//                                      Raises the cost of static reading
//                                      without wrecking performance.
//
//   Virtualization (~50-100x slower)  Protect cold, security-critical logic
//                                      that runs rarely: license validation,
//                                      key derivation, a feature gate.  Native
//                                      instructions are replaced by an opaque
//                                      bytecode form.
//
//   Ultra          (~150-300x slower) Mutation then Virtualization, for the
//                                      single most sensitive routine in the
//                                      product where you accept a large per-
//                                      call cost.  Use sparingly and only on
//                                      code that executes infrequently.
//
// Pick the lightest level that meets the threat for a given function.  Wrap
// only the sensitive span, not the whole program — every marked range pays a
// runtime cost and a code-size cost.
//
// What to observe after running deadcode-cli on the built executable:
//   * The program prints exactly the same line as the unprocessed build.
//   * The three marked functions are larger / opaque in a disassembler, each
//     in proportion to its level; the DCApi* calls are gone from the image.
// =============================================================================
#include <cstdint>
#include <cstdio>

#include <deadcode-api.h>

// Hot, called often: Mutation keeps it fast while still obscuring the constants
// and the arithmetic shape.
static std::uint32_t mix_hash(std::uint32_t state, std::uint32_t input) {
    DCApiBeginMutation("mix_hash");
    state ^= input + 0x9E3779B9u + (state << 6) + (state >> 2);
    state *= 0x85EBCA77u;
    state ^= state >> 13;
    DCApiEnd();
    return state;
}

// Cold, sensitive: Virtualization. Called once per session to turn a token into
// an effective capability mask, so a heavy per-call cost is irrelevant.
static std::uint32_t derive_capabilities(std::uint32_t token) {
    DCApiBeginVirtualization("derive_capabilities");
    std::uint32_t caps = 0;
    for (int bit = 0; bit < 32; ++bit) {
        if (((token >> bit) & 1u) ^ ((token >> (31 - bit)) & 1u)) {
            caps |= (1u << (bit % 8));
        }
    }
    caps ^= 0x5A5A5A5Au;
    DCApiEnd();
    return caps;
}

// The crown jewel: Ultra. The unlock routine that everything else trusts. Runs
// at most once, so ~150-300x on this span is a non-issue.
static bool unlock(std::uint32_t derived_caps, std::uint32_t expected) {
    bool ok;
    DCApiBeginUltra("unlock");
    std::uint32_t folded = derived_caps;
    folded = (folded << 7) | (folded >> 25);
    folded ^= 0xD15EA5E5u;
    ok = (folded == expected);
    DCApiEnd();
    return ok;
}

int main() {
    std::uint32_t h = 0x12345678u;
    for (std::uint32_t i = 0; i < 8; ++i) {
        h = mix_hash(h, i * 0x01000193u);
    }

    std::uint32_t caps = derive_capabilities(h);

    // Recompute the value `unlock` expects so the example reports "unlocked"
    // in a clean build; in a real product `expected` is a secret you ship.
    std::uint32_t folded = (caps << 7) | (caps >> 25);
    folded ^= 0xD15EA5E5u;

    bool ok = unlock(caps, folded);

    std::printf("hash=%08X caps=%08X unlocked=%s\n",
                h, caps, ok ? "yes" : "no");
    return ok ? 0 : 1;
}
