/*
 * zlib kfun extensions
 *
 * This code is released into the public domain.
 */

# include <stdint.h>
# include "deflate.h"
# include "inftrees.h"
# include "inflate.h"
# include "lpc_ext.h"


# define STRSTR(s)		#s
# define STR(s)			STRSTR(s)

typedef struct {
    LPC_dataspace data;		/* dataspace */
    LPC_array array;		/* storage array */
    int index;			/* index in storage array */
} AllocContext;

typedef struct Output {
    struct Output *next;	/* next in linked list */
    int size;			/* size of output buffer */
    char buffer[65535];		/* output buffer */
} Output;

/*
 * ({ "deflate", stream, tails, allocbits... })
 */

# define A_SIGNATURE	0	/* gzip, gunzip, deflate, inflate */
# define A_STREAM	1	/* stream struct */
# define A_BYTES	2	/* extra bytes */
# define A_ALLOCATED	3	/* first of allocated blocks */

/*
 * save a chunk as a string in the state array
 */
static void *save(LPC_dataspace data, LPC_array arr, int index, void *buffer,
		  int size)
{
    LPC_string str;
    LPC_value val;

    str = lpc_string_new(data, buffer, size);
    val = lpc_value_temp(data);
    lpc_string_putval(val, str);
    lpc_array_assign(data, arr, index, val);

    return (void *) lpc_string_text(str);
}

/*
 * restore a chunk from the state array
 */
static void *restore(LPC_array arr, int index)
{
    LPC_value val;

    val = lpc_array_index(arr, index);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	return NULL;
    }

    return (void *) lpc_string_text(lpc_string_getval(val));
}

/*
 * restore and refresh a chunk
 */
static void *restore_new(LPC_dataspace data, LPC_array arr, int index)
{
    LPC_value val;
    LPC_string str;

    val = lpc_array_index(arr, index);
    if (lpc_value_type(val) != LPC_TYPE_STRING) {
	return NULL;
    }
    str = lpc_string_getval(val);
    str = lpc_string_new(data, lpc_string_text(str), lpc_string_length(str));
    val = lpc_value_temp(data);
    lpc_string_putval(val, str);
    lpc_array_assign(data, arr, index, val);

    return lpc_string_text(str);
}

/*
 * allocate a string as storage for a chunk
 */
static void *alloc(void *opaque, unsigned int items, unsigned int size)
{
    AllocContext *context;
    unsigned int len;

    context = opaque;
    len = items * size;
    if (len == 65536) {
	len = 65535;	/* string followed by \0 byte */
    }

    return save(context->data, context->array, context->index++, NULL, len);
}

/*
 * free memory (actually, don't)
 */
static void dealloc(void *opaque, void *mem)
{
}

/*
 * save the tail byte of a block in a separate location
 */
static void save_tail(char *tail, LPC_array arr, int index)
{
    LPC_string str;
    char *text;

    text = restore(arr, A_ALLOCATED + index);
    tail[index] = text[65535];
    text[65535] = '\0';
}

/*
 * restore a preserved tail byte to the original spot
 */
static void restore_tail(char *tail, LPC_array arr, int index)
{
    char *text;

    text = restore(arr, A_ALLOCATED + index);
    text[65535] = tail[index];
}

# define PDIFF(p1, p2)		(void *) ((char *) (void *) p1 - \
					  (char *) (void *) p2)
# define PADD(p1, p2)		(void *) ((char *) (void *) p2 + (intptr_t) p1)

/*
 * save datastructures for deflate
 */
static void save_deflate(LPC_array arr, z_stream *stream)
{
    char *tail;
    deflate_state *state;

    tail = restore(arr, A_BYTES);
    save_tail(tail, arr, 1);
    save_tail(tail, arr, 2);
    save_tail(tail, arr, 3);
    save_tail(tail, arr, 4);

    state = (deflate_state *) stream->state;
    state->pending_out = PDIFF(state->pending_out, state->pending_buf);
    state->l_buf = PDIFF(state->l_buf, state->pending_buf);
    state->d_buf = PDIFF(state->d_buf, state->pending_buf);
}

/*
 * restore datastructures for deflate
 */
