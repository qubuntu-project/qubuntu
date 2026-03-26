# pqc_kem.ko & liboqs_wrapper.ko — ML-KEM-768 Kernel Modules

## Overview

This repository provides two Linux kernel modules implementing post-quantum cryptography
based on ML-KEM-768 (formerly Kyber-768), the NIST-standardized Key Encapsulation
Mechanism (FIPS 203, 2024).

### Module Architecture

```
┌─────────────────────────────────────────┐
│           Userspace / cryptsetup        │
│         AF_ALG socket interface         │
└──────────────────┬──────────────────────┘
                   │ Kernel Crypto API
┌──────────────────▼──────────────────────┐
│              pqc_kem.ko                 │
│   ML-KEM-768 algorithm implementation  │
│   Registers "ml-kem-768" in /proc/crypto│
└──────────────────┬──────────────────────┘
                   │ depends on
┌──────────────────▼──────────────────────┐
│           liboqs_wrapper.ko             │
│   Kernel-safe wrappers for liboqs APIs  │
│   Memory management, RNG, cleansing     │
└─────────────────────────────────────────┘
```

---

## liboqs_wrapper.ko

`liboqs_wrapper.ko` is the foundation layer. It bridges the [liboqs](https://github.com/open-quantum-safe/liboqs)
API conventions to kernel-safe equivalents. PQC algorithms like ML-KEM require memory
allocation, secure memory clearing, and random number generation — all of which behave
differently in kernel space than in userspace.

### What it provides

| liboqs API | Kernel implementation | Notes |
|---|---|---|
| `OQS_MEM_malloc` | `kmalloc(GFP_KERNEL)` | Kernel heap allocation |
| `OQS_MEM_calloc` | `kcalloc(GFP_KERNEL)` | Zero-initialized allocation |
| `OQS_MEM_free` | `kfree` | Standard kernel free |
| `OQS_MEM_secure_free` | `memzero_explicit` + `kfree` | Constant-time clearing before free |
| `OQS_MEM_cleanse` | `memzero_explicit` | Constant-time memory wipe |
| `OQS_randombytes` | `get_random_bytes` | ChaCha20-based CSPRNG |

### Why secure memory clearing matters

Private keys and shared secrets must be wiped from memory immediately after use.
`memzero_explicit` is guaranteed not to be optimized away by the compiler — unlike
`memset`, which compilers may elide if the memory is not read afterwards. This prevents
key material from lingering in kernel memory.

### RNG: ChaCha20-based CSPRNG

The kernel's `get_random_bytes` uses a ChaCha20-based CSPRNG seeded from hardware
entropy sources (RDRAND, TPM, interrupt timing). ChaCha20 is a symmetric cipher —
Grover's algorithm provides only a quadratic speedup against symmetric primitives,
so 256-bit keys remain secure (equivalent to 128-bit post-quantum security).

### Loading

```bash
sudo modprobe liboqs_wrapper
sudo dmesg | grep liboqs
# [  xx.xxx] liboqs kernel wrapper initialized (NIST-compliant)
# [  xx.xxx] RNG: ChaCha20-based CSPRNG (quantum-resistant)
```

---

## pqc_kem.ko

`pqc_kem.ko` implements ML-KEM-768 and registers it with the Linux Kernel Crypto API.
Once loaded, the algorithm is registered in the Kernel Crypto API and visible in
`/proc/crypto`. The current implementation registers with a custom type
(`CRYPTO_ALG_TYPE_PQC_KEM`) which is not yet supported by the standard `AF_ALG`
userspace interface — direct kernel-to-kernel use (e.g. from `dm-crypt` or a
companion module) is the intended access path.

### Verification

```bash
sudo modprobe pqc_kem
grep -A8 "name.*ml-kem" /proc/crypto
```

Expected output:
```
name         : ml-kem-768
driver       : ml-kem-768-generic
module       : pqc_kem
priority     : 100
selftest     : passed
type         : larval
```

### ML-KEM-768 Security Parameters

| Parameter | Value |
|-----------|-------|
| NIST Security Level | 3 (≈ AES-192) |
| Public key size | 1184 bytes |
| Secret key size | 2400 bytes |
| Ciphertext size | 1088 bytes |
| Shared secret size | 32 bytes |
| Classical security | ~180 bits |
| Post-quantum security | ~180 bits |

---

## Key Concept: KEM vs. Encryption

**ML-KEM is not an encryption algorithm.** It is a Key Encapsulation Mechanism.

```
Standard LUKS2 (local disk, no network):
  Passphrase → Argon2/PBKDF2 → AES-Key → AES-XTS (data encryption)
                                  ↑
                     No key exchange needed — pqc_kem.ko not involved

PQC use case (network key transport):
  ML-KEM-768 encapsulate → shared_secret → HKDF → AES-Key → AES-XTS
       ↑
  pqc_kem.ko is essential here
```

AES-XTS with 512-bit keys is already quantum-resistant (Grover's algorithm only
halves the effective key length, leaving 256-bit post-quantum security). ML-KEM
protects the **transport** of keys, not the encryption itself.

---

## Use Cases

### 1. Remote LUKS Unlock (Tang/Clevis with PQC)

**Problem:** Clevis/Tang uses ECDH for network-based automatic LUKS unlock. A quantum
attacker recording today's unlock traffic can recover the LUKS master key once a
sufficiently powerful quantum computer exists ("harvest now, decrypt later").

**Solution:**

```
Client (boot)                          Tang server
     |                                      |
     |-- ML-KEM-768 Encapsulate ----------->|
     |   (server's public key)              |
     |<-- ciphertext ----------------------|
     |                                      |
     shared_secret (never transmitted)      |
     → HKDF → LUKS key → open volume        |
```

The shared secret is derived locally and never sent over the network. An attacker
capturing the ciphertext cannot derive the shared secret without the server's
private key — even with a quantum computer.

### 2. TPM Key Sealing with PQC-Protected Transport

**Problem:** TPM 2.0 uses RSA-2048 or ECC for key sealing and remote attestation.
The sealed key transport is vulnerable to quantum attacks.

**Solution:**

```
Seal:
  LUKS key
    → ML-KEM-768 encapsulate with recipient's public key
    → store KEM ciphertext (replaces RSA-encrypted key blob)

Unseal:
  recipient's private key + KEM ciphertext → shared_secret
  shared_secret → HKDF → LUKS key
  TPM PCR verification → open volume
```

`pqc_kem.ko` exposes ML-KEM via the Kernel Crypto API so TPM userspace tools
(tpm2-tools) can use it through `AF_ALG` without a separate userspace implementation.

### 3. Hybrid Key Wrapping (Recommended for Transition Period)

**Problem:** LUKS master keys transferred between systems are often wrapped with
RSA or ECDH — both broken by quantum computers.

**Solution — Hybrid approach:**

```
Wrapped key = AES-256-GCM(
    key = HKDF(ML-KEM-768 shared_secret || ECDH shared_secret),
    plaintext = LUKS master key
)
```

This is secure as long as **at least one** of the two key exchange algorithms
remains unbroken. This is the approach recommended by NIST and BSI during the
classical-to-post-quantum transition period.

### 4. Network-Attached Encrypted Volumes (NBD/iSCSI with PQC-TLS)

**Problem:** Network Block Devices and iSCSI targets use TLS with ECDH for the
handshake. The data stream is vulnerable to "harvest now, decrypt later" attacks.

**Solution:**
TLS 1.3 with `X25519MLKEM768` (combining X25519 and ML-KEM-768, see IETF draft)
provides hybrid post-quantum key exchange. The kernel TLS stack (ktls) can perform
the ML-KEM portion of the handshake in-kernel via `pqc_kem.ko`, avoiding context
switches to userspace for each key exchange.

### 5. Future: LUKS3 PQC Keyslots

LUKS2 currently has no support for PQC KEMs in keyslots. A future LUKS3
specification could use ML-KEM-768 as follows:

```
LUKS3 keyslot (PQC-protected):
  master key
    → ML-KEM-768 encapsulate with user's public key
    → store KEM ciphertext in LUKS header

Unlock:
  user's private key + KEM ciphertext → master key → open volume
```

`pqc_kem.ko` is the kernel-side preparation for this integration.
The `crypto_kernel.c` patch in this repository extends cryptsetup's crypto
backend to detect ML-KEM-768 availability via the Kernel Crypto API.

---

## Loading & Autoload

```bash
# Load manually (correct order)
sudo modprobe liboqs_wrapper
sudo modprobe pqc_kem

# Or use the included helper script (also runs pqc_kem_test)
./pqc-modules.sh load    # load all modules + show test results
./pqc-modules.sh status  # show module state + /proc/crypto entry
./pqc-modules.sh unload  # unload in correct reverse order

# Autoload on boot (systemd)
echo -e "liboqs_wrapper\npqc_kem" | sudo tee /etc/modules-load.d/pqc.conf
```

## Testing

`pqc_kem_test.ko` is a kernel-space test module that validates `liboqs_wrapper`
without triggering the `AF_ALG` limitation:

```bash
sudo modprobe pqc_kem_test
sudo dmesg | grep pqc_test
```

Tests performed:
- `OQS_MEM_calloc`: zero-initialized allocation
- `OQS_MEM_cleanse`: constant-time memory wipe
- `OQS_MEM_secure_free`: wipe + free without crash
- `OQS_randombytes`: non-zero output, two calls produce distinct results
- ML-KEM-768 key length constants match FIPS 203 specification

---

## References

- [FIPS 203 — ML-KEM Standard](https://csrc.nist.gov/pubs/fips/203/final)
- [NIST Post-Quantum Cryptography](https://csrc.nist.gov/projects/post-quantum-cryptography)
- [Linux Kernel Crypto API](https://www.kernel.org/doc/html/latest/crypto/index.html)
- [Open Quantum Safe — liboqs](https://github.com/open-quantum-safe/liboqs)
- [Clevis/Tang Framework](https://github.com/latchset/clevis)
- [IETF Draft: X25519MLKEM768 for TLS 1.3](https://datatracker.ietf.org/doc/draft-kwiatkowski-tls-ecdhe-mlkem/)
- [BSI: Migration to Post-Quantum Cryptography](https://www.bsi.bund.de/EN/Themen/Unternehmen-und-Organisationen/Informationen-und-Empfehlungen/Quantentechnologien-und-Post-Quanten-Kryptografie/Post-Quanten-Kryptografie/post-quanten-kryptografie_node.html)
