# Integrity checks

Transformation markers make code hard to *read*. Integrity-check markers make
modification of code detectable at *runtime*. They are complementary: a
sensitive routine is often both protected and checked.

## The three calls

| Call | Role |
|------|------|
| `DCApiBeginCheck(id)`     | Open a region whose bytes must stay unchanged. |
| `DCApiEndCheck(id)`       | Close the region with the same `id`. |
| `DCApiCheckIntegrity(id)` | Return `1` if the region is intact, `0` if tampered. |

At obfuscation time the engine computes a CRC32 over the bytes of the region
bracketed by `BeginCheck(id)`/`EndCheck(id)` and captures that value. At runtime
`DCApiCheckIntegrity(id)` recomputes the CRC32 of the same range and compares.
Any change to the bytes in the range — a debugger software breakpoint (which
overwrites an instruction byte), an inline hook, or a static patch — makes the
recomputed CRC differ, so the call returns `0`.

In an unprocessed build `DCApiCheckIntegrity` always returns `1`, so development
builds run normally.

## Using IDs

`id` is a small integer that ties a region to the checks that test it. The same
`id` must appear on the `BeginCheck`, the `EndCheck`, and every
`CheckIntegrity` that tests that region.

- Use a **distinct id per independent region**. A binary can contain many
  regions at once.
- `BeginCheck(id)`/`EndCheck(id)` must be balanced and live in the **same
  function**.
- `CheckIntegrity(id)` can be called from **anywhere**, including a different
  function or translation unit, and as many times as you like.

```cpp
// Region 1: the comparison that decides a license verdict.
DCApiBeginCheck(1);
bool ok = (computed == expected);
DCApiEndCheck(1);

// Region 2: a critical state transition elsewhere.
void apply_entitlements() {
    DCApiBeginCheck(2);
    // ...
    DCApiEndCheck(2);
}

// Tested from unrelated code paths:
if (!DCApiCheckIntegrity(1)) abort_securely();
```

## Recommended patterns

**Check at startup.** Verify critical regions before the protected logic runs,
so a tampered binary stops early.

```cpp
int main() {
    if (!DCApiCheckIntegrity(1) || !DCApiCheckIntegrity(2)) {
        return EXIT_FAILURE;     // refuse to run
    }
    // ... normal startup ...
}
```

**Check periodically, not just once.** A single startup check can be neutralized
once located. Re-verify from a few different points during normal operation —
after a network event, on entering a paid feature, on a timer — so there is no
single guard to disable.

**Distribute the checks.** Spread `CheckIntegrity` calls across unrelated parts
of the codebase and react to failure in different ways (return an error, corrupt
a result, exit later rather than immediately). The goal is that removing
protection is not a single, obvious edit.

**Separate detection from reaction.** Don't always `exit()` on the line after
the check. A delayed or indirect response (degrade a computation now, fail
somewhere unrelated later) is harder to trace back to the check than an
immediate abort.

**Cover what matters, mind the cost.** Verifying a region recomputes a CRC32
over its bytes, which is cheap but not free. Check meaningful regions at a
sensible cadence rather than hashing large ranges in a tight loop.

## What integrity checks do *not* do

- They detect modification of the **marked code bytes**. They are not a data
  validator, an anti-debug primitive on their own, or a license server.
- They do not hide what the code does — pair them with a transformation level
  for that. See [protection-levels.md](protection-levels.md).
