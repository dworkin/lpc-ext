# LPC extension interface

## 1. Introduction

DGD presents an interface for extending the driver with new kfuns and special
object types.  Extension modules can be specified in the config file using
the modules parameter:
```
    modules = ([ "/path/to/extension/module" : "module configuration" ]);
```
The extension module will be loaded into DGD at runtime, and the function
`int lpc_ext_init(int major, int minor, char *config)` will be called to
initialize it; this function must return a non-zero value to indicate that
initialization was successful.


## 2. Kernel functions

Kernel functions may be added during the initialization phase by calling the
following function:
```
    void lpc_ext_kfun(LPC_ext_kfun *kf, int n);
```
where `kf` is an array of `LPC_ext_kfun` structs, and `n` the number of
elements in that array.  `lpc_ext_kfun()` must not redeclare any existing kfuns.

The `LPC_ext_kfun` struct has the following fields:
```
    char *name;		/* kfun name */
    char *proto;	/* kfun prototype */
    func;		/* kfun function pointer */
```
Here `func` is a pointer to a C function with the following prototype:
```
    void func(LPC_frame f, int nargs, LPC_value retval);
```
Calls to the kfun with name `name` will be routed to that C function.  The
argument `f` contains the function context, `nargs` is the number of
arguments on the stack, and `retval` is a pointer to the return value.  The
function should not attempt to push or pop arguments; instead, arguments should
be accessed using the relevant `lpc_frame_*()` functions listed below, and the
return value, if any, should be stored in the `retval` value using one of the
`lpc_*_putval()` functions.  The default return value is `nil`.

Functions with lvalue parameters should put the values to be assigned in an
array, which is stored in the scratch value returned by `lpc_value_temp()`.

Kfuns can call `lpc_runtime_error()` to indicate that an error occurred.  Errors
will interrupt the flow of execution, so `lpc_runtime_error()` is best called
before any LPC values are constructed or changed, including the return value of
the kfun.

Note that at present, the extension interface does not define a way to call an
LPC function from a kfun.
```
    LPC_object	  lpc_frame_object(LPC_frame f);	/* current object */
    LPC_dataspace lpc_frame_dataspace(LPC_frame f);	/* current dataspace */
    LPC_value	  lpc_frame_arg(LPC_frame f, int nargs, int n); /* argument n */
    int 	  lpc_frame_atomic(LPC_frame f);		/* atomic? */

    void	  lpc_runtime_error(LPC_frame f, char *error);
```
The `proto` field of the `LPC_ext_kfun` struct is a prototype for the function,
represented as a 0-terminated array of chars.  For example, the LPC prototypes
```
    string lower_case(string str);
    string concat(string str...);
    int foo(int *a, varargs object **b);
    void bar(void);
    void gnu();
    int sscanf(string str, string format, mixed...);	/* lvalue arguments */
```
would be represented as
```
    char lower_case_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING, 0 };
    char concat_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING,
			    LPC_TYPE_ELLIPSIS, 0 };
    char foo_proto[] = { LPC_TYPE_INT, LPC_TYPE_ARRAY_OF(LPC_TYPE_INT),
			 LPC_TYPE_VARARGS,
			 LPC_TYPE_ARRAY_OF(LPC_TYPE_ARRAY_OF(LPC_TYPE_OBJECT)),
			 0 };
    char bar_proto[] = { LPC_TYPE_VOID, LPC_TYPE_VOID, 0 };
    char gnu_proto[] = { LPC_TYPE_VOID, 0 };
    char sscanf_proto[] = { LPC_TYPE_INT, LPC_TYPE_STRING, LPC_TYPE_STRING,
			    LPC_TYPE_LVALUE, LPC_TYPE_ELLIPSIS, 0 };
```
Note that the prototypes of `bar()` and `gnu()` are effectively identical, just
as they would be for LPC functions; also note that `varargs` must be specified
among the arguments of the prototype rather than in front of the return type.

The building blocks for prototypes are:
```
    LPC_TYPE_VOID
    LPC_TYPE_INT
    LPC_TYPE_FLOAT
    LPC_TYPE_STRING
    LPC_TYPE_OBJECT
    LPC_TYPE_ARRAY_OF(lpc_type)
    LPC_TYPE_MAPPING
    LPC_TYPE_MIXED
    LPC_TYPE_LVALUE
    LPC_TYPE_VARARGS
    LPC_TYPE_ELLIPSIS
```

## 3. Special objects

DGD defines various types of special objects: user objects, editor objects
and parser objects.  Extensions may define their own additional special
object type.  Special objects have an additional LPC value.

Objects should only be marked as special if they are not special already.
If an object is to be unmarked, and a special LPC value has been set, it
should be reset to `lpc_value_nil()` first.
```
    int  lpc_object_isspecial(LPC_frame, LPC_object obj);/* special object */
    int  lpc_object_ismarked(LPC_frame, LPC_object obj); /* user-def special */
    void lpc_object_mark(LPC_frame, LPC_object obj);     /* mark as special */
    void lpc_object_unmark(LPC_frame, LPC_object obj);   /* unmark as special */
```
To retrieve or set the LPC value associated with a special object, the
following functions can be used.
```
    LPC_value	lpc_data_get_val(LPC_dataspace data);
    void	lpc_data_set_val(LPC_dataspace data, LPC_value val);
```

