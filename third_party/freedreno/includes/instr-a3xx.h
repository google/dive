/*
 * Copyright (c) 2013 Rob Clark <robdclark@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef INSTR_A3XX_H_
#define INSTR_A3XX_H_

// GOOGLE: A pragma pack directive is needed to pack a structure in Windows.
#if defined(__GNUC__)
#define PACKED __attribute__((__packed__))
#elif defined(_MSC_VER)
#    define PACKED
#    pragma pack(push, 1)
#else
#    error "Unsupported compiler"
#endif


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef enum {
	/* category 0: */
	OPC_NOP = 0,
	OPC_BR = 1,
	OPC_JUMP = 2,
	OPC_CALL = 3,
	OPC_RET = 4,
	OPC_KILL = 5,
	OPC_END = 6,
	OPC_EMIT = 7,
	OPC_CUT = 8,
	OPC_CHMASK = 9,
	OPC_CHSH = 10,
	OPC_FLOW_REV = 11,

	/* category 1: */
	/* no opc.. all category 1 are variants of mov */

	/* category 2: */
	OPC_ADD_F = 0,
	OPC_MIN_F = 1,
	OPC_MAX_F = 2,
	OPC_MUL_F = 3,
	OPC_SIGN_F = 4,
	OPC_CMPS_F = 5,
	OPC_ABSNEG_F = 6,
	OPC_CMPV_F = 7,
	/* 8 - invalid */
	OPC_FLOOR_F = 9,
	OPC_CEIL_F = 10,
	OPC_RNDNE_F = 11,
	OPC_RNDAZ_F = 12,
	OPC_TRUNC_F = 13,
	/* 14-15 - invalid */
	OPC_ADD_U = 16,
	OPC_ADD_S = 17,
	OPC_SUB_U = 18,
	OPC_SUB_S = 19,
	OPC_CMPS_U = 20,
	OPC_CMPS_S = 21,
	OPC_MIN_U = 22,
	OPC_MIN_S = 23,
	OPC_MAX_U = 24,
	OPC_MAX_S = 25,
	OPC_ABSNEG_S = 26,
	/* 27 - invalid */
	OPC_AND_B = 28,
	OPC_OR_B = 29,
	OPC_NOT_B = 30,
	OPC_XOR_B = 31,
	/* 32 - invalid */
	OPC_CMPV_U = 33,
	OPC_CMPV_S = 34,
	/* 35-47 - invalid */
	OPC_MUL_U = 48,
	OPC_MUL_S = 49,
	OPC_MULL_U = 50,
	OPC_BFREV_B = 51,
	OPC_CLZ_S = 52,
	OPC_CLZ_B = 53,
	OPC_SHL_B = 54,
	OPC_SHR_B = 55,
	OPC_ASHR_B = 56,
	OPC_BARY_F = 57,
	OPC_MGEN_B = 58,
	OPC_GETBIT_B = 59,
	OPC_SETRM = 60,
	OPC_CBITS_B = 61,
	OPC_SHB = 62,
	OPC_MSAD = 63,

	/* category 3: */
	OPC_MAD_U16 = 0,
	OPC_MADSH_U16 = 1,
	OPC_MAD_S16 = 2,
	OPC_MADSH_M16 = 3,   /* should this be .s16? */
	OPC_MAD_U24 = 4,
	OPC_MAD_S24 = 5,
	OPC_MAD_F16 = 6,
	OPC_MAD_F32 = 7,
	OPC_SEL_B16 = 8,
	OPC_SEL_B32 = 9,
	OPC_SEL_S16 = 10,
	OPC_SEL_S32 = 11,
	OPC_SEL_F16 = 12,
	OPC_SEL_F32 = 13,
	OPC_SAD_S16 = 14,
	OPC_SAD_S32 = 15,

	/* category 4: */
	OPC_RCP = 0,
	OPC_RSQ = 1,
	OPC_LOG2 = 2,
	OPC_EXP2 = 3,
	OPC_SIN = 4,
	OPC_COS = 5,
	OPC_SQRT = 6,
	// 7-63 - invalid

	/* category 5: */
	OPC_ISAM = 0,
	OPC_ISAML = 1,
	OPC_ISAMM = 2,
	OPC_SAM = 3,
	OPC_SAMB = 4,
	OPC_SAML = 5,
	OPC_SAMGQ = 6,
	OPC_GETLOD = 7,
	OPC_CONV = 8,
	OPC_CONVM = 9,
	OPC_GETSIZE = 10,
	OPC_GETBUF = 11,
	OPC_GETPOS = 12,
	OPC_GETINFO = 13,
	OPC_DSX = 14,
	OPC_DSY = 15,
	OPC_GATHER4R = 16,
	OPC_GATHER4G = 17,
	OPC_GATHER4B = 18,
	OPC_GATHER4A = 19,
	OPC_SAMGP0 = 20,
	OPC_SAMGP1 = 21,
	OPC_SAMGP2 = 22,
	OPC_SAMGP3 = 23,
	OPC_DSXPP_1 = 24,
	OPC_DSYPP_1 = 25,
	OPC_RGETPOS = 26,
	OPC_RGETINFO = 27,

	/* category 6: */
	OPC_LDG = 0,        /* load-global */
	OPC_LDL = 1,
	OPC_LDP = 2,
	OPC_STG = 3,        /* store-global */
	OPC_STL = 4,
	OPC_STP = 5,
	OPC_STI = 6,
	OPC_G2L = 7,
	OPC_L2G = 8,
	OPC_PREFETCH = 9,
	OPC_LDLW = 10,
	OPC_STLW = 11,
	OPC_RESFMT = 14,
	OPC_RESINFO = 15,
	OPC_ATOMIC_ADD = 16,
	OPC_ATOMIC_SUB = 17,
	OPC_ATOMIC_XCHG = 18,
	OPC_ATOMIC_INC = 19,
	OPC_ATOMIC_DEC = 20,
	OPC_ATOMIC_CMPXCHG = 21,
	OPC_ATOMIC_MIN = 22,
	OPC_ATOMIC_MAX = 23,
	OPC_ATOMIC_AND = 24,
	OPC_ATOMIC_OR = 25,
	OPC_ATOMIC_XOR = 26,
	OPC_LDGB = 27,
	OPC_STGB = 28,
	OPC_STIB = 29,
	OPC_LDC = 30,
	OPC_LDLV = 31,

	/* category 7: */
	OPC_BAR = 0,
	OPC_FENCE = 1,

} opc_t;

