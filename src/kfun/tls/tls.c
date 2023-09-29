/*
 * TLS kfun extensions
 * This code is released into the public domain.
 */

# include <stdbool.h>
# include <string.h>
# include <openssl/ssl.h>
# include <openssl/bio.h>
# include <openssl/err.h>
# include "lpc_ext.h"


# define TRUE	1
# define FALSE	0

static char dir[1024];	/* directory containing certificate and key */

/*
 * check that a TLS session can be started or continued
 */
static void check(LPC_frame f, bool start)
{
    LPC_object obj;
    LPC_dataspace data;
    LPC_array array;
    LPC_value val;

    if (lpc_frame_atomic(f)) {
	lpc_runtime_error(f, "TLS within atomic");
    }
    obj = lpc_frame_object(f);
    if (obj == NULL) {
	lpc_runtime_error(f, "Object is not persistent");
    }
    data = lpc_frame_dataspace(f);
    if (start) {
	if (lpc_object_isspecial(obj)) {
	    lpc_runtime_error(f, "Object is already special");
	}
    } else {
	if (!lpc_object_ismarked(obj)) {
	    lpc_runtime_error(f, "TLS session not started");
	}

	val = lpc_data_get_val(data);
	if (lpc_value_type(val) != LPC_TYPE_ARRAY) {
	    lpc_runtime_error(f, "TLS session not started");
	}
	array = lpc_array_getval(val);
	val = lpc_array_index(array, 0);
	if (lpc_value_type(val) != LPC_TYPE_STRING ||
	    strcmp(lpc_string_text(lpc_string_getval(val)), "tls") != 0) {
	    lpc_runtime_error(f, "TLS session not started");
	}
    }
}

/*
 * save references in the object
 */
static void save(LPC_dataspace data, SSL *tls, BIO *bio)
{
    LPC_array array;
    LPC_value val;
    LPC_string str;
    char *text;

    array = lpc_array_new(data, 2);
    val = lpc_value_temp(data);
    lpc_array_putval(val, array);
    lpc_data_set_val(data, val);
    lpc_string_putval(val, lpc_string_new(data, "tls", 3));
    lpc_array_assign(data, array, 0, val);
    str = lpc_string_new(data, NULL, 2 * sizeof(void *));
    text = lpc_string_text(str);
    memcpy(text, &tls, sizeof(void *));
    memcpy(text + sizeof(void *), &bio, sizeof(void *));
    lpc_string_putval(val, str);
    lpc_array_assign(data, array, 1, val);
}

/*
 * restore references from the object
 */
static void restore(LPC_dataspace data, SSL **ptls, BIO **pbio)
{
    LPC_array array;
    char *text;

    array = lpc_array_getval(lpc_data_get_val(data));
    text = lpc_string_text(lpc_string_getval(lpc_array_index(array, 1)));
    memcpy(ptls, text, sizeof(void *));
    memcpy(pbio, text + sizeof(void *), sizeof(void *));
}

/*
 * start a new session
 */
static bool init(LPC_frame f, const SSL_METHOD *method, SSL **ptls, BIO **pbio)
{
    SSL_CTX *context;
    SSL *tls;
    BIO *internal, *external;

    context = SSL_CTX_new(method);
    if (context == NULL) {
	ERR_clear_error();
	return FALSE;
    }
    tls = SSL_new(context);
    if (tls == NULL) {
	ERR_clear_error();
	SSL_CTX_free(context);
	return FALSE;
    }
    if (BIO_new_bio_pair(&internal, 0, &external, 0) <= 0) {
	ERR_clear_error();
	SSL_free(tls);
	SSL_CTX_free(context);
	return FALSE;
    }
    SSL_set_bio(tls, internal, internal);

    *ptls = tls;
    *pbio = external;
    return TRUE;
}

/*
 * prepare to use a certification chain and key (PEM format)
 */
static bool cert_key(SSL *tls, char *certificate, char *key)
{
    char buffer[2048];

    if (strpbrk(certificate, "/\\") != NULL || strlen(certificate) > 1023 ||
	strpbrk(key, "/\\") != NULL || strlen(key) > 1023) {
	return FALSE;
    }
    sprintf(buffer, "%s/%s", dir, certificate);
    if (SSL_use_certificate_chain_file(tls, buffer) <= 0) {
	ERR_clear_error();
	return FALSE;
    }
    sprintf(buffer, "%s/%s", dir, key);
    if (SSL_use_PrivateKey_file(tls, buffer, SSL_FILETYPE_PEM) <= 0) {
	ERR_clear_error();
	return FALSE;
    }
    return TRUE;
}

/*
 * start a TLS server session
 */
