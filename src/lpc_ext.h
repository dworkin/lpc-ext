# ifndef LPC_EXT_H
# define LPC_EXT_H

# include <sys/types.h>
# include <stdint.h>


# define LPC_EXT_VERSION_MAJOR	1
# define LPC_EXT_VERSION_MINOR	3

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
# define LPC_TYPE_LVALUE	10
# define LPC_TYPE_VARARGS	16
# define LPC_TYPE_ELLIPSIS	16
# define LPC_TYPE_ARRAY_OF(t)	((t) + 16)

typedef int32_t			LPC_int;
# ifndef NOFLOAT
typedef long double		LPC_float;
# endif
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
typedef struct _lpc_database_	LPC_db;
typedef struct _lpc_db_object_	LPC_db_object;
typedef uint64_t		LPC_db_index;
typedef uint64_t		LPC_db_sector;
typedef struct {
    void *data;			/* address */
    uint64_t offset;		/* offset of request */
    uint64_t size;		/* size of request */
} LPC_db_request;
typedef struct {
    int (*valid)		(const char*);
    LPC_db (*creat)		(const char*);
    LPC_db (*open_rw)		(const char*);
    LPC_db (*open_r)		(const char*);
    void (*close)		(LPC_db*);
    LPC_db_object (*new_obj)	(LPC_db*, LPC_db_index);
    LPC_db_object (*load_obj)	(LPC_db*, LPC_db_index, LPC_db_sector);
    int (*del_obj)		(LPC_db*, LPC_db_object*);
    int (*resize_obj)		(LPC_db*, LPC_db_object*, uint64_t,
				 LPC_db_sector*);
    int (*read)			(LPC_db*, LPC_db_object*, LPC_db_request*, int);
    int (*write)		(LPC_db*, LPC_db_object*, LPC_db_request*, int);
    int (*erase_obj)		(LPC_db*, LPC_db_object*);
    int (*save)			(LPC_db*, LPC_db_request*);
    int (*restore)		(LPC_db*, LPC_db_request*);
    int (*save_snapshot)	(LPC_db*, LPC_db_request*);
    int (*restore_snapshot)	(LPC_db*, LPC_db_request*);
} LPC_ext_dbase;
typedef void (*LPC_function)	(void**, LPC_frame);
typedef int		      (*LPC_jit_init)(int, int, size_t, size_t, int,
					      int, int, uint8_t*, size_t,
					      void**);
typedef void		      (*LPC_jit_finish)(void);
typedef void		      (*LPC_jit_compile)(uint64_t, uint64_t, int,
						 uint8_t*, size_t, int,
						 uint8_t*, size_t, uint8_t*,
						 size_t);
typedef int		      (*LPC_jit_execute)(uint64_t, uint64_t, int, int,
						 void*);
typedef void		      (*LPC_jit_release)(uint64_t, uint64_t);
typedef int		      (*LPC_jit_functions)(uint64_t, uint64_t, int,
						   LPC_function**);


extern int			lpc_ext_init(int, int, const char*);
extern int			lpc_ext_spawn(const char*);
extern int			lpc_ext_read(void*, int);
extern int			lpc_ext_write(const void*, int);
extern int			lpc_ext_writeback(const void*, int);

# ifndef LPCEXT
#  define LPCEXT extern
# endif

LPCEXT void			(*lpc_ext_kfun)(const LPC_ext_kfun*, int);
LPCEXT void			(*lpc_ext_dbase)(LPC_ext_dbase*);
LPCEXT int			(*lpc_ext_jit)(LPC_jit_init, LPC_jit_finish,
					       LPC_jit_compile, LPC_jit_execute,
					       LPC_jit_release,
					       LPC_jit_functions);

LPCEXT LPC_object		(*lpc_frame_object)(LPC_frame);
LPCEXT LPC_dataspace		(*lpc_frame_dataspace)(LPC_frame);
LPCEXT LPC_value		(*lpc_frame_arg)(LPC_frame, int, int);
LPCEXT int			(*lpc_frame_atomic)(LPC_frame);

LPCEXT LPC_value		(*lpc_data_get_val)(LPC_dataspace);
LPCEXT void			(*lpc_data_set_val)(LPC_dataspace, LPC_value);

LPCEXT int			(*lpc_value_type)(LPC_value);
LPCEXT LPC_value		(*lpc_value_nil)(void);
LPCEXT LPC_value		(*lpc_value_temp)(LPC_dataspace);
LPCEXT LPC_value		(*lpc_value_temp2)(LPC_dataspace);

LPCEXT LPC_int			(*lpc_int_getval)(LPC_value);
LPCEXT void			(*lpc_int_putval)(LPC_value, LPC_int);

# ifndef NOFLOAT
LPCEXT LPC_float		(*lpc_float_getval)(LPC_value);
LPCEXT void			(*lpc_float_putval)(LPC_value, LPC_float);
# endif

LPCEXT LPC_string		(*lpc_string_getval)(LPC_value);
LPCEXT void			(*lpc_string_putval)(LPC_value, LPC_string);
LPCEXT LPC_string		(*lpc_string_new)(LPC_dataspace, const char*,
						  int);
LPCEXT char *			(*lpc_string_text)(LPC_string);
LPCEXT int			(*lpc_string_length)(LPC_string);

LPCEXT void			(*lpc_object_putval)(LPC_value, LPC_object);
LPCEXT const char *		(*lpc_object_name)(LPC_frame, LPC_object,
						   char*);
LPCEXT int			(*lpc_object_isspecial)(LPC_object);
LPCEXT int			(*lpc_object_ismarked)(LPC_object);
LPCEXT void			(*lpc_object_mark)(LPC_object);
LPCEXT void			(*lpc_object_unmark)(LPC_object);

LPCEXT LPC_array		(*lpc_array_getval)(LPC_value);
LPCEXT void			(*lpc_array_putval)(LPC_value, LPC_array);
LPCEXT LPC_array		(*lpc_array_new)(LPC_dataspace, int);
LPCEXT LPC_value		(*lpc_array_index)(LPC_array, int);
LPCEXT void			(*lpc_array_assign)(LPC_dataspace, LPC_array,
						    int, LPC_value);
LPCEXT int			(*lpc_array_size)(LPC_array);

LPCEXT LPC_mapping		(*lpc_mapping_getval)(LPC_value);
LPCEXT void			(*lpc_mapping_putval)(LPC_value, LPC_mapping);
LPCEXT LPC_mapping		(*lpc_mapping_new)(LPC_dataspace);
LPCEXT LPC_value		(*lpc_mapping_index)(LPC_mapping, LPC_value);
LPCEXT void			(*lpc_mapping_assign)(LPC_dataspace,
						      LPC_mapping, LPC_value,
						      LPC_value);
LPCEXT LPC_value		(*lpc_mapping_enum)(LPC_mapping, int);
LPCEXT int			(*lpc_mapping_size)(LPC_mapping);

LPCEXT void			(*lpc_runtime_error)(LPC_frame, char*);
LPCEXT void			(*lpc_md5_start)(uint32_t*);
LPCEXT void			(*lpc_md5_block)(uint32_t*,
						 const unsigned char*);
LPCEXT void			(*lpc_md5_end)(unsigned char*, uint32_t*,
					       unsigned char*, uint16_t,
					       uint32_t);
LPCEXT void			(*lpc_runtime_ticks)(LPC_frame, int);
LPCEXT void			(*lpc_runtime_check)(LPC_frame, int);
# endif	/* LPC_EXT_H */