typedef enum {
	TYPE_F16 = 0,
	TYPE_F32 = 1,
	TYPE_U16 = 2,
	TYPE_U32 = 3,
	TYPE_S16 = 4,
	TYPE_S32 = 5,
	TYPE_U8  = 6,
	TYPE_S8  = 7,  // XXX I assume?
} type_t;

static inline uint32_t type_size(type_t type)
{
	switch (type) {
	case TYPE_F32:
	case TYPE_U32:
	case TYPE_S32:
		return 32;
	case TYPE_F16:
	case TYPE_U16:
	case TYPE_S16:
		return 16;
	case TYPE_U8:
	case TYPE_S8:
		return 8;
	default:
		assert(0); /* invalid type */
		return 0;
	}
}

static inline int type_float(type_t type)
{
	return (type == TYPE_F32) || (type == TYPE_F16);
}

static inline int type_uint(type_t type)
{
	return (type == TYPE_U32) || (type == TYPE_U16) || (type == TYPE_U8);
}

static inline int type_sint(type_t type)
{
	return (type == TYPE_S32) || (type == TYPE_S16) || (type == TYPE_S8);
}

typedef union PACKED {
	/* normal gpr or const src register: */
	struct PACKED {
		uint32_t comp  : 2;
		uint32_t num   : 10;
	};
	/* for immediate val: */
	int32_t  iim_val   : 11;
	/* to make compiler happy: */
	uint32_t dummy32;
	uint32_t dummy10   : 10;
	int32_t  idummy10  : 10;
	uint32_t dummy11   : 11;
	uint32_t dummy12   : 12;
	uint32_t dummy13   : 13;
	uint32_t dummy8    : 8;
} reg_t;

/* special registers: */
#define REG_A0 61       /* address register */
#define REG_P0 62       /* predicate register */

static inline int reg_special(reg_t reg)
{
	return (reg.num == REG_A0) || (reg.num == REG_P0);
}

