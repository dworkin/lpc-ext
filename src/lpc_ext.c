# ifdef WIN32
# include <Windows.h>
# define DLLEXPORT		__declspec(dllexport)
# else
# define DLLEXPORT		/* nothing */
# endif

# define LPCEXT			/* declare */
# include "lpc_ext.h"
# include <stdarg.h>


/*
 * NAME:	ext->cb()
 * DESCRIPTION:	set up callbacks
 */
static int ext_cb(void *ftab[], int size, int n, ...)
{
    va_list args;
    void **func;

    if (size < n) {
	return 0;
    }

    va_start(args, n);
    do {
	func = va_arg(args, void**);
	*func = *ftab++;
    } while (--n > 0);
    va_end(args);

    return 1;
}

/*
 * NAME:	ext->init()
 * DESCRIPTION:	initialize extension handling
 */
DLLEXPORT int ext_init(int major, int minor, void **ftabs[], int sizes[],
		       const char *config)
{
    return (major == LPC_EXT_VERSION_MAJOR && minor >= LPC_EXT_VERSION_MINOR &&
           ext_cb(ftabs[0], sizes[0], 4,
		   &lpc_ext_kfun,
		   &lpc_ext_dbase,
		   &lpc_ext_jit,
		   &lpc_ext_compiled) &&
	    ext_cb(ftabs[1], sizes[1], 4,
		   &lpc_frame_object,
		   &lpc_frame_dataspace,
		   &lpc_frame_arg,
		   &lpc_frame_atomic) &&
	    ext_cb(ftabs[2], sizes[2], 2,
		   &lpc_data_get_val,
		   &lpc_data_set_val) &&
	    ext_cb(ftabs[3], sizes[3], 4,
		   &lpc_value_type,
		   &lpc_value_nil,
		   &lpc_value_temp,
		   &lpc_value_temp2) &&
	    ext_cb(ftabs[4], sizes[4], 2,
		   &lpc_int_getval,
		   &lpc_int_putval) &&
# ifndef NOFLOAT
	    ext_cb(ftabs[5], sizes[5], 2,
		   &lpc_float_getval,
		   &lpc_float_putval) &&
# endif
	    ext_cb(ftabs[6], sizes[6], 5,
		   &lpc_string_getval,
		   &lpc_string_putval,
		   &lpc_string_new,
		   &lpc_string_text,
		   &lpc_string_length) &&
	    ext_cb(ftabs[7], sizes[7], 6,
		   &lpc_object_putval,
		   &lpc_object_name,
		   &lpc_object_isspecial,
		   &lpc_object_ismarked,
		   &lpc_object_mark,
		   &lpc_object_unmark) &&
	    ext_cb(ftabs[8], sizes[8], 6,
		   &lpc_array_getval,
		   &lpc_array_putval,
		   &lpc_array_new,
		   &lpc_array_index,
		   &lpc_array_assign,
		   &lpc_array_size) &&
	    ext_cb(ftabs[9], sizes[9], 7,
		   &lpc_mapping_getval,
		   &lpc_mapping_putval,
		   &lpc_mapping_new,
		   &lpc_mapping_index,
		   &lpc_mapping_assign,
		   &lpc_mapping_enum,
		   &lpc_mapping_size) &&
	    ext_cb(ftabs[10], sizes[10], 1,
		   &lpc_runtime_error) &&
	    lpc_ext_init(major, minor, config));
}
