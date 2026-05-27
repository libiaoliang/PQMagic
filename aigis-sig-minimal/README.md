# Aigis-sig Minimal Build

This directory provides a small CMake entrypoint for compiling only the Aigis-sig
signing implementation and its direct support code.

All objects are built as position-independent code by default so the static
archive can be linked into a shared object or TA image.

It includes:

- `sig/aigis-sig/std/sign.c`
- `sig/aigis-sig/std/packing.c`
- `sig/aigis-sig/std/poly.c`
- `sig/aigis-sig/std/polyvec.c`
- `sig/aigis-sig/std/ntt.c`
- `sig/aigis-sig/std/reduce.c`
- `sig/aigis-sig/std/rounding.c`
- SM3 by default: `hash/sm3/x86-64/sm3.c` and `hash/sm3/sm3_extended.c`
- Linux random bytes provider: `utils/randombytes.c`
- a minimal provider wrapper exposing key management and signing callbacks

Build all three Aigis-sig modes:

```bash
aigis-sig-minimal/compile.sh
```

Build only Aigis-sig2:

```bash
aigis-sig-minimal/compile.sh --mode 2
```

Build with AddressSanitizer:

```bash
aigis-sig-minimal/compile.sh --asan
```

For a static archive, the final executable or shared object that consumes the
archive must also be linked with `-fsanitize=address`.

Run the minimized build against the original Aigis-sig basic test:

```bash
aigis-sig-minimal/test.sh
```

Test only Aigis-sig2:

```bash
aigis-sig-minimal/test.sh --mode 2
```

Run tests with AddressSanitizer:

```bash
aigis-sig-minimal/test.sh --asan
```

The static library target is:

```text
pqmagic_aigis_sig_minimal_static
```

The shared library target is:

```text
pqmagic_aigis_sig_minimal
```

The provider wrapper registers only these operation groups:

- `CRYPT_EAL_OPERAID_KEYMGMT`
- `CRYPT_EAL_OPERAID_SIGN`

Algorithm IDs:

```c
PQMAGIC_PKEY_AIGIS_SIG1
PQMAGIC_PKEY_AIGIS_SIG2
PQMAGIC_PKEY_AIGIS_SIG3
```

Key parameter IDs:

```c
PQMAGIC_AIGIS_PARAM_PUBKEY
PQMAGIC_AIGIS_PARAM_PRVKEY
```

The copied EAL/BSL compatibility headers used for standalone builds live under
`src/provider_compat/`; they are intentionally kept out of the public `include/`
directory to avoid colliding with the real provider framework headers.

For a TEE build, replace `utils/randombytes.c` with a TA-specific implementation
if the Linux provider is unavailable:

```c
void randombytes(uint8_t *out, size_t outlen);
```

Two build paths are available:

```bash
aigis-sig-minimal/compile.sh --randombytes-source path/to/randombytes_tee.c
```

or, for a static archive, leave `randombytes` unresolved and provide it from the
final TA project:

```bash
aigis-sig-minimal/compile.sh --target pqmagic_aigis_sig_minimal_static --external-randombytes
```

See `examples/randombytes_tee.c.example` for a minimal `TEE_GenerateRandom`
adapter.