typedef struct PACKED {
	/* dword0: */
	union PACKED {
		struct PACKED {
			int16_t  immed    : 16;
			uint32_t dummy1   : 16;
		} a3xx;
		struct PACKED {
			int32_t  immed    : 20;
			uint32_t dummy1   : 12;
		} a4xx;
		struct PACKED {
			int32_t immed     : 32;
		} a5xx;
	};

	/* dword1: */
	uint32_t dummy2   : 8;
	uint32_t repeat   : 3;
	uint32_t dummy3   : 1;
	uint32_t ss       : 1;
	uint32_t dummy4   : 7;
	uint32_t inv      : 1;
	uint32_t comp     : 2;
	uint32_t opc      : 4;
	uint32_t jmp_tgt  : 1;
	uint32_t sync     : 1;
	uint32_t opc_cat  : 3;
} instr_cat0_t;

typedef struct PACKED {
	/* dword0: */
	union PACKED {
		/* for normal src register: */
		struct PACKED {
			uint32_t src : 11;
			/* at least low bit of pad must be zero or it will
			 * look like a address relative src
			 */
			uint32_t pad : 21;
		};
		/* for address relative: */
		struct PACKED {
			int32_t  off : 10;
			uint32_t src_rel_c : 1;
			uint32_t src_rel : 1;
			uint32_t unknown : 20;
		};
		/* for immediate: */
		int32_t iim_val;
		uint32_t uim_val;
		float   fim_val;
	};

	/* dword1: */
	uint32_t dst        : 8;
	uint32_t repeat     : 3;
	uint32_t src_r      : 1;
	uint32_t ss         : 1;
	uint32_t ul         : 1;
	uint32_t dst_type   : 3;
	uint32_t dst_rel    : 1;
	uint32_t src_type   : 3;
	uint32_t src_c      : 1;
	uint32_t src_im     : 1;
	uint32_t even       : 1;
	uint32_t pos_inf    : 1;
	uint32_t must_be_0  : 2;
	uint32_t jmp_tgt    : 1;
	uint32_t sync       : 1;
	uint32_t opc_cat    : 3;
} instr_cat1_t;

typedef struct PACKED {
	/* dword0: */
	union PACKED {
		struct PACKED {
			uint32_t src1         : 11;
			uint32_t must_be_zero1: 2;
			uint32_t src1_im      : 1;   /* immediate */
			uint32_t src1_neg     : 1;   /* negate */
			uint32_t src1_abs     : 1;   /* absolute value */
		};
		struct PACKED {
			uint32_t src1         : 10;
			uint32_t src1_c       : 1;   /* relative-const */
			uint32_t src1_rel     : 1;   /* relative address */
			uint32_t must_be_zero : 1;
			uint32_t dummy        : 3;
		} rel1;
		struct PACKED {
			uint32_t src1         : 12;
			uint32_t src1_c       : 1;   /* const */
			uint32_t dummy        : 3;
		} c1;
	};

	union PACKED {
		struct PACKED {
			uint32_t src2         : 11;
			uint32_t must_be_zero2: 2;
			uint32_t src2_im      : 1;   /* immediate */
			uint32_t src2_neg     : 1;   /* negate */
			uint32_t src2_abs     : 1;   /* absolute value */
		};
		struct PACKED {
			uint32_t src2         : 10;
			uint32_t src2_c       : 1;   /* relative-const */
			uint32_t src2_rel     : 1;   /* relative address */
			uint32_t must_be_zero : 1;
			uint32_t dummy        : 3;
		} rel2;
		struct PACKED {
			uint32_t src2         : 12;
			uint32_t src2_c       : 1;   /* const */
			uint32_t dummy        : 3;
		} c2;
	};

	/* dword1: */
	uint32_t dst      : 8;
	uint32_t repeat   : 2;
	uint32_t sat      : 1;
	uint32_t src1_r   : 1;
	uint32_t ss       : 1;
	uint32_t ul       : 1;   /* dunno */
	uint32_t dst_half : 1;   /* or widen/narrow.. ie. dst hrN <-> rN */
	uint32_t ei       : 1;
	uint32_t cond     : 3;
	uint32_t src2_r   : 1;
	uint32_t full     : 1;   /* not half */
	uint32_t opc      : 6;
	uint32_t jmp_tgt  : 1;
	uint32_t sync     : 1;
	uint32_t opc_cat  : 3;
} instr_cat2_t;

