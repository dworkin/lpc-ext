/*
 * crypto kfun extensions
 *
 * This code is released into the public domain.
 */

# include <openssl/evp.h>
# include <openssl/rand.h>
# include <openssl/x509v3.h>
# include <openssl/x509_vfy.h>
# include <openssl/bn.h>
# include <openssl/err.h>
# include <string.h>
# include <alloca.h>
# include <stdint.h>
# include "lpc_ext.h"


static const EVP_MD *md_md5, *md_sha1, *md_sha224, *md_sha256, *md_sha384,
		    *md_sha512;
static const EVP_CIPHER *cipher_aes_128_gcm, *cipher_aes_256_gcm,
			*cipher_chacha20_poly1305, *cipher_aes_128_ccm;

/*
 * hash a string
 */
static void hash(const EVP_MD *md, LPC_frame f, int nargs, LPC_value retval)
{
    EVP_MD_CTX *ctx;
    unsigned char digest[EVP_MAX_MD_SIZE];
    int i;
    unsigned int length;
    LPC_string str;

    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md, NULL);

    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	EVP_DigestUpdate(ctx, lpc_string_text(str), lpc_string_length(str));
    }

    EVP_DigestFinal_ex(ctx, digest, &length);
    EVP_MD_CTX_free(ctx);

    /* make a digest string */
    str = lpc_string_new(lpc_frame_dataspace(f), (char *) digest, (int) length);

    /* put result in return value */
    lpc_string_putval(retval, str);
}

/*
 * produce MD5 hash
 */
static void md5(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_runtime_check(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_runtime_check(f, lpc_string_length(str));
    }
    hash(md_md5, f, nargs, retval);
}

/*
 * produce SHA1 hash
 */
static void sha1(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_runtime_check(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_runtime_check(f, lpc_string_length(str));
    }
    hash(md_sha1, f, nargs, retval);
}

/*
 * produce SHA224 hash
 */
static void sha224(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_runtime_check(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_runtime_check(f, lpc_string_length(str));
    }
    hash(md_sha224, f, nargs, retval);
}

/*
 * produce SHA256 hash
 */
static void sha256(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_runtime_check(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_runtime_check(f, lpc_string_length(str));
    }
    hash(md_sha256, f, nargs, retval);
}

/*
 * produce SHA384 hash
 */
static void sha384(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_runtime_check(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_runtime_check(f, lpc_string_length(str));
    }
    hash(md_sha384, f, nargs, retval);
}

/*
 * produce SHA512 hash
 */
static void sha512(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_runtime_check(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_runtime_check(f, lpc_string_length(str));
    }
    hash(md_sha512, f, nargs, retval);
}

/*
 * create a string of securely random bytes
 */
static void secure_random(LPC_frame f, int nargs, LPC_value retval)
{
    char buffer[65535];
    LPC_int n;
    LPC_string str;

    n = lpc_int_getval(lpc_frame_arg(f, nargs, 0));
    if (n <= 0 || n > sizeof(buffer)) {
	lpc_runtime_error(f, "Invalid number of random bytes");
    }
    lpc_runtime_check(f, 3 * n);
    if (RAND_bytes((unsigned char *) buffer, n) <= 0) {
	ERR_error_string(ERR_get_error(), buffer);
	lpc_runtime_error(f, buffer);
    }

    str = lpc_string_new(lpc_frame_dataspace(f), buffer, n);
    lpc_string_putval(retval, str);
}

/*
 * failure_reason = verify_certificate(purpose, certificate, intermediates...)
 */
static void verify_certificate(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_string str;
    int purpose, i;
    const unsigned char *p;
    X509 *certificate, *intermediate;
    STACK_OF(X509) *intermediates;
    X509_STORE_CTX *context;
    X509_STORE *store;

    /* purpose */
    str = lpc_string_getval(lpc_frame_arg(f, nargs, 0));
    if (lpc_string_length(str) != 10) {
	lpc_runtime_error(f, "Invalid purpose");
    }
    if (strcmp(lpc_string_text(str), "TLS server") == 0) {
	purpose = X509_PURPOSE_SSL_SERVER;
    } else if (strcmp(lpc_string_text(str), "TLS client") == 0) {
	purpose = X509_PURPOSE_SSL_CLIENT;
    } else {
	lpc_runtime_error(f, "Invalid purpose");
    }
    lpc_runtime_check(f, 4096 * (nargs - 1));

    /* target certificate */
    str = lpc_string_getval(lpc_frame_arg(f, nargs, 1));
    p = (const unsigned char *) lpc_string_text(str);
    certificate = d2i_X509(NULL, &p, lpc_string_length(str));
    if (certificate == NULL) {
	ERR_clear_error();
	lpc_runtime_error(f, "Bad certificate");
    }

    /* intermediate certificates */
    intermediates = sk_X509_new_reserve(NULL, 1);
    if (intermediates == NULL) {
	ERR_clear_error();
	X509_free(certificate);
	lpc_runtime_error(f, "SSL stack error");
    }
    for (i = 2; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	p = (const unsigned char *) lpc_string_text(str);
	intermediate = d2i_X509(NULL, &p, lpc_string_length(str));
	if (intermediate == NULL) {
	    ERR_clear_error();
	    sk_X509_pop_free(intermediates, X509_free);
	    X509_free(certificate);
	    lpc_runtime_error(f, "Bad intermediate certificate");
	}
	sk_X509_push(intermediates, intermediate);
    }

    /* prepare context */
    store = X509_STORE_new();
    context = X509_STORE_CTX_new();
    if (store == NULL || context == NULL ||
	X509_STORE_set_default_paths(store) <= 0 ||
	X509_STORE_CTX_init(context, store, certificate, intermediates) <= 0 ||
	X509_STORE_CTX_set_purpose(context, purpose) <= 0) {
	ERR_clear_error();
	if (context != NULL) {
	    X509_STORE_CTX_free(context);
	}
	if (store != NULL) {
	    X509_STORE_free(store);
	}
	sk_X509_pop_free(intermediates, X509_free);
	X509_free(certificate);
	lpc_runtime_error(f, "SSL context error");
    }

    /* verify */
    i = X509_verify_cert(context);
    if (i <= 0) {
	int code;
	const char *error;

	code = X509_STORE_CTX_get_error(context);
	error = X509_verify_cert_error_string(code);
	if (i < 0) {
	    ERR_clear_error();
	    X509_STORE_CTX_free(context);
	    X509_STORE_free(store);
	    sk_X509_pop_free(intermediates, X509_free);
	    X509_free(certificate);
	    lpc_runtime_error(f, error);
	}
	lpc_string_putval(retval, lpc_string_new(lpc_frame_dataspace(f),
						 error, strlen(error)));
    }

    /* cleanup */
    X509_STORE_CTX_free(context);
    X509_STORE_free(store);
    sk_X509_pop_free(intermediates, X509_free);
    X509_free(certificate);
}

