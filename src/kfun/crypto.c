/*
 * crypto kfun extensions
 *
 * This code is released into the public domain.
 */

# include <openssl/evp.h>
# include <openssl/rand.h>
# include <openssl/err.h>
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
    int i, length;
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
    str = lpc_string_new(lpc_frame_dataspace(f), digest, length);

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
    unsigned char buffer[256 / 8];
    LPC_int n;
    LPC_string str;

    n = lpc_int_getval(lpc_frame_arg(f, nargs, 0));
    if (n <= 0 || n > 256 / 8) {
	lpc_runtime_error(f, "Invalid number of random bytes");
    }
    if (RAND_bytes(buffer, n) <= 0) {
	sprintf(buffer, "OpenSSL error %lu", ERR_get_error());
	lpc_runtime_error(f, buffer);
    }

    str = lpc_string_new(lpc_frame_dataspace(f), (char *) buffer, n);
    lpc_string_putval(retval, str);
}

/*
 * encrypt using AES-GCM
 */
static void encrypt_aes_gcm(LPC_frame f, const EVP_CIPHER *cipher, int nbytes,
			    LPC_value retval)
{
    LPC_string key, str, iv, aad;
    LPC_dataspace data;
    int length, aadlen, len;
    char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    key = lpc_string_getval(lpc_frame_arg(f, 4, 0));
    if (lpc_string_length(key) != nbytes) {
	lpc_runtime_error(f, "Bad key");
    }
    iv = lpc_string_getval(lpc_frame_arg(f, 4, 1));
    if (lpc_string_length(iv) != 12) {
	lpc_runtime_error(f, "Bad IV");
    }
    aad = lpc_string_getval(lpc_frame_arg(f, 4, 2));
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    str = lpc_string_getval(lpc_frame_arg(f, 4, 3));
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad plaintext");
    }
    plaintext = lpc_string_text(str);

    len = length;
    length += 16;
    ciphertext = alloca(length);
    tag = ciphertext + len;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_EncryptInit_ex(ctx, cipher, NULL, lpc_string_text(key),
			   lpc_string_text(iv)) <= 0 ||
	EVP_EncryptUpdate(ctx, NULL, &aadlen, lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, len) <= 0 ||
	EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) <= 0) {
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Encryption failed");
    }
    EVP_CIPHER_CTX_free(ctx);

    str = lpc_string_new(lpc_frame_dataspace(f), ciphertext, length);
    lpc_string_putval(retval, str);
}

/*
 * ciphertext = encrypt("AEAD_AES_128_GCM", key, iv, aad, plaintext)
 */
static void encrypt_aes_128_gcm(LPC_frame f, int nargs, LPC_value retval)
{
    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    encrypt_aes_gcm(f, cipher_aes_128_gcm, 16, retval);
}

/*
 * ciphertext = encrypt("AEAD_AES_256_GCM", key, iv, aad, plaintext)
 */
static void encrypt_aes_256_gcm(LPC_frame f, int nargs, LPC_value retval)
{
    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    encrypt_aes_gcm(f, cipher_aes_256_gcm, 32, retval);
}

/*
 * ciphertext = encrypt("AEAD_CHACHS20_POLY1305", key, iv, aad, plaintext)
 */
static void encrypt_chacha20_poly1305(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_string key, str, iv, aad;
    LPC_dataspace data;
    int length, aadlen, len;
    char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    key = lpc_string_getval(lpc_frame_arg(f, 4, 0));
    if (lpc_string_length(key) != 32) {
	lpc_runtime_error(f, "Bad key");
    }
    iv = lpc_string_getval(lpc_frame_arg(f, 4, 1));
    if (lpc_string_length(iv) != 12) {
	lpc_runtime_error(f, "Bad IV");
    }
    aad = lpc_string_getval(lpc_frame_arg(f, 4, 2));
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    str = lpc_string_getval(lpc_frame_arg(f, 4, 3));
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad plaintext");
    }
    plaintext = lpc_string_text(str);

    len = length;
    length += 16;
    ciphertext = alloca(length);
    tag = ciphertext + len;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_EncryptInit_ex(ctx, cipher_chacha20_poly1305, NULL,
			   lpc_string_text(key), lpc_string_text(iv)) <= 0 ||
	EVP_EncryptUpdate(ctx, NULL, &aadlen, lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, len) <= 0 ||
	EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag) <= 0) {
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Encryption failed");
    }
    EVP_CIPHER_CTX_free(ctx);

    str = lpc_string_new(lpc_frame_dataspace(f), ciphertext, length);
    lpc_string_putval(retval, str);
}

/*
 * encrypt using AES-CCM
 */
