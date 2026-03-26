/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _CRYPTO_PQC_H
#define _CRYPTO_PQC_H

#include <linux/crypto.h>
#include <linux/types.h>

#define CRYPTO_ALG_TYPE_PQC_KEM 0x00000010

struct crypto_pqc_kem {
	struct crypto_tfm base;
	unsigned int public_key_len;
	unsigned int secret_key_len;
	unsigned int ciphertext_len;
	unsigned int shared_secret_len;
	void *pqc_ctx;
};

static inline struct crypto_pqc_kem *__crypto_pqc_kem_tfm(struct crypto_tfm *tfm)
{
	return container_of(tfm, struct crypto_pqc_kem, base);
}

int crypto_register_pqc_kem(struct crypto_alg *alg);
void crypto_unregister_pqc_kem(struct crypto_alg *alg);

#endif