## 4. Operations on LPC values

The type of an LPC value can be determined with
```
    int lpc_value_type(LPC_value val);
```
The return values can be:
```
    LPC_TYPE_NIL
    LPC_TYPE_INT	=>	LPC_int
    LPC_TYPE_FLOAT	=>	LPC_float
    LPC_TYPE_STRING	=>	LPC_string
    LPC_TYPE_OBJECT	=>	LPC_object
    LPC_TYPE_ARRAY	=>	LPC_array
    LPC_TYPE_MAPPING	=>	LPC_mapping
    LPC_TYPE_LWOBJ	=>	LPC_lwobj
```
`LPC_int` and `LPC_float` can be used directly, and are represented by an
integral and a floating point type, respectively.  The other types must be
handled by the proper functions.  Light-weight objects cannot be manipulated by
an extension kfun, at all.
```
    int		lpc_value_type(LPC_value val);
    LPC_value	lpc_value_nil();			/* the nil value */
    LPC_value	lpc_value_temp(LPC_datspace data);	/* temporary value 1 */
    LPC_value	lpc_value_temp2(LPC_datspace data);	/* temporary value 2 */
```
To save data, it must first be stored in a value.  Since `LPC_value` is
actually a pointer, there is no way to construct a new value.
`lpc_value_temp()` can be used as temporary storage (it always returns the
same pointer), which can then be copied to some other value.
```
    LPC_int	lpc_int_getval(LPC_value val);
    void	lpc_int_putval(LPC_value val, LPC_int num);

    LPC_float	lpc_float_getval(LPC_value val);
    void	lpc_float_putval(LPC_value val, LPC_float flt);

    LPC_string	lpc_string_getval(LPC_value val);
    void	lpc_string_putval(LPC_value val, LPC_string str);
    LPC_string	lpc_string_new(LPC_dataspace data, const char *text, int len);
    char       *lpc_string_text(LPC_string str);
    int		lpc_string_length(LPC_string str);
```
The current object may be stored as an array or mapping element.
```
    void	lpc_object_putval(LPC_value val, LPC_object obj);
    char       *lpc_object_name(LPC_frame f, LPC_object obj, char *buffer);
```
Don't modify array elements directly, but use `lpc_array_assign()` to change
their value.
```
    LPC_array	lpc_array_getval(LPC_value val);
    void	lpc_array_putval(LPC_value val, LPC_array arr);
    LPC_array	lpc_array_new(LPC_dataspace data, int size);
    LPC_value	lpc_array_index(LPC_array arr, int i);
    void	lpc_array_assign(LPC_dataspace data, LPC_array arr, int index,
				 LPC_value val);
    int		lpc_array_size(LPC_array arr);
```
Don't modify mapping elements directly, but use `lpc_mapping_assign()` to
change their value.  `lpc_mapping_enum()` can be used to enumerate the
indices of a mapping.
```
    LPC_mapping	lpc_mapping_getval(LPC_value val);
    void	lpc_mapping_putval(LPC_value val, LPC_mapping map);
    LPC_mapping	lpc_mapping_new(LPC_dataspace data);
    LPC_value	lpc_mapping_index(LPC_mapping map, LPC_value index);
    void	lpc_mapping_assign(LPC_dataspace data, LPC_mapping map,
				   LPC_value index, LPC_value val);
    LPC_value	lpc_mapping_enum(LPC_mapping map, unsigned int i);
    int		lpc_mapping_size(LPC_mapping map);
```

## 5. Spawning a process

A program may be executed from an extension module.  This can only be done
once per module, and must be done from `lpc_ext_init()`.  The process can be
communicated with using pipes connected to standard input and output.
```
    void	lpc_ext_spawn(const char *cmdline);
    int		lpc_ext_read(void *buffer, int len);
    int		lpc_ext_write(const void *buffer, int len);
```
Data can also be put directly into the input pipe, to be read back with
`lpc_ext_read()`, as if it came from the external process.
```
    int		lpc_ext_writeback(const void *buffer, int len);
```

This is intended for modules that offload some of their functionality into
an external program.  It cannot be used to execute an arbitrary program and
communicate with it from LPC code.

## 6. Example

The following code implements a `lower_case()` kfun.
```
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
    lpc_string_putval(retval, str);
}

static char lower_case_proto[] = { LPC_TYPE_STRING, LPC_TYPE_STRING, 0 };
static LPC_ext_kfun kf[1] = {
    "lower_case",
    lower_case_proto,
    &lower_case
};

int lpc_ext_init(int major, int minor, char *config)
{
    lpc_ext_kfun(kf, 1);
    return 1;
}
```
