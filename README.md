# deadcode-api

[![CI](https://github.com/deadmeowside/deadcode-api/actions/workflows/ci.yml/badge.svg)](https://github.com/deadmeowside/deadcode-api/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Standard: C99 / C++17](https://img.shields.io/badge/standard-C99%20%7C%20C%2B%2B17-blue.svg)](#)

The public SDK for **deadcode**, a commercial software-protection product for
Windows x86/x64 PE binaries. You annotate the sensitive regions of your own
source — license validation, key handling, unlock logic, anti-tamper hot spots
— with lightweight markers; the `deadcode-cli` engine (sold separately) reads
those markers after your build and replaces each marked region with a hardened
form. The result raises the cost of reverse engineering, software piracy, and
tampering, and lets you enforce licensing in the field, while your day-to-day
unprocessed builds stay ordinary and debuggable.

This repository is the **SDK only** — the header and static library you link
into your application. It is MIT-licensed and open. The engine is a separate
closed-source product.

## Quick start

```cpp
#include <deadcode-api.h>

int decrypt_license_key(const char* in, char* out) {
    DCApiBeginUltra("decrypt_license_key");
    // ... sensitive code here ...
    DCApiEnd();
    return 1;
}

bool check_self_integrity() {
    DCApiBeginCheck(1);
    // ... code whose bytes must remain unchanged at runtime ...
    DCApiEndCheck(1);
    return DCApiCheckIntegrity(1) != 0;
}
```

1. Build the SDK (below) to get `deadcode-api.lib` and add `include/` to your
   include path.
2. Link the library into your application.
3. Place markers around the regions you want protected.
4. Build your application **with debug info and `/DEBUG`** so it ships a PDB.
5. Run the engine on the build output: `deadcode-cli your-app.exe`.

In an **unprocessed** build the markers are no-ops and your program behaves
exactly as written — handy during development. See
[`docs/integration.md`](docs/integration.md) for the full setup.

## Features

- **Three transformation levels** — Mutation (~2-5×), Virtualization (~50-100×),
  and Ultra (~150-300×), chosen per marked region. See
  [`docs/protection-levels.md`](docs/protection-levels.md).
- **Runtime integrity checks** — mark a code region and test at runtime whether
  its bytes are unmodified, to detect breakpoints, hooks, and patches. See
  [`docs/integrity-checks.md`](docs/integrity-checks.md).
- **Source-level granularity** — protect a specific span inside a function, not
  whole modules; works inside C++ methods, templates, and lambdas.
- **Zero-cost when unprocessed** — markers are empty no-op calls until the
  engine processes the binary, so debug builds stay fast and steppable.
- **Plain `extern "C"` API** — usable from C and C++, MSVC and clang-cl.
- **Markers vanish from the protected binary** — the marker calls are removed
  during processing; only the transformed code remains.

## Building

Requires CMake 3.15+ and a C++17 compiler.

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release        # runs the symbol-survival test
cmake --install build --prefix _install  # installs the header + static lib
```

On Windows this produces `deadcode-api.lib`; on Linux, `libdeadcode-api.a`.
Examples build into `build/` (`01_protection_levels`, `02_license_check`,
`03_self_integrity`, `04_cpp_member_function`). Toggle parts of the build with
`-DDEADCODE_API_BUILD_EXAMPLES=OFF` / `-DDEADCODE_API_BUILD_TESTS=OFF`.

> Any binary you intend to protect must be built with debug info and `/DEBUG`
> and shipped to the engine alongside its PDB — see
> [`docs/integration.md`](docs/integration.md).

## How it works

The SDK is **not header-only**, and that is the whole point. Its marker
functions are real, exported `extern "C"` symbols with empty bodies, compiled
into a static library you link. Because they are real functions, the linker
emits genuine symbols for them into your application's binary and **PDB**.

`deadcode-cli` (sold separately) runs **after your build**, as a post-processor.
It reads the binary's PDB, locates the marker call sites by symbol name, and
replaces the code range between each matched `Begin`/`End` with the requested
transformation. For integrity zones it captures a CRC32 of the marked range and
rewrites `DCApiCheckIntegrity` to verify that value at runtime. The marker calls
themselves are removed, so they do not appear in the protected binary.

The SDK takes care to keep the markers findable through the toolchain: each is
`noinline` (so the call site is not optimized away) and has a distinct body (so
identical-code folding cannot merge the markers). The
[symbol-survival test](test/symbol_survival.cpp) verifies this holds in both
Debug and Release.

## Platform support

| Platform | Header + library | Engine processing |
|----------|:----------------:|-------------------|
| Windows (MSVC, clang-cl), x86 + x64 | yes | **yes** |
| Linux (GCC, Clang) | yes | no |

Engine support is **Windows only**. The Linux build keeps the SDK itself
portable (the header compiles and the library builds), but `deadcode-cli`
processes PE binaries — there is no ELF support.

## Documentation

- [Integration](docs/integration.md) — CMake, MSBuild, and manual builds.
- [Marker reference](docs/markers.md) — every function, parameters, balancing
  rules.
- [Protection levels](docs/protection-levels.md) — Mutation vs Virtualization
  vs Ultra, and how to choose.
- [Integrity checks](docs/integrity-checks.md) — patterns for self-verification.
- [FAQ](docs/faq.md) — PDB requirements, overhead, nesting, balancing.

## Status

The **SDK** in this repository is public and MIT-licensed (see [LICENSE](LICENSE)).
The **`deadcode-cli` engine** is a separate, closed-source commercial product
and is not distributed here.

For engine licensing and evaluation, contact **licensing@deadcode.example**
*(placeholder — replace with your real contact address before publishing)*.

## License

MIT — see [LICENSE](LICENSE).
