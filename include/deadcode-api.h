// =============================================================================
// deadcode-api — source-level protection markers consumed by deadcode-cli at
// obfuscation time.  Link with deadcode-api.lib; the post-processor detects
// marker call sites by symbol name in the user's PDB and applies the requested
// transformation to the code range between matched Begin/End calls.
//
// Quick start (C/C++):
//
//   #include <deadcode-api.h>
//
//   int decrypt_license_key(const char* in, char* out) {
//       DCApiBeginUltra("decrypt_license_key");
//       // ... sensitive code here ...
//       DCApiEnd();
//       return 1;
//   }
//
//   bool check_self_integrity() {
//       DCApiBeginCheck(1);
//       // ... code whose bytes must remain unchanged at runtime ...
//       DCApiEndCheck(1);
//       return DCApiCheckIntegrity(1) != 0;
//   }
//
// All marker functions are real, exported C-linkage functions with empty
// bodies — they vanish from the protected binary at obfuscation time
// (deadcode-cli patches every call site).  In an UNPROCESSED build (no
// deadcode-cli pass) the markers are no-ops and the program runs normally.
// =============================================================================
#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Begin/End markers — wrap a range of code inside a function.  Ranges may
// straddle basic blocks but MUST be balanced (one End per Begin, same nesting
// within a function).  `name` is a user-visible debug label that appears in
// deadcode-cli's log when this marker is processed; it has no runtime effect.
//
// Three flavours:
//   * Virtualization  — translate the range to VM bytecode + entry stub.
//                        Best protection, ~50-100× slowdown for the marked range.
//   * Mutation        — keep native instructions but expand each via MBA
//                        rewriting + opaque predicates + polynomial wraps.
//                        Lighter (~2-5×), better preserves debugger usability.
//   * Ultra           — pre-mutate at IR-level, THEN virtualize.  Strongest
//                        anti-reverse; ~150-300× slowdown.
// -----------------------------------------------------------------------------
void DCApiBeginVirtualization(const char* name);
void DCApiBeginMutation     (const char* name);
void DCApiBeginUltra        (const char* name);
void DCApiEnd               (void);

// -----------------------------------------------------------------------------
// Integrity check zones — mark a code region whose bytes must remain unchanged
// at runtime.  deadcode-cli computes the expected CRC32 of the marked range
// at obfuscation time and bakes it into the body of DCApiCheckIntegrity(id).
// At runtime, DCApiCheckIntegrity recomputes the CRC and compares; returns 1
// if intact, 0 if tampered (e.g., debugger breakpoint, instrumentation,
// hot-patch).
//
// `check_id` is a small integer that matches Begin/End/CheckIntegrity calls
// — the same value identifies the same code region.  Multiple distinct
// regions may coexist in one binary by using different IDs.
// -----------------------------------------------------------------------------
void DCApiBeginCheck   (int check_id);
void DCApiEndCheck     (int check_id);
int  DCApiCheckIntegrity(int check_id);   // 1 = intact, 0 = tampered

#if defined(__cplusplus)
}   // extern "C"
#endif

// -----------------------------------------------------------------------------
// Optional ergonomic macros — opt-in via DCAPI_USE_MACROS before include.
// -----------------------------------------------------------------------------
#if defined(DCAPI_USE_MACROS)
#define DCAPI_BEGIN_VM(label)        DCApiBeginVirtualization(label)
#define DCAPI_BEGIN_MUTATION(label)  DCApiBeginMutation(label)
#define DCAPI_BEGIN_ULTRA(label)     DCApiBeginUltra(label)
#define DCAPI_END()                  DCApiEnd()
#define DCAPI_BEGIN_CHECK(id)        DCApiBeginCheck(id)
#define DCAPI_END_CHECK(id)          DCApiEndCheck(id)
#define DCAPI_CHECK_INTEGRITY(id)    DCApiCheckIntegrity(id)
#endif
