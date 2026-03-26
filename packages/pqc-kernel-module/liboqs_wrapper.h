/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LIBOQS_WRAPPER_H
#define _LIBOQS_WRAPPER_H

#include <linux/types.h>

void *OQS_MEM_malloc(size_t size);
void *OQS_MEM_calloc(size_t nmemb, size_t size);
void OQS_MEM_free(void *ptr);
void OQS_MEM_secure_free(void *ptr, size_t len);
void OQS_MEM_cleanse(void *ptr, size_t len);
int OQS_randombytes(uint8_t *random_array, size_t bytes_to_read);

/* ML-KEM-768 key length constants (FIPS 203) */
#define ML_KEM_768_PUBLIC_KEY_LEN    1184
#define ML_KEM_768_SECRET_KEY_LEN    2400
#define ML_KEM_768_CIPHERTEXT_LEN    1088
#define ML_KEM_768_SHARED_SECRET_LEN 32

#endif