typedef struct PACKED {
	/* dword0: */
	union PACKED {
		struct PACKED {
			uint32_t src1         : 11;
			uint32_t must_be_zero1: 2;
			uint32_t src2_c       : 1;
			uint32_t src1_neg     : 1;
			uint32_t src2_r       : 1;
		};
		struct PACKED {
			uint32_t src1         : 10;
			uint32_t src1_c       : 1;
			uint32_t src1_rel     : 1;
			uint32_t must_be_zero : 1;
			uint32_t dummy        : 3;
		} rel1;
		struct PACKED {
			uint32_t src1         : 12;
			uint32_t src1_c       : 1;
			uint32_t dummy        : 3;
		} c1;
	};

	union PACKED {
		struct PACKED {
			uint32_t src3         : 11;
			uint32_t must_be_zero2: 2;
			uint32_t src3_r       : 1;
			uint32_t src2_neg     : 1;
			uint32_t src3_neg     : 1;
		};
		struct PACKED {
			uint32_t src3         : 10;
			uint32_t src3_c       : 1;
			uint32_t src3_rel     : 1;
			uint32_t must_be_zero : 1;
			uint32_t dummy        : 3;
		} rel2;
		struct PACKED {
			uint32_t src3         : 12;
			uint32_t src3_c       : 1;
			uint32_t dummy        : 3;
		} c2;
	};

	/* dword1: */
	uint32_t dst      : 8;
	uint32_t repeat   : 2;
	uint32_t sat      : 1;
	uint32_t src1_r   : 1;
	uint32_t ss       : 1;
	uint32_t ul       : 1;
	uint32_t dst_half : 1;   /* or widen/narrow.. ie. dst hrN <-> rN */
	uint32_t src2     : 8;
	uint32_t opc      : 4;
	uint32_t jmp_tgt  : 1;
	uint32_t sync     : 1;
	uint32_t opc_cat  : 3;
} instr_cat3_t;

static inline bool instr_cat3_full(instr_cat3_t *cat3)
{
	switch (_OPC(3, cat3->opc)) {
	case OPC_MAD_F16:
	case OPC_MAD_U16:
	case OPC_MAD_S16:
	case OPC_SEL_B16:
	case OPC_SEL_S16:
	case OPC_SEL_F16:
	case OPC_SAD_S16:
	case OPC_SAD_S32:  // really??
		return false;
	default:
		return true;
	}
}

typedef struct PACKED {
	/* dword0: */
	union PACKED {
		struct PACKED {
			uint32_t src          : 11;
			uint32_t must_be_zero1: 2;
			uint32_t src_im       : 1;   /* immediate */
			uint32_t src_neg      : 1;   /* negate */
			uint32_t src_abs      : 1;   /* absolute value */
		};
		struct PACKED {
			uint32_t src          : 10;
			uint32_t src_c        : 1;   /* relative-const */
			uint32_t src_rel      : 1;   /* relative address */
			uint32_t must_be_zero : 1;
			uint32_t dummy        : 3;
		} rel;
		struct PACKED {
			uint32_t src          : 12;
			uint32_t src_c        : 1;   /* const */
			uint32_t dummy        : 3;
		} c;
	};
	uint32_t dummy1   : 16;  /* seem to be ignored */

	/* dword1: */
	uint32_t dst      : 8;
	uint32_t repeat   : 2;
	uint32_t sat      : 1;
	uint32_t src_r    : 1;
	uint32_t ss       : 1;
	uint32_t ul       : 1;
	uint32_t dst_half : 1;   /* or widen/narrow.. ie. dst hrN <-> rN */
	uint32_t dummy2   : 5;   /* seem to be ignored */
	uint32_t full     : 1;   /* not half */
	uint32_t opc      : 6;
	uint32_t jmp_tgt  : 1;
	uint32_t sync     : 1;
	uint32_t opc_cat  : 3;
} instr_cat4_t;

typedef struct PACKED {
	/* dword0: */
	union PACKED {
		/* normal case: */
		struct PACKED {
			uint32_t full     : 1;   /* not half */
			uint32_t src1     : 8;
			uint32_t src2     : 8;
			uint32_t dummy1   : 4;   /* seem to be ignored */
			uint32_t samp     : 4;
			uint32_t tex      : 7;
		} norm;
		/* s2en case: */
		struct PACKED {
			uint32_t full     : 1;   /* not half */
			uint32_t src1     : 8;
			uint32_t src2     : 11;
			uint32_t dummy1   : 1;
			uint32_t src3     : 8;
			uint32_t dummy2   : 3;
		} s2en;
		/* same in either case: */
		// XXX I think, confirm this
		struct PACKED {
			uint32_t full     : 1;   /* not half */
			uint32_t src1     : 8;
			uint32_t pad      : 23;
		};
	};

	/* dword1: */
	uint32_t dst      : 8;
	uint32_t wrmask   : 4;   /* write-mask */
	uint32_t type     : 3;
	uint32_t dummy2   : 1;   /* seems to be ignored */
	uint32_t is_3d    : 1;

	uint32_t is_a     : 1;
	uint32_t is_s     : 1;
	uint32_t is_s2en  : 1;
	uint32_t is_o     : 1;
	uint32_t is_p     : 1;

	uint32_t opc      : 5;
	uint32_t jmp_tgt  : 1;
	uint32_t sync     : 1;
	uint32_t opc_cat  : 3;
} instr_cat5_t;