static z_stream *restore_deflate(LPC_dataspace data, LPC_array arr)
{
    deflate_state resetState;
    z_stream resetStream;
    deflate_state *state;
    z_stream *stream;
    char *tail;

    state = restore_new(data, arr, A_ALLOCATED);
    state->window = restore_new(data, arr, A_ALLOCATED + 1);
    state->prev = restore_new(data, arr, A_ALLOCATED + 2);
    state->head = restore_new(data, arr, A_ALLOCATED + 3);
    state->pending_buf = restore_new(data, arr, A_ALLOCATED + 4);
    state->pending_out = PADD(state->pending_out, state->pending_buf);
    state->l_buf = PADD(state->l_buf, state->pending_buf);
    state->d_buf = PADD(state->d_buf, state->pending_buf);
    state->l_desc.dyn_tree = state->dyn_ltree;
    state->d_desc.dyn_tree = state->dyn_dtree;
    state->bl_desc.dyn_tree = state->bl_tree;
    state->strm = stream = restore_new(data, arr, A_STREAM);
    stream->state = (void *) state;
    stream->zalloc = &alloc;
    stream->zfree = &dealloc;

    tail = restore_new(data, arr, A_BYTES);
    restore_tail(tail, arr, 1);
    restore_tail(tail, arr, 2);
    restore_tail(tail, arr, 3);
    restore_tail(tail, arr, 4);

    resetState.strm = &resetStream;
    resetState.status = INIT_STATE;
    resetStream.zalloc = &alloc;
    resetStream.zfree = &dealloc;
    resetStream.state = (void *) &resetState;
    deflateResetKeep(&resetStream);
    state->l_desc.stat_desc = resetState.l_desc.stat_desc;
    state->d_desc.stat_desc = resetState.d_desc.stat_desc;
    state->bl_desc.stat_desc = resetState.bl_desc.stat_desc;

    return stream;
}

/*
 * save datastructures for inflate
 */
static void save_inflate(LPC_array arr, z_stream *stream)
{
    struct inflate_state *state;
    char *flags;

    state = (void *) stream->state;
    flags = restore(arr, A_BYTES);

    if (state->lencode >= state->codes &&
	state->lencode <= &state->codes[ENOUGH]) {
	state->lencode = PDIFF(state->lencode, state->codes);
	flags[0] = 1;
    } else {
	flags[0] = 0;
    }
    if (state->distcode >= state->codes &&
	state->distcode <= &state->codes[ENOUGH]) {
	state->distcode = PDIFF(state->distcode, state->codes);
	flags[1] = 1;
    } else {
	flags[1] = 0;
    }
    state->next = PDIFF(state->next, state->codes);
}

/*
 * restore datastructures for inflate
 */
static z_stream *restore_inflate(LPC_dataspace data, LPC_array arr)
{
    struct inflate_state *state;
    z_stream *stream;
    char *flags;

    state = restore_new(data, arr, A_ALLOCATED);
    state->window = restore_new(data, arr, A_ALLOCATED + 1);
    state->strm = stream = restore_new(data, arr, A_STREAM);
    stream->state = (void *) state;
    stream->zalloc = &alloc;
    stream->zfree = &dealloc;

    flags = restore_new(data, arr, A_BYTES);
    if (flags[0]) {
	state->lencode = PADD(state->lencode, state->codes);
    }
    if (flags[1]) {
	state->distcode = PADD(state->distcode, state->codes);
    }
    state->next = PADD(state->next, state->codes);

    return stream;
}

/*
 * check that the current object is, or can be used for (de)compression
 */
static LPC_array check(LPC_frame f, const char *signature)
{
    LPC_object obj;
    LPC_dataspace data;
    LPC_value val;
    LPC_array array;

    if (strcmp(ZLIB_VERSION, STR(VERSION)) != 0 ||
	strcmp(ZLIB_VERSION, zlibVersion()) != 0) {
	lpc_runtime_error(f, "Wrong zlib version");
    }

    obj = lpc_frame_object(f);
    if (obj == NULL) {
	lpc_runtime_error(f, "Object is not persistent");
    }

    data = lpc_frame_dataspace(f);
    val = lpc_data_get_val(data);
    if (lpc_object_ismarked(obj)) {
	if (lpc_value_type(val) != LPC_TYPE_ARRAY) {
	    lpc_runtime_error(f, "Object is already special");
	}

	array = lpc_array_getval(val);
	val = lpc_array_index(array, A_SIGNATURE);
	if (lpc_value_type(val) != LPC_TYPE_STRING ||
	    strcmp(lpc_string_text(lpc_string_getval(val)), signature) != 0) {
	    lpc_runtime_error(f, "Object is already special");
	}

	return array;
    } else {
	lpc_object_mark(obj);
	return NULL;
    }
}

/*
 * initialize (de)compression state
 */
