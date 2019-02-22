/*
 * VM Version 2.1
 * 7 January 2015
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
# define VERSION_VM_MINOR		1

# define FETCH1U(pc)	(*(pc)++)
# define FETCH1S(pc)	((int8_t) *(pc)++)
# define GET2(pc)	(((uint16_t) (pc)[-2] << 8) + (pc)[-1])
# define FETCH2U(pc)	((pc) += 2, GET2(pc))
# define FETCH2S(pc)	((pc) += 2, ((int16_t) (int8_t) (pc)[-2] << 8) + \
				    (pc)[-1])
# define GET3(pc)	(((uint32_t) (pc)[-3] << 16) + GET2(pc))
# define FETCH3U(pc)	((pc) += 3, GET3(pc))
# define FETCH3S(pc)	((pc) += 3, ((int32_t) (int8_t) (pc)[-3] << 16) + \
				    GET2(pc))
# define GET4(pc)	(((uint32_t) (pc)[-4] << 24) + GET3(pc))
# define FETCH4U(pc)	((pc) += 4, GET4(pc))
# define FETCH4S(pc)	((pc) += 4, ((int32_t) (int8_t) (pc)[-4] << 24) + \
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

# define KF_CKRANGEFT	51
# define KF_CKRANGEF	52
# define KF_CKRANGET	53
# define KF_SUM		89
# define KF_BUILTINS	128

# define SUM_SIMPLE	-2
# define SUM_AGGREGATE	-6
