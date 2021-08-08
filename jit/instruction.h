/*
 * VM Version 2.3
 * 8 August 2021
 */

# define I_INSTR_MASK			0x3f

# define I_INT1				0x00
# define I_INT2				0x20
# define I_INT4				0x01
# define I_INT8				0x21
# define I_FLOAT6			0x03
# define I_FLOAT12			0x23
# define I_STRING			0x04
# define I_NEAR_STRING			0x24
# define I_FAR_STRING			0x05
# define I_LOCAL			0x25
# define I_GLOBAL			0x06
# define I_FAR_GLOBAL			0x26
# define I_INDEX			0x07
# define I_INDEX2			0x08
# define I_SPREAD			0x28
# define I_AGGREGATE			0x09
# define I_CAST				0x0a
# define I_INSTANCEOF			0x0b
# define I_STORES			0x0c
# define I_STORE_GLOBAL_INDEX		0x0d
# define I_CALL_EFUNC			0x0e
# define I_CALL_CEFUNC			0x0f
# define I_CALL_CKFUNC			0x10
# define I_STORE_LOCAL			0x11
# define I_STORE_GLOBAL			0x12
# define I_STORE_FAR_GLOBAL		0x13
# define I_STORE_INDEX			0x14
# define I_STORE_LOCAL_INDEX		0x15
# define I_STORE_FAR_GLOBAL_INDEX	0x16
# define I_STORE_INDEX_INDEX		0x17
# define I_JUMP_ZERO			0x18
# define I_JUMP_NONZERO			0x38
# define I_JUMP				0x19
# define I_SWITCH			0x39
# define I_CALL_KFUNC			0x1a
# define I_CALL_AFUNC			0x1b
# define I_CALL_DFUNC			0x1c
# define I_CALL_FUNC			0x1d
# define I_CATCH			0x1e
# define I_RLIMITS			0x1f
# define I_RETURN			0x3f

# define I_POP_BIT			0x20
# define I_LINE_MASK			0xc0
# define I_LINE_SHIFT			6

# define I_SWITCH_INT			0
# define I_SWITCH_RANGE			1
# define I_SWITCH_STRING		2

# define VERSION_VM_MAJOR		2
# define VERSION_VM_MINOR		3

# define FETCH1U(pc)	(*(pc)++)
# define FETCH1S(pc)	((int64_t) (int8_t) *(pc)++)
# define GET2(pc)	(((uint16_t) (pc)[-2] << 8) + (pc)[-1])
# define FETCH2U(pc)	((pc) += 2, GET2(pc))
# define FETCH2S(pc)	((pc) += 2, ((int64_t) (int8_t) (pc)[-2] << 8) + \
				    (pc)[-1])
# define GET3(pc)	(((uint32_t) (pc)[-3] << 16) + GET2(pc))
# define FETCH3U(pc)	((pc) += 3, GET3(pc))
# define FETCH3S(pc)	((pc) += 3, ((int64_t) (int8_t) (pc)[-3] << 16) + \
				    GET2(pc))
# define GET4(pc)	(((uint32_t) (pc)[-4] << 24) + GET3(pc))
# define FETCH4U(pc)	((pc) += 4, GET4(pc))
# define FETCH4S(pc)	((pc) += 4, ((int64_t) (int8_t) (pc)[-4] << 24) + \
				    GET3(pc))
# define GET5(pc)	(((uint64_t) (pc)[-5] << 32) + GET4(pc))
# define FETCH5U(pc)	((pc) += 5, GET5(pc))
# define FETCH5S(pc)	((pc) += 5, ((int64_t) (int8_t) (pc)[-5] << 32) + \
				    GET4(pc))
# define GET6(pc)	(((uint64_t) (pc)[-6] << 40) + GET5(pc))
# define FETCH6U(pc)	((pc) += 6,GET6(pc))
# define FETCH6S(pc)	((pc) += 6, ((int64_t) (int8_t) (pc)[-6] << 40) + \
				    GET5(pc))
# define GET7(pc)	(((uint64_t) (pc)[-7] << 48) + GET6(pc))
# define FETCH7U(pc)	((pc) += 7, GET7(pc))
# define FETCH7S(pc)	((pc) += 7, ((int64_t) (int8_t) (pc)[-7] << 48) + \
				    GET6(pc))
