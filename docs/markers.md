# Marker reference

Every marker is a real `extern "C"` function exported by the static library. In
an unprocessed build the calls are ordinary no-ops; `deadcode-cli` finds them
through the PDB at obfuscation time and transforms the marked code. The marker
calls do not survive into the protected binary.

This page documents the contract you integrate against. It does not describe
how the engine performs the transformations.

---

## Transformation markers

These bracket a span of code inside a single function and request a
transformation for the instructions between them.

### `void DCApiBeginVirtualization(const char* name)`
Open a range to be **virtualized** — native instructions in the range are
replaced by an opaque bytecode form executed by an embedded interpreter.
Strongest per-instruction protection; largest slowdown. See
[protection-levels.md](protection-levels.md).

### `void DCApiBeginMutation(const char* name)`
Open a range to be **mutated** — the instructions stay native but are rewritten
into a larger, harder-to-read equivalent. Lightest option; best debugger
usability.

### `void DCApiBeginUltra(const char* name)`
Open a range to be **mutated and then virtualized**. Strongest available;
largest slowdown. Reserve for rarely executed, highly sensitive code.

### `void DCApiEnd(void)`
Close the most recently opened transformation range in the current function.

**Parameters.** `name` is a human-readable label used only for diagnostics in
the engine's processing log. It has no runtime effect and does not need to be
unique, though distinct names make logs easier to read.

**Balancing rules.**
- Exactly one `DCApiEnd()` per `DCApiBegin*`.
- A range must open and close **in the same function**. Do not let a range
  straddle a function boundary (e.g. open in a constructor, close in a
  destructor).
- Close every range on every path out of it. With early returns, call
  `DCApiEnd()` before each `return` inside the range rather than relying on a
  scope guard.
- Nesting is allowed when fully contained (`Begin … Begin … End … End`); ranges
  must not interleave. See [the FAQ](faq.md#can-i-nest-markers) for details.
- Keep the marked span meaningful — a few instructions or more. Wrapping a
  single trivial statement spends transformation cost for little gain.

---

## Integrity-check markers

These mark a code region whose bytes must remain unchanged at runtime and give
you a runtime predicate to test it.

### `void DCApiBeginCheck(int check_id)`
Open an integrity-check region identified by `check_id`.

### `void DCApiEndCheck(int check_id)`
Close the region with the matching `check_id`.

### `int DCApiCheckIntegrity(int check_id)`
Return `1` if the region identified by `check_id` is intact, `0` if its bytes
have changed since obfuscation time. In an unprocessed build this always
returns `1`.

**How they relate.** At obfuscation time the engine computes a CRC32 over the
marked range and captures it. `DCApiCheckIntegrity` recomputes the CRC32 of that
range at runtime and compares. A software breakpoint, an inline hook, or a
static patch within the range changes its bytes and makes the check return `0`.

**Using `check_id`.** The id is a small integer that ties a
`BeginCheck`/`EndCheck` pair to the `CheckIntegrity` calls that test it. Use a
different id per independent region; you may have many regions in one binary.
The id is the link between the three calls — keep it consistent.

**Balancing rules.**
- One `DCApiEndCheck(id)` per `DCApiBeginCheck(id)`, in the same function.
- `DCApiCheckIntegrity(id)` may be called from anywhere, any number of times,
  including functions other than the one that defines the region.
- Call `CheckIntegrity` only with an id that has a defined region, or it has
  nothing to verify.

See [integrity-checks.md](integrity-checks.md) for recommended usage patterns.

---

## Optional macros

Define `DCAPI_USE_MACROS` before including the header to get upper-case aliases
(`DCAPI_BEGIN_VM`, `DCAPI_BEGIN_MUTATION`, `DCAPI_BEGIN_ULTRA`, `DCAPI_END`,
`DCAPI_BEGIN_CHECK`, `DCAPI_END_CHECK`, `DCAPI_CHECK_INTEGRITY`). They expand to
the functions above and are purely cosmetic.