/*
 * masked = mask_xor(mask, message)
 */
void mask_xor(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_string str;
    unsigned char *data, *masked, *mdata;
    uint64_t mask;
    unsigned int len;

    lpc_runtime_check(f, 5);
    str = lpc_string_getval(lpc_frame_arg(f, 2, 0));
    data = lpc_string_text(str);
    switch (lpc_string_length(str)) {
    case 1:
	mask = data[0];
	mask |= mask << 8;
	mask |= mask << 16;
	mask |= mask << 32;
	break;

    case 2:
	mask = *(uint16_t *) data;
	mask |= mask << 16;
	mask |= mask << 32;
	break;

    case 4:
	mask = *(uint32_t *) data;
	mask |= mask << 32;
	break;

    case 8:
	mask = *(uint64_t *) data;
	break;

    default:
	lpc_runtime_error(f, "Bad mask");
    }

    str = lpc_string_getval(lpc_frame_arg(f, 2, 1));
    len = lpc_string_length(str);
    data = lpc_string_text(str);
    str = lpc_string_new(lpc_frame_dataspace(f), NULL, len);
    masked = lpc_string_text(str);
    while (len >= 8) {
	*(uint64_t *) masked = *(uint64_t *) data ^ mask;
	masked += 8;
	data += 8;
	len -= 8;
    }
    for (mdata = (unsigned char *) &mask; len > 0; --len) {
	*masked++ = *data++ ^ *mdata++;
    }

    lpc_string_putval(retval, str);
}

/*
 * encrypt using AES-GCM
 */
static void encrypt_aes_gcm(LPC_frame f, int nargs, LPC_value retval,
			    const EVP_CIPHER *cipher, int nbytes)
{
    LPC_value arg;
    LPC_string key, str, iv, aad;
    LPC_int taglen;
    int length, ivlen, aadlen, len;
    unsigned char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    if (nargs != 5) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    arg = lpc_frame_arg(f, 5, 0);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun encrypt");
    }
    key = lpc_string_getval(lpc_frame_arg(f, 5, 0));
    if (lpc_string_length(key) != nbytes) {
	lpc_runtime_error(f, "Bad key");
    }
    arg = lpc_frame_arg(f, 5, 1);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun encrypt");
    }
    iv = lpc_string_getval(arg);
    ivlen = lpc_string_length(iv);
    if (ivlen == 0 || ivlen > 16) {
	lpc_runtime_error(f, "Bad IV");
    }
    arg = lpc_frame_arg(f, 5, 2);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 4 for kfun encrypt");
    }
    aad = lpc_string_getval(arg);
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    arg = lpc_frame_arg(f, 5, 3);
    if (lpc_value_type(arg) != LPC_TYPE_INT) {
	lpc_runtime_error(f, "Bad argument 5 for kfun encrypt");
    }
    taglen = lpc_int_getval(arg);
    if (taglen <= 0 || taglen > 16) {
	lpc_runtime_error(f, "Bad tag length");
    }
    arg = lpc_frame_arg(f, 5, 4);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 6 for kfun encrypt");
    }
    str = lpc_string_getval(arg);
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad plaintext");
    }
    plaintext = (unsigned char *) lpc_string_text(str);
    lpc_runtime_check(f, lpc_string_length(str));

    len = length;
    length += taglen;
    ciphertext = alloca(length);
    tag = ciphertext + len;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, ivlen, NULL) <= 0 ||
	EVP_EncryptInit_ex(ctx, NULL, NULL,
			   (unsigned char *) lpc_string_text(key),
			   (unsigned char *) lpc_string_text(iv)) <= 0 ||
	EVP_EncryptUpdate(ctx, NULL, &aadlen,
			  (unsigned char *) lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, len) <= 0 ||
	EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, (int) taglen, tag) <= 0)
    {
	ERR_clear_error();
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Encryption failed");
    }
    EVP_CIPHER_CTX_free(ctx);

    str = lpc_string_new(lpc_frame_dataspace(f), (char *) ciphertext, length);
    lpc_string_putval(retval, str);
}

/*
 * ciphertext = encrypt("AES-128-GCM", key, iv, aad, taglen, plaintext)
 */
static void encrypt_aes_128_gcm(LPC_frame f, int nargs, LPC_value retval)
{
    encrypt_aes_gcm(f, nargs, retval, cipher_aes_128_gcm, 16);
}

/*
 * ciphertext = encrypt("AES-256-GCM", key, iv, aad, taglen, plaintext)
 */
static void encrypt_aes_256_gcm(LPC_frame f, int nargs, LPC_value retval)
{
    encrypt_aes_gcm(f, nargs, retval, cipher_aes_256_gcm, 32);
}

/*
 * ciphertext = encrypt("ChaCha20-Poly1305", key, iv, aad, taglen, plaintext)
 */
static void encrypt_chacha20_poly1305(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_value arg;
    LPC_string key, str, iv, aad;
    LPC_int taglen;
    int length, ivlen, aadlen, len;
    unsigned char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    if (nargs != 5) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    arg = lpc_frame_arg(f, 5, 0);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun encrypt");
    }
    key = lpc_string_getval(arg);
    if (lpc_string_length(key) != 32) {
	lpc_runtime_error(f, "Bad key");
    }
    arg = lpc_frame_arg(f, 5, 1);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun encrypt");
    }
    iv = lpc_string_getval(arg);
    ivlen = lpc_string_length(iv);
    if (ivlen == 0 || ivlen > 12) {
	lpc_runtime_error(f, "Bad IV");
    }
    arg = lpc_frame_arg(f, 5, 2);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 4 for kfun encrypt");
    }
    aad = lpc_string_getval(arg);
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    arg = lpc_frame_arg(f, 5, 3);
    if (lpc_value_type(arg) != LPC_TYPE_INT) {
	lpc_runtime_error(f, "Bad argument 5 for kfun encrypt");
    }
    taglen = lpc_int_getval(arg);
    if (taglen <= 0 || taglen > 16) {
	lpc_runtime_error(f, "Bag tag length");
    }
    arg = lpc_frame_arg(f, 5, 4);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 6 for kfun encrypt");
    }
    str = lpc_string_getval(arg);
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad plaintext");
    }
    plaintext = (unsigned char *) lpc_string_text(str);
    lpc_runtime_check(f, 3 * lpc_string_length(str));

    len = length;
    length += taglen;
    ciphertext = alloca(length);
    tag = ciphertext + len;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_EncryptInit_ex(ctx, cipher_chacha20_poly1305, NULL, NULL,
			   NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, ivlen, NULL) <= 0 ||
	EVP_EncryptInit_ex(ctx, NULL, NULL,
			   (unsigned char *) lpc_string_text(key),
			   (unsigned char *) lpc_string_text(iv)) <= 0 ||
	EVP_EncryptUpdate(ctx, NULL, &aadlen,
			  (unsigned char *) lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, len) <= 0 ||
	EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, (int) taglen, tag) <= 0)
    {
	ERR_clear_error();
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Encryption failed");
    }
    EVP_CIPHER_CTX_free(ctx);

    str = lpc_string_new(lpc_frame_dataspace(f), (char *) ciphertext, length);
    lpc_string_putval(retval, str);
}

