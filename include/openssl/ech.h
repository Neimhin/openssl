/*
 * Copyright 2024 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * This has the externally-visible data structures and prototypes
 * for handling Encrypted ClientHello (ECH)
 * See the documentation in SSL_ech_set1_echconfig.pod
 */
#ifndef OPENSSL_ECH_H
# define OPENSSL_ECH_H
# pragma once

# include <openssl/ssl.h>
# include <openssl/hpke.h>
# include <openssl/evp.h>

# ifndef OPENSSL_NO_ECH

/*
 * Some externally visible limits - most used for sanity checks that could be
 * bigger if needed, but that work for now
 */
#  define OSSL_ECH_MAX_PAYLOAD_LEN 1500 /* max ECH ciphertext to en/decode */
#  define OSSL_ECH_MIN_ECHCONFIG_LEN 32 /* min for all encodings */
#  define OSSL_ECH_MAX_ECHCONFIG_LEN 1500 /* max for all encodings */
#  define OSSL_ECH_MAX_ECHCONFIGEXT_LEN 512 /* ECHConfig extension max */
#  define OSSL_ECH_MAX_MAXNAMELEN 255 /* ECHConfig max for max name length */
#  define OSSL_ECH_MAX_PUBLICNAME 255 /* max ECHConfig public name length */
#  define OSSL_ECH_MAX_ALPNLEN 255 /* max alpn length */
#  define OSSL_ECH_OUTERS_MAX 20 /* max extensions we compress via outer-exts */
#  define OSSL_ECH_ALLEXTS_MAX 32 /* max total number of extension we allow */

/*
 * ECH version. We only support RFC XXXX as of now.  As/if new ECHConfig
 * versions are added, those will be noted here.
 * TODO: Replace XXXX with the actual RFC number once known.
 */
#  define OSSL_ECH_RFCXXXX_VERSION 0xfe0d /* official ECHConfig version */
/* latest version from an RFC */
#  define OSSL_ECH_CURRENT_VERSION OSSL_ECH_RFCXXXX_VERSION

/* Return codes from SSL_ech_get_status */
#  define SSL_ECH_STATUS_BACKEND    4 /* ECH backend: saw an ech_is_inner */
#  define SSL_ECH_STATUS_GREASE_ECH 3 /* GREASEd and got an ECH in return */
#  define SSL_ECH_STATUS_GREASE     2 /* ECH GREASE happened  */
#  define SSL_ECH_STATUS_SUCCESS    1 /* Success */
#  define SSL_ECH_STATUS_FAILED     0 /* Some internal or protocol error */
#  define SSL_ECH_STATUS_BAD_CALL   -100 /* Some in/out arguments were NULL */
#  define SSL_ECH_STATUS_NOT_TRIED  -101 /* ECH wasn't attempted  */
#  define SSL_ECH_STATUS_BAD_NAME   -102 /* ECH ok but server cert bad */
#  define SSL_ECH_STATUS_NOT_CONFIGURED -103 /* ECH wasn't configured */
#  define SSL_ECH_STATUS_FAILED_ECH -105 /* We tried, failed and got an ECH, from a good name */
#  define SSL_ECH_STATUS_FAILED_ECH_BAD_NAME -106 /* We tried, failed and got an ECH, from a bad name */

#  define SSL_SECH_STATUS_SUCCESS   1 /* tried and succeeded with stealthy ECH, either on server or client */
#  define SSL_SECH_STATUS_FAILED    0 /* generic failure of attempted SECH, either on server or client */

/*
 * Define this if you want to allow a test where we inject an ECH
 * extension into a TLSv1.2 client hello message via the custom
 * extensions handler in order to test that doesn't break anything.
 * Without this, we won't allow adding an ECH via the custom exts
 * API, as ECH is a standard supported extension.
 * There is a test case in test/ech_test.c that is run when this is
 * defined, but not otherwise. The code to allow injection is in
 * ssl/statem/extension_cust.c.
 * This should probably be defined elsewhere, in some header that's
 * included in both test/ech_test.c and ssl/statem/extension_cust.c
 * but I'm not sure where that'd be so here will do for now. Or maybe
 * there's a better way to do this test.
 */
#  define OPENSSL_ECH_ALLOW_CUST_INJECT

/*
 * Application-visible form of ECH information from the DNS, from config
 * files, or from earlier API calls. APIs produce/process an array of these.
 */
