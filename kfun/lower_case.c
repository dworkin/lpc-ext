# include "lpc_ext.h"

static void lower_case(LPC_frame f, int nargs, LPC_value retval)
{
    LPC_value val;
    LPC_string str;
    LPC_dataspace data;
    char *p;
    unsigned int i;

    /* fetch the argument string */
    val = lpc_frame_arg(f, nargs, 0);
    str = lpc_string_getval(val);

    /* make a copy */
    data = lpc_frame_dataspace(f);
    str = lpc_string_new(data, lpc_string_text(str), lpc_string_length(str));

    /* turn to lowercase */
    p = lpc_string_text(str);
    for (i = lpc_string_length(str); i != 0; --i) {
	if (*p >= 'A' && *p <= 'Z') {
	    *p += 'a' - 'A';
	}
	p++;
    }

    /* put result in return value */
    lpc_str_putval(retval, str);
    lpc_value_return(retval);
}

static char lower_case_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING, 0 };
static LPC_ext_kfun kf[1] = {
    "lower_case",
    lower_case_proto,
    &lower_case
};

void lpc_ext_init(void)
{
    lpc_ext_kfun(kf, 1);
}