/*
 * ciphertext = encrypt("AES-128-CCM", key, iv, aad, taglen, plaintext)
 */
static void encrypt_aes_128_ccm(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_value arg;
    LPC_string key, str, iv, aad;
    LPC_int taglen;
    int length, ivlen, aadlen, len;
    unsigned char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    if (nargs != 5) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    arg = lpc_frame_arg(f, 5, 0);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun encrypt");
    }
    key = lpc_string_getval(arg);
    if (lpc_string_length(key) != 16) {
	lpc_runtime_error(f, "Bad key");
    }
    arg = lpc_frame_arg(f, 5, 1);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun encrypt");
    }
    iv = lpc_string_getval(arg);
    ivlen = lpc_string_length(iv);
    if (ivlen < 7 || ivlen > 13) {
	lpc_runtime_error(f, "Bad IV");
    }
    arg = lpc_frame_arg(f, 5, 2);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 4 for kfun encrypt");
    }
    aad = lpc_string_getval(arg);
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    arg = lpc_frame_arg(f, 5, 3);
    if (lpc_value_type(arg) != LPC_TYPE_INT) {
	lpc_runtime_error(f, "Bad argument 5 for kfun encrypt");
    }
    taglen = lpc_int_getval(arg);
    if (taglen < 4 || taglen > 16 || (taglen & 1)) {
	lpc_runtime_error(f, "Bad tag length");
    }
    arg = lpc_frame_arg(f, 5, 4);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 6 for kfun encrypt");
    }
    str = lpc_string_getval(arg);
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad plaintext");
    }
    plaintext = (unsigned char *) lpc_string_text(str);
    lpc_runtime_check(f, lpc_string_length(str));

    len = length;
    length += taglen;
    ciphertext = alloca(length);
    tag = ciphertext + len;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_EncryptInit_ex(ctx, cipher_aes_128_ccm, NULL, NULL, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, ivlen, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, taglen, NULL) <= 0 ||
	EVP_EncryptInit_ex(ctx, NULL, NULL,
			   (unsigned char *) lpc_string_text(key),
			   (unsigned char *) lpc_string_text(iv)) <= 0 ||
	EVP_EncryptUpdate(ctx, NULL, &len, NULL, len) <= 0 ||
	EVP_EncryptUpdate(ctx, NULL, &aadlen,
			  (unsigned char *) lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, len) <= 0 ||
	EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_GET_TAG, (int) taglen, tag) <= 0)
    {
	ERR_clear_error();
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Encryption failed");
    }
    EVP_CIPHER_CTX_free(ctx);

    str = lpc_string_new(lpc_frame_dataspace(f), (char *) ciphertext, length);
    lpc_string_putval(retval, str);
}

/*
 * EC key generation
 */
static void ec_key(LPC_frame f, int nargs, int nid, int len, LPC_value retval)
{
    EVP_PKEY_CTX *context;
    EVP_PKEY *key;
    const EC_KEY *ec;
    unsigned char *p;
    size_t size;
    const BIGNUM *priv;
    LPC_dataspace data;
    LPC_array a;
    LPC_value val;
    LPC_string str;

    if (nargs != 0) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    lpc_runtime_check(f, 12 * len);

    /* create EC public and private key */
    context = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    key = NULL;
    if (context == NULL || EVP_PKEY_keygen_init(context) <= 0 ||
	EVP_PKEY_CTX_set_ec_paramgen_curve_nid(context, nid) <= 0 ||
	EVP_PKEY_keygen(context, &key) <= 0 ||
	(ec=EVP_PKEY_get0_EC_KEY(key)) == NULL) {
	ERR_clear_error();
	if (key != NULL) {
	    EVP_PKEY_free(key);
	}
	if (context != NULL) {
	    EVP_PKEY_CTX_free(context);
	}
	lpc_runtime_error(f, "Key generation failed");
    }

    /* obtain public and private key */
    size = EC_KEY_key2buf(ec, POINT_CONVERSION_UNCOMPRESSED, &p, NULL);
    priv = EC_KEY_get0_private_key(ec);

    /* store public and private key in LPC array */
    data = lpc_frame_dataspace(f);
    a = lpc_array_new(data, 2);
    val = lpc_value_temp(data);
    str = lpc_string_new(data, p, size);
    lpc_string_putval(val, str);
    lpc_array_assign(data, a, 0, val);
    str = lpc_string_new(data, NULL, BN_num_bytes(priv));
    BN_bn2bin(priv, lpc_string_text(str));
    lpc_string_putval(val, str);
    lpc_array_assign(data, a, 1, val);

    OPENSSL_free(p);
    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(context);

    /* return ({ public, private }) */
    lpc_array_putval(retval, a);
}

/*
 * ({ public, privkey }) = encrypt("SECP256R1 key")
 */
static void secp256r1_key(LPC_frame f, int nargs, LPC_value retval)
{
    ec_key(f, nargs, NID_X9_62_prime256v1, 32, retval);
}

/*
 * ({ public, privkey }) = encrypt("SECP384R1 key")
 */
static void secp384r1_key(LPC_frame f, int nargs, LPC_value retval)
{
    ec_key(f, nargs, NID_secp384r1, 48, retval);
}

/*
 * ({ public, privkey }) = encrypt("SECP521R1 key")
 */
static void secp521r1_key(LPC_frame f, int nargs, LPC_value retval)
{
    ec_key(f, nargs, NID_secp521r1, 64, retval);
}


/*
 * ECX key generation
 */
