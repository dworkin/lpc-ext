/*
 * crypto kfun extensions
 * This code is released into the public domain.
 */

# include <openssl/evp.h>
# include "lpc_ext.h"

static const EVP_MD *md_md5, *md_sha1, *md_sha256, *md_sha512;

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

static void md5(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_check_ticks(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_check_ticks(f, lpc_string_length(str));
    }
    hash(md_md5, f, nargs, retval);
}

static void sha1(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_check_ticks(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_check_ticks(f, lpc_string_length(str));
    }
    hash(md_sha1, f, nargs, retval);
}

static void sha256(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_check_ticks(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_check_ticks(f, lpc_string_length(str));
    }
    hash(md_sha256, f, nargs, retval);
}

static void sha512(LPC_frame f, int nargs, LPC_value retval)
{
    int i;
    LPC_string str;

    lpc_check_ticks(f, 3 * nargs + 64);
    for (i = 0; i < nargs; i++) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, i));
	lpc_check_ticks(f, lpc_string_length(str));
    }
    hash(md_sha512, f, nargs, retval);
}

static char md5_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING, LPC_TYPE_ELLIPSIS,
			    0 };
static char sha1_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
			     LPC_TYPE_ELLIPSIS, 0 };
static char sha256_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_ELLIPSIS, 0 };
static char sha512_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_ELLIPSIS, 0 };
static LPC_ext_kfun kf[] = {
    { "hash MD5", md5_proto, &md5 },
    { "hash SHA1", sha1_proto, &sha1 },
    { "hash SHA256", sha256_proto, &sha256 },
    { "hash SHA512", sha512_proto, &sha512 }
};

int lpc_ext_init(int major, int minor, const char *config)
{
    md_md5 = EVP_get_digestbyname("MD5");
    md_sha1 = EVP_get_digestbyname("SHA1");
    md_sha256 = EVP_get_digestbyname("SHA256");
    md_sha512 = EVP_get_digestbyname("SHA512");

    lpc_ext_kfun(kf, 4);
    return 1;
}