static void kf_tls_server(LPC_frame f, int nargs, LPC_value retval)
{
    SSL *tls;
    BIO *bio;
    LPC_value val;
    char *certificate;
    char *key;

    check(f, TRUE);
    if (!init(f, TLS_server_method(), &tls, &bio)) {
	lpc_runtime_error(f, "Failed to initialize TLS session");
    }

    /* process arguments */
    val = lpc_frame_arg(f, nargs, 0);
    certificate = lpc_string_text(lpc_string_getval(val));
    val = lpc_frame_arg(f, nargs, 1);
    key = lpc_string_text(lpc_string_getval(val));
    if (!cert_key(tls, certificate, key)) {
	SSL_CTX *context;

	context = SSL_get_SSL_CTX(tls);
	SSL_free(tls);
	BIO_free(bio);
	SSL_CTX_free(context);
	lpc_runtime_error(f, "Bad certificate/key");
    }

    /* start session */
    lpc_object_mark(lpc_frame_object(f));
    save(lpc_frame_dataspace(f), tls, bio);
    SSL_accept(tls);
}

/*
 * start a TLS client session
 */
static void kf_tls_client(LPC_frame f, int nargs, LPC_value retval)
{
    SSL *tls;
    BIO *bio;
    LPC_value val;
    char *certificate;
    char *key;
    LPC_dataspace data;
    size_t size;
    LPC_string str;

    if (nargs == 1) {
	lpc_runtime_error(f, "Too few arguments for kfun tls_client");
    }

    check(f, TRUE);
    if (!init(f, TLS_client_method(), &tls, &bio)) {
	lpc_runtime_error(f, "Failed to initialize TLS session");
    }
    if (nargs != 0) {
	/* process arguments */
	val = lpc_frame_arg(f, nargs, 0);
	certificate = lpc_string_text(lpc_string_getval(val));
	val = lpc_frame_arg(f, nargs, 1);
	key = lpc_string_text(lpc_string_getval(val));
	if (!cert_key(tls, certificate, key)) {
	    SSL_CTX *context;

	    context = SSL_get_SSL_CTX(tls);
	    SSL_free(tls);
	    BIO_free(bio);
	    SSL_CTX_free(context);
	    lpc_runtime_error(f, "Bad certificate/key");
	}
    }

    /* start session */
    data = lpc_frame_dataspace(f);
    lpc_object_mark(lpc_frame_object(f));
    save(data, tls, bio);
    SSL_connect(tls);

    /* return TLS handshake */
    size = BIO_ctrl_pending(bio);
    str = lpc_string_new(data, NULL, size);
    BIO_read(bio, lpc_string_text(str), size);
    lpc_string_putval(retval, str);
}

/*
 * send a message
 */
static void kf_tls_send(LPC_frame f, int nargs, LPC_value retval)
{
    char buffer[65535];
    LPC_dataspace data;
    SSL *tls;
    BIO *bio;
    LPC_string str;
    const char *text;
    size_t len, size, buflen;
    int ret;
    LPC_array array;
    LPC_value val;

    check(f, FALSE);
    data = lpc_frame_dataspace(f);
    restore(data, &tls, &bio);

    str = lpc_string_getval(lpc_frame_arg(f, nargs, 0));
    text = lpc_string_text(str);
    len = lpc_string_length(str);
    buflen = 0;
    do {
	/* send bytes through TLS */
	size = 0;
	ret = SSL_write_ex(tls, text, len, &size);
	if (ret <= 0) {
	    ret = SSL_get_error(tls, ret);
	    if (ret != SSL_ERROR_WANT_READ &&
		ret != SSL_ERROR_WANT_WRITE) {
		if (ret == SSL_ERROR_SSL) {
		    text = ERR_reason_error_string(ERR_get_error());
		    if (text == NULL) {
			text = "unknown error";
		    }
		} else {
		    text = "unknown error";
		}
		sprintf(buffer, "TLS: %s", text);
		ERR_clear_error();
		lpc_runtime_error(f, buffer);
	    }
	    ERR_clear_error();
	}
	text += size;
	len -= size;

	/* retrieve sent bytes */
	if (BIO_ctrl_pending(bio) != 0) {
	    BIO_read_ex(bio, buffer + buflen, sizeof(buffer) - buflen, &size);
	    buflen += size;
	}
    } while (len != 0 && size != 0 && buflen != sizeof(buffer));

    /* return ({ left-to-send, sent }) */
    array = lpc_array_new(data, 2);
    val = lpc_value_temp(data);
    if (len != 0) {
	str = lpc_string_new(data, text + lpc_string_length(str) - len, len);
	lpc_string_putval(val, str);
	lpc_array_assign(data, array, 0, val);
    }
    if (buflen != 0) {
	str = lpc_string_new(data, buffer, buflen);
	lpc_string_putval(val, str);
	lpc_array_assign(data, array, 1, val);
    }
    lpc_array_putval(retval, array);
}

/*
 * receive a message
 */