static void ecx_key(LPC_frame f, int nargs, int id, int len, LPC_value retval)
{
    EVP_PKEY_CTX *context;
    EVP_PKEY *key;
    LPC_dataspace data;
    LPC_array a;
    LPC_value val;
    LPC_string str;
    size_t size;

    if (nargs != 0) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    lpc_runtime_check(f, 8 * len);

    /* create ECX public and private key */
    context = EVP_PKEY_CTX_new_id(id, NULL);
    key = NULL;
    if (context == NULL || EVP_PKEY_keygen_init(context) <= 0 ||
	EVP_PKEY_keygen(context, &key) <= 0) {
	ERR_clear_error();
	if (key != NULL) {
	    EVP_PKEY_free(key);
	}
	if (context != NULL) {
	    EVP_PKEY_CTX_free(context);
	}
	lpc_runtime_error(f, "Key generation failed");
    }

    /* store public and private key in LPC array */
    data = lpc_frame_dataspace(f);
    a = lpc_array_new(data, 2);
    val = lpc_value_temp(data);
    str = lpc_string_new(data, NULL, len);
    size = len;
    EVP_PKEY_get_raw_public_key(key, lpc_string_text(str), &size);
    lpc_string_putval(val, str);
    lpc_array_assign(data, a, 0, val);
    str = lpc_string_new(data, NULL, len);
    EVP_PKEY_get_raw_private_key(key, lpc_string_text(str), &size);
    lpc_string_putval(val, str);
    lpc_array_assign(data, a, 1, val);

    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(context);

    /* return ({ public, private }) */
    lpc_array_putval(retval, a);
}

/*
 * ({ public, private }) = encrypt("X25519 key")
 */
static void x25519_key(LPC_frame f, int nargs, LPC_value retval)
{
    ecx_key(f, nargs, EVP_PKEY_X25519, 32, retval);
}

/*
 * ({ public, private }) = encrypt("X448 key")
 */
static void x448_key(LPC_frame f, int nargs, LPC_value retval)
{
    ecx_key(f, nargs, EVP_PKEY_X448, 56, retval);
}

/*
 * EC sign
 */
static void ec_sign(LPC_frame f, int nargs, int nid, const EVP_MD *md, int len,
		    LPC_value retval)
{
    unsigned char buffer[1024];
    LPC_value val;
    LPC_string priv, message, signature;
    BIGNUM *bn;
    EC_KEY *ec;
    EVP_PKEY *key;
    EVP_MD_CTX *context;
    size_t size;

    /* retrieve arguments */
    if (nargs != 2) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    val = lpc_frame_arg(f, nargs, 0);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun encrypt");
    }
    priv = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 1);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun encrypt");
    }
    message = lpc_string_getval(val);
    lpc_runtime_check(f, 16 * len);

    /* set up private key */
    bn = BN_bin2bn(lpc_string_text(priv), lpc_string_length(priv), NULL);
    ec = NULL;
    key = NULL;
    if (bn == NULL ||
	(ec=EC_KEY_new_by_curve_name(nid)) == NULL ||
	EC_KEY_set_private_key(ec, bn) <= 0 || (key=EVP_PKEY_new()) == NULL ||
	EVP_PKEY_assign_EC_KEY(key, ec) <= 0) {
	ERR_clear_error();
	if (key != NULL) {
	    EVP_PKEY_free(key);
	}
	if (ec != NULL) {
	    EC_KEY_free(ec);
	}
	if (bn != NULL) {
	    BN_free(bn);
	}
	lpc_runtime_error(f, "Sign key failed");
    }
    BN_free(bn);

    /* produce signature */
    size = sizeof(buffer);
    context = EVP_MD_CTX_new();
    if (context == NULL ||
	EVP_DigestSignInit(context, NULL, md, NULL, key) <= 0 ||
	EVP_DigestSign(context, buffer, &size, lpc_string_text(message),
		       lpc_string_length(message)) <= 0) {
	ERR_clear_error();
	if (context != NULL) {
	    EVP_MD_CTX_free(context);
	}
	lpc_runtime_error(f, "Signature failed");
    }
    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);

    signature = lpc_string_new(lpc_frame_dataspace(f), buffer, size);
    lpc_string_putval(retval, signature);
}

/*
 * signature = encrypt("ECDSA-SECP256R1-SHA256 sign", privkey, message)
 */
static void secp256r1_sign(LPC_frame f, int nargs, LPC_value retval)
{
    ec_sign(f, nargs, NID_X9_62_prime256v1, md_sha256, 32, retval);
}

/*
 * signature = encrypt("ECDSA-SECP384R1-SHA384 sign", privkey, message)
 */
static void secp384r1_sign(LPC_frame f, int nargs, LPC_value retval)
{
    ec_sign(f, nargs, NID_secp384r1, md_sha384, 48, retval);
}

/*
 * signature = encrypt("ECDSA-SECP521R1-SHA521 sign", privkey, message)
 */
static void secp521r1_sign(LPC_frame f, int nargs, LPC_value retval)
{
    ec_sign(f, nargs, NID_secp521r1, md_sha512, 64, retval);
}

/*
 * ECX sign
 */
static void ecx_sign(LPC_frame f, int nargs, int id, int len, LPC_value retval)
{
    unsigned char buffer[1024];
    LPC_value val;
    LPC_string priv, message, signature;
    EVP_PKEY *key;
    EVP_MD_CTX *context;
    size_t size;

    /* retrieve arguments */
    if (nargs != 2) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    val = lpc_frame_arg(f, nargs, 0);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun encrypt");
    }
    priv = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 1);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun encrypt");
    }
    message = lpc_string_getval(val);
    lpc_runtime_check(f, 8 * len);

    /* set up private key */
    key = EVP_PKEY_new_raw_private_key(id, NULL, lpc_string_text(priv),
				       lpc_string_length(priv));
    if (key == NULL) {
	ERR_clear_error();
	lpc_runtime_error(f, "Sign key failed");
    }

    /* produce signature */
    size = sizeof(buffer);
    context = EVP_MD_CTX_new();
    if (context == NULL ||
	EVP_DigestSignInit(context, NULL, NULL, NULL, key) <= 0 ||
	EVP_DigestSign(context, buffer, &size, lpc_string_text(message),
		       lpc_string_length(message)) <= 0) {
	ERR_clear_error();
	if (context != NULL) {
	    EVP_MD_CTX_free(context);
	}
	lpc_runtime_error(f, "Signature failed");
    }
    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);

    signature = lpc_string_new(lpc_frame_dataspace(f), buffer, size);
    lpc_string_putval(retval, signature);
}