static LPC_array init(LPC_dataspace data, const char *signature, int size)
{
    LPC_array array;
    LPC_value val;
    LPC_string str;
    z_stream *stream;

    array = lpc_array_new(data, A_ALLOCATED + size);
    val = lpc_value_temp(data);
    lpc_array_putval(val, array);
    lpc_data_set_val(data, val);
    lpc_string_putval(val, lpc_string_new(data, signature, strlen(signature)));
    lpc_array_assign(data, array, A_SIGNATURE, val);
    str = lpc_string_new(data, NULL, sizeof(z_stream));
    lpc_string_putval(val, str);
    lpc_array_assign(data, array, A_STREAM, val);
    lpc_string_putval(val, lpc_string_new(data, NULL, size));
    lpc_array_assign(data, array, A_BYTES, val);

    stream = (void *) lpc_string_text(str);
    stream->zalloc = &alloc;
    stream->zfree = &dealloc;

    return array;
}

/*
 * discard (de)compression state
 */
static void finish(LPC_frame f)
{
    lpc_data_set_val(lpc_frame_dataspace(f), lpc_value_nil());
    lpc_object_unmark(lpc_frame_object(f));
}

/*
 * process a (de)compression operation
 */
static LPC_array process(z_stream *stream, int (*func)(z_stream*, int),
			 LPC_dataspace data, int mode)
{
    Output out, *buf, *next;
    LPC_string str;
    int n;
    LPC_array array;
    LPC_value val;

    buf = &out;
    buf->next = NULL;
    buf->size = 0;

    stream->next_out = (Bytef *) buf->buffer;
    stream->avail_out = 65535;
    for (n = 1; ; n++) {
	(*func)(stream, mode);
	if (stream->avail_out != 0) {
	    break;
	}
	buf->size = 65535;
	next = buf;
	buf = (Output *) malloc(sizeof(Output));
	buf->next = next;
	buf->size = 0;
	stream->next_out = (Bytef *) buf->buffer;
	stream->avail_out = 65535;
    }
    buf->size = 65535 - stream->avail_out;

    array = lpc_array_new(data, n);
    val = lpc_value_temp(data);
    for (;;) {
	lpc_string_putval(val, lpc_string_new(data, buf->buffer, buf->size));
	lpc_array_assign(data, array, --n, val);
	if (n == 0) {
	    break;
	}
	next = buf->next;
	free(buf);
	buf = next;
    }

    return array;
}

/*
 * gzip compression
 */
static void kf_gzip(LPC_frame f, int nargs, LPC_value retval)
{
    AllocContext context;
    int n;
    LPC_array array;
    z_stream *stream;
    LPC_string str;
    int flush;

    n = (nargs > 1) ? lpc_int_getval(lpc_frame_arg(f, nargs, 1)) : 0;
    context.data = lpc_frame_dataspace(f);
    context.index = A_ALLOCATED;

    context.array = check(f, "gzip");
    if (context.array == NULL) {
	context.array = init(context.data, "gzip", 5);
	stream = restore(context.array, A_STREAM);
	stream->next_in = NULL;
	stream->avail_in = 0;
	stream->opaque = &context;
	if (n < 1 || n > 9) {
	    n = Z_DEFAULT_COMPRESSION;
	}
	if (deflateInit2(stream, n, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY)
								    != Z_OK) {
	    lpc_runtime_error(f, stream->msg);
	}
	flush = Z_NO_FLUSH;
    } else {
	stream = restore_deflate(context.data, context.array);
	stream->next_in = NULL;
	stream->avail_in = 0;
	stream->opaque = &context;
	flush = (n != 0) ? Z_PARTIAL_FLUSH : Z_NO_FLUSH;
    }

    if (nargs > 0) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, 0));
	stream->next_in = (Bytef *) lpc_string_text(str);
	stream->avail_in = lpc_string_length(str);
	array = process(stream, &deflate, context.data, flush);
	save_deflate(context.array, stream);
    } else {
	/* finish */
	array = process(stream, &deflate, context.data, Z_FINISH);
	deflateEnd(stream);
	finish(f);
    }

    lpc_array_putval(retval, array);
}

/*
 * gunzip decompression
 */
static void kf_gunzip(LPC_frame f, int nargs, LPC_value retval)
{
    AllocContext context;
    LPC_array array;
    z_stream *stream;
    LPC_string str;
    int flush;

    context.data = lpc_frame_dataspace(f);
    context.index = A_ALLOCATED;

    context.array = check(f, "gunzip");
    if (context.array == NULL) {
	context.array = init(context.data, "gunzip", 2);
	stream = restore(context.array, A_STREAM);
	stream->next_in = NULL;
	stream->avail_in = 0;
	stream->opaque = &context;
	if (inflateInit2(stream, 31) != Z_OK) {
	    lpc_runtime_error(f, stream->msg);
	}
    } else {
	stream = restore_inflate(context.data, context.array);
	stream->next_in = NULL;
	stream->avail_in = 0;
	stream->opaque = &context;
    }

    flush = (nargs > 1 && lpc_int_getval(lpc_frame_arg(f, nargs, 1)) != 0) ?
	     Z_PARTIAL_FLUSH : Z_NO_FLUSH;
    if (nargs > 0) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, 0));
	stream->next_in = (Bytef *) lpc_string_text(str);
	stream->avail_in = lpc_string_length(str);
	array = process(stream, &inflate, context.data, flush);
	save_inflate(context.array, stream);
    } else {
	/* finish */
	array = process(stream, &inflate, context.data, Z_FINISH);
	inflateEnd(stream);
	finish(f);
    }

    lpc_array_putval(retval, array);
}

