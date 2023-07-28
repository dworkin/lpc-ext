/*
 * crypto kfun extensions
 *
 * This code is released into the public domain.
 */

# include <openssl/evp.h>
# include <openssl/rand.h>
# include <openssl/x509v3.h>
# include <openssl/x509_vfy.h>
# include <openssl/err.h>
# include <string.h>
# include <alloca.h>
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

    /* target certificate */
    str = lpc_string_getval(lpc_frame_arg(f, nargs, 1));
    p = (const unsigned char *) lpc_string_text(str);
    certificate = d2i_X509(NULL, &p, lpc_string_length(str));
    if (certificate == NULL) {
	lpc_runtime_error(f, "Bad certificate");
    }

    /* intermediate certificates */
    intermediates = sk_X509_new_reserve(NULL, 1);
    if (intermediates == NULL) {
	X509_free(certificate);
	lpc_runtime_error(f, "SSL stack error");
    }
    for (i = 2; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	p = (const unsigned char *) lpc_string_text(str);
	intermediate = d2i_X509(NULL, &p, lpc_string_length(str));
	if (intermediate == NULL) {
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

static char hash_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING, LPC_TYPE_STRING,
			     LPC_TYPE_ELLIPSIS, 0 };
static char secure_random_proto[] = { LPC_TYPE_STRING, LPC_TYPE_INT, 0 };
static char verify_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_ELLIPSIS, 0 };
static char cipher_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_STRING, LPC_TYPE_STRING, LPC_TYPE_INT,
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
    { "encrypt AES-128-GCM", cipher_proto, &encrypt_aes_128_gcm },
    { "encrypt AES-256-GCM", cipher_proto, &encrypt_aes_256_gcm },
    { "encrypt ChaCha20-Poly1305", cipher_proto, &encrypt_chacha20_poly1305 },
    { "encrypt AES-128-CCM", cipher_proto, &encrypt_aes_128_ccm },
    { "decrypt AES-128-GCM", cipher_proto, &decrypt_aes_128_gcm },
    { "decrypt AES-256-GCM", cipher_proto, &decrypt_aes_256_gcm },
    { "decrypt ChaCha20-Poly1305", cipher_proto, &decrypt_chacha20_poly1305 },
    { "decrypt AES-128-CCM", cipher_proto, &decrypt_aes_128_ccm }
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

    lpc_ext_kfun(kf, 16);
    return 1;
}