/*
 * signature = encrypt("Ed25519 sign", key, message)
 */
static void ed25519_sign(LPC_frame f, int nargs, LPC_value retval)
{
    ecx_sign(f, nargs, EVP_PKEY_ED25519, 32, retval);
}

/*
 * signature = encrypt("Ed448 sign", key, message)
 */
static void ed448_sign(LPC_frame f, int nargs, LPC_value retval)
{
    ecx_sign(f, nargs, EVP_PKEY_ED448, 56, retval);
}

/*
 * decrypt using AES-GCM
 */
static void decrypt_aes_gcm(LPC_frame f, int nargs, LPC_value retval,
			    const EVP_CIPHER *cipher, int nbytes)
{
    LPC_value arg;
    LPC_string key, str, iv, aad;
    LPC_int taglen;
    int length, ivlen, aadlen, len;
    unsigned char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    if (nargs != 5) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    arg = lpc_frame_arg(f, 5, 0);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun decrypt");
    }
    key = lpc_string_getval(arg);
    if (lpc_string_length(key) != nbytes) {
	lpc_runtime_error(f, "Bad key");
    }
    arg = lpc_frame_arg(f, 5, 1);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun decrypt");
    }
    iv = lpc_string_getval(arg);
    ivlen = lpc_string_length(iv);
    if (ivlen == 0 || ivlen > 16) {
	lpc_runtime_error(f, "Bad IV");
    }
    arg = lpc_frame_arg(f, 5, 2);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 4 for kfun decrypt");
    }
    aad = lpc_string_getval(arg);
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    arg = lpc_frame_arg(f, 5, 3);
    if (lpc_value_type(arg) != LPC_TYPE_INT) {
	lpc_runtime_error(f, "Bad argument 5 for kfun decrypt");
    }
    taglen = lpc_int_getval(arg);
    if (taglen <= 0 || taglen > 16) {
	lpc_runtime_error(f, "Bad tag length");
    }
    arg = lpc_frame_arg(f, 5, 4);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 6 for kfun decrypt");
    }
    str = lpc_string_getval(arg);
    length = lpc_string_length(str);
    if (length <= taglen) {
	lpc_runtime_error(f, "Bad ciphertext");
    }
    ciphertext = (unsigned char *) lpc_string_text(str);
    lpc_runtime_check(f, lpc_string_length(str));

    len = length -= taglen;
    plaintext = alloca(length);
    tag = ciphertext + length;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_DecryptInit_ex(ctx, cipher, NULL, NULL, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, ivlen, NULL) <= 0 ||
	EVP_DecryptInit_ex(ctx, NULL, NULL,
			   (unsigned char *) lpc_string_text(key),
			   (unsigned char *) lpc_string_text(iv)) <= 0 ||
	EVP_DecryptUpdate(ctx, NULL, &aadlen,
			  (unsigned char *) lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, len) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int) taglen, tag) <= 0)
    {
	ERR_clear_error();
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Decryption failed");
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) > 0) {
	str = lpc_string_new(lpc_frame_dataspace(f), (char *) plaintext,
			     length);
	lpc_string_putval(retval, str);
    }
    EVP_CIPHER_CTX_free(ctx);
}

/*
 * plaintext = decrypt("AES-128-GCM", key, iv, aad, taglen, ciphertext)
 */
static void decrypt_aes_128_gcm(LPC_frame f, int nargs, LPC_value retval)
{
    decrypt_aes_gcm(f, nargs, retval, cipher_aes_128_gcm, 16);
}

/*
 * plaintext = decrypt("AES-256-GCM", key, iv, aad, taglen, ciphertext)
 */
static void decrypt_aes_256_gcm(LPC_frame f, int nargs, LPC_value retval)
{
    decrypt_aes_gcm(f, nargs, retval, cipher_aes_256_gcm, 32);
}

/*
 * plaintext = decrypt("ChaCha20-Poly1305", key, iv, aad, taglen, ciphertext)
 */
static void decrypt_chacha20_poly1305(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_value arg;
    LPC_string key, str, iv, aad;
    LPC_int taglen;
    int length, ivlen, aadlen, len;
    unsigned char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    if (nargs != 5) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    arg = lpc_frame_arg(f, 5, 0);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun decrypt");
    }
    key = lpc_string_getval(arg);
    if (lpc_string_length(key) != 32) {
	lpc_runtime_error(f, "Bad key");
    }
    arg = lpc_frame_arg(f, 5, 1);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun decrypt");
    }
    iv = lpc_string_getval(arg);
    ivlen = lpc_string_length(iv);
    if (ivlen == 0 || ivlen > 12) {
	lpc_runtime_error(f, "Bad IV");
    }
    arg = lpc_frame_arg(f, 5, 2);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 4 for kfun decrypt");
    }
    aad = lpc_string_getval(arg);
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    arg = lpc_frame_arg(f, 5, 3);
    if (lpc_value_type(arg) != LPC_TYPE_INT) {
	lpc_runtime_error(f, "Bad argument 5 for kfun decrypt");
    }
    taglen = lpc_int_getval(arg);
    if (taglen <= 0 || taglen > 16) {
	lpc_runtime_error(f, "Bad tag length");
    }
    arg = lpc_frame_arg(f, 5, 4);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 6 for kfun decrypt");
    }
    str = lpc_string_getval(arg);
    length = lpc_string_length(str);
    if (length <= taglen) {
	lpc_runtime_error(f, "Bad ciphertext");
    }
    ciphertext = (unsigned char *) lpc_string_text(str);
    lpc_runtime_check(f, 3 * lpc_string_length(str));

    len = length -= taglen;
    plaintext = alloca(length);
    tag = ciphertext + length;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_DecryptInit_ex(ctx, cipher_chacha20_poly1305, NULL, NULL,
			   NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, ivlen, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, (int) taglen,
			    tag) <= 0 ||
	EVP_DecryptInit_ex(ctx, NULL, NULL,
			   (unsigned char *) lpc_string_text(key),
			   (unsigned char *) lpc_string_text(iv)) <= 0 ||
	EVP_DecryptUpdate(ctx, NULL, &aadlen,
			  (unsigned char *) lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, len) <= 0) {
	ERR_clear_error();
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Decryption failed");
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) > 0) {
	str = lpc_string_new(lpc_frame_dataspace(f), (char *) plaintext,
			     length);
	lpc_string_putval(retval, str);
    }
    EVP_CIPHER_CTX_free(ctx);
}

/*
 * plaintext = decrypt("AES-128-CCM", key, iv, aad, taglen, ciphertext)
 */
