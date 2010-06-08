# ifndef LPC_EXT_H
# define LPC_EXT_H

# include <stdint.h>


# define LPC_TYPE_NIL		0
# define LPC_TYPE_INT		1
# define LPC_TYPE_FLOAT		2
# define LPC_TYPE_STRING	3
# define LPC_TYPE_OBJECT	4
# define LPC_TYPE_ARRAY		5
# define LPC_TYPE_MAPPING	6
# define LPC_TYPE_LWOBJ		7
# define LPC_TYPE_MIXED		8
# define LPC_TYPE_VOID		9
# define LPC_TYPE_VARARGS	16
# define LPC_TYPE_ELLIPSIS	16
# define LPC_TYPE_ARRAY_OF(t)	((t) + 16)

typedef int32_t			LPC_int;
typedef double			LPC_float;
typedef struct _string_	       *LPC_string;
typedef struct _object_	       *LPC_object;
typedef struct _array_	       *LPC_array;
typedef struct _array_	       *LPC_mapping;
typedef struct _array_	       *LPC_lwobj;
typedef struct _value_	       *LPC_value;
typedef struct _frame_	       *LPC_frame;
typedef struct _dataspace_     *LPC_dataspace;
typedef void		      (*LPC_kfun)(LPC_frame, int, LPC_value);
typedef struct {
    char *name;			/* kfun name */
    char *proto;		/* kfun prototype */
    LPC_kfun func;		/* kfun address */
} LPC_ext_kfun;

void				(*lpc_ext_kfun)(LPC_ext_kfun*, int);
void				(*lpc_error)(LPC_frame, char*);

LPC_object			(*lpc_frame_object)(LPC_frame);
LPC_dataspace			(*lpc_frame_dataspace)(LPC_frame);
LPC_value			(*lpc_frame_arg)(LPC_frame, int, int);
int				(*lpc_frame_atomic)(LPC_frame);

LPC_value			(*lpc_data_get_val)(LPC_dataspace);
void				(*lpc_data_set_val)(LPC_dataspace, LPC_value);

int				(*lpc_value_type)(LPC_value);
LPC_value			(*lpc_value_temp)(LPC_dataspace);
void				(*lpc_value_nil)(LPC_value);

LPC_int				(*lpc_int_getval)(LPC_value);
void				(*lpc_int_putval)(LPC_value, LPC_int);

LPC_float			(*lpc_float_getval)(LPC_value);
void				(*lpc_float_putval)(LPC_value, LPC_float);

LPC_string			(*lpc_string_getval)(LPC_value);
void				(*lpc_string_putval)(LPC_value, LPC_string);
LPC_string			(*lpc_string_new)(LPC_dataspace, char*, int);
char *				(*lpc_string_text)(LPC_string);
int				(*lpc_string_length)(LPC_string);

LPC_object			(*lpc_object_getval)(LPC_value);
void				(*lpc_object_putval)(LPC_value, LPC_object);
void				(*lpc_object_name)(LPC_frame, LPC_object,
						   char*);
int				(*lpc_object_isspecial)(LPC_object);
int				(*lpc_object_ismarked)(LPC_object);
void				(*lpc_object_mark)(LPC_object);
void				(*lpc_object_unmark)(LPC_object);

LPC_array			(*lpc_array_getval)(LPC_value);
void				(*lpc_array_putval)(LPC_value, LPC_array);
LPC_array			(*lpc_array_new)(LPC_dataspace, int);
LPC_value			(*lpc_array_index)(LPC_array, int);
void				(*lpc_array_assign)(LPC_dataspace, LPC_array,
						    int, LPC_value);
int				(*lpc_array_size)(LPC_array);

LPC_mapping			(*lpc_mapping_getval)(LPC_value);
void				(*lpc_mapping_putval)(LPC_value, LPC_mapping);
LPC_mapping			(*lpc_mapping_new)(LPC_dataspace);
LPC_value			(*lpc_mapping_index)(LPC_mapping, LPC_value);
void				(*lpc_mapping_assign)(LPC_dataspace,
						      LPC_mapping, LPC_value,
						      LPC_value);
int				(*lpc_mapping_size)(LPC_mapping);
# endif	/* LPC_EXT_H */
