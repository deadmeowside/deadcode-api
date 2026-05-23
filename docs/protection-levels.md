# Protection levels

The SDK offers three transformation levels for a marked range. They differ in
how hard the result is to analyze and how much runtime cost they add. Pick the
lightest level that meets the threat for a given piece of code.

| Level | Marker | Typical slowdown (marked range) | Debugger usability | Code-size growth |
|-------|--------|:-------------------------------:|--------------------|------------------|
| Mutation | `DCApiBeginMutation` | ~2-5× | Largely preserved | Moderate |
| Virtualization | `DCApiBeginVirtualization` | ~50-100× | Poor | High |
| Ultra | `DCApiBeginUltra` | ~150-300× | Poor | Highest |

The slowdown figures are order-of-magnitude guidance for the marked range only,
not the whole program. Actual numbers depend on the instructions in the range
and how often it runs. **Measure your own hot paths.**

---

## Mutation (~2-5×)

Instructions stay native but are rewritten into a larger, equivalent form that
is harder to read statically. Control flow and arithmetic are obscured while the
code still executes as ordinary machine instructions.

**Use when:** the code runs often and must stay fast, or you still need to step
through it occasionally. Mutation is the right default for protecting logic on a
hot path without a perf cliff.

**Trade-offs:** the weakest of the three against a determined analyst, because
the result is still native code. Best perf and best debugger experience.

---

## Virtualization (~50-100×)

Native instructions in the range are replaced by an opaque bytecode executed by
an embedded interpreter. There is no longer native code to read for the marked
range, which raises the analysis cost substantially.

**Use when:** the code is cold and security-critical — license validation, key
derivation, a feature gate, an unlock path. These run rarely, so a 50-100×
cost on the range is usually invisible end to end.

**Trade-offs:** large slowdown and code-size growth; the range is effectively
not steppable in a normal debugger. Do not virtualize hot loops.

---

## Ultra (~150-300×)

The range is mutated first, then virtualized. This is the strongest option and
also the most expensive.

**Use when:** a single routine is the linchpin of your protection — the one
function whose compromise undermines everything else — and it executes
infrequently. Reserve Ultra for that crown-jewel code; do not spread it across
the codebase.

**Trade-offs:** the largest slowdown and code-size growth of the three. Apply to
the smallest meaningful span that still captures the sensitive logic.

---

## Choosing a level: a perf budget

Think in terms of how often the marked code runs:

- **Per frame / inner loop / per request hot path** → Mutation, or leave it
  unprotected and protect the data it depends on instead.
- **Once per session / per launch / per file open** → Virtualization is
  usually affordable.
- **Once, ever, and it must not be lifted** → Ultra.

Two practical rules:

1. **Wrap the smallest meaningful span.** Every marked range pays a cost on
   every execution. Protect the sensitive comparison or derivation, not the
   surrounding bookkeeping.
2. **Layer with integrity checks.** Transformation hides *what* the code does;
   integrity checks detect *modification* of it. Sensitive routines often want
   both — see [integrity-checks.md](integrity-checks.md).