static void encrypt_aes_ccm(LPC_frame f, int taglen, LPC_value retval)
{
    LPC_string key, str, iv, aad;
    LPC_dataspace data;
    int length, aadlen, len;
    char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    key = lpc_string_getval(lpc_frame_arg(f, 4, 0));
    if (lpc_string_length(key) != 16) {
	lpc_runtime_error(f, "Bad key");
    }
    iv = lpc_string_getval(lpc_frame_arg(f, 4, 1));
    if (lpc_string_length(iv) != 12) {
	lpc_runtime_error(f, "Bad IV");
    }
    aad = lpc_string_getval(lpc_frame_arg(f, 4, 2));
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    str = lpc_string_getval(lpc_frame_arg(f, 4, 3));
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad plaintext");
    }
    plaintext = lpc_string_text(str);

    len = length;
    length += taglen;
    ciphertext = alloca(length);
    tag = ciphertext + len;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_EncryptInit_ex(ctx, cipher_aes_128_ccm, NULL, NULL, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, 12, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, taglen, NULL) <= 0 ||
	EVP_EncryptInit_ex(ctx, NULL, NULL, lpc_string_text(key),
			   lpc_string_text(iv)) <= 0 ||
	EVP_EncryptUpdate(ctx, NULL, &len, NULL, len) <= 0 ||
	EVP_EncryptUpdate(ctx, NULL, &aadlen, lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, len) <= 0 ||
	EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_GET_TAG, taglen, tag) <= 0) {
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Encryption failed");
    }
    EVP_CIPHER_CTX_free(ctx);

    str = lpc_string_new(lpc_frame_dataspace(f), ciphertext, length);
    lpc_string_putval(retval, str);
}

/*
 * ciphertext = encrypt("AEAD_AES_128_CCM", key, iv, aad, plaintext)
 */
static void encrypt_aes_128_ccm(LPC_frame f, int nargs, LPC_value retval)
{
    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    encrypt_aes_ccm(f, 16, retval);
}

/*
 * ciphertext = encrypt("AEAD_AES_128_CCM_8", key, iv, aad, plaintext)
 */
static void encrypt_aes_128_ccm_8(LPC_frame f, int nargs, LPC_value retval)
{
    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun encrypt");
    }
    encrypt_aes_ccm(f, 8, retval);
}

/*
 * decrypt using AES-GCM
 */
static void decrypt_aes_gcm(LPC_frame f, const EVP_CIPHER *cipher, int nbytes,
			    LPC_value retval)
{
    LPC_string key, str, iv, aad;
    LPC_dataspace data;
    int length, aadlen, len;
    char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    key = lpc_string_getval(lpc_frame_arg(f, 4, 0));
    if (lpc_string_length(key) != nbytes) {
	lpc_runtime_error(f, "Bad key");
    }
    iv = lpc_string_getval(lpc_frame_arg(f, 4, 1));
    if (lpc_string_length(iv) != 12) {
	lpc_runtime_error(f, "Bad IV");
    }
    aad = lpc_string_getval(lpc_frame_arg(f, 4, 2));
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    str = lpc_string_getval(lpc_frame_arg(f, 4, 3));
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad ciphertext");
    }
    ciphertext = lpc_string_text(str);

    len = length -= 16;
    plaintext = alloca(length);
    tag = ciphertext + length;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_DecryptInit_ex(ctx, cipher, NULL, lpc_string_text(key),
			   lpc_string_text(iv)) <= 0 ||
	EVP_DecryptUpdate(ctx, NULL, &aadlen, lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, len) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag) <= 0) {
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Decryption failed");
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) > 0) {
	str = lpc_string_new(lpc_frame_dataspace(f), plaintext, length);
	lpc_string_putval(retval, str);
    }
    EVP_CIPHER_CTX_free(ctx);
}

/*
 * plaintext = decrypt("AEAD_AES_128_GCM", key, iv, aad, ciphertext)
 */
static void decrypt_aes_128_gcm(LPC_frame f, int nargs, LPC_value retval)
{
    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    decrypt_aes_gcm(f, cipher_aes_128_gcm, 16, retval);
}

/*
 * plaintext = decrypt("AEAD_AES_256_GCM", key, iv, aad, ciphertext)
 */
static void decrypt_aes_256_gcm(LPC_frame f, int nargs, LPC_value retval)
{
    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    decrypt_aes_gcm(f, cipher_aes_256_gcm, 32, retval);
}

/*
 * plaintext = decrypt("AEAD_CHACHA20_POLY1305", key, iv, aad, ciphertext)
 */
static void decrypt_chacha20_poly1305(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_string key, str, iv, aad;
    LPC_dataspace data;
    int length, aadlen, len;
    char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    key = lpc_string_getval(lpc_frame_arg(f, 4, 0));
    if (lpc_string_length(key) != 32) {
	lpc_runtime_error(f, "Bad key");
    }
    iv = lpc_string_getval(lpc_frame_arg(f, 4, 1));
    if (lpc_string_length(iv) != 12) {
	lpc_runtime_error(f, "Bad IV");
    }
    aad = lpc_string_getval(lpc_frame_arg(f, 4, 2));
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    str = lpc_string_getval(lpc_frame_arg(f, 4, 3));
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad ciphertext");
    }
    ciphertext = lpc_string_text(str);

    len = length -= 16;
    plaintext = alloca(length);
    tag = ciphertext + length;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_DecryptInit_ex(ctx, cipher_chacha20_poly1305, NULL, NULL,
			   NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, 16, tag) <= 0 ||
	EVP_DecryptInit_ex(ctx, NULL, NULL, lpc_string_text(key),
			   lpc_string_text(iv)) <= 0 ||
	EVP_DecryptUpdate(ctx, NULL, &aadlen, lpc_string_text(aad),
			  aadlen) <= 0 ||
	EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, len) <= 0) {
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Decryption failed");
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) > 0) {
	str = lpc_string_new(lpc_frame_dataspace(f), plaintext, length);
	lpc_string_putval(retval, str);
    }
    EVP_CIPHER_CTX_free(ctx);
}