# define GET8(pc)	(((uint64_t) (pc)[-8] << 56) + GET7(pc))
# define FETCH8U(pc)	((pc) += 8, GET8(pc))
# define FETCH8S(pc)	((pc) += 8, (int64_t) GET8(pc))

# define PROTO_CLASS(p)	((p)[0])
# define PROTO_NARGS(p)	((p)[1])
# define PROTO_VARGS(p)	((p)[2])
# define PROTO_FTYPE(p)	((p)[5])

# define KF_ADD		0
# define KF_ADD_INT	1
# define KF_ADD1	2
# define KF_ADD1_INT	3
# define KF_AND		4
# define KF_AND_INT	5
# define KF_DIV		6
# define KF_DIV_INT	7
# define KF_EQ		8
# define KF_EQ_INT	9
# define KF_GE		10
# define KF_GE_INT	11
# define KF_GT		12
# define KF_GT_INT	13
# define KF_LE		14
# define KF_LE_INT	15
# define KF_LSHIFT	16
# define KF_LSHIFT_INT	17
# define KF_LT		18
# define KF_LT_INT	19
# define KF_MOD		20
# define KF_MOD_INT	21
# define KF_MULT	22
# define KF_MULT_INT	23
# define KF_NE		24
# define KF_NE_INT	25
# define KF_NEG		26
# define KF_NEG_INT	27
# define KF_NOT		28
# define KF_NOT_INT	29
# define KF_OR		30
# define KF_OR_INT	31
# define KF_RANGEFT	32
# define KF_RANGEF	33
# define KF_RANGET	34
# define KF_RANGE	35
# define KF_RSHIFT	36
# define KF_RSHIFT_INT	37
# define KF_SUB		38
# define KF_SUB_INT	39
# define KF_SUB1	40
# define KF_SUB1_INT	41
# define KF_TOFLOAT	42
# define KF_TOINT	43
# define KF_TST		44
# define KF_TST_INT	45
# define KF_UMIN	46
# define KF_UMIN_INT	47
# define KF_XOR		48
# define KF_XOR_INT	49
# define KF_TOSTRING	50
# define KF_CKRANGEFT	51
# define KF_CKRANGEF	52
# define KF_CKRANGET	53
# define KF_CALL_OTHER	54
# define KF_STATUS_IDX	55
# define KF_STATUSO_IDX	56
# define KF_CALLTR_IDX	57
# define KF_NIL		58
# define KF_STATUS	59
# define KF_CALL_TRACE	60
# define KF_ADD_FLT	61
# define KF_ADD_FLT_STR	62
# define KF_ADD_INT_STR	63
# define KF_ADD_STR	64
# define KF_ADD_STR_FLT	65
# define KF_ADD_STR_INT	66
# define KF_ADD1_FLT	67
# define KF_DIV_FLT	68
# define KF_EQ_FLT	69
# define KF_EQ_STR	70
# define KF_GE_FLT	71
# define KF_GE_STR	72
# define KF_GT_FLT	73
# define KF_GT_STR	74
# define KF_LE_FLT	75
# define KF_LE_STR	76
# define KF_LT_FLT	77
# define KF_LT_STR	78
# define KF_MULT_FLT	79
# define KF_NE_FLT	80
# define KF_NE_STR	81
# define KF_NOT_FLT	82
# define KF_NOT_STR	83
# define KF_SUB_FLT	84
# define KF_SUB1_FLT	85
# define KF_TST_FLT	86
# define KF_TST_STR	87
# define KF_UMIN_FLT	88
# define KF_SUM		89
# define KF_FABS	90
# define KF_FLOOR	91
# define KF_CEIL	92
# define KF_FMOD	93
# define KF_FREXP	94
# define KF_LDEXP	95
# define KF_MODF	96
# define KF_EXP		97
# define KF_LOG		98
# define KF_LOG10	99
# define KF_POW		100
# define KF_SQRT	101
# define KF_COS		102
# define KF_SIN		103
# define KF_TAN		104
# define KF_ACOS	105
# define KF_ASIN	106
# define KF_ATAN	107
# define KF_ATAN2	108
# define KF_COSH	109
# define KF_SINH	110
# define KF_TANH	111
# define KF_BUILTINS	128

# define SUM_SIMPLE	-2
# define SUM_AGGREGATE	-6