/*
 * deflate compression
 */
static void kf_deflate(LPC_frame f, int nargs, LPC_value retval)
{
    AllocContext context;
    int n;
    LPC_array array;
    z_stream *stream;
    LPC_string str;
    int flush;

    n = (nargs > 1) ? lpc_int_getval(lpc_frame_arg(f, nargs, 1)) : 0;
    context.data = lpc_frame_dataspace(f);
    context.index = A_ALLOCATED;

    context.array = check(f, "deflate");
    if (context.array == NULL) {
	context.array = init(context.data, "deflate", 5);
	stream = restore(context.array, A_STREAM);
	stream->next_in = NULL;
	stream->avail_in = 0;
	stream->opaque = &context;
	if (n < 1 || n > 9) {
	    n = Z_DEFAULT_COMPRESSION;
	}
	if (deflateInit2(stream, n, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY)
								    != Z_OK) {
	    lpc_runtime_error(f, stream->msg);
	}
	flush = Z_NO_FLUSH;
    } else {
	stream = restore_deflate(context.data, context.array);
	stream->next_in = NULL;
	stream->avail_in = 0;
	stream->opaque = &context;
	flush = (n != 0) ? Z_PARTIAL_FLUSH : Z_NO_FLUSH;
    }

    if (nargs > 0) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, 0));
	stream->next_in = (Bytef *) lpc_string_text(str);
	stream->avail_in = lpc_string_length(str);
	array = process(stream, &deflate, context.data, flush);
	save_deflate(context.array, stream);
    } else {
	/* finish */
	array = process(stream, &deflate, context.data, Z_FINISH);
	deflateEnd(stream);
	finish(f);
    }

    lpc_array_putval(retval, array);
}

/*
 * inflate decompression
 */
static void kf_inflate(LPC_frame f, int nargs, LPC_value retval)
{
    AllocContext context;
    LPC_array array;
    z_stream *stream;
    LPC_string str;
    int flush;

    context.data = lpc_frame_dataspace(f);
    context.index = A_ALLOCATED;

    context.array = check(f, "inflate");
    if (context.array == NULL) {
	context.array = init(context.data, "inflate", 2);
	stream = restore(context.array, A_STREAM);
	stream->next_in = NULL;
	stream->avail_in = 0;
	stream->opaque = &context;
	if (inflateInit2(stream, 15) != Z_OK) {
	    lpc_runtime_error(f, stream->msg);
	}
    } else {
	stream = restore_inflate(context.data, context.array);
	stream->next_in = NULL;
	stream->avail_in = 0;
	stream->opaque = &context;
    }

    flush = (nargs > 1 && lpc_int_getval(lpc_frame_arg(f, nargs, 1)) != 0) ?
	     Z_PARTIAL_FLUSH : Z_NO_FLUSH;
    if (nargs > 0) {
	str = lpc_string_getval(lpc_frame_arg(f, nargs, 0));
	stream->next_in = (Bytef *) lpc_string_text(str);
	stream->avail_in = lpc_string_length(str);
	array = process(stream, &inflate, context.data, flush);
	save_inflate(context.array, stream);
    } else {
	/* finish */
	array = process(stream, &inflate, context.data, Z_FINISH);
	inflateEnd(stream);
	finish(f);
    }

    lpc_array_putval(retval, array);
}

static char prototype[] = { LPC_TYPE_ARRAY_OF(LPC_TYPE_STRING),
			    LPC_TYPE_VARARGS, LPC_TYPE_STRING, LPC_TYPE_INT,
			    0 };

static LPC_ext_kfun kf[4] = {
    {
	"gzip",
	prototype,
	&kf_gzip
    },
    {
	"gunzip",
	prototype,
	&kf_gunzip
    },
    {
	"deflate",
	prototype,
	&kf_deflate
    },
    {
	"inflate",
	prototype,
	&kf_inflate
    }
};

int lpc_ext_init(int major, int minor, const char *config)
{
    lpc_ext_kfun(kf, 4);
    return 1;
}
