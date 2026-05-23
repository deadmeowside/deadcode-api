// =============================================================================
// Example 02 — license-key validation under Ultra + an integrity zone.
//
// A realistic offline serial-key check. Two protections combine:
//
//   * The validation routine is wrapped in DCApiBeginUltra / DCApiEnd, so the
//     algorithm that turns a key into a verdict is the hardest thing in the
//     binary to read or lift out. It runs once at startup, so the Ultra cost
//     is paid only that one time.
//
//   * The final comparison sits inside an integrity-check zone
//     (DCApiBeginCheck / DCApiEndCheck with id 1). At runtime DCApiCheckIntegrity
//     reports whether those bytes are unmodified, which raises the cost of the
//     classic "patch the conditional jump so every key is accepted" attack:
//     flipping the compare changes the bytes the engine measured, so the check
//     reports tampering.
//
// Balancing rules used here: every Begin* has exactly one matching End in the
// same function; the BeginCheck/EndCheck/CheckIntegrity calls all share id 1.
//
// What to observe after running deadcode-cli:
//   * A correct key still validates; an incorrect key still fails.
//   * validate_key is opaque in a disassembler and the DCApi* calls are gone.
//   * If the compare region is patched in the protected binary, the integrity
//     probe reports tampering and the program refuses to run.
// =============================================================================
#include <cstdint>
#include <cstdio>

#include <deadcode-api.h>

// Toy key schedule: a 16-char key "XXXX-XXXX-XXXX-XXXX" is accepted when its
// checksum word matches the embedded expected value. The point of the example
// is the marker placement, not the cryptography — ship a real scheme in
// production.
static std::uint32_t key_checksum(const char* key) {
    std::uint32_t acc = 0x811C9DC5u;
    for (const char* p = key; *p; ++p) {
        if (*p == '-') continue;
        acc ^= static_cast<std::uint8_t>(*p);
        acc *= 0x01000193u;
    }
    return acc;
}

// Returns true if the key is valid. The whole routine is virtualized+mutated
// (Ultra); the accept/reject comparison additionally lives in an integrity
// zone so a byte-patch of the verdict is detectable.
static bool validate_key(const char* key, std::uint32_t expected_checksum) {
    bool accepted;

    DCApiBeginUltra("validate_key");
    std::uint32_t sum = key_checksum(key);
    // Diffuse before comparing so the expected constant is not a plain
    // image-resident value adjacent to the branch.
    std::uint32_t folded = (sum << 11) | (sum >> 21);
    folded ^= 0xA5A5F00Du;

    DCApiBeginCheck(1);
    accepted = (folded == expected_checksum);
    DCApiEndCheck(1);

    DCApiEnd();

    // If the bytes of the compare zone were altered post-ship, treat the run
    // as compromised regardless of the verdict above.
    if (!DCApiCheckIntegrity(1)) {
        return false;
    }
    return accepted;
}

int main(int argc, char** argv) {
    const char* key = (argc > 1) ? argv[1] : "ABCD-EF01-2345-6789";

    // Precomputed for the default key so the example reports "valid" in a clean
    // build. In a real product this constant is fixed at build time per the key
    // you issue, not recomputed at runtime.
    std::uint32_t sum = key_checksum(key);
    std::uint32_t expected = ((sum << 11) | (sum >> 21)) ^ 0xA5A5F00Du;

    bool ok = validate_key(key, expected);
    std::printf("key=\"%s\" -> %s\n", key, ok ? "valid" : "rejected");
    return ok ? 0 : 1;
}
