# include <sys/types.h>
# include <stdlib.h>
# include "libiberty/xregex.h"
# include "dgd_ext.h"

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
static DGD_ARRAY_T rgx_new(DGD_DATASPACE_T data, DGD_OBJECT_T obj)
{
    DGD_ARRAY_T a;
    DGD_STRING_T str;
    DGD_VALUE_T val;

    /* create the regexp container */
    a = DGD_ARRAY_NEW(data, RGX_SIZE);
    str = DGD_STRING_NEW(data, "regexp", 6);
    DGD_STRING_PUTVAL(val, str);
    DGD_ARRAY_ASSIGN(data, a, RGX_HEADER, val);

    /* associate it with the given object */
    DGD_OBJECT_MARK(obj);
    DGD_ARRAY_PUTVAL(val, a);
    DGD_DATA_SET_VAL(data, val);

    return a;
}

/*
 * NAME:	rgx->get()
 * DESCRIPTION:	retrieve a regexp container from an object
 */
static DGD_ARRAY_T rgx_get(DGD_FRAME_T f, DGD_DATASPACE_T data,
			   DGD_OBJECT_T obj)
{
    DGD_ARRAY_T a;
    DGD_STRING_T str;
    DGD_VALUE_T val;
    int special;

    special = 0;
    if (DGD_OBJECT_ISSPECIAL(obj)) {
	/*
	 * special object
	 */
	special = 1;
	if (DGD_OBJECT_ISMARKED(obj)) {
	    /*
	     * object was marked by kfun extension
	     */
	    val = DGD_DATA_GET_VAL(data);
	    if (DGD_TYPEOF(val) == DGD_TYPE_ARRAY) {
		a = DGD_ARRAY_GETVAL(val);
		if (DGD_ARRAY_SIZE(a) == RGX_SIZE) {
		    val = DGD_ARRAY_INDEX(a, RGX_HEADER);
		    if (DGD_TYPEOF(val) == DGD_TYPE_STRING) {
			str = DGD_STRING_GETVAL(val);
			if (DGD_STRING_LENGTH(str) == 6 &&
			    strcmp(DGD_STRING_TEXT(str), "regexp") == 0) {
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
	DGD_ERROR(f, "Regexp in special object");
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
static int rgx_same(DGD_ARRAY_T a, DGD_STRING_T pattern, int casef)
{
    DGD_STRING_T str;
    DGD_VALUE_T val;

    /*
     * check whether the pattern is the same
     */
    val = DGD_ARRAY_INDEX(a, RGX_PATTERN);
    if (DGD_TYPEOF(val) == DGD_TYPE_STRING) {
	str = DGD_STRING_GETVAL(val);
	if (DGD_STRING_LENGTH(str) == DGD_STRING_LENGTH(pattern) &&
	    memcmp(DGD_STRING_TEXT(str), DGD_STRING_TEXT(pattern),
		   DGD_STRING_LENGTH(str)) == 0) {
	    /*
	     * same pattern, now check the flags
	     */
	    val = DGD_ARRAY_INDEX(a, RGX_BITS);
	    if (!!(DGD_INT_GETVAL(val) & BITS_NOCASE) == casef) {
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
static void rgx_compile(DGD_FRAME_T f, DGD_DATASPACE_T data, DGD_ARRAY_T a,
			DGD_STRING_T pattern, int casef)
{
    struct re_pattern_buffer regbuf;
    char fastmap[256];
    char *err;
    DGD_STRING_T str;
    DGD_VALUE_T val;
    int bits;

    /* initialize */
    memset(&regbuf, '\0', sizeof(struct re_pattern_buffer));
    if (casef) {
	regbuf.translate = trans;
    }
    regbuf.fastmap = fastmap;
    regbuf.syntax = re_syntax_options;

    /* compile pattern */
    err = (char *) re_compile_pattern(DGD_STRING_TEXT(pattern),
				      DGD_STRING_LENGTH(pattern), &regbuf);
    if (err != NULL) {
	regbuf.translate = NULL;
	regbuf.fastmap = NULL;
	regfree(&regbuf);
	DGD_ERROR(f, err);
    }
    if (regbuf.allocated > 65535) {
	regbuf.translate = NULL;
	regbuf.fastmap = NULL;
	regfree(&regbuf);
	DGD_ERROR(f, "Regular expression too large");
    }

    /* compile fastmap */
    if (re_compile_fastmap(&regbuf) != 0) {
	regbuf.translate = NULL;
	regbuf.fastmap = NULL;
	regfree(&regbuf);
	DGD_ERROR(f, "Regexp internal error");
    }

    /* pattern */
    DGD_STRING_PUTVAL(val, pattern);
    DGD_ARRAY_ASSIGN(data, a, RGX_PATTERN, val);
    /* compiled */
    str = DGD_STRING_NEW(data, (char *) regbuf.buffer, regbuf.allocated);
    DGD_STRING_PUTVAL(val, str);
    DGD_ARRAY_ASSIGN(data, a, RGX_COMPILED, val);
    /* used */
    DGD_INT_PUTVAL(val, regbuf.used);
    DGD_ARRAY_ASSIGN(data, a, RGX_USED, val);
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
    DGD_INT_PUTVAL(val, bits);
    DGD_ARRAY_ASSIGN(data, a, RGX_BITS, val);
    /* fastmap */
    str = DGD_STRING_NEW(data, fastmap, 256);
    DGD_STRING_PUTVAL(val, str);
    DGD_ARRAY_ASSIGN(data, a, RGX_FASTMAP, val);

    /* clean up */
    regbuf.translate = NULL;
    regbuf.fastmap = NULL;
    regfree(&regbuf);
}

/*
 * NAME:	rgx->start()
 * DESCRIPTION:	initialize a regbuf from a regexp container
 */
static void rgx_start(struct re_pattern_buffer *regbuf, DGD_ARRAY_T a)
{
    DGD_VALUE_T val;
    DGD_STRING_T str;
    int bits;

    memset(regbuf, '\0', sizeof(struct re_pattern_buffer));
    regbuf->syntax = re_syntax_options;

    /* compiled */
    val = DGD_ARRAY_INDEX(a, RGX_COMPILED);
    str = DGD_STRING_GETVAL(val);
    regbuf->buffer = (unsigned char *) DGD_STRING_TEXT(str);
    regbuf->allocated = DGD_STRING_LENGTH(str);
    /* used */
    val = DGD_ARRAY_INDEX(a, RGX_USED);
    regbuf->used = DGD_INT_GETVAL(val);
    /* bits */
    val = DGD_ARRAY_INDEX(a, RGX_BITS);
    bits = DGD_INT_GETVAL(val);
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
    val = DGD_ARRAY_INDEX(a, RGX_FASTMAP);
    str = DGD_STRING_GETVAL(val);
    regbuf->fastmap = DGD_STRING_TEXT(str);
}

/*
 * NAME:	regexp()
 * DESCRIPTION:	regular expression kfun
 */
static void regexp(DGD_FRAME_T f, int nargs, DGD_VALUE_T *retval)
{
    DGD_VALUE_T val;
    DGD_STRING_T pattern, str;
    int casef, reverse, len, size, i;
    DGD_DATASPACE_T data;
    DGD_OBJECT_T obj;
    DGD_ARRAY_T a;
    struct re_pattern_buffer regbuf;
    struct re_registers regs;
    regoff_t starts[RE_NREGS + 1], ends[RE_NREGS + 1];

    data = DGD_FRAME_DATASPACE(f);
    obj = DGD_FRAME_OBJECT(f);

    /* getopt */
    casef = reverse = 0;
    val = DGD_FRAME_ARG(f, nargs, 0);	    pattern = DGD_STRING_GETVAL(val);
    val = DGD_FRAME_ARG(f, nargs, 1);	    str = DGD_STRING_GETVAL(val);
    if (nargs >= 3) {
	val = DGD_FRAME_ARG(f, nargs, 2);   casef = !!DGD_INT_GETVAL(val);
    }
    if (nargs >= 4) {
	val = DGD_FRAME_ARG(f, nargs, 3);   reverse = DGD_INT_GETVAL(val);
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
    len = DGD_STRING_LENGTH(str);
    if (re_search(&regbuf, DGD_STRING_TEXT(str), len, (reverse) ? len : 0,
		  (reverse) ? -len : len, &regs) != -1) {
	/*
	 * match found, create an array with results
	 */
	size = regbuf.re_nsub + 1;
	a = DGD_ARRAY_NEW(data, size * 2);
	for (i = 0; i < size; i++) {
	    DGD_INT_PUTVAL(val, regs.start[i]);
	    DGD_ARRAY_ASSIGN(data, a, i * 2, val);
	    DGD_INT_PUTVAL(val, regs.end[i] - 1);
	    DGD_ARRAY_ASSIGN(data, a, i * 2 + 1, val);
	}
	DGD_RETVAL_ARR(retval, a);
    }
}

static char regexp_proto[] = { DGD_TYPE_ARRAY_OF(DGD_TYPE_INT),
			       DGD_TYPE_STRING, DGD_TYPE_STRING,
			       DGD_TYPE_VARARGS, DGD_TYPE_INT, DGD_TYPE_INT };
static DGD_EXTKFUN_T kf_regexp[1] = {
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
    DGD_EXT_KFUN(kf_regexp, 1);
}
