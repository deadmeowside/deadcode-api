// =============================================================================
// Example 04 — markers inside C++ class member functions.
//
// The markers are plain extern "C" functions, so they work unchanged inside
// member functions, including methods that touch `this`. The engine keys on
// the marker call sites, not on the enclosing function's linkage, so a
// name-mangled C++ method, a template instantiation, or a lambda body all work
// the same way as a free function.
//
// Two things worth noting for C++ specifically:
//   * Put a Begin/End pair inside one function body. A range must open and
//     close in the same function — do not open in a constructor and close in a
//     destructor, and do not let it straddle a method boundary.
//   * RAII and early returns: keep the End on every path out of the marked
//     range. Here the range has a single exit, which is the simplest correct
//     shape. If you need early returns, close the range before each return
//     rather than relying on a scope guard.
//
// What to observe after running deadcode-cli:
//   * The class behaves identically; LicenseVault::derive is the protected
//     method and its DCApi* calls are gone from the image.
// =============================================================================
#include <cstdint>
#include <cstdio>
#include <string>

#include <deadcode-api.h>

class LicenseVault {
public:
    explicit LicenseVault(std::uint64_t seed) : state_(seed) {}

    // Virtualize the per-instance key derivation. `this`-relative member access
    // is fine inside a marked range.
    std::uint64_t derive(const std::string& feature) {
        std::uint64_t out;
        DCApiBeginVirtualization("LicenseVault::derive");
        std::uint64_t acc = state_;
        for (unsigned char c : feature) {
            acc = (acc ^ c) * 0x100000001B3ull;
            acc ^= acc >> 27;
        }
        state_ ^= acc;        // mutate instance state under protection
        out = acc;
        DCApiEnd();
        return out;
    }

    // Mutate a small, frequently called accessor: cheap protection that still
    // obscures the rotation constant.
    std::uint32_t fingerprint() const {
        std::uint32_t fp;
        DCApiBeginMutation("LicenseVault::fingerprint");
        std::uint32_t lo = static_cast<std::uint32_t>(state_ & 0xFFFFFFFFu);
        std::uint32_t hi = static_cast<std::uint32_t>(state_ >> 32);
        fp = ((lo << 13) | (lo >> 19)) ^ hi;
        DCApiEnd();
        return fp;
    }

private:
    std::uint64_t state_;
};

int main() {
    LicenseVault vault(0xDEADC0DEull);

    std::uint64_t a = vault.derive("export");
    std::uint64_t b = vault.derive("collaborate");
    std::uint32_t fp = vault.fingerprint();

    std::printf("export=%016llX collaborate=%016llX fp=%08X\n",
                static_cast<unsigned long long>(a),
                static_cast<unsigned long long>(b),
                fp);
    return 0;
}
