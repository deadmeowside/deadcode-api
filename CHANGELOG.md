# Changelog

All notable changes to the deadcode-api SDK are documented here. This project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html), and the
format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [0.1.0] - 2026-05-23

Initial public release of the SDK.

### Added
- Public `extern "C"` header `include/deadcode-api.h` with the transformation
  markers (`DCApiBeginVirtualization`, `DCApiBeginMutation`, `DCApiBeginUltra`,
  `DCApiEnd`) and integrity-check markers (`DCApiBeginCheck`, `DCApiEndCheck`,
  `DCApiCheckIntegrity`), plus opt-in `DCAPI_USE_MACROS` aliases.
- Static-library implementation with non-inlined, individually addressable
  marker symbols that survive `/OPT:REF` and `/OPT:ICF`.
- CMake build producing the static library, examples, an install/package
  target, and a CTest symbol-survival check (dumpbin/map on Windows, nm on
  Linux).
- Four worked examples: protection levels, license-key validation, self
  integrity check, and markers in C++ member functions.
- Documentation: integration, marker reference, protection levels, integrity
  checks, and FAQ.
- CI for MSVC and clang-cl (x86 + x64, Debug + Release, with the symbol-survival
  test) and for GCC + Clang on Linux (build + nm symbol check).

[0.1.0]: https://github.com/deadmeowside/deadcode-api/releases/tag/v0.1.0
