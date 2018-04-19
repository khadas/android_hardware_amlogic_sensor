/*
 *
 *  Bluetooth low-complexity, subband codec (SBC) library
 *
 *  Copyright (C) 2004-2006  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2004-2005  Henryk Ploetz <henryk@ploetzli.ch>
 *  Copyright (C) 2005-2006  Brad Midgley <bmidgley@xmission.com>
 *
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#define fabs(x) ((x) < 0 ? -(x) : (x))
/* C does not provide an explicit arithmetic shift right but this will
   always be correct and every compiler *should* generate optimal code */
#define ASR(val, bits) ((-2 >> 1 == -1) ? \
         ((int32_t)(val)) >> (bits) : ((int32_t) (val)) / (1 << (bits)))
#define ASR_64(val, bits) ((-2 >> 1 == -1) ? \
         ((long long)(val)) >> (bits) : ((long long) (val)) / (1 << (bits)))

#define SCALE_PROTO4_TBL    15
#define SCALE_ANA4_TBL      16
#define SCALE_PROTO8_TBL    15
#define SCALE_ANA8_TBL      16
#define SCALE_SPROTO4_TBL   16
#define SCALE_SPROTO8_TBL   16
#define SCALE_NPROTO4_TBL   10
#define SCALE_NPROTO8_TBL   12
#define SCALE_SAMPLES       14
#define SCALE4_STAGE1_BITS  10
#define SCALE4_STAGE2_BITS  21
#define SCALE4_STAGED1_BITS 18
#define SCALE4_STAGED2_BITS 23
#define SCALE8_STAGE1_BITS  8
#define SCALE8_STAGE2_BITS  24
#define SCALE8_STAGED1_BITS 18
#define SCALE8_STAGED2_BITS 23

//typedef int     int32_t;
//typedef int32_t sbc_fixed_t;
//typedef long long sbc_extended_t;

#define SCALE4_STAGE1(src)  ASR_64(src, SCALE4_STAGE1_BITS)
#define SCALE4_STAGE2(src)  ASR_64(src, SCALE4_STAGE2_BITS)
#define SCALE4_STAGED1(src) ASR_64(src, SCALE4_STAGED1_BITS)
#define SCALE4_STAGED2(src) ASR_64(src, SCALE4_STAGED2_BITS)
#define SCALE8_STAGE1(src)  ASR_64(src, SCALE8_STAGE1_BITS)
#define SCALE8_STAGE2(src)  ASR_64(src, SCALE8_STAGE2_BITS)
#define SCALE8_STAGED1(src) ASR_64(src, SCALE8_STAGED1_BITS)
#define SCALE8_STAGED2(src) ASR_64(src, SCALE8_STAGED2_BITS)

#define SBC_FIXED_0(val) { val = 0; }
#define ADD(dst, src)    { dst += src; }
#define SUB(dst, src)    { dst -= src; }
#define MUL(dst, a, b)   { dst = (sbc_extended_t) (a) * (b); }
#define MULA(dst, a, b)  { dst += (sbc_extended_t) (a) * (b); }
#define DIV2(dst, src)   { dst = ASR(src, 1); }
