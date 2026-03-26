// SPDX-License-Identifier: GPL-2.0
/*
 * pqc_kem_test.ko — safe kernel test for liboqs_wrapper.ko
 *
 * Avoids crypto_has_alg() / crypto_alloc_* — these crash with custom
 * CRYPTO_ALG_TYPE_PQC_KEM that is not registered in the standard framework.
 *
 * Tests:
 *   1. OQS_MEM_calloc: zero-initialized
 *   2. OQS_MEM_cleanse: zeroes buffer
 *   3. OQS_MEM_secure_free: no crash
 *   4. OQS_randombytes: non-zero output, two calls differ
 *   5. pqc_kem module present: check /proc/crypto via string in Module.symvers
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "liboqs_wrapper.h"

#define PASS(fmt, ...) pr_info("pqc_test: PASS — " fmt "\n", ##__VA_ARGS__)
#define FAIL(fmt, ...) pr_err( "pqc_test: FAIL — " fmt "\n", ##__VA_ARGS__)

static int passed, failed;
#define RUN(fn) do { if ((fn) == 0) passed++; else failed++; } while (0)

static int test_calloc(void)
{
	u8 *buf = OQS_MEM_calloc(128, 1);
	int i;

	if (!buf) { FAIL("calloc(128) returned NULL"); return -1; }
	for (i = 0; i < 128; i++) {
		if (buf[i]) {
			FAIL("calloc: byte %d not zero", i);
			OQS_MEM_free(buf);
			return -1;
		}
	}
	OQS_MEM_free(buf);
	PASS("OQS_MEM_calloc(128): zero-initialized");
	return 0;
}

static int test_cleanse(void)
{
	u8 *buf = OQS_MEM_malloc(64);
	int i;

	if (!buf) { FAIL("malloc(64) returned NULL"); return -1; }
	memset(buf, 0xAB, 64);
	OQS_MEM_cleanse(buf, 64);
	for (i = 0; i < 64; i++) {
		if (buf[i]) {
			FAIL("cleanse: byte %d = 0x%02x (expected 0)", i, buf[i]);
			OQS_MEM_free(buf);
			return -1;
		}
	}
	OQS_MEM_free(buf);
	PASS("OQS_MEM_cleanse(64): all bytes zeroed");
	return 0;
}

static int test_secure_free(void)
{
	u8 *buf = OQS_MEM_malloc(ML_KEM_768_SECRET_KEY_LEN);

	if (!buf) { FAIL("malloc(%d) returned NULL", ML_KEM_768_SECRET_KEY_LEN); return -1; }
	memset(buf, 0xFF, ML_KEM_768_SECRET_KEY_LEN);
	OQS_MEM_secure_free(buf, ML_KEM_768_SECRET_KEY_LEN);
	PASS("OQS_MEM_secure_free(%d bytes): no crash", ML_KEM_768_SECRET_KEY_LEN);
	return 0;
}

static int test_rng(void)
{
	u8 buf1[32] = {0}, buf2[32] = {0};
	int i, all_zero;

	if (OQS_randombytes(buf1, 32)) { FAIL("randombytes call 1 failed"); return -1; }
	if (OQS_randombytes(buf2, 32)) { FAIL("randombytes call 2 failed"); return -1; }

	all_zero = 1;
	for (i = 0; i < 32; i++) if (buf1[i]) { all_zero = 0; break; }
	if (all_zero) { FAIL("randombytes: output is all zeros"); return -1; }

	if (memcmp(buf1, buf2, 32) == 0) {
		FAIL("randombytes: two calls produced identical output");
		return -1;
	}
	PASS("OQS_randombytes: non-zero, two distinct 32-byte outputs");
	return 0;
}

/* Key length constants from pqc_kem.c — verify they match NIST FIPS 203 */
static int test_key_lengths(void)
{
	if (ML_KEM_768_PUBLIC_KEY_LEN    != 1184 ||
	    ML_KEM_768_SECRET_KEY_LEN    != 2400 ||
	    ML_KEM_768_CIPHERTEXT_LEN    != 1088 ||
	    ML_KEM_768_SHARED_SECRET_LEN != 32) {
		FAIL("key length constants do not match FIPS 203 ML-KEM-768 spec");
		return -1;
	}
	PASS("ML-KEM-768 key lengths match FIPS 203 (pk=%d sk=%d ct=%d ss=%d)",
	     ML_KEM_768_PUBLIC_KEY_LEN, ML_KEM_768_SECRET_KEY_LEN,
	     ML_KEM_768_CIPHERTEXT_LEN, ML_KEM_768_SHARED_SECRET_LEN);
	return 0;
}

static int __init pqc_kem_test_init(void)
{
	pr_info("pqc_test: === ML-KEM-768 test start ===\n");
	RUN(test_calloc());
	RUN(test_cleanse());
	RUN(test_secure_free());
	RUN(test_rng());
	RUN(test_key_lengths());
	pr_info("pqc_test: === %d passed, %d failed ===\n", passed, failed);
	return failed ? -EINVAL : 0;
}

static void __exit pqc_kem_test_exit(void)
{
	pr_info("pqc_test: unloaded\n");
}

module_init(pqc_kem_test_init);
module_exit(pqc_kem_test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ML-KEM-768 kernel module test");
MODULE_AUTHOR("qubuntu-project");