static void decrypt_aes_128_ccm(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_value arg;
    LPC_string key, str, iv, aad;
    LPC_int taglen;
    int length, ivlen, aadlen, len;
    unsigned char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    if (nargs != 5) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    arg = lpc_frame_arg(f, 5, 0);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun decrypt");
    }
    key = lpc_string_getval(arg);
    if (lpc_string_length(key) != 16) {
	lpc_runtime_error(f, "Bad key");
    }
    arg = lpc_frame_arg(f, 5, 1);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun decrypt");
    }
    iv = lpc_string_getval(arg);
    ivlen = lpc_string_length(iv);
    if (ivlen < 7 || ivlen > 13) {
	lpc_runtime_error(f, "Bad IV");
    }
    arg = lpc_frame_arg(f, 5, 2);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 4 for kfun decrypt");
    }
    aad = lpc_string_getval(arg);
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    arg = lpc_frame_arg(f, 5, 3);
    if (lpc_value_type(arg) != LPC_TYPE_INT) {
	lpc_runtime_error(f, "Bad argument 5 for kfun decrypt");
    }
    taglen = lpc_int_getval(arg);
    if (taglen < 4 || taglen > 16 || (taglen & 1)) {
	lpc_runtime_error(f, "Bad tag length");
    }
    arg = lpc_frame_arg(f, 5, 4);
    if (lpc_value_type(arg) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 6 for kfun decrypt");
    }
    str = lpc_string_getval(arg);
    length = lpc_string_length(str);
    if (length <= taglen) {
	lpc_runtime_error(f, "Bad ciphertext");
    }
    ciphertext = (unsigned char *) lpc_string_text(str);
    lpc_runtime_check(f, lpc_string_length(str));

    len = length -= taglen;
    plaintext = alloca(length);
    tag = ciphertext + length;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_DecryptInit_ex(ctx, cipher_aes_128_ccm, NULL, NULL, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, ivlen, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, (int) taglen,
			    tag) <= 0 ||
	EVP_DecryptInit_ex(ctx, NULL, NULL,
			   (unsigned char *) lpc_string_text(key),
			   (unsigned char *) lpc_string_text(iv)) <= 0 ||
	EVP_DecryptUpdate(ctx, NULL, &len, NULL, len) <= 0 ||
	EVP_DecryptUpdate(ctx, NULL, &aadlen,
			  (unsigned char *) lpc_string_text(aad),
			  aadlen) <= 0) {
	ERR_clear_error();
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Decryption failed");
    }

    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, len) > 0) {
	str = lpc_string_new(lpc_frame_dataspace(f), (char *) plaintext,
			     length);
	lpc_string_putval(retval, str);
    }
    EVP_CIPHER_CTX_free(ctx);
}

/*
 * EC derive shared secret
 */
static void ec_derive(LPC_frame f, int nargs, int nid, int len,
		      LPC_value retval)
{
    unsigned char buffer[1024];
    LPC_value val;
    LPC_string priv, peer, secret;
    BIGNUM *bn;
    EC_KEY *ec;
    EVP_PKEY *key;
    EVP_PKEY_CTX *context;
    size_t size;

    /* retrieve arguments */
    if (nargs != 2) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    val = lpc_frame_arg(f, nargs, 0);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun decrypt");
    }
    priv = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 1);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun decrypt");
    }
    peer = lpc_string_getval(val);
    lpc_runtime_check(f, 16 * len);

    /* EC context with private key */
    bn = BN_bin2bn(lpc_string_text(priv), lpc_string_length(priv), NULL);
    ec = NULL;
    key = NULL;
    if (bn == NULL ||
	(ec=EC_KEY_new_by_curve_name(nid)) == NULL ||
	EC_KEY_set_private_key(ec, bn) <= 0 || (key=EVP_PKEY_new()) == NULL ||
	EVP_PKEY_assign_EC_KEY(key, ec) <= 0 ||
	(context=EVP_PKEY_CTX_new(key, NULL)) == NULL) {
	ERR_clear_error();
	if (key != NULL) {
	    EVP_PKEY_free(key);
	} else if (ec != NULL) {
	    EC_KEY_free(ec);
	}
	if (bn != NULL) {
	    BN_free(bn);
	}
	lpc_runtime_error(f, "Derive key failed");
    }
    EVP_PKEY_free(key);
    BN_free(bn);

    /* peer key */
    ec = EC_KEY_new_by_curve_name(nid);
    key = NULL;
    if (ec == NULL ||
	EC_KEY_oct2key(ec, lpc_string_text(peer), lpc_string_length(peer),
		       NULL) <= 0 ||
	(key=EVP_PKEY_new()) == NULL || EVP_PKEY_assign_EC_KEY(key, ec) <= 0) {
	ERR_clear_error();
	if (key != NULL) {
	    EVP_PKEY_free(key);
	}
	if (ec != NULL) {
	    EC_KEY_free(ec);
	}
	lpc_runtime_error(f, "Derive peer failed");
    }

    /* derive shared secret */
    size = sizeof(buffer);
    if (EVP_PKEY_derive_init(context) <= 0 ||
	EVP_PKEY_derive_set_peer(context, key) <= 0 ||
	EVP_PKEY_derive(context, buffer, &size) <= 0) {
	ERR_clear_error();
	EVP_PKEY_free(key);
	EVP_PKEY_CTX_free(context);
	lpc_runtime_error(f, "Derive failed");
    }
    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(context);

    secret = lpc_string_new(lpc_frame_dataspace(f), buffer, size);
    lpc_string_putval(retval, secret);
}

/*
 * shared_secret = decrypt("SECP256R1 derive", privkey, peerx, peery)
 */
static void secp256r1_derive(LPC_frame f, int nargs, LPC_value retval)
{
    ec_derive(f, nargs, NID_X9_62_prime256v1, 32, retval);
}

/*
 * shared_secret = decrypt("SECP384R1 derive", privkey, peerx, peery)
 */
static void secp384r1_derive(LPC_frame f, int nargs, LPC_value retval)
{
    ec_derive(f, nargs, NID_secp384r1, 48, retval);
}

/*
 * shared_secret = decrypt("SECP521R1 derive", privkey, peerx, peery)
 */
static void secp521r1_derive(LPC_frame f, int nargs, LPC_value retval)
{
    ec_derive(f, nargs, NID_secp521r1, 64, retval);
}

/*
 * ECX derive shared secret
 */
