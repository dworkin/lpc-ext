# ifndef LPC_EXT_H
# define LPC_EXT_H

/*
 * interface
 */
# define LPC_EXT_KFUN(ekf, n)	kf_ext_kfun((ekf), (n))
# define LPC_ERROR		lpc_error
# define LPC_ECONTEXT_PUSH(f)	ec_push((ec_ftn) NULL)
# define LPC_ECONTEXT_POP(f)	ec_pop()

/*
 * types
 */
typedef string *lpc_string_t;
typedef object *lpc_object_t;
typedef array *lpc_array_t;
typedef frame *lpc_frame_t;
typedef dataspace *lpc_dataspace_t;

# define LPC_INT_T		Int
# define LPC_FLOAT_T		xfloat
# define LPC_STRING_T		lpc_string_t
# define LPC_OBJECT_T		lpc_object_t
# define LPC_ARRAY_T		lpc_array_t
# define LPC_MAPPING_T		lpc_array_t
# define LPC_LWOBJ_T		lpc_array_t
# define LPC_VALUE_T		value
# define LPC_FRAME_T		lpc_frame_t
# define LPC_DATASPACE_T	lpc_dataspace_t
# define LPC_EXTKFUN_T		extkfunc
# define LPC_EINDEX_T		eindex

/*
 * prototype and value types
 */
# define LPC_TYPE_VOID		T_VOID
# define LPC_TYPE_NIL		T_NIL
# define LPC_TYPE_INT		T_INT
# define LPC_TYPE_FLOAT		T_FLOAT
# define LPC_TYPE_STRING	T_STRING
# define LPC_TYPE_OBJECT	T_OBJECT
# define LPC_TYPE_ARRAY		T_ARRAY
# define LPC_TYPE_MAPPING	T_MAPPING
# define LPC_TYPE_LWOBJ		T_LWOBJECT
# define LPC_TYPE_MIXED		T_MIXED

# define LPC_TYPE_ARRAY_OF(t)	((t) + (1 << REFSHIFT))
# define LPC_TYPE_VARARGS	T_VARARGS
# define LPC_TYPE_ELLIPSIS	T_ELLIPSIS

/*
 * frame
 */
# define LPC_FRAME_OBJECT(f)	(((f)->lwobj == (array *) NULL) ? \
				  OBJW((f)->oindex) : (object *) NULL)
# define LPC_FRAME_DATASPACE(f)	((f)->data)
# define LPC_FRAME_ARG(f, n, i)	(*((f)->sp + (n) - ((i) + 1)))
# define LPC_FRAME_ATOMIC(f)	((f)->level != 0)

/*
 * dataspace
 */
# define LPC_DATA_GET_VAL(d)	(*d_get_extravar((d)))
# define LPC_DATA_SET_VAL(d, v)	d_set_extravar((d), &(v))

/*
 * value
 */
# define LPC_TYPEOF(v)		((v).type)

# define LPC_RETVAL_INT(v, i)	PUT_INTVAL((v), (i))
# define LPC_RETVAL_FLT(v, f)	PUT_FLTVAL((v), (f))
# define LPC_RETVAL_STR(v, s)	PUT_STRVAL((v), (s))
# define LPC_RETVAL_OBJ(v, o)	PUT_OBJVAL((v), (o))
# define LPC_RETVAL_ARR(v, a)	PUT_ARRVAL((v), (a))
# define LPC_RETVAL_MAP(v, m)	PUT_MAPVAL((v), (m))

/*
 * nil
 */
# define LPC_NIL_VALUE		nil_value

/*
 * int
 */
# define LPC_INT_GETVAL(v)	((v).u.number)
# define LPC_INT_PUTVAL(v, i)	PUT_INTVAL(&(v), (i))

/*
 * float
 */
# define LPC_FLOAT_GETVAL(v, f)		GET_FLT(&(v), (f))
# define LPC_FLOAT_PUTVAL(v, f)		PUT_FLTVAL(&(v), (f))
# define LPC_FLOAT_GET(f, s, e, m)	((((f).high | (f).low) == 0) ? \
					  ((s) = 0, (e) = 0, (m) = 0) : \
					  ((s) = (f).high >> 15, \
					   (e) = (((f).high >> 4) & 0x7ff) - \
						 1023, \
					   (m) = 0x10 | ((f).high & 0xf), \
					   (m) <<= 32, (m) |= (f).low))
# define LPC_FLOAT_PUT(f, s, e, m)	((f).high = (((m) == 0) ? 0 : \
						      ((s) << 15) | \
						      (((e) + 1023) << 4) | \
						      (((m) >> 32) & 0xf)), \
					 (f).low = (m))

/*
 * string
 */
# define LPC_STRING_GETVAL(v)		((v).u.string)
# define LPC_STRING_PUTVAL(v, s)	PUT_STRVAL_NOREF(&(v), (s))
# define LPC_STRING_NEW(d, t, n)	str_new((t), (long) (n))
# define LPC_STRING_TEXT(s)		((s)->text)
# define LPC_STRING_LENGTH(s)		((unsigned int) (s)->len)

/*
 * object
 */
# define LPC_OBJECT_PUTVAL(v, o)	PUT_OBJVAL(&(v), (o))
# define LPC_OBJECT_NAME(f, buf, o)	o_name((buf), (o))
# define LPC_OBJECT_ISSPECIAL(o)	(((o)->flags & O_SPECIAL) != 0)
# define LPC_OBJECT_ISMARKED(o)		(((o)->flags & O_SPECIAL) == O_SPECIAL)
# define LPC_OBJECT_MARK(o)		((o)->flags |= O_SPECIAL)
# define LPC_OBJECT_UNMARK(o)		((o)->flags &= ~O_SPECIAL)

/*
 * array
 */
# define LPC_ARRAY_GETVAL(v)		((v).u.array)
# define LPC_ARRAY_PUTVAL(v, a)		PUT_ARRVAL_NOREF(&(v), (a))
# define LPC_ARRAY_NEW(d, n)		arr_ext_new((d), (long) (n))
# define LPC_ARRAY_ELTS(a)		d_get_elts((a))
# define LPC_ARRAY_SIZE(a)		((unsigned int) (a)->size)
# define LPC_ARRAY_INDEX(a, i)		(d_get_elts((a))[(i)])
# define LPC_ARRAY_ASSIGN(d, a, i, v)	d_assign_elt((d), (a), \
						    &d_get_elts((a))[(i)], &(v))
/*
 * mapping
 */
# define LPC_MAPPING_GETVAL(v)		((v).u.array)
# define LPC_MAPPING_PUTVAL(v, m)	PUT_MAPVAL_NOREF(&(v), (m))
# define LPC_MAPPING_NEW(d)		map_new((d), 0L)
# define LPC_MAPPING_ELTS(m)		(map_compact((m)), d_get_elts((m)))
# define LPC_MAPPING_SIZE(m)		((unsigned int) map_size((m)))
# define LPC_MAPPING_INDEX(m, i)	(*map_index((m)->primary->data, (m), \
						    &(i), (value *) NULL))
# define LPC_MAPPING_ASSIGN(d, m, i, v)	map_index((d), (m), &(i), &(v))

# endif	/* LPC_EXT_H */
