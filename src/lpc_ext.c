# define LPCEXT			/* declare */
# include "lpc_ext.h"
# include <stdarg.h>


typedef struct {
    int num;			/* number of function pointers */
    void *ftab[1];		/* function pointers */
} callback;


static int ext_cb(callback *cb, int n, ...)
{
    va_list args;
    void **ftab, **func;

    if (cb->num < n) {
	return 0;
    }

    va_start(args, n);
    ftab = cb->ftab;
    do {
	func = va_arg(args, void*);
	*func = *ftab++;
    } while (--n > 0);
    va_end(args);
}

int ext_init(int major, int minor, void *ext, void *frame, void *data,
	     void *value, void *lpcint, void *lpcfloat, void *string,
	     void *object, void *array, void *mapping)
{
    if (major != LPC_EXT_VERSION_MAJOR ||
	!ext_cb(ext, 2, &lpc_error, &lpc_ext_kfun) ||
	!ext_cb(frame, 4, &lpc_frame_object, &lpc_frame_dataspace,
		&lpc_frame_arg, &lpc_frame_atomic) ||
	!ext_cb(data, 2, &lpc_data_get_val, &lpc_data_set_val) ||
	!ext_cb(value, 3, &lpc_value_type, &lpc_value_temp, &lpc_value_nil) ||
	!ext_cb(lpcint, 2, &lpc_int_getval, &lpc_int_putval) ||
	!ext_cb(lpcfloat, 2, &lpc_float_getval, &lpc_float_putval) ||
	!ext_cb(string, 5, &lpc_string_getval, &lpc_string_putval,
		&lpc_string_new, &lpc_string_text, &lpc_string_length) ||
	!ext_cb(object, 7, &lpc_object_getval, &lpc_object_putval,
		&lpc_object_name, &lpc_object_isspecial, &lpc_object_ismarked,
		&lpc_object_mark, &lpc_object_unmark) ||
	!ext_cb(array, 6, &lpc_array_getval, &lpc_array_putval, &lpc_array_new,
		&lpc_array_index, &lpc_array_assign, &lpc_array_size) ||
	!ext_cb(mapping, 6, &lpc_mapping_getval, &lpc_mapping_putval,
		&lpc_mapping_new, &lpc_mapping_index, &lpc_mapping_assign,
		&lpc_mapping_size)) {
	return 0;
    }

    lpc_ext_init();
}