static void ecx_derive(LPC_frame f, int nargs, int id, int len,
		       LPC_value retval)
{
    unsigned char buffer[1024];
    LPC_value val;
    LPC_string priv, peer, secret;
    EVP_PKEY *key;
    EVP_PKEY_CTX *context;
    size_t size;

    /* retrieve arguments */
    if (nargs != 2) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    val = lpc_frame_arg(f, nargs, 0);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun decrypt");
    }
    priv = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 1);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun decrypt");
    }
    peer = lpc_string_getval(val);
    lpc_runtime_check(f, 8 * len);

    /* ECX context with private key */
    key = EVP_PKEY_new_raw_private_key(id, NULL, lpc_string_text(priv),
				       lpc_string_length(priv));
    if (key == NULL || (context=EVP_PKEY_CTX_new(key, NULL)) == NULL) {
	ERR_clear_error();
	if (key != NULL) {
	    EVP_PKEY_free(key);
	}
	lpc_runtime_error(f, "Derive key failed");
    }
    EVP_PKEY_free(key);

    /* peer key */
    key = EVP_PKEY_new_raw_public_key(id, NULL, lpc_string_text(peer),
				      lpc_string_length(peer));
    if (key == NULL) {
	ERR_clear_error();
	lpc_runtime_error(f, "Derive peer failed");
    }

    /* derive shared secret */
    size = sizeof(buffer);
    if (EVP_PKEY_derive_init(context) <= 0 ||
	EVP_PKEY_derive_set_peer(context, key) <= 0 ||
	EVP_PKEY_derive(context, buffer, &size) <= 0) {
	ERR_clear_error();
	EVP_PKEY_free(key);
	EVP_PKEY_CTX_free(context);
	lpc_runtime_error(f, "Derive failed");
    }
    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(context);

    secret = lpc_string_new(lpc_frame_dataspace(f), buffer, size);
    lpc_string_putval(retval, secret);
}

/*
 * secret = decrypt("X25519 derive", peerkey, privkey)
 */
static void x25519_derive(LPC_frame f, int nargs, LPC_value retval)
{
    ecx_derive(f, nargs, EVP_PKEY_X25519, 32, retval);
}

/*
 * secret = decrypt("X448 derive", peerkey, privkey)
 */
static void x448_derive(LPC_frame f, int nargs, LPC_value retval)
{
    ecx_derive(f, nargs, EVP_PKEY_X448, 56, retval);
}

/*
 * EC verify
 */
static void ec_verify(LPC_frame f, int nargs, int nid, const EVP_MD *md,
		      int len, LPC_value retval)
{
    LPC_value val;
    LPC_string pub, message, signature;
    BIGNUM *bnx, *bny;
    EC_KEY *ec;
    EVP_PKEY *key;
    EVP_MD_CTX *context;
    size_t size;
    int result;

    /* retrieve arguments */
    if (nargs != 3) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    val = lpc_frame_arg(f, nargs, 0);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun decrypt");
    }
    pub = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 1);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun decrypt");
    }
    signature = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 2);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 4 for kfun decrypt");
    }
    message = lpc_string_getval(val);
    lpc_runtime_check(f, 16 * len);

    /* set up public key */
    ec = EC_KEY_new_by_curve_name(nid);
    key = NULL;
    if (ec == NULL ||
	EC_KEY_oct2key(ec, lpc_string_text(pub), lpc_string_length(pub),
		       NULL) <= 0 ||
	(key=EVP_PKEY_new()) == NULL || EVP_PKEY_assign_EC_KEY(key, ec) <= 0) {
	ERR_clear_error();
	if (key != NULL) {
	    EVP_PKEY_free(key);
	}
	if (ec != NULL) {
	    EC_KEY_free(ec);
	}
	lpc_runtime_error(f, "Verify key failed");
    }

    /* verify signature */
    context = EVP_MD_CTX_new();
    if (context == NULL ||
	EVP_DigestVerifyInit(context, NULL, md, NULL, key) <= 0) {
	ERR_clear_error();
	if (context != NULL) {
	    EVP_MD_CTX_free(context);
	}
	lpc_runtime_error(f, "Verify failed");
    }
    result = EVP_DigestVerify(context, lpc_string_text(signature),
			      lpc_string_length(signature),
			      lpc_string_text(message),
			      lpc_string_length(message));
    if (result > 0) {
	lpc_int_putval(retval, 1);
    } else if (result == 0) {
	lpc_int_putval(retval, 0);
    } else {
	ERR_clear_error();
	EVP_MD_CTX_free(context);
	EVP_PKEY_free(key);
	lpc_runtime_error(f, "Verify failed");
    }

    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);
}

/*
 * bool = decrypt("ECDSA-SECP256R1-SHA256 verify", x, y, signature, message)
 */
static void secp256r1_verify(LPC_frame f, int nargs, LPC_value retval)
{
    ec_verify(f, nargs, NID_X9_62_prime256v1, md_sha256, 32, retval);
}

/*
 * bool = decrypt("ECDSA-SECP384R1-SHA384 verify", x, y, signature, message)
 */
static void secp384r1_verify(LPC_frame f, int nargs, LPC_value retval)
{
    ec_verify(f, nargs, NID_secp384r1, md_sha384, 48, retval);
}

/*
 * bool = decrypt("ECDSA-SECP521R1-SHA521 verify", x, y, signature, message)
 */
static void secp521r1_verify(LPC_frame f, int nargs, LPC_value retval)
{
    ec_verify(f, nargs, NID_secp521r1, md_sha512, 64, retval);
}

/*
 * ECX verify
 */
static void ecx_verify(LPC_frame f, int nargs, int id, int len,
		       LPC_value retval)
{
    LPC_value val;
    LPC_string pub, message, signature;
    EVP_PKEY *key;
    EVP_MD_CTX *context;
    size_t size;
    int result;

    /* retrieve arguments */
    if (nargs != 3) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    val = lpc_frame_arg(f, nargs, 0);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 2 for kfun decrypt");
    }
    pub = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 1);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 3 for kfun decrypt");
    }
    signature = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 2);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	lpc_runtime_error(f, "Bad argument 4 for kfun decrypt");
    }
    message = lpc_string_getval(val);
    lpc_runtime_check(f, 8 * len);

    /* set up public key */
    key = EVP_PKEY_new_raw_public_key(id, NULL, lpc_string_text(pub),
				      lpc_string_length(pub));
    if (key == NULL) {
	ERR_clear_error();
	lpc_runtime_error(f, "Verify key failed");
    }

    /* verify signature */
    context = EVP_MD_CTX_new();
    if (context == NULL ||
	EVP_DigestVerifyInit(context, NULL, NULL, NULL, key) <= 0) {
	ERR_clear_error();
	if (context != NULL) {
	    EVP_MD_CTX_free(context);
	}
	lpc_runtime_error(f, "Verify failed");
    }
    result = EVP_DigestVerify(context, lpc_string_text(signature),
			      lpc_string_length(signature),
			      lpc_string_text(message),
			      lpc_string_length(message));
    if (result > 0) {
	lpc_int_putval(retval, 1);
    } else if (result == 0) {
	lpc_int_putval(retval, 0);
    } else {
	ERR_clear_error();
	EVP_MD_CTX_free(context);
	EVP_PKEY_free(key);
	lpc_runtime_error(f, "Verify failed");
    }

    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);
}

