# cryptsetup-pqc

Patch for [cryptsetup 2.8.4](https://github.com/mbroz/cryptsetup/commit/542914c3e262313e17d05d1590cd5318a1a5576d) adding ML-KEM-768 availability detection via the Linux Kernel Crypto API.

## What this patch does

Adds `crypt_backend_mlkem768_available()` to the cryptsetup kernel crypto backend.
The function checks whether the ML-KEM-768 algorithm is accessible via `/proc/crypto`,
indicating that `pqc_kem.ko` (from `packages/pqc-kernel-module`) is loaded.

**Changed files:**
- `lib/crypto_backend/crypto_backend.h` — function declaration
- `lib/crypto_backend/crypto_kernel.c` — function implementation

The existing API is unchanged — this is a strictly opt-in addition.

## Applying the patch

```bash
git clone https://github.com/mbroz/cryptsetup.git
cd cryptsetup
git checkout v2.8.4
git apply 0001-pqc-mlkem768-availability.patch
```

## Building cryptsetup with the patch

```bash
./autogen.sh
./configure --with-crypto_backend=kernel
make -j$(nproc)
```

## Usage

```c
#include "lib/crypto_backend/crypto_backend.h"

if (crypt_backend_mlkem768_available()) {
    // pqc_kem.ko is loaded, ML-KEM-768 is available
}
```

## Upstream reference

Base commit: [`542914c`](https://github.com/mbroz/cryptsetup/commit/542914c3e262313e17d05d1590cd5318a1a5576d) — cryptsetup 2.8.4