static void kf_tls_receive(LPC_frame f, int nargs, LPC_value retval)
{
    char buffer[65535], outbuf[65535];
    LPC_dataspace data;
    SSL *tls;
    BIO *bio;
    LPC_string str;
    const char *text;
    size_t len, progress, size, buflen, outbuflen;
    int ret;
    LPC_array array;
    LPC_value val;

    check(f, FALSE);
    data = lpc_frame_dataspace(f);
    restore(data, &tls, &bio);

    str = lpc_string_getval(lpc_frame_arg(f, nargs, 0));
    text = lpc_string_text(str);
    len = lpc_string_length(str);
    buflen = outbuflen = 0;
    do {
	progress = 0;

	/* received bytes */
	if (len != 0) {
	    BIO_write_ex(bio, text, len, &size);
	    text += size;
	    len -= size;
	    progress = size;
	}

	/* retrieve bytes through TLS */
	size = 0;
	ret = SSL_read_ex(tls, buffer + buflen, sizeof(buffer) - buflen, &size);
	if (ret <= 0) {
	    ret = SSL_get_error(tls, ret);
	    if (ret != SSL_ERROR_WANT_READ &&
		ret != SSL_ERROR_WANT_WRITE &&
		ret != SSL_ERROR_ZERO_RETURN) {
		if (ret == SSL_ERROR_SSL) {
		    text = ERR_reason_error_string(ERR_get_error());
		    if (text == NULL) {
			text = "unknown error";
		    }
		} else {
		    text = "unknown error";
		}
		sprintf(buffer, "TLS: %s", text);
		ERR_clear_error();
		lpc_runtime_error(f, buffer);
	    }
	    ERR_clear_error();
	} else {
	    progress += size;
	}
	buflen += size;

	if (BIO_ctrl_pending(bio) != 0) {
	    BIO_read_ex(bio, outbuf + outbuflen, sizeof(outbuf) - outbuflen,
			&size);
	    outbuflen += size;
	    progress += size;
	}
    } while (len != 0 && progress != 0 && buflen != sizeof(buffer) &&
	     outbuflen != sizeof(outbuf));

    /* return ({ left-to-receive, received, sent, "EOF" }) */
    array = lpc_array_new(data, 4);
    val = lpc_value_temp(data);
    if (len != 0) {
	str = lpc_string_new(data, text + lpc_string_length(str) - len, len);
	lpc_string_putval(val, str);
	lpc_array_assign(data, array, 0, val);
    }
    if (buflen != 0) {
	lpc_string_putval(val, lpc_string_new(data, buffer, buflen));
	lpc_array_assign(data, array, 1, val);
    }
    if (outbuflen != 0) {
	lpc_string_putval(val, lpc_string_new(data, outbuf, outbuflen));
	lpc_array_assign(data, array, 2, val);
    }
    if (BIO_eof(bio)) {
	lpc_string_putval(val, lpc_string_new(data, "EOF", 3));
	lpc_array_assign(data, array, 3, val);
    }
    lpc_array_putval(retval, array);
}

/*
 * close a TLS session
 */
static void kf_tls_close(LPC_frame f, int nargs, LPC_value retval)
{
    char buffer[65535];
    LPC_dataspace data;
    SSL *tls;
    BIO *bio;
    size_t buflen;
    SSL_CTX *context;

    check(f, FALSE);
    data = lpc_frame_dataspace(f);
    restore(data, &tls, &bio);

    /* close TLS session */
    SSL_shutdown(tls);
    buflen = 0;
    if (BIO_ctrl_pending(bio) != 0) {
	/* flush output buffer */
	BIO_read_ex(bio, buffer, sizeof(buffer), &buflen);
    }

    /* clean up */
    context = SSL_get_SSL_CTX(tls);
    SSL_free(tls);
    BIO_free(bio);
    SSL_CTX_free(context);
    lpc_object_unmark(lpc_frame_object(f));

    /* return flushed buffer */
    if (buflen != 0) {
	lpc_string_putval(retval, lpc_string_new(data, buffer, buflen));
    }
}


static char tls_server_proto[] = { LPC_TYPE_VOID, LPC_TYPE_STRING,
				   LPC_TYPE_STRING, 0 };
static char tls_client_proto[] = { LPC_TYPE_STRING, LPC_TYPE_VARARGS,
				   LPC_TYPE_STRING, LPC_TYPE_STRING, 0 };
static char tls_sendrecv_proto[] = { LPC_TYPE_ARRAY_OF(LPC_TYPE_STRING),
				 LPC_TYPE_STRING, 0 };
static char tls_close_proto[] = { LPC_TYPE_STRING, 0 };

static LPC_ext_kfun kf[] = {
    { "tls_server", tls_server_proto, &kf_tls_server },
    { "tls_client", tls_client_proto, &kf_tls_client },
    { "tls_send", tls_sendrecv_proto, &kf_tls_send },
    { "tls_receive", tls_sendrecv_proto, &kf_tls_receive },
    { "tls_close", tls_close_proto, &kf_tls_close }
};

/*
 * initialize TLS extension module
 */
int lpc_ext_init(int major, int minor, const char *config)
{
    strcpy(dir, config);
    lpc_ext_kfun(kf, 5);
    return 1;
}