/*
 * bool = decrypt("Ed25519 verify", pubkey, signature, message)
 */
static void ed25519_verify(LPC_frame f, int nargs, LPC_value retval)
{
    ecx_verify(f, nargs, EVP_PKEY_ED25519, 32, retval);
}

/*
 * bool = decrypt("Ed448 verify", pubkey, signature, message)
 */
static void ed448_verify(LPC_frame f, int nargs, LPC_value retval)
{
    ecx_verify(f, nargs, EVP_PKEY_ED448, 56, retval);
}

static char hash_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING, LPC_TYPE_STRING,
			     LPC_TYPE_ELLIPSIS, 0 };
static char secure_random_proto[] = { LPC_TYPE_STRING, LPC_TYPE_INT, 0 };
static char verify_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_ELLIPSIS, 0 };
static char mask_xor_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
				 LPC_TYPE_STRING, 0 };
static char cipher_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_STRING, LPC_TYPE_STRING, LPC_TYPE_INT,
			       LPC_TYPE_STRING, 0 };
static char ec_key_proto[] = { LPC_TYPE_ARRAY_OF(LPC_TYPE_STRING),
			       LPC_TYPE_STRING, 0 };
static char ec_derive_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
				  LPC_TYPE_STRING, LPC_TYPE_STRING, 0 };
static char ec_sign_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
				LPC_TYPE_STRING, LPC_TYPE_STRING, 0 };
static char ec_verify_proto[] = { LPC_TYPE_INT, LPC_TYPE_STRING,
				  LPC_TYPE_STRING, LPC_TYPE_STRING,
				  LPC_TYPE_STRING, 0 };

static LPC_ext_kfun kf[] = {
    { "hash MD5", hash_proto, &md5 },
    { "hash SHA1", hash_proto, &sha1 },
    { "hash SHA224", hash_proto, &sha224 },
    { "hash SHA256", hash_proto, &sha256 },
    { "hash SHA384", hash_proto, &sha384 },
    { "hash SHA512", hash_proto, &sha512 },
    { "secure_random", secure_random_proto, &secure_random },
    { "verify_certificate", verify_proto, &verify_certificate },
    { "mask_xor", mask_xor_proto, mask_xor },
    { "encrypt AES-128-GCM", cipher_proto, &encrypt_aes_128_gcm },
    { "encrypt AES-256-GCM", cipher_proto, &encrypt_aes_256_gcm },
    { "encrypt ChaCha20-Poly1305", cipher_proto, &encrypt_chacha20_poly1305 },
    { "encrypt AES-128-CCM", cipher_proto, &encrypt_aes_128_ccm },
    { "encrypt SECP256R1 key", ec_key_proto, &secp256r1_key },
    { "encrypt SECP384R1 key", ec_key_proto, &secp384r1_key },
    { "encrypt SECP521R1 key", ec_key_proto, &secp521r1_key },
    { "encrypt X25519 key", ec_key_proto, &x25519_key },
    { "encrypt X448 key", ec_key_proto, &x448_key },
    { "encrypt ECDSA-SECP256R1-SHA256 sign", ec_sign_proto, secp256r1_sign },
    { "encrypt ECDSA-SECP384R1-SHA384 sign", ec_sign_proto, secp384r1_sign },
    { "encrypt ECDSA-SECP521R1-SHA512 sign", ec_sign_proto, secp521r1_sign },
    { "encrypt Ed25519 sign", ec_sign_proto, ed25519_sign },
    { "encrypt Ed448 sign", ec_sign_proto, ed448_sign },
    { "decrypt AES-128-GCM", cipher_proto, &decrypt_aes_128_gcm },
    { "decrypt AES-256-GCM", cipher_proto, &decrypt_aes_256_gcm },
    { "decrypt ChaCha20-Poly1305", cipher_proto, &decrypt_chacha20_poly1305 },
    { "decrypt AES-128-CCM", cipher_proto, &decrypt_aes_128_ccm },
    { "decrypt SECP256R1 derive", ec_derive_proto, &secp256r1_derive },
    { "decrypt SECP384R1 derive", ec_derive_proto, &secp384r1_derive },
    { "decrypt SECP521R1 derive", ec_derive_proto, &secp521r1_derive },
    { "decrypt X25519 derive", ec_derive_proto, &x25519_derive },
    { "decrypt X448 derive", ec_derive_proto, &x448_derive },
    { "decrypt ECDSA-SECP256R1-SHA256 verify", ec_verify_proto,
      secp256r1_verify },
    { "decrypt ECDSA-SECP384R1-SHA384 verify", ec_verify_proto,
      secp384r1_verify },
    { "decrypt ECDSA-SECP521R1-SHA512 verify", ec_verify_proto,
      secp521r1_verify },
    { "decrypt Ed25519 verify", ec_verify_proto, ed25519_verify },
    { "decrypt Ed448 verify", ec_verify_proto, ed448_verify }
};

int lpc_ext_init(int major, int minor, const char *config)
{
    md_md5 = EVP_get_digestbyname("MD5");
    md_sha1 = EVP_get_digestbyname("SHA1");
    md_sha224 = EVP_get_digestbyname("SHA224");
    md_sha256 = EVP_get_digestbyname("SHA256");
    md_sha384 = EVP_get_digestbyname("SHA384");
    md_sha512 = EVP_get_digestbyname("SHA512");
    cipher_aes_128_gcm = EVP_get_cipherbyname("id-aes128-GCM");
    cipher_aes_256_gcm = EVP_get_cipherbyname("id-aes256-GCM");
    cipher_chacha20_poly1305 = EVP_get_cipherbyname("ChaCha20-Poly1305");
    cipher_aes_128_ccm = EVP_get_cipherbyname("id-aes128-CCM");

    lpc_ext_kfun(kf, sizeof(kf) / sizeof(LPC_ext_kfun));
    return 1;
}