typedef struct ossl_ech_info_st {
    int index; /* externally re-usable reference to this value */
    char *public_name; /* public_name from API or ECHConfig */
    char *inner_name; /* server-name (for inner CH if doing ECH) */
    char *outer_alpns; /* outer ALPN string */
    char *inner_alpns; /* inner ALPN string */
    char *echconfig; /* a JSON-like version of the associated ECHConfig */
} OSSL_ECH_INFO;

/* Values for the for_retry inputs */
#  define SSL_ECH_USE_FOR_RETRY 1
#  define SSL_ECH_NOT_FOR_RETRY 0

/*
 * API calls based around SSL* values - mostly for clients
 */
int SSL_ech_set1_echconfig(SSL *ssl, const unsigned char *val, size_t len);
int SSL_CTX_ech_set1_echconfig(SSL_CTX *ctx, const unsigned char *val,
                               size_t len);

int SSL_CTX_set_sech_version(SSL_CTX *ctx, int version);
int SSL_CTX_set_sech_symmetric_key(SSL_CTX *ctx, const char * key, size_t key_len);
int SSL_CTX_set_sech_inner_servername(SSL_CTX *ctx, char* inner_servername, int inner_servername_len);
int SSL_get_sech_status(SSL *s, char **inner_sni, char **outer_sni);

int SSL_ech_set_server_names(SSL *s, const char *inner_name,
                             const char *outer_name, int no_outer);
/* TODO: Add back SSL_ech_set_outer_alpn_protos() */

int SSL_ech_set_outer_server_name(SSL *s, const char *outer_name, int no_outer);

void OSSL_ECH_INFO_free(OSSL_ECH_INFO *info, int count);
int OSSL_ECH_INFO_print(BIO *out, OSSL_ECH_INFO *info, int count);
int SSL_ech_get_info(SSL *s, OSSL_ECH_INFO **info, int *count);
int SSL_ech_reduce(SSL *s, int index);

int SSL_ech_get_status(SSL *s, char **inner_sni, char **outer_sni);

int SSL_ech_set_grease_suite(SSL *s, const char *suite);
int SSL_ech_set_grease_type(SSL *s, uint16_t type);

typedef unsigned int (*SSL_ech_cb_func)(SSL *s, const char *str);
void SSL_ech_set_callback(SSL *s, SSL_ech_cb_func f);

int SSL_ech_get_retry_config(SSL *s, unsigned char **ec, size_t *eclen);

/* API calls based around SSL_CTX* values - mostly for servers */

int SSL_CTX_ech_set_outer_alpn_protos(SSL_CTX *s, const unsigned char *protos,
                                      const size_t protos_len);

int SSL_CTX_ech_server_enable_file(SSL_CTX *ctx, const char *file,
                                   int for_retry);
int SSL_CTX_ech_server_enable_buffer(SSL_CTX *ctx, const unsigned char *buf,
                                     const size_t blen, int for_retry);
int SSL_CTX_ech_server_enable_dir(SSL_CTX *ctx, int *loaded,
                                  const char *echdir, int for_retry);
int SSL_CTX_ech_server_get_key_status(SSL_CTX *ctx, int *numkeys);
int SSL_CTX_ech_server_flush_keys(SSL_CTX *ctx, unsigned int age);

int SSL_CTX_ech_raw_decrypt(SSL_CTX *ctx,
                            int *decrypted_ok,
                            char **inner_sni, char **outer_sni,
                            unsigned char *outer_ch, size_t outer_len,
                            unsigned char *inner_ch, size_t *inner_len,
                            unsigned char **hrrtok, size_t *toklen);
void SSL_CTX_ech_set_callback(SSL_CTX *ctx, SSL_ech_cb_func f);

/* Misc API calls */
int OSSL_ech_make_echconfig(unsigned char *echconfig, size_t *echconfiglen,
                            unsigned char *priv, size_t *privlen,
                            uint16_t ekversion, uint16_t max_name_length,
                            const char *public_name, OSSL_HPKE_SUITE suite,
                            const unsigned char *extvals, size_t extlen);

int OSSL_ech_find_echconfigs(int *num_echs,
                             unsigned char ***echconfigs, size_t **echlens,
                             const unsigned char *val, size_t len);

# endif
#endif
