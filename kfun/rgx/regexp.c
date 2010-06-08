# include <sys/types.h>
# include <stdlib.h>
# include <string.h>
# include "libiberty/xregex.h"
# include "lpc_ext.h"

/*
 * ({ "regexp", pattern, compiled, used, bits, fastmap })
 */
# define RGX_HEADER	0		/* "regexp" */
# define RGX_PATTERN	1		/* the pattern */
# define RGX_COMPILED	2		/* the compiled pattern */
# define RGX_USED	3		/* bytes used in compiled pattern */
# define RGX_BITS	4		/* various values and bitflags */
# define RGX_FASTMAP	5		/* fastmap buffer */
# define RGX_SIZE	6		/* size of the regexp container */

# define BITS_NSUB	0x00ffff	/* # subexpressions */
# define BITS_NOCASE	0x010000	/* does case not matter? */
# define BITS_CANBENULL	0x020000	/* match empty string? */
# define BITS_FASTACC	0x040000	/* fastmap accurate? */
# define BITS_NOSUB	0x080000	/* no info about subexpressions? */
# define BITS_NOTBOL	0x100000	/* anchor does not match BOL? */
# define BITS_NOTEOL	0x200000	/* anchor does not match EOL? */
# define BITS_NLANCHOR	0x400000	/* anchor matches NL? */


static char trans[256];			/* translation to lower case */

/*
 * NAME:	rgx->init()
 * DESCRIPTION:	initialize regular expression handling
 */
static void rgx_init(void)
{
    int i;

    for (i = 0; i < 256; i++) {
	trans[i] = i;
    }
    for (i = 'A'; i <= 'Z'; i++) {
	trans[i] = i + 'a' - 'A';
    }

    /*
     * Set regexp syntax here, if required.  For example:
     *
     * re_set_syntax(_RE_SYNTAX_POSIX_BASIC);
     */
}

/*
 * NAME:	rgx->new()
 * DESCRIPTION:	create a regexp container, and associate it with the object
 */
static LPC_array rgx_new(LPC_dataspace data, LPC_object obj)
{
    LPC_array a;
    LPC_string str;
    LPC_value val;

    /* create the regexp container */
    a = lpc_array_new(data, RGX_SIZE);
    str = lpc_string_new(data, "regexp", 6);
    val = lpc_value_temp(data);
    lpc_string_putval(val, str);
    lpc_array_assign(data, a, RGX_HEADER, val);

    /* associate it with the given object */
    lpc_object_mark(obj);
    lpc_array_putval(val, a);
    lpc_data_set_val(data, val);

    return a;
}

/*
 * NAME:	rgx->get()
 * DESCRIPTION:	retrieve a regexp container from an object
 */
static LPC_array rgx_get(LPC_frame f, LPC_dataspace data, LPC_object obj)
{
    LPC_array a;
    LPC_string str;
    LPC_value val;
    int special;

    special = 0;
    if (lpc_object_isspecial(obj)) {
	/*
	 * special object
	 */
	special = 1;
	if (lpc_object_ismarked(obj)) {
	    /*
	     * object was marked by kfun extension
	     */
	    val = lpc_data_get_val(data);
	    if (lpc_value_type(val) == LPC_TYPE_ARRAY) {
		a = lpc_array_getval(val);
		if (lpc_array_size(a) == RGX_SIZE) {
		    val = lpc_array_index(a, RGX_HEADER);
		    if (lpc_value_type(val) == LPC_TYPE_STRING) {
			str = lpc_string_getval(val);
			if (lpc_string_length(str) == 6 &&
			    strcmp(lpc_string_text(str), "regexp") == 0) {
			    /*
			     * value is an array of 6 elements, and the
			     * first element is the string "regexp"
			     */
			    return a;
			}
		    }
		}
	    }
	}
    }
    if (special) {
	/*
	 * special object, possibly marked by a different kfun extension
	 */
	lpc_error(f, "Regexp in special object");
    }

    /*
     * nothing stored for this object yet
     */
    return NULL;
}

/*
 * NAME:	rgx->same()
 * DESCRIPTION:	return 1 if the given regular expression is the same as the
 *		one in the regexp container, or 0 otherwise
 */
