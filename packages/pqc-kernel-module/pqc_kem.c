// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ML-KEM-768 Implementation - Minimal
 */

#include <linux/module.h>
#include <linux/crypto.h>
#include <linux/string.h>
#include <crypto/algapi.h>
#include <crypto/pqc.h>

#define ML_KEM_768_PUBLIC_KEY_LEN   1184
#define ML_KEM_768_SECRET_KEY_LEN   2400
#define ML_KEM_768_CIPHERTEXT_LEN   1088
#define ML_KEM_768_SHARED_SECRET_LEN 32

struct mlkem768_ctx {
	u8 public_key[ML_KEM_768_PUBLIC_KEY_LEN];
	u8 secret_key[ML_KEM_768_SECRET_KEY_LEN];
};

static int mlkem768_init_tfm(struct crypto_tfm *tfm)
{
	struct crypto_pqc_kem *pqc = __crypto_pqc_kem_tfm(tfm);
	struct mlkem768_ctx *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	pqc->pqc_ctx = ctx;
	pqc->public_key_len = ML_KEM_768_PUBLIC_KEY_LEN;
	pqc->secret_key_len = ML_KEM_768_SECRET_KEY_LEN;
	pqc->ciphertext_len = ML_KEM_768_CIPHERTEXT_LEN;
	pqc->shared_secret_len = ML_KEM_768_SHARED_SECRET_LEN;

	pr_info("ML-KEM-768 initialized\n");
	return 0;
}

static void mlkem768_exit_tfm(struct crypto_tfm *tfm)
{
	struct crypto_pqc_kem *pqc = __crypto_pqc_kem_tfm(tfm);
	struct mlkem768_ctx *ctx = pqc->pqc_ctx;

	if (ctx) {
		memzero_explicit(ctx, sizeof(*ctx));
		kfree(ctx);
	}
}

static struct crypto_alg mlkem768_alg = {
	.cra_name = "ml-kem-768",
	.cra_driver_name = "ml-kem-768-generic",
	.cra_priority = 100,
	.cra_flags = CRYPTO_ALG_TYPE_PQC_KEM,
	.cra_blocksize = 1,
	.cra_ctxsize = sizeof(struct mlkem768_ctx),
	.cra_module = THIS_MODULE,
	.cra_init = mlkem768_init_tfm,
	.cra_exit = mlkem768_exit_tfm,
};

int crypto_register_pqc_kem(struct crypto_alg *alg)
{
	return crypto_register_alg(alg);
}
EXPORT_SYMBOL_GPL(crypto_register_pqc_kem);

void crypto_unregister_pqc_kem(struct crypto_alg *alg)
{
	crypto_unregister_alg(alg);
}
EXPORT_SYMBOL_GPL(crypto_unregister_pqc_kem);

static int __init pqc_kem_init(void)
{
	int ret = crypto_register_pqc_kem(&mlkem768_alg);
	if (ret)
		pr_err("Failed to register ML-KEM-768: %d\n", ret);
	else
		pr_info("ML-KEM-768 registered\n");
	return ret;
}

static void __exit pqc_kem_exit(void)
{
	crypto_unregister_pqc_kem(&mlkem768_alg);
	pr_info("ML-KEM-768 unregistered\n");
}

module_init(pqc_kem_init);
module_exit(pqc_kem_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ML-KEM-768 Post-Quantum KEM");
MODULE_ALIAS_CRYPTO("ml-kem-768");
