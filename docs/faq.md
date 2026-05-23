# FAQ

### What if I forget `/DEBUG` (or build without debug info)?

The engine finds markers through your binary's PDB. With no PDB, there is
nothing for it to read, so it cannot locate the marker call sites and cannot
protect anything. Build with `/Zi` and link with `/DEBUG` in **every**
configuration you ship, including Release, and provide the `.pdb` to
`deadcode-cli`. See [integration.md](integration.md).

### What if I strip or lose the PDB before processing?

Same outcome: without the PDB the engine cannot map symbols to addresses, so the
binary cannot be processed. The PDB is required input to `deadcode-cli`, not an
optional debugging extra. Keep the `.pdb` produced for the exact build you
intend to protect — a PDB from a different build will not match.

### Do the markers affect a build that is never processed?

No. In an unprocessed build the markers are ordinary function calls with empty
bodies and `DCApiCheckIntegrity` always returns `1` (intact). Your program
behaves exactly as written, which is what makes day-to-day development and
debugging straightforward — you protect only at release time.

### What is the runtime overhead of a marker call in an unprocessed build?

One non-inlined function call that writes a constant to a `volatile` local and
returns — a handful of instructions, no allocation, no I/O. Negligible for
anything but the very tightest loop. After processing, the marker calls are gone
entirely; what remains is the cost of the transformation you requested on the
marked range (see [protection-levels.md](protection-levels.md)).

### Why are the marker functions not inlined or merged? Is that a bug?

No, it is deliberate. Each marker is `noinline` so its call site survives for
the engine to find, and each has a distinct body so the linker's identical-code
folding (`/OPT:ICF`) cannot merge the seven markers into one address. Without
these guards the markers would become indistinguishable or disappear. The
symbol-survival test verifies they survive in both Debug and Release.

### Can I nest markers?

Yes, when one range is **fully contained** in another:

```cpp
DCApiBeginMutation("outer");
    // ...
    DCApiBeginVirtualization("inner");
        // ...
    DCApiEnd();   // closes inner
    // ...
DCApiEnd();       // closes outer
```

Ranges must not **interleave** (`Begin A … Begin B … End A … End B` is invalid).
`DCApiEnd()` always closes the most recently opened range in the current
function. Integrity-check regions are matched by `id` rather than by nesting, so
they may overlap transformation ranges freely.

### What happens if Begin/End are unbalanced?

Treat unbalanced markers as a build error in your own code, not something the
engine will paper over:

- **Begin without End** (or End without Begin) in a function — the range has no
  well-defined extent. The engine cannot reliably determine what to transform
  and will report the imbalance during processing rather than guess.
- **Interleaved ranges** — same: rejected as malformed.
- **`EndCheck(id)` without a matching `BeginCheck(id)`**, or `CheckIntegrity(id)`
  for an id with no defined region — there is no region to measure or verify.

Keep one `End` per `Begin` and one `EndCheck(id)` per `BeginCheck(id)`, in the
same function, on every path out. See the balancing rules in
[markers.md](markers.md).

### Can a range cross a function boundary?

No. A `Begin*`/`End` pair (and a `BeginCheck`/`EndCheck` pair) must open and
close within the same function. Do not open in one function and close in another,
and do not open in a constructor to close in a destructor.

### Does it work inside C++ methods, templates, and lambdas?

Yes. The markers are `extern "C"` and the engine keys on the call sites, not the
linkage of the enclosing function. Name-mangled methods, template
instantiations, and lambda bodies all work the same as free functions. See
`examples/04_cpp_member_function.cpp`.

### Windows only? What about Linux?

The engine processes Windows PE binaries (x86 and x64). The SDK header compiles
cleanly on Linux and the static library builds there, so the SDK is portable,
but there is no ELF processing. Treat Linux as build-portability of the SDK
itself, not a protection target.

### Where do I get `deadcode-cli`?

The engine is a separate, closed-source commercial product. This repository is
the SDK only. See the contact details in the project [README](../README.md).