static int rgx_same(LPC_array a, LPC_string pattern, int casef)
{
    LPC_string str;
    LPC_value val;

    /*
     * check whether the pattern is the same
     */
    val = lpc_array_index(a, RGX_PATTERN);
    if (lpc_value_type(val) == LPC_TYPE_STRING) {
	str = lpc_string_getval(val);
	if (lpc_string_length(str) == lpc_string_length(pattern) &&
	    memcmp(lpc_string_text(str), lpc_string_text(pattern),
		   lpc_string_length(str)) == 0) {
	    /*
	     * same pattern, now check the flags
	     */
	    val = lpc_array_index(a, RGX_BITS);
	    if (!!(lpc_int_getval(val) & BITS_NOCASE) == casef) {
		return 1;	/* same */
	    }
	}
    }

    return 0;
}

/*
 * NAME:	rgx->compile()
 * DESCRIPTION:	compile a regular expression, and store it in the regexp
 *		container
 */
static void rgx_compile(LPC_frame f, LPC_dataspace data, LPC_array a,
			LPC_string pattern, int casef)
{
    struct re_pattern_buffer regbuf;
    char fastmap[256];
    char *err;
    LPC_string str;
    LPC_value val;
    int bits;

    /* initialize */
    memset(&regbuf, '\0', sizeof(struct re_pattern_buffer));
    if (casef) {
	regbuf.translate = trans;
    }
    regbuf.fastmap = fastmap;
    regbuf.syntax = re_syntax_options;

    /* compile pattern */
    err = (char *) re_compile_pattern(lpc_string_text(pattern),
				      lpc_string_length(pattern), &regbuf);
    if (err != NULL) {
	regbuf.translate = NULL;
	regbuf.fastmap = NULL;
	regfree(&regbuf);
	lpc_error(f, err);
    }
    if (regbuf.allocated > 65535) {
	regbuf.translate = NULL;
	regbuf.fastmap = NULL;
	regfree(&regbuf);
	lpc_error(f, "Regular expression too large");
    }

    /* compile fastmap */
    if (re_compile_fastmap(&regbuf) != 0) {
	regbuf.translate = NULL;
	regbuf.fastmap = NULL;
	regfree(&regbuf);
	lpc_error(f, "Regexp internal error");
    }

    /* pattern */
    val = lpc_value_temp(data);
    lpc_string_putval(val, pattern);
    lpc_array_assign(data, a, RGX_PATTERN, val);
    /* compiled */
    str = lpc_string_new(data, (char *) regbuf.buffer, regbuf.allocated);
    lpc_string_putval(val, str);
    lpc_array_assign(data, a, RGX_COMPILED, val);
    /* used */
    lpc_int_putval(val, regbuf.used);
    lpc_array_assign(data, a, RGX_USED, val);
    /* bits */
    bits = regbuf.re_nsub;
    if (casef) {
	bits |= BITS_NOCASE;
    }
    if (regbuf.can_be_null) {
	bits |= BITS_CANBENULL;
    }
    if (regbuf.fastmap_accurate) {
	bits |= BITS_FASTACC;
    }
    if (regbuf.no_sub) {
	bits |= BITS_NOSUB;
    }
    if (regbuf.not_bol) {
	bits |= BITS_NOTBOL;
    }
    if (regbuf.not_eol) {
	bits |= BITS_NOTEOL;
    }
    if (regbuf.newline_anchor) {
	bits |= BITS_NLANCHOR;
    }
    lpc_int_putval(val, bits);
    lpc_array_assign(data, a, RGX_BITS, val);
    /* fastmap */
    str = lpc_string_new(data, fastmap, 256);
    lpc_string_putval(val, str);
    lpc_array_assign(data, a, RGX_FASTMAP, val);

    /* clean up */
    regbuf.translate = NULL;
    regbuf.fastmap = NULL;
    regfree(&regbuf);
}

/*
 * NAME:	rgx->start()
 * DESCRIPTION:	initialize a regbuf from a regexp container
 */