/* dword0 encoding for src_off: [src1 + off], src2: */
typedef struct PACKED {
	/* dword0: */
	uint32_t mustbe1  : 1;
	int32_t  off      : 13;
	uint32_t src1     : 8;
	uint32_t src1_im  : 1;
	uint32_t src2_im  : 1;
	uint32_t src2     : 8;

	/* dword1: */
	uint32_t dword1;
} instr_cat6a_t;

/* dword0 encoding for !src_off: [src1], src2 */
typedef struct PACKED {
	/* dword0: */
	uint32_t mustbe0  : 1;
	uint32_t src1     : 13;
	uint32_t ignore0  : 8;
	uint32_t src1_im  : 1;
	uint32_t src2_im  : 1;
	uint32_t src2     : 8;

	/* dword1: */
	uint32_t dword1;
} instr_cat6b_t;

/* dword1 encoding for dst_off: */
typedef struct PACKED {
	/* dword0: */
	uint32_t dword0;

	/* note: there is some weird stuff going on where sometimes
	 * cat6->a.off is involved.. but that seems like a bug in
	 * the blob, since it is used even if !cat6->src_off
	 * It would make sense for there to be some more bits to
	 * bring us to 11 bits worth of offset, but not sure..
	 */
	int32_t off       : 8;
	uint32_t mustbe1  : 1;
	uint32_t dst      : 8;
	uint32_t pad1     : 15;
} instr_cat6c_t;

/* dword1 encoding for !dst_off: */
typedef struct PACKED {
	/* dword0: */
	uint32_t dword0;

	uint32_t dst      : 8;
	uint32_t mustbe0  : 1;
	uint32_t idx      : 8;
	uint32_t pad0     : 15;
} instr_cat6d_t;

/* ldgb and atomics.. atomics use 3rd src and pad0=1, pad3=1.  For
 * ldgb pad0=0, pad3=2
 *
 * note: we can probably merge these..
 */
typedef struct PACKED {
	/* dword0: */
	uint32_t pad0     : 1;
	uint32_t src3     : 8;
	uint32_t d        : 2;
	uint32_t typed    : 1;
	uint32_t type_size : 2;
	uint32_t src1     : 8;
	uint32_t src1_im  : 1;
	uint32_t src2_im  : 1;
	uint32_t src2     : 8;

	/* dword1: */
	uint32_t dst      : 8;
	uint32_t mustbe0  : 1;
	uint32_t src_ssbo : 8;
	uint32_t pad2     : 3;  // type
	uint32_t g        : 1;
	uint32_t pad3     : 1;
	uint32_t pad4     : 10; // opc/jmp_tgt/sync/opc_cat
} instr_cat6ldgb_t;

/* stgb, pad0=0, pad3=2
 */
typedef struct PACKED {
	/* dword0: */
	uint32_t mustbe1  : 1;  // ???
	uint32_t src1     : 8;
	uint32_t d        : 2;
	uint32_t typed    : 1;
	uint32_t type_size : 2;
	uint32_t pad0     : 9;
	uint32_t src2_im  : 1;
	uint32_t src2     : 8;

	/* dword1: */
	uint32_t src3     : 8;
	uint32_t src3_im  : 1;
	uint32_t dst_ssbo : 8;
	uint32_t pad2     : 3;  // type
	uint32_t pad3     : 2;
	uint32_t pad4     : 10; // opc/jmp_tgt/sync/opc_cat
} instr_cat6stgb_t;

typedef union PACKED {
	instr_cat6a_t a;
	instr_cat6b_t b;
	instr_cat6c_t c;
	instr_cat6d_t d;
	instr_cat6ldgb_t ldgb;
	instr_cat6stgb_t stgb;
	struct PACKED {
		/* dword0: */
		uint32_t src_off  : 1;
		uint32_t pad1     : 31;

		/* dword1: */
		uint32_t pad2     : 8;
		uint32_t dst_off  : 1;
		uint32_t pad3     : 8;
		uint32_t type     : 3;
		uint32_t g        : 1;  /* or in some cases it means dst immed */
		uint32_t pad4     : 1;
		uint32_t opc      : 5;
		uint32_t jmp_tgt  : 1;
		uint32_t sync     : 1;
		uint32_t opc_cat  : 3;
	};
} instr_cat6_t;

