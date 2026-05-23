# Contributing

Thanks for your interest in the deadcode-api SDK. This repository covers the
public SDK only — the header, the static library, examples, docs, and tests. The
`deadcode-cli` engine is a separate closed-source product and is out of scope
here; engine bugs and feature requests are not tracked in this repository (see
the contact address in the [README](README.md)).

## Reporting issues

Open a GitHub issue and include:

- What you expected to happen and what actually happened.
- A minimal reproducer where possible.
- Your platform and toolchain: OS, compiler and version (MSVC, clang-cl, GCC,
  Clang), target architecture (x86 / x64), and Debug vs Release.
- For build or symbol problems, the CMake configure/build output and, if
  relevant, the output of the symbol-survival test.

Please do **not** include details of any protected/processed binary or anything
covered by your engine license in a public issue.

## Pull requests

- Open an issue first for anything beyond a small fix, so we can agree on the
  approach before you invest time.
- Keep the **public API in `include/deadcode-api.h` stable.** Changing marker
  signatures or semantics affects every consumer and every engine version; such
  changes need discussion first.
- Match the existing style: dry, technical comments that explain *why*
  (especially where compiler, ABI, or linker behavior is involved), not just
  *what*.
- If you touch the library or build, make sure `ctest` still passes on your
  platform — the symbol-survival test must remain green in both Debug and
  Release. Add or update tests and docs alongside code changes.
- Keep commits focused and write clear commit messages.

## Building and testing locally

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```

By contributing, you agree that your contributions are licensed under the
repository's [MIT License](LICENSE).