static void rgx_start(struct re_pattern_buffer *regbuf, LPC_array a)
{
    LPC_value val;
    LPC_string str;
    int bits;

    memset(regbuf, '\0', sizeof(struct re_pattern_buffer));
    regbuf->syntax = re_syntax_options;

    /* compiled */
    val = lpc_array_index(a, RGX_COMPILED);
    str = lpc_string_getval(val);
    regbuf->buffer = (unsigned char *) lpc_string_text(str);
    regbuf->allocated = lpc_string_length(str);
    /* used */
    val = lpc_array_index(a, RGX_USED);
    regbuf->used = lpc_int_getval(val);
    /* bits */
    val = lpc_array_index(a, RGX_BITS);
    bits = lpc_int_getval(val);
    regbuf->re_nsub = bits & BITS_NSUB;
    if (bits & BITS_NOCASE) {
	regbuf->translate = trans;
    }
    if (bits & BITS_CANBENULL) {
	regbuf->can_be_null = 1;
    }
    if (bits & BITS_FASTACC) {
	regbuf->fastmap_accurate = 1;
    }
    if (bits & BITS_NOSUB) {
	regbuf->no_sub = 1;
    }
    if (bits & BITS_NOTBOL) {
	regbuf->not_bol = 1;
    }
    if (bits & BITS_NOTEOL) {
	regbuf->not_eol = 1;
    }
    if (bits & BITS_NLANCHOR) {
	regbuf->newline_anchor = 1;
    }
    /* fastmap */
    val = lpc_array_index(a, RGX_FASTMAP);
    str = lpc_string_getval(val);
    regbuf->fastmap = lpc_string_text(str);
}

/*
 * NAME:	regexp()
 * DESCRIPTION:	regular expression kfun
 */
static void regexp(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_value val;
    LPC_string pattern, str;
    int casef, reverse, len, size, i;
    LPC_dataspace data;
    LPC_object obj;
    LPC_array a;
    struct re_pattern_buffer regbuf;
    struct re_registers regs;
    regoff_t starts[RE_NREGS + 1], ends[RE_NREGS + 1];

    data = lpc_frame_dataspace(f);
    obj = lpc_frame_object(f);

    /* getopt */
    casef = reverse = 0;
    val = lpc_frame_arg(f, nargs, 0);	    pattern = lpc_string_getval(val);
    val = lpc_frame_arg(f, nargs, 1);	    str = lpc_string_getval(val);
    if (nargs >= 3) {
	val = lpc_frame_arg(f, nargs, 2);   casef = !!lpc_int_getval(val);
    }
    if (nargs >= 4) {
	val = lpc_frame_arg(f, nargs, 3);   reverse = lpc_int_getval(val);
    }

    /* get regexp container */
    a = rgx_get(f, data, obj);
    if (a == NULL) {
	a = rgx_new(data, obj);
    }
    if (!rgx_same(a, pattern, casef)) {
	rgx_compile(f, data, a, pattern, casef);
    }

    /* initialize from regexp container */
    rgx_start(&regbuf, a);
    regbuf.regs_allocated = REGS_FIXED;
    regs.num_regs = RE_NREGS;
    regs.start = starts;
    regs.end = ends;

    /* match */
    len = lpc_string_length(str);
    if (re_search(&regbuf, lpc_string_text(str), len, (reverse) ? len : 0,
		  (reverse) ? -len : len, &regs) != -1) {
	/*
	 * match found, create an array with results
	 */
	val = lpc_value_temp(data);
	size = regbuf.re_nsub + 1;
	a = lpc_array_new(data, size * 2);
	for (i = 0; i < size; i++) {
	    lpc_int_putval(val, regs.start[i]);
	    lpc_array_assign(data, a, i * 2, val);
	    lpc_int_putval(val, regs.end[i] - 1);
	    lpc_array_assign(data, a, i * 2 + 1, val);
	}
	lpc_array_putval(retval, a);
    }
}

static char regexp_proto[] = { LPC_TYPE_ARRAY_OF(LPC_TYPE_INT),
			       LPC_TYPE_STRING, LPC_TYPE_STRING,
			       LPC_TYPE_VARARGS, LPC_TYPE_INT, LPC_TYPE_INT };
static LPC_ext_kfun kf_regexp[1] = {
    "regexp",
    regexp_proto,
    &regexp
};

/*
 * NAME:	extension_init()
 * DESCRIPTION:	add regexp kfun
 */
void extension_init(void)
{
    rgx_init();
    lpc_ext_kfun(kf_regexp, 1);
}