typedef struct PACKED {
	/* dword0: */
	uint32_t pad1     : 32;

	/* dword1: */
	uint32_t pad2     : 12;
	uint32_t ss       : 1;  /* maybe in the encoding, but blob only uses (sy) */
	uint32_t pad3     : 6;
	uint32_t w        : 1;  /* write */
	uint32_t r        : 1;  /* read */
	uint32_t l        : 1;  /* local */
	uint32_t g        : 1;  /* global */
	uint32_t opc      : 4;  /* presumed, but only a couple known OPCs */
	uint32_t jmp_tgt  : 1;  /* (jp) */
	uint32_t sync     : 1;  /* (sy) */
	uint32_t opc_cat  : 3;
} instr_cat7_t;

typedef union PACKED {
	instr_cat0_t cat0;
	instr_cat1_t cat1;
	instr_cat2_t cat2;
	instr_cat3_t cat3;
	instr_cat4_t cat4;
	instr_cat5_t cat5;
	instr_cat6_t cat6;
	instr_cat7_t cat7;
	struct PACKED {
		/* dword0: */
		uint32_t pad1     : 32;

		/* dword1: */
		uint32_t pad2     : 12;
		uint32_t ss       : 1;  /* cat1-cat4 (cat0??) and cat7 (?) */
		uint32_t ul       : 1;  /* cat2-cat4 (and cat1 in blob.. which may be bug??) */
		uint32_t pad3     : 13;
		uint32_t jmp_tgt  : 1;
		uint32_t sync     : 1;
		uint32_t opc_cat  : 3;
	};
} instr_t;

// GOOGLE: Restores the original packing alignment after pragma pack has been used in Windows.
#if defined(_MSC_VER)
#    pragma pack(pop)
#endif

static inline uint32_t instr_repeat(instr_t *instr)
{
	switch (instr->opc_cat) {
	case 0:  return instr->cat0.repeat;
	case 1:  return instr->cat1.repeat;
	case 2:  return instr->cat2.repeat;
	case 3:  return instr->cat3.repeat;
	case 4:  return instr->cat4.repeat;
	default: return 0;
	}
}

static inline bool instr_sat(instr_t *instr)
{
	switch (instr->opc_cat) {
	case 2:  return instr->cat2.sat;
	case 3:  return instr->cat3.sat;
	case 4:  return instr->cat4.sat;
	default: return false;
	}
}

static inline uint32_t instr_opc(instr_t *instr)
{
	switch (instr->opc_cat) {
	case 0:  return instr->cat0.opc;
	case 1:  return 0;
	case 2:  return instr->cat2.opc;
	case 3:  return instr->cat3.opc;
	case 4:  return instr->cat4.opc;
	case 5:  return instr->cat5.opc;
	case 6:  return instr->cat6.opc;
	case 7:  return instr->cat7.opc;
	default: return 0;
	}
}

static inline bool is_mad(opc_t opc)
{
	switch (opc) {
	case OPC_MAD_U16:
	case OPC_MAD_S16:
	case OPC_MAD_U24:
	case OPC_MAD_S24:
	case OPC_MAD_F16:
	case OPC_MAD_F32:
		return true;
	default:
		return false;
	}
}

static inline bool is_madsh(opc_t opc)
{
	switch (opc) {
	case OPC_MADSH_U16:
	case OPC_MADSH_M16:
		return true;
	default:
		return false;
	}
}

static inline bool is_atomic(opc_t opc)
{
	switch (opc) {
	case OPC_ATOMIC_ADD:
	case OPC_ATOMIC_SUB:
	case OPC_ATOMIC_XCHG:
	case OPC_ATOMIC_INC:
	case OPC_ATOMIC_DEC:
	case OPC_ATOMIC_CMPXCHG:
	case OPC_ATOMIC_MIN:
	case OPC_ATOMIC_MAX:
	case OPC_ATOMIC_AND:
	case OPC_ATOMIC_OR:
	case OPC_ATOMIC_XOR:
		return true;
	default:
		return false;
	}
}

#endif /* INSTR_A3XX_H_ */