/*
 * decrypt using AES-CCM
 */
static void decrypt_aes_ccm(LPC_frame f, int taglen, LPC_value retval)
{
    LPC_string key, str, iv, aad;
    LPC_dataspace data;
    int length, aadlen, len;
    char *plaintext, *ciphertext, *tag;
    EVP_CIPHER_CTX *ctx;

    key = lpc_string_getval(lpc_frame_arg(f, 4, 0));
    if (lpc_string_length(key) != 16) {
	lpc_runtime_error(f, "Bad key");
    }
    iv = lpc_string_getval(lpc_frame_arg(f, 4, 1));
    if (lpc_string_length(iv) != 12) {
	lpc_runtime_error(f, "Bad IV");
    }
    aad = lpc_string_getval(lpc_frame_arg(f, 4, 2));
    aadlen = lpc_string_length(aad);
    if (aadlen == 0) {
	lpc_runtime_error(f, "Bad AAD");
    }
    str = lpc_string_getval(lpc_frame_arg(f, 4, 3));
    length = lpc_string_length(str);
    if (length == 0) {
	lpc_runtime_error(f, "Bad ciphertext");
    }
    ciphertext = lpc_string_text(str);

    len = length -= taglen;
    plaintext = alloca(length);
    tag = ciphertext + length;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL ||
	EVP_DecryptInit_ex(ctx, cipher_aes_128_ccm, NULL, NULL, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, 12, NULL) <= 0 ||
	EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, taglen, tag) <= 0 ||
	EVP_DecryptInit_ex(ctx, NULL, NULL, lpc_string_text(key),
			   lpc_string_text(iv)) <= 0 ||
	EVP_DecryptUpdate(ctx, NULL, &len, NULL, len) <= 0 ||
	EVP_DecryptUpdate(ctx, NULL, &aadlen, lpc_string_text(aad),
			  aadlen) <= 0) {
	if (ctx != NULL) {
	    EVP_CIPHER_CTX_free(ctx);
	}
	lpc_runtime_error(f, "Decryption failed");
    }

    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, len) > 0) {
	str = lpc_string_new(lpc_frame_dataspace(f), plaintext, length);
	lpc_string_putval(retval, str);
    }
    EVP_CIPHER_CTX_free(ctx);
}

/*
 * plaintext = decrypt("AEAD_AES_128_CCM", key, iv, aad, ciphertext)
 */
static void decrypt_aes_128_ccm(LPC_frame f, int nargs, LPC_value retval)
{
    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    decrypt_aes_ccm(f, 16, retval);
}

/*
 * plaintext = decrypt("AEAD_AES_128_CCM_8", key, iv, aad, ciphertext)
 */
static void decrypt_aes_128_ccm_8(LPC_frame f, int nargs, LPC_value retval)
{
    if (nargs != 4) {
	lpc_runtime_error(f, "Wrong number of arguments for kfun decrypt");
    }
    decrypt_aes_ccm(f, 8, retval);
}

static char hash_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING, LPC_TYPE_STRING,
			     LPC_TYPE_ELLIPSIS, 0 };
static char secure_random_proto[] = { LPC_TYPE_STRING, LPC_TYPE_INT, 0 };
static char cipher_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
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
    { "encrypt AEAD_AES_128_GCM", cipher_proto, &encrypt_aes_128_gcm },
    { "encrypt AEAD_AES_256_GCM", cipher_proto, &encrypt_aes_256_gcm },
    { "encrypt AEAD_CHACHA20_POLY1305", cipher_proto, &encrypt_chacha20_poly1305 },
    { "encrypt AEAD_AES_128_CCM", cipher_proto, &encrypt_aes_128_ccm },
    { "encrypt AEAD_AES_128_CCM_8", cipher_proto, &encrypt_aes_128_ccm_8 },
    { "decrypt AEAD_AES_128_GCM", cipher_proto, &decrypt_aes_128_gcm },
    { "decrypt AEAD_AES_256_GCM", cipher_proto, &decrypt_aes_256_gcm },
    { "decrypt AEAD_CHACHA20_POLY1305", cipher_proto, &decrypt_chacha20_poly1305 },
    { "decrypt AEAD_AES_128_CCM", cipher_proto, &decrypt_aes_128_ccm },
    { "decrypt AEAD_AES_128_CCM_8", cipher_proto, &decrypt_aes_128_ccm_8 },
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

    lpc_ext_kfun(kf, 17);
    return 1;
}
