/*
 *  PowerPC emulation helpers for qemu.
 *
 *  Copyright (c) 2003-2007 Jocelyn Mayer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "exec.h"
#include "host-utils.h"
#include "helper.h"

#include "helper_regs.h"

//#define DEBUG_OP
//#define DEBUG_EXCEPTIONS
//#define DEBUG_SOFTWARE_TLB

/*****************************************************************************/
/* Exceptions processing helpers */

void helper_raise_exception_err (uint32_t exception, uint32_t error_code)
{
#if 0
    printf("Raise exception %3x code : %d\n", exception, error_code);
#endif
    env->exception_index = exception;
    env->error_code = error_code;
    cpu_loop_exit();
}

void helper_raise_exception (uint32_t exception)
{
    helper_raise_exception_err(exception, 0);
}

/*****************************************************************************/
/* Registers load and stores */
target_ulong helper_load_cr (void)
{
    return (env->crf[0] << 28) |
           (env->crf[1] << 24) |
           (env->crf[2] << 20) |
           (env->crf[3] << 16) |
           (env->crf[4] << 12) |
           (env->crf[5] << 8) |
           (env->crf[6] << 4) |
           (env->crf[7] << 0);
}

void helper_store_cr (target_ulong val, uint32_t mask)
{
    int i, sh;

    for (i = 0, sh = 7; i < 8; i++, sh--) {
        if (mask & (1 << sh))
            env->crf[i] = (val >> (sh * 4)) & 0xFUL;
    }
}

/*****************************************************************************/
/* SPR accesses */
void helper_load_dump_spr (uint32_t sprn)
{
    if (loglevel != 0) {
        fprintf(logfile, "Read SPR %d %03x => " ADDRX "\n",
                sprn, sprn, env->spr[sprn]);
    }
}

void helper_store_dump_spr (uint32_t sprn)
{
    if (loglevel != 0) {
        fprintf(logfile, "Write SPR %d %03x <= " ADDRX "\n",
                sprn, sprn, env->spr[sprn]);
    }
}

target_ulong helper_load_tbl (void)
{
    return cpu_ppc_load_tbl(env);
}

target_ulong helper_load_tbu (void)
{
    return cpu_ppc_load_tbu(env);
}

target_ulong helper_load_atbl (void)
{
    return cpu_ppc_load_atbl(env);
}

target_ulong helper_load_atbu (void)
{
    return cpu_ppc_load_atbu(env);
}

target_ulong helper_load_601_rtcl (void)
{
    return cpu_ppc601_load_rtcl(env);
}

target_ulong helper_load_601_rtcu (void)
{
    return cpu_ppc601_load_rtcu(env);
}

#if !defined(CONFIG_USER_ONLY)
#if defined (TARGET_PPC64)
void helper_store_asr (target_ulong val)
{
    ppc_store_asr(env, val);
}
#endif

void helper_store_sdr1 (target_ulong val)
{
    ppc_store_sdr1(env, val);
}

void helper_store_tbl (target_ulong val)
{
    cpu_ppc_store_tbl(env, val);
}

void helper_store_tbu (target_ulong val)
{
    cpu_ppc_store_tbu(env, val);
}

void helper_store_atbl (target_ulong val)
{
    cpu_ppc_store_atbl(env, val);
}

void helper_store_atbu (target_ulong val)
{
    cpu_ppc_store_atbu(env, val);
}

void helper_store_601_rtcl (target_ulong val)
{
    cpu_ppc601_store_rtcl(env, val);
}

void helper_store_601_rtcu (target_ulong val)
{
    cpu_ppc601_store_rtcu(env, val);
}

target_ulong helper_load_decr (void)
{
    return cpu_ppc_load_decr(env);
}

void helper_store_decr (target_ulong val)
{
    cpu_ppc_store_decr(env, val);
}

void helper_store_hid0_601 (target_ulong val)
{
    target_ulong hid0;

    hid0 = env->spr[SPR_HID0];
    if ((val ^ hid0) & 0x00000008) {
        /* Change current endianness */
        env->hflags &= ~(1 << MSR_LE);
        env->hflags_nmsr &= ~(1 << MSR_LE);
        env->hflags_nmsr |= (1 << MSR_LE) & (((val >> 3) & 1) << MSR_LE);
        env->hflags |= env->hflags_nmsr;
        if (loglevel != 0) {
            fprintf(logfile, "%s: set endianness to %c => " ADDRX "\n",
                    __func__, val & 0x8 ? 'l' : 'b', env->hflags);
        }
    }
    env->spr[SPR_HID0] = (uint32_t)val;
}

void helper_store_403_pbr (uint32_t num, target_ulong value)
{
    if (likely(env->pb[num] != value)) {
        env->pb[num] = value;
        /* Should be optimized */
        tlb_flush(env, 1);
    }
}

target_ulong helper_load_40x_pit (void)
{
    return load_40x_pit(env);
}

void helper_store_40x_pit (target_ulong val)
{
    store_40x_pit(env, val);
}

void helper_store_40x_dbcr0 (target_ulong val)
{
    store_40x_dbcr0(env, val);
}

void helper_store_40x_sler (target_ulong val)
{
    store_40x_sler(env, val);
}

void helper_store_booke_tcr (target_ulong val)
{
    store_booke_tcr(env, val);
}

void helper_store_booke_tsr (target_ulong val)
{
    store_booke_tsr(env, val);
}

void helper_store_ibatu (uint32_t nr, target_ulong val)
{
    ppc_store_ibatu(env, nr, val);
}

void helper_store_ibatl (uint32_t nr, target_ulong val)
{
    ppc_store_ibatl(env, nr, val);
}

void helper_store_dbatu (uint32_t nr, target_ulong val)
{
    ppc_store_dbatu(env, nr, val);
}

void helper_store_dbatl (uint32_t nr, target_ulong val)
{
    ppc_store_dbatl(env, nr, val);
}

void helper_store_601_batl (uint32_t nr, target_ulong val)
{
    ppc_store_ibatl_601(env, nr, val);
}

void helper_store_601_batu (uint32_t nr, target_ulong val)
{
    ppc_store_ibatu_601(env, nr, val);
}
#endif

/*****************************************************************************/
/* Memory load and stores */

static always_inline target_ulong addr_add(target_ulong addr, target_long arg)
{
#if defined(TARGET_PPC64)
        if (!msr_sf)
            return (uint32_t)(addr + arg);
        else
#endif
            return addr + arg;
}

void helper_lmw (target_ulong addr, uint32_t reg)
{
    for (; reg < 32; reg++) {
        if (msr_le)
            env->gpr[reg] = bswap32(ldl(addr));
        else
            env->gpr[reg] = ldl(addr);
	addr = addr_add(addr, 4);
    }
}

void helper_stmw (target_ulong addr, uint32_t reg)
{
    for (; reg < 32; reg++) {
        if (msr_le)
            stl(addr, bswap32((uint32_t)env->gpr[reg]));
        else
            stl(addr, (uint32_t)env->gpr[reg]);
	addr = addr_add(addr, 4);
    }
}

void helper_lsw(target_ulong addr, uint32_t nb, uint32_t reg)
{
    int sh;
    for (; nb > 3; nb -= 4) {
        env->gpr[reg] = ldl(addr);
        reg = (reg + 1) % 32;
	addr = addr_add(addr, 4);
    }
    if (unlikely(nb > 0)) {
        env->gpr[reg] = 0;
        for (sh = 24; nb > 0; nb--, sh -= 8) {
            env->gpr[reg] |= ldub(addr) << sh;
	    addr = addr_add(addr, 1);
        }
    }
}
/* PPC32 specification says we must generate an exception if
 * rA is in the range of registers to be loaded.
 * In an other hand, IBM says this is valid, but rA won't be loaded.
 * For now, I'll follow the spec...
 */
void helper_lswx(target_ulong addr, uint32_t reg, uint32_t ra, uint32_t rb)
{
    if (likely(xer_bc != 0)) {
        if (unlikely((ra != 0 && reg < ra && (reg + xer_bc) > ra) ||
                     (reg < rb && (reg + xer_bc) > rb))) {
            helper_raise_exception_err(POWERPC_EXCP_PROGRAM,
                                       POWERPC_EXCP_INVAL |
                                       POWERPC_EXCP_INVAL_LSWX);
        } else {
            helper_lsw(addr, xer_bc, reg);
        }
    }
}

void helper_stsw(target_ulong addr, uint32_t nb, uint32_t reg)
{
    int sh;
    for (; nb > 3; nb -= 4) {
        stl(addr, env->gpr[reg]);
        reg = (reg + 1) % 32;
	addr = addr_add(addr, 4);
    }
    if (unlikely(nb > 0)) {
        for (sh = 24; nb > 0; nb--, sh -= 8)
            stb(addr, (env->gpr[reg] >> sh) & 0xFF);
	    addr = addr_add(addr, 1);
    }
}

static void do_dcbz(target_ulong addr, int dcache_line_size)
{
    addr &= ~(dcache_line_size - 1);
    int i;
    for (i = 0 ; i < dcache_line_size ; i += 4) {
        stl(addr + i , 0);
    }
    if (env->reserve == addr)
        env->reserve = (target_ulong)-1ULL;
}

void helper_dcbz(target_ulong addr)
{
    do_dcbz(addr, env->dcache_line_size);
}

void helper_dcbz_970(target_ulong addr)
{
    if (((env->spr[SPR_970_HID5] >> 7) & 0x3) == 1)
        do_dcbz(addr, 32);
    else
        do_dcbz(addr, env->dcache_line_size);
}

void helper_icbi(target_ulong addr)
{
    uint32_t tmp;

    addr &= ~(env->dcache_line_size - 1);
    /* Invalidate one cache line :
     * PowerPC specification says this is to be treated like a load
     * (not a fetch) by the MMU. To be sure it will be so,
     * do the load "by hand".
     */
    tmp = ldl(addr);
    tb_invalidate_page_range(addr, addr + env->icache_line_size);
}

// XXX: to be tested
target_ulong helper_lscbx (target_ulong addr, uint32_t reg, uint32_t ra, uint32_t rb)
{
    int i, c, d;
    d = 24;
    for (i = 0; i < xer_bc; i++) {
        c = ldub(addr);
	addr = addr_add(addr, 1);
        /* ra (if not 0) and rb are never modified */
        if (likely(reg != rb && (ra == 0 || reg != ra))) {
            env->gpr[reg] = (env->gpr[reg] & ~(0xFF << d)) | (c << d);
        }
        if (unlikely(c == xer_cmp))
            break;
        if (likely(d != 0)) {
            d -= 8;
        } else {
            d = 24;
            reg++;
            reg = reg & 0x1F;
        }
    }
    return i;
}

/*****************************************************************************/
/* Fixed point operations helpers */
#if defined(TARGET_PPC64)

/* multiply high word */
uint64_t helper_mulhd (uint64_t arg1, uint64_t arg2)
{
    uint64_t tl, th;

    muls64(&tl, &th, arg1, arg2);
    return th;
}

/* multiply high word unsigned */
uint64_t helper_mulhdu (uint64_t arg1, uint64_t arg2)
{
    uint64_t tl, th;

    mulu64(&tl, &th, arg1, arg2);
    return th;
}

uint64_t helper_mulldo (uint64_t arg1, uint64_t arg2)
{
    int64_t th;
    uint64_t tl;

    muls64(&tl, (uint64_t *)&th, arg1, arg2);
    /* If th != 0 && th != -1, then we had an overflow */
    if (likely((uint64_t)(th + 1) <= 1)) {
        env->xer &= ~(1 << XER_OV);
    } else {
        env->xer |= (1 << XER_OV) | (1 << XER_SO);
    }
    return (int64_t)tl;
}
#endif

target_ulong helper_cntlzw (target_ulong t)
{
    return clz32(t);
}

#if defined(TARGET_PPC64)
target_ulong helper_cntlzd (target_ulong t)
{
    return clz64(t);
}
#endif

/* shift right arithmetic helper */
target_ulong helper_sraw (target_ulong value, target_ulong shift)
{
    int32_t ret;

    if (likely(!(shift & 0x20))) {
        if (likely((uint32_t)shift != 0)) {
            shift &= 0x1f;
            ret = (int32_t)value >> shift;
            if (likely(ret >= 0 || (value & ((1 << shift) - 1)) == 0)) {
                env->xer &= ~(1 << XER_CA);
            } else {
                env->xer |= (1 << XER_CA);
            }
        } else {
            ret = (int32_t)value;
            env->xer &= ~(1 << XER_CA);
        }
    } else {
        ret = (int32_t)value >> 31;
        if (ret) {
            env->xer |= (1 << XER_CA);
        } else {
            env->xer &= ~(1 << XER_CA);
        }
    }
    return (target_long)ret;
}

#if defined(TARGET_PPC64)
target_ulong helper_srad (target_ulong value, target_ulong shift)
{
    int64_t ret;

    if (likely(!(shift & 0x40))) {
        if (likely((uint64_t)shift != 0)) {
            shift &= 0x3f;
            ret = (int64_t)value >> shift;
            if (likely(ret >= 0 || (value & ((1 << shift) - 1)) == 0)) {
                env->xer &= ~(1 << XER_CA);
            } else {
                env->xer |= (1 << XER_CA);
            }
        } else {
            ret = (int64_t)value;
            env->xer &= ~(1 << XER_CA);
        }
    } else {
        ret = (int64_t)value >> 63;
        if (ret) {
            env->xer |= (1 << XER_CA);
        } else {
            env->xer &= ~(1 << XER_CA);
        }
    }
    return ret;
}
#endif

target_ulong helper_popcntb (target_ulong val)
{
    val = (val & 0x55555555) + ((val >>  1) & 0x55555555);
    val = (val & 0x33333333) + ((val >>  2) & 0x33333333);
    val = (val & 0x0f0f0f0f) + ((val >>  4) & 0x0f0f0f0f);
    return val;
}

#if defined(TARGET_PPC64)
target_ulong helper_popcntb_64 (target_ulong val)
{
    val = (val & 0x5555555555555555ULL) + ((val >>  1) & 0x5555555555555555ULL);
    val = (val & 0x3333333333333333ULL) + ((val >>  2) & 0x3333333333333333ULL);
    val = (val & 0x0f0f0f0f0f0f0f0fULL) + ((val >>  4) & 0x0f0f0f0f0f0f0f0fULL);
    return val;
}
#endif

/*****************************************************************************/
/* Floating point operations helpers */
uint64_t helper_float32_to_float64(uint32_t arg)
{
    CPU_FloatU f;
    CPU_DoubleU d;
    f.l = arg;
    d.d = float32_to_float64(f.f, &env->fp_status);
    return d.ll;
}

uint32_t helper_float64_to_float32(uint64_t arg)
{
    CPU_FloatU f;
    CPU_DoubleU d;
    d.ll = arg;
    f.f = float64_to_float32(d.d, &env->fp_status);
    return f.l;
}

static always_inline int isden (float64 d)
{
    CPU_DoubleU u;

    u.d = d;

    return ((u.ll >> 52) & 0x7FF) == 0;
}

uint32_t helper_compute_fprf (uint64_t arg, uint32_t set_fprf)
{
    CPU_DoubleU farg;
    int isneg;
    int ret;
    farg.ll = arg;
    isneg = float64_is_neg(farg.d);
    if (unlikely(float64_is_nan(farg.d))) {
        if (float64_is_signaling_nan(farg.d)) {
            /* Signaling NaN: flags are undefined */
            ret = 0x00;
        } else {
            /* Quiet NaN */
            ret = 0x11;
        }
    } else if (unlikely(float64_is_infinity(farg.d))) {
        /* +/- infinity */
        if (isneg)
            ret = 0x09;
        else
            ret = 0x05;
    } else {
        if (float64_is_zero(farg.d)) {
            /* +/- zero */
            if (isneg)
                ret = 0x12;
            else
                ret = 0x02;
        } else {
            if (isden(farg.d)) {
                /* Denormalized numbers */
                ret = 0x10;
            } else {
                /* Normalized numbers */
                ret = 0x00;
            }
            if (isneg) {
                ret |= 0x08;
            } else {
                ret |= 0x04;
            }
        }
    }
    if (set_fprf) {
        /* We update FPSCR_FPRF */
        env->fpscr &= ~(0x1F << FPSCR_FPRF);
        env->fpscr |= ret << FPSCR_FPRF;
    }
    /* We just need fpcc to update Rc1 */
    return ret & 0xF;
}

/* Floating-point invalid operations exception */
static always_inline uint64_t fload_invalid_op_excp (int op)
{
    uint64_t ret = 0;
    int ve;

    ve = fpscr_ve;
    switch (op) {
    case POWERPC_EXCP_FP_VXSNAN:
        env->fpscr |= 1 << FPSCR_VXSNAN;
	break;
    case POWERPC_EXCP_FP_VXSOFT:
        env->fpscr |= 1 << FPSCR_VXSOFT;
	break;
    case POWERPC_EXCP_FP_VXISI:
        /* Magnitude subtraction of infinities */
        env->fpscr |= 1 << FPSCR_VXISI;
        goto update_arith;
    case POWERPC_EXCP_FP_VXIDI:
        /* Division of infinity by infinity */
        env->fpscr |= 1 << FPSCR_VXIDI;
        goto update_arith;
    case POWERPC_EXCP_FP_VXZDZ:
        /* Division of zero by zero */
        env->fpscr |= 1 << FPSCR_VXZDZ;
        goto update_arith;
    case POWERPC_EXCP_FP_VXIMZ:
        /* Multiplication of zero by infinity */
        env->fpscr |= 1 << FPSCR_VXIMZ;
        goto update_arith;
    case POWERPC_EXCP_FP_VXVC:
        /* Ordered comparison of NaN */
        env->fpscr |= 1 << FPSCR_VXVC;
        env->fpscr &= ~(0xF << FPSCR_FPCC);
        env->fpscr |= 0x11 << FPSCR_FPCC;
        /* We must update the target FPR before raising the exception */
        if (ve != 0) {
            env->exception_index = POWERPC_EXCP_PROGRAM;
            env->error_code = POWERPC_EXCP_FP | POWERPC_EXCP_FP_VXVC;
            /* Update the floating-point enabled exception summary */
            env->fpscr |= 1 << FPSCR_FEX;
            /* Exception is differed */
            ve = 0;
        }
        break;
    case POWERPC_EXCP_FP_VXSQRT:
        /* Square root of a negative number */
        env->fpscr |= 1 << FPSCR_VXSQRT;
    update_arith:
        env->fpscr &= ~((1 << FPSCR_FR) | (1 << FPSCR_FI));
        if (ve == 0) {
            /* Set the result to quiet NaN */
            ret = 0xFFF8000000000000ULL;
            env->fpscr &= ~(0xF << FPSCR_FPCC);
            env->fpscr |= 0x11 << FPSCR_FPCC;
        }
        break;
    case POWERPC_EXCP_FP_VXCVI:
        /* Invalid conversion */
        env->fpscr |= 1 << FPSCR_VXCVI;
        env->fpscr &= ~((1 << FPSCR_FR) | (1 << FPSCR_FI));
        if (ve == 0) {
            /* Set the result to quiet NaN */
            ret = 0xFFF8000000000000ULL;
            env->fpscr &= ~(0xF << FPSCR_FPCC);
            env->fpscr |= 0x11 << FPSCR_FPCC;
        }
        break;
    }
    /* Update the floating-point invalid operation summary */
    env->fpscr |= 1 << FPSCR_VX;
    /* Update the floating-point exception summary */
    env->fpscr |= 1 << FPSCR_FX;
    if (ve != 0) {
        /* Update the floating-point enabled exception summary */
        env->fpscr |= 1 << FPSCR_FEX;
        if (msr_fe0 != 0 || msr_fe1 != 0)
            helper_raise_exception_err(POWERPC_EXCP_PROGRAM, POWERPC_EXCP_FP | op);
    }
    return ret;
}

static always_inline void float_zero_divide_excp (void)
{
    env->fpscr |= 1 << FPSCR_ZX;
    env->fpscr &= ~((1 << FPSCR_FR) | (1 << FPSCR_FI));
    /* Update the floating-point exception summary */
    env->fpscr |= 1 << FPSCR_FX;
    if (fpscr_ze != 0) {
        /* Update the floating-point enabled exception summary */
        env->fpscr |= 1 << FPSCR_FEX;
        if (msr_fe0 != 0 || msr_fe1 != 0) {
            helper_raise_exception_err(POWERPC_EXCP_PROGRAM,
                                       POWERPC_EXCP_FP | POWERPC_EXCP_FP_ZX);
        }
    }
}

static always_inline void float_overflow_excp (void)
{
    env->fpscr |= 1 << FPSCR_OX;
    /* Update the floating-point exception summary */
    env->fpscr |= 1 << FPSCR_FX;
    if (fpscr_oe != 0) {
        /* XXX: should adjust the result */
        /* Update the floating-point enabled exception summary */
        env->fpscr |= 1 << FPSCR_FEX;
        /* We must update the target FPR before raising the exception */
        env->exception_index = POWERPC_EXCP_PROGRAM;
        env->error_code = POWERPC_EXCP_FP | POWERPC_EXCP_FP_OX;
    } else {
        env->fpscr |= 1 << FPSCR_XX;
        env->fpscr |= 1 << FPSCR_FI;
    }
}

static always_inline void float_underflow_excp (void)
{
    env->fpscr |= 1 << FPSCR_UX;
    /* Update the floating-point exception summary */
    env->fpscr |= 1 << FPSCR_FX;
    if (fpscr_ue != 0) {
        /* XXX: should adjust the result */
        /* Update the floating-point enabled exception summary */
        env->fpscr |= 1 << FPSCR_FEX;
        /* We must update the target FPR before raising the exception */
        env->exception_index = POWERPC_EXCP_PROGRAM;
        env->error_code = POWERPC_EXCP_FP | POWERPC_EXCP_FP_UX;
    }
}

static always_inline void float_inexact_excp (void)
{
    env->fpscr |= 1 << FPSCR_XX;
    /* Update the floating-point exception summary */
    env->fpscr |= 1 << FPSCR_FX;
    if (fpscr_xe != 0) {
        /* Update the floating-point enabled exception summary */
        env->fpscr |= 1 << FPSCR_FEX;
        /* We must update the target FPR before raising the exception */
        env->exception_index = POWERPC_EXCP_PROGRAM;
        env->error_code = POWERPC_EXCP_FP | POWERPC_EXCP_FP_XX;
    }
}

static always_inline void fpscr_set_rounding_mode (void)
{
    int rnd_type;

    /* Set rounding mode */
    switch (fpscr_rn) {
    case 0:
        /* Best approximation (round to nearest) */
        rnd_type = float_round_nearest_even;
        break;
    case 1:
        /* Smaller magnitude (round toward zero) */
        rnd_type = float_round_to_zero;
        break;
    case 2:
        /* Round toward +infinite */
        rnd_type = float_round_up;
        break;
    default:
    case 3:
        /* Round toward -infinite */
        rnd_type = float_round_down;
        break;
    }
    set_float_rounding_mode(rnd_type, &env->fp_status);
}

void helper_fpscr_clrbit (uint32_t bit)
{
    int prev;

    prev = (env->fpscr >> bit) & 1;
    env->fpscr &= ~(1 << bit);
    if (prev == 1) {
        switch (bit) {
        case FPSCR_RN1:
        case FPSCR_RN:
            fpscr_set_rounding_mode();
            break;
        default:
            break;
        }
    }
}

void helper_fpscr_setbit (uint32_t bit)
{
    int prev;

    prev = (env->fpscr >> bit) & 1;
    env->fpscr |= 1 << bit;
    if (prev == 0) {
        switch (bit) {
        case FPSCR_VX:
            env->fpscr |= 1 << FPSCR_FX;
            if (fpscr_ve)
                goto raise_ve;
        case FPSCR_OX:
            env->fpscr |= 1 << FPSCR_FX;
            if (fpscr_oe)
                goto raise_oe;
            break;
        case FPSCR_UX:
            env->fpscr |= 1 << FPSCR_FX;
            if (fpscr_ue)
                goto raise_ue;
            break;
        case FPSCR_ZX:
            env->fpscr |= 1 << FPSCR_FX;
            if (fpscr_ze)
                goto raise_ze;
            break;
        case FPSCR_XX:
            env->fpscr |= 1 << FPSCR_FX;
            if (fpscr_xe)
                goto raise_xe;
            break;
        case FPSCR_VXSNAN:
        case FPSCR_VXISI:
        case FPSCR_VXIDI:
        case FPSCR_VXZDZ:
        case FPSCR_VXIMZ:
        case FPSCR_VXVC:
        case FPSCR_VXSOFT:
        case FPSCR_VXSQRT:
        case FPSCR_VXCVI:
            env->fpscr |= 1 << FPSCR_VX;
            env->fpscr |= 1 << FPSCR_FX;
            if (fpscr_ve != 0)
                goto raise_ve;
            break;
        case FPSCR_VE:
            if (fpscr_vx != 0) {
            raise_ve:
                env->error_code = POWERPC_EXCP_FP;
                if (fpscr_vxsnan)
                    env->error_code |= POWERPC_EXCP_FP_VXSNAN;
                if (fpscr_vxisi)
                    env->error_code |= POWERPC_EXCP_FP_VXISI;
                if (fpscr_vxidi)
                    env->error_code |= POWERPC_EXCP_FP_VXIDI;
                if (fpscr_vxzdz)
                    env->error_code |= POWERPC_EXCP_FP_VXZDZ;
                if (fpscr_vximz)
                    env->error_code |= POWERPC_EXCP_FP_VXIMZ;
                if (fpscr_vxvc)
                    env->error_code |= POWERPC_EXCP_FP_VXVC;
                if (fpscr_vxsoft)
                    env->error_code |= POWERPC_EXCP_FP_VXSOFT;
                if (fpscr_vxsqrt)
                    env->error_code |= POWERPC_EXCP_FP_VXSQRT;
                if (fpscr_vxcvi)
                    env->error_code |= POWERPC_EXCP_FP_VXCVI;
                goto raise_excp;
            }
            break;
        case FPSCR_OE:
            if (fpscr_ox != 0) {
            raise_oe:
                env->error_code = POWERPC_EXCP_FP | POWERPC_EXCP_FP_OX;
                goto raise_excp;
            }
            break;
        case FPSCR_UE:
            if (fpscr_ux != 0) {
            raise_ue:
                env->error_code = POWERPC_EXCP_FP | POWERPC_EXCP_FP_UX;
                goto raise_excp;
            }
            break;
        case FPSCR_ZE:
            if (fpscr_zx != 0) {
            raise_ze:
                env->error_code = POWERPC_EXCP_FP | POWERPC_EXCP_FP_ZX;
                goto raise_excp;
            }
            break;
        case FPSCR_XE:
            if (fpscr_xx != 0) {
            raise_xe:
                env->error_code = POWERPC_EXCP_FP | POWERPC_EXCP_FP_XX;
                goto raise_excp;
            }
            break;
        case FPSCR_RN1:
        case FPSCR_RN:
            fpscr_set_rounding_mode();
            break;
        default:
            break;
        raise_excp:
            /* Update the floating-point enabled exception summary */
            env->fpscr |= 1 << FPSCR_FEX;
                /* We have to update Rc1 before raising the exception */
            env->exception_index = POWERPC_EXCP_PROGRAM;
            break;
        }
    }
}

void helper_store_fpscr (uint64_t arg, uint32_t mask)
{
    /*
     * We use only the 32 LSB of the incoming fpr
     */
    uint32_t prev, new;
    int i;

    prev = env->fpscr;
    new = (uint32_t)arg;
    new &= ~0x60000000;
    new |= prev & 0x60000000;
    for (i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            env->fpscr &= ~(0xF << (4 * i));
            env->fpscr |= new & (0xF << (4 * i));
        }
    }
    /* Update VX and FEX */
    if (fpscr_ix != 0)
        env->fpscr |= 1 << FPSCR_VX;
    else
        env->fpscr &= ~(1 << FPSCR_VX);
    if ((fpscr_ex & fpscr_eex) != 0) {
        env->fpscr |= 1 << FPSCR_FEX;
        env->exception_index = POWERPC_EXCP_PROGRAM;
        /* XXX: we should compute it properly */
        env->error_code = POWERPC_EXCP_FP;
    }
    else
        env->fpscr &= ~(1 << FPSCR_FEX);
    fpscr_set_rounding_mode();
}

void helper_float_check_status (void)
{
#ifdef CONFIG_SOFTFLOAT
    if (env->exception_index == POWERPC_EXCP_PROGRAM &&
        (env->error_code & POWERPC_EXCP_FP)) {
        /* Differred floating-point exception after target FPR update */
        if (msr_fe0 != 0 || msr_fe1 != 0)
            helper_raise_exception_err(env->exception_index, env->error_code);
    } else {
        int status = get_float_exception_flags(&env->fp_status);
        if (status & float_flag_divbyzero) {
            float_zero_divide_excp();
        } else if (status & float_flag_overflow) {
            float_overflow_excp();
        } else if (status & float_flag_underflow) {
            float_underflow_excp();
        } else if (status & float_flag_inexact) {
            float_inexact_excp();
        }
    }
#else
    if (env->exception_index == POWERPC_EXCP_PROGRAM &&
        (env->error_code & POWERPC_EXCP_FP)) {
        /* Differred floating-point exception after target FPR update */
        if (msr_fe0 != 0 || msr_fe1 != 0)
            helper_raise_exception_err(env->exception_index, env->error_code);
    }
#endif
}

#ifdef CONFIG_SOFTFLOAT
void helper_reset_fpstatus (void)
{
    set_float_exception_flags(0, &env->fp_status);
}
#endif

/* fadd - fadd. */
uint64_t helper_fadd (uint64_t arg1, uint64_t arg2)
{
    CPU_DoubleU farg1, farg2;

    farg1.ll = arg1;
    farg2.ll = arg2;
#if USE_PRECISE_EMULATION
    if (unlikely(float64_is_signaling_nan(farg1.d) ||
                 float64_is_signaling_nan(farg2.d))) {
        /* sNaN addition */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely(float64_is_infinity(farg1.d) && float64_is_infinity(farg2.d) &&
                      float64_is_neg(farg1.d) != float64_is_neg(farg2.d))) {
        /* Magnitude subtraction of infinities */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXISI);
    } else {
        farg1.d = float64_add(farg1.d, farg2.d, &env->fp_status);
    }
#else
    farg1.d = float64_add(farg1.d, farg2.d, &env->fp_status);
#endif
    return farg1.ll;
}

/* fsub - fsub. */
uint64_t helper_fsub (uint64_t arg1, uint64_t arg2)
{
    CPU_DoubleU farg1, farg2;

    farg1.ll = arg1;
    farg2.ll = arg2;
#if USE_PRECISE_EMULATION
{
    if (unlikely(float64_is_signaling_nan(farg1.d) ||
                 float64_is_signaling_nan(farg2.d))) {
        /* sNaN subtraction */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely(float64_is_infinity(farg1.d) && float64_is_infinity(farg2.d) &&
                      float64_is_neg(farg1.d) == float64_is_neg(farg2.d))) {
        /* Magnitude subtraction of infinities */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXISI);
    } else {
        farg1.d = float64_sub(farg1.d, farg2.d, &env->fp_status);
    }
}
#else
    farg1.d = float64_sub(farg1.d, farg2.d, &env->fp_status);
#endif
    return farg1.ll;
}

/* fmul - fmul. */
uint64_t helper_fmul (uint64_t arg1, uint64_t arg2)
{
    CPU_DoubleU farg1, farg2;

    farg1.ll = arg1;
    farg2.ll = arg2;
#if USE_PRECISE_EMULATION
    if (unlikely(float64_is_signaling_nan(farg1.d) ||
                 float64_is_signaling_nan(farg2.d))) {
        /* sNaN multiplication */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely((float64_is_infinity(farg1.d) && float64_is_zero(farg2.d)) ||
                        (float64_is_zero(farg1.d) && float64_is_infinity(farg2.d)))) {
        /* Multiplication of zero by infinity */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXIMZ);
    } else {
        farg1.d = float64_mul(farg1.d, farg2.d, &env->fp_status);
    }
#else
    farg1.d = float64_mul(farg1.d, farg2.d, &env->fp_status);
#endif
    return farg1.ll;
}

/* fdiv - fdiv. */
uint64_t helper_fdiv (uint64_t arg1, uint64_t arg2)
{
    CPU_DoubleU farg1, farg2;

    farg1.ll = arg1;
    farg2.ll = arg2;
#if USE_PRECISE_EMULATION
    if (unlikely(float64_is_signaling_nan(farg1.d) ||
                 float64_is_signaling_nan(farg2.d))) {
        /* sNaN division */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely(float64_is_infinity(farg1.d) && float64_is_infinity(farg2.d))) {
        /* Division of infinity by infinity */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXIDI);
    } else if (unlikely(float64_is_zero(farg1.d) && float64_is_zero(farg2.d))) {
        /* Division of zero by zero */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXZDZ);
    } else {
        farg1.d = float64_div(farg1.d, farg2.d, &env->fp_status);
    }
#else
    farg1.d = float64_div(farg1.d, farg2.d, &env->fp_status);
#endif
    return farg1.ll;
}

/* fabs */
uint64_t helper_fabs (uint64_t arg)
{
    CPU_DoubleU farg;

    farg.ll = arg;
    farg.d = float64_abs(farg.d);
    return farg.ll;
}

/* fnabs */
uint64_t helper_fnabs (uint64_t arg)
{
    CPU_DoubleU farg;

    farg.ll = arg;
    farg.d = float64_abs(farg.d);
    farg.d = float64_chs(farg.d);
    return farg.ll;
}

/* fneg */
uint64_t helper_fneg (uint64_t arg)
{
    CPU_DoubleU farg;

    farg.ll = arg;
    farg.d = float64_chs(farg.d);
    return farg.ll;
}

/* fctiw - fctiw. */
uint64_t helper_fctiw (uint64_t arg)
{
    CPU_DoubleU farg;
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN conversion */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN | POWERPC_EXCP_FP_VXCVI);
    } else if (unlikely(float64_is_nan(farg.d) || float64_is_infinity(farg.d))) {
        /* qNan / infinity conversion */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXCVI);
    } else {
        farg.ll = float64_to_int32(farg.d, &env->fp_status);
#if USE_PRECISE_EMULATION
        /* XXX: higher bits are not supposed to be significant.
         *     to make tests easier, return the same as a real PowerPC 750
         */
        farg.ll |= 0xFFF80000ULL << 32;
#endif
    }
    return farg.ll;
}

/* fctiwz - fctiwz. */
uint64_t helper_fctiwz (uint64_t arg)
{
    CPU_DoubleU farg;
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN conversion */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN | POWERPC_EXCP_FP_VXCVI);
    } else if (unlikely(float64_is_nan(farg.d) || float64_is_infinity(farg.d))) {
        /* qNan / infinity conversion */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXCVI);
    } else {
        farg.ll = float64_to_int32_round_to_zero(farg.d, &env->fp_status);
#if USE_PRECISE_EMULATION
        /* XXX: higher bits are not supposed to be significant.
         *     to make tests easier, return the same as a real PowerPC 750
         */
        farg.ll |= 0xFFF80000ULL << 32;
#endif
    }
    return farg.ll;
}

#if defined(TARGET_PPC64)
/* fcfid - fcfid. */
uint64_t helper_fcfid (uint64_t arg)
{
    CPU_DoubleU farg;
    farg.d = int64_to_float64(arg, &env->fp_status);
    return farg.ll;
}

/* fctid - fctid. */
uint64_t helper_fctid (uint64_t arg)
{
    CPU_DoubleU farg;
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN conversion */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN | POWERPC_EXCP_FP_VXCVI);
    } else if (unlikely(float64_is_nan(farg.d) || float64_is_infinity(farg.d))) {
        /* qNan / infinity conversion */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXCVI);
    } else {
        farg.ll = float64_to_int64(farg.d, &env->fp_status);
    }
    return farg.ll;
}

/* fctidz - fctidz. */
uint64_t helper_fctidz (uint64_t arg)
{
    CPU_DoubleU farg;
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN conversion */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN | POWERPC_EXCP_FP_VXCVI);
    } else if (unlikely(float64_is_nan(farg.d) || float64_is_infinity(farg.d))) {
        /* qNan / infinity conversion */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXCVI);
    } else {
        farg.ll = float64_to_int64_round_to_zero(farg.d, &env->fp_status);
    }
    return farg.ll;
}

#endif

static always_inline uint64_t do_fri (uint64_t arg, int rounding_mode)
{
    CPU_DoubleU farg;
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN round */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN | POWERPC_EXCP_FP_VXCVI);
    } else if (unlikely(float64_is_nan(farg.d) || float64_is_infinity(farg.d))) {
        /* qNan / infinity round */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXCVI);
    } else {
        set_float_rounding_mode(rounding_mode, &env->fp_status);
        farg.ll = float64_round_to_int(farg.d, &env->fp_status);
        /* Restore rounding mode from FPSCR */
        fpscr_set_rounding_mode();
    }
    return farg.ll;
}

uint64_t helper_frin (uint64_t arg)
{
    return do_fri(arg, float_round_nearest_even);
}

uint64_t helper_friz (uint64_t arg)
{
    return do_fri(arg, float_round_to_zero);
}

uint64_t helper_frip (uint64_t arg)
{
    return do_fri(arg, float_round_up);
}

uint64_t helper_frim (uint64_t arg)
{
    return do_fri(arg, float_round_down);
}

/* fmadd - fmadd. */
uint64_t helper_fmadd (uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    CPU_DoubleU farg1, farg2, farg3;

    farg1.ll = arg1;
    farg2.ll = arg2;
    farg3.ll = arg3;
#if USE_PRECISE_EMULATION
    if (unlikely(float64_is_signaling_nan(farg1.d) ||
                 float64_is_signaling_nan(farg2.d) ||
                 float64_is_signaling_nan(farg3.d))) {
        /* sNaN operation */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely((float64_is_infinity(farg1.d) && float64_is_zero(farg2.d)) ||
                        (float64_is_zero(farg1.d) && float64_is_infinity(farg2.d)))) {
        /* Multiplication of zero by infinity */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXIMZ);
    } else {
#ifdef FLOAT128
        /* This is the way the PowerPC specification defines it */
        float128 ft0_128, ft1_128;

        ft0_128 = float64_to_float128(farg1.d, &env->fp_status);
        ft1_128 = float64_to_float128(farg2.d, &env->fp_status);
        ft0_128 = float128_mul(ft0_128, ft1_128, &env->fp_status);
        if (unlikely(float128_is_infinity(ft0_128) && float64_is_infinity(farg3.d) &&
                     float128_is_neg(ft0_128) != float64_is_neg(farg3.d))) {
            /* Magnitude subtraction of infinities */
            farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXISI);
        } else {
            ft1_128 = float64_to_float128(farg3.d, &env->fp_status);
            ft0_128 = float128_add(ft0_128, ft1_128, &env->fp_status);
            farg1.d = float128_to_float64(ft0_128, &env->fp_status);
        }
#else
        /* This is OK on x86 hosts */
        farg1.d = (farg1.d * farg2.d) + farg3.d;
#endif
    }
#else
    farg1.d = float64_mul(farg1.d, farg2.d, &env->fp_status);
    farg1.d = float64_add(farg1.d, farg3.d, &env->fp_status);
#endif
    return farg1.ll;
}

/* fmsub - fmsub. */
uint64_t helper_fmsub (uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    CPU_DoubleU farg1, farg2, farg3;

    farg1.ll = arg1;
    farg2.ll = arg2;
    farg3.ll = arg3;
#if USE_PRECISE_EMULATION
    if (unlikely(float64_is_signaling_nan(farg1.d) ||
                 float64_is_signaling_nan(farg2.d) ||
                 float64_is_signaling_nan(farg3.d))) {
        /* sNaN operation */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely((float64_is_infinity(farg1.d) && float64_is_zero(farg2.d)) ||
                        (float64_is_zero(farg1.d) && float64_is_infinity(farg2.d)))) {
        /* Multiplication of zero by infinity */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXIMZ);
    } else {
#ifdef FLOAT128
        /* This is the way the PowerPC specification defines it */
        float128 ft0_128, ft1_128;

        ft0_128 = float64_to_float128(farg1.d, &env->fp_status);
        ft1_128 = float64_to_float128(farg2.d, &env->fp_status);
        ft0_128 = float128_mul(ft0_128, ft1_128, &env->fp_status);
        if (unlikely(float128_is_infinity(ft0_128) && float64_is_infinity(farg3.d) &&
                     float128_is_neg(ft0_128) == float64_is_neg(farg3.d))) {
            /* Magnitude subtraction of infinities */
            farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXISI);
        } else {
            ft1_128 = float64_to_float128(farg3.d, &env->fp_status);
            ft0_128 = float128_sub(ft0_128, ft1_128, &env->fp_status);
            farg1.d = float128_to_float64(ft0_128, &env->fp_status);
        }
#else
        /* This is OK on x86 hosts */
        farg1.d = (farg1.d * farg2.d) - farg3.d;
#endif
    }
#else
    farg1.d = float64_mul(farg1.d, farg2.d, &env->fp_status);
    farg1.d = float64_sub(farg1.d, farg3.d, &env->fp_status);
#endif
    return farg1.ll;
}

/* fnmadd - fnmadd. */
uint64_t helper_fnmadd (uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    CPU_DoubleU farg1, farg2, farg3;

    farg1.ll = arg1;
    farg2.ll = arg2;
    farg3.ll = arg3;

    if (unlikely(float64_is_signaling_nan(farg1.d) ||
                 float64_is_signaling_nan(farg2.d) ||
                 float64_is_signaling_nan(farg3.d))) {
        /* sNaN operation */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely((float64_is_infinity(farg1.d) && float64_is_zero(farg2.d)) ||
                        (float64_is_zero(farg1.d) && float64_is_infinity(farg2.d)))) {
        /* Multiplication of zero by infinity */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXIMZ);
    } else {
#if USE_PRECISE_EMULATION
#ifdef FLOAT128
        /* This is the way the PowerPC specification defines it */
        float128 ft0_128, ft1_128;

        ft0_128 = float64_to_float128(farg1.d, &env->fp_status);
        ft1_128 = float64_to_float128(farg2.d, &env->fp_status);
        ft0_128 = float128_mul(ft0_128, ft1_128, &env->fp_status);
        if (unlikely(float128_is_infinity(ft0_128) && float64_is_infinity(farg3.d) &&
                     float128_is_neg(ft0_128) != float64_is_neg(farg3.d))) {
            /* Magnitude subtraction of infinities */
            farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXISI);
        } else {
            ft1_128 = float64_to_float128(farg3.d, &env->fp_status);
            ft0_128 = float128_add(ft0_128, ft1_128, &env->fp_status);
            farg1.d = float128_to_float64(ft0_128, &env->fp_status);
        }
#else
        /* This is OK on x86 hosts */
        farg1.d = (farg1.d * farg2.d) + farg3.d;
#endif
#else
        farg1.d = float64_mul(farg1.d, farg2.d, &env->fp_status);
        farg1.d = float64_add(farg1.d, farg3.d, &env->fp_status);
#endif
        if (likely(!float64_is_nan(farg1.d)))
            farg1.d = float64_chs(farg1.d);
    }
    return farg1.ll;
}

/* fnmsub - fnmsub. */
uint64_t helper_fnmsub (uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    CPU_DoubleU farg1, farg2, farg3;

    farg1.ll = arg1;
    farg2.ll = arg2;
    farg3.ll = arg3;

    if (unlikely(float64_is_signaling_nan(farg1.d) ||
                 float64_is_signaling_nan(farg2.d) ||
                 float64_is_signaling_nan(farg3.d))) {
        /* sNaN operation */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely((float64_is_infinity(farg1.d) && float64_is_zero(farg2.d)) ||
                        (float64_is_zero(farg1.d) && float64_is_infinity(farg2.d)))) {
        /* Multiplication of zero by infinity */
        farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXIMZ);
    } else {
#if USE_PRECISE_EMULATION
#ifdef FLOAT128
        /* This is the way the PowerPC specification defines it */
        float128 ft0_128, ft1_128;

        ft0_128 = float64_to_float128(farg1.d, &env->fp_status);
        ft1_128 = float64_to_float128(farg2.d, &env->fp_status);
        ft0_128 = float128_mul(ft0_128, ft1_128, &env->fp_status);
        if (unlikely(float128_is_infinity(ft0_128) && float64_is_infinity(farg3.d) &&
                     float128_is_neg(ft0_128) == float64_is_neg(farg3.d))) {
            /* Magnitude subtraction of infinities */
            farg1.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXISI);
        } else {
            ft1_128 = float64_to_float128(farg3.d, &env->fp_status);
            ft0_128 = float128_sub(ft0_128, ft1_128, &env->fp_status);
            farg1.d = float128_to_float64(ft0_128, &env->fp_status);
        }
#else
        /* This is OK on x86 hosts */
        farg1.d = (farg1.d * farg2.d) - farg3.d;
#endif
#else
        farg1.d = float64_mul(farg1.d, farg2.d, &env->fp_status);
        farg1.d = float64_sub(farg1.d, farg3.d, &env->fp_status);
#endif
        if (likely(!float64_is_nan(farg1.d)))
            farg1.d = float64_chs(farg1.d);
    }
    return farg1.ll;
}

/* frsp - frsp. */
uint64_t helper_frsp (uint64_t arg)
{
    CPU_DoubleU farg;
    float32 f32;
    farg.ll = arg;

#if USE_PRECISE_EMULATION
    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN square root */
       farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else {
       f32 = float64_to_float32(farg.d, &env->fp_status);
       farg.d = float32_to_float64(f32, &env->fp_status);
    }
#else
    f32 = float64_to_float32(farg.d, &env->fp_status);
    farg.d = float32_to_float64(f32, &env->fp_status);
#endif
    return farg.ll;
}

/* fsqrt - fsqrt. */
uint64_t helper_fsqrt (uint64_t arg)
{
    CPU_DoubleU farg;
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN square root */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely(float64_is_neg(farg.d) && !float64_is_zero(farg.d))) {
        /* Square root of a negative nonzero number */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSQRT);
    } else {
        farg.d = float64_sqrt(farg.d, &env->fp_status);
    }
    return farg.ll;
}

/* fre - fre. */
uint64_t helper_fre (uint64_t arg)
{
    CPU_DoubleU fone, farg;
    fone.ll = 0x3FF0000000000000ULL; /* 1.0 */
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN reciprocal */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else {
        farg.d = float64_div(fone.d, farg.d, &env->fp_status);
    }
    return farg.d;
}

/* fres - fres. */
uint64_t helper_fres (uint64_t arg)
{
    CPU_DoubleU fone, farg;
    float32 f32;
    fone.ll = 0x3FF0000000000000ULL; /* 1.0 */
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN reciprocal */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else {
        farg.d = float64_div(fone.d, farg.d, &env->fp_status);
        f32 = float64_to_float32(farg.d, &env->fp_status);
        farg.d = float32_to_float64(f32, &env->fp_status);
    }
    return farg.ll;
}

/* frsqrte  - frsqrte. */
uint64_t helper_frsqrte (uint64_t arg)
{
    CPU_DoubleU fone, farg;
    float32 f32;
    fone.ll = 0x3FF0000000000000ULL; /* 1.0 */
    farg.ll = arg;

    if (unlikely(float64_is_signaling_nan(farg.d))) {
        /* sNaN reciprocal square root */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    } else if (unlikely(float64_is_neg(farg.d) && !float64_is_zero(farg.d))) {
        /* Reciprocal square root of a negative nonzero number */
        farg.ll = fload_invalid_op_excp(POWERPC_EXCP_FP_VXSQRT);
    } else {
        farg.d = float64_sqrt(farg.d, &env->fp_status);
        farg.d = float64_div(fone.d, farg.d, &env->fp_status);
        f32 = float64_to_float32(farg.d, &env->fp_status);
        farg.d = float32_to_float64(f32, &env->fp_status);
    }
    return farg.ll;
}

/* fsel - fsel. */
uint64_t helper_fsel (uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    CPU_DoubleU farg1;

    farg1.ll = arg1;

    if (!float64_is_neg(farg1.d) || float64_is_zero(farg1.d))
        return arg2;
    else
        return arg3;
}

void helper_fcmpu (uint64_t arg1, uint64_t arg2, uint32_t crfD)
{
    CPU_DoubleU farg1, farg2;
    uint32_t ret = 0;
    farg1.ll = arg1;
    farg2.ll = arg2;

    if (unlikely(float64_is_nan(farg1.d) ||
                 float64_is_nan(farg2.d))) {
        ret = 0x01UL;
    } else if (float64_lt(farg1.d, farg2.d, &env->fp_status)) {
        ret = 0x08UL;
    } else if (!float64_le(farg1.d, farg2.d, &env->fp_status)) {
        ret = 0x04UL;
    } else {
        ret = 0x02UL;
    }

    env->fpscr &= ~(0x0F << FPSCR_FPRF);
    env->fpscr |= ret << FPSCR_FPRF;
    env->crf[crfD] = ret;
    if (unlikely(ret == 0x01UL
                 && (float64_is_signaling_nan(farg1.d) ||
                     float64_is_signaling_nan(farg2.d)))) {
        /* sNaN comparison */
        fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN);
    }
}

void helper_fcmpo (uint64_t arg1, uint64_t arg2, uint32_t crfD)
{
    CPU_DoubleU farg1, farg2;
    uint32_t ret = 0;
    farg1.ll = arg1;
    farg2.ll = arg2;

    if (unlikely(float64_is_nan(farg1.d) ||
                 float64_is_nan(farg2.d))) {
        ret = 0x01UL;
    } else if (float64_lt(farg1.d, farg2.d, &env->fp_status)) {
        ret = 0x08UL;
    } else if (!float64_le(farg1.d, farg2.d, &env->fp_status)) {
        ret = 0x04UL;
    } else {
        ret = 0x02UL;
    }

    env->fpscr &= ~(0x0F << FPSCR_FPRF);
    env->fpscr |= ret << FPSCR_FPRF;
    env->crf[crfD] = ret;
    if (unlikely (ret == 0x01UL)) {
        if (float64_is_signaling_nan(farg1.d) ||
            float64_is_signaling_nan(farg2.d)) {
            /* sNaN comparison */
            fload_invalid_op_excp(POWERPC_EXCP_FP_VXSNAN |
                                  POWERPC_EXCP_FP_VXVC);
        } else {
            /* qNaN comparison */
            fload_invalid_op_excp(POWERPC_EXCP_FP_VXVC);
        }
    }
}

#if !defined (CONFIG_USER_ONLY)
void helper_store_msr (target_ulong val)
{
    val = hreg_store_msr(env, val, 0);
    if (val != 0) {
        env->interrupt_request |= CPU_INTERRUPT_EXITTB;
        helper_raise_exception(val);
    }
}

static always_inline void do_rfi (target_ulong nip, target_ulong msr,
                                    target_ulong msrm, int keep_msrh)
{
#if defined(TARGET_PPC64)
    if (msr & (1ULL << MSR_SF)) {
        nip = (uint64_t)nip;
        msr &= (uint64_t)msrm;
    } else {
        nip = (uint32_t)nip;
        msr = (uint32_t)(msr & msrm);
        if (keep_msrh)
            msr |= env->msr & ~((uint64_t)0xFFFFFFFF);
    }
#else
    nip = (uint32_t)nip;
    msr &= (uint32_t)msrm;
#endif
    /* XXX: beware: this is false if VLE is supported */
    env->nip = nip & ~((target_ulong)0x00000003);
    hreg_store_msr(env, msr, 1);
#if defined (DEBUG_OP)
    cpu_dump_rfi(env->nip, env->msr);
#endif
    /* No need to raise an exception here,
     * as rfi is always the last insn of a TB
     */
    env->interrupt_request |= CPU_INTERRUPT_EXITTB;
}

void helper_rfi (void)
{
    do_rfi(env->spr[SPR_SRR0], env->spr[SPR_SRR1],
           ~((target_ulong)0xFFFF0000), 1);
}

#if defined(TARGET_PPC64)
void helper_rfid (void)
{
    do_rfi(env->spr[SPR_SRR0], env->spr[SPR_SRR1],
           ~((target_ulong)0xFFFF0000), 0);
}

void helper_hrfid (void)
{
    do_rfi(env->spr[SPR_HSRR0], env->spr[SPR_HSRR1],
           ~((target_ulong)0xFFFF0000), 0);
}
#endif
#endif

void helper_tw (target_ulong arg1, target_ulong arg2, uint32_t flags)
{
    if (!likely(!(((int32_t)arg1 < (int32_t)arg2 && (flags & 0x10)) ||
                  ((int32_t)arg1 > (int32_t)arg2 && (flags & 0x08)) ||
                  ((int32_t)arg1 == (int32_t)arg2 && (flags & 0x04)) ||
                  ((uint32_t)arg1 < (uint32_t)arg2 && (flags & 0x02)) ||
                  ((uint32_t)arg1 > (uint32_t)arg2 && (flags & 0x01))))) {
        helper_raise_exception_err(POWERPC_EXCP_PROGRAM, POWERPC_EXCP_TRAP);
    }
}

#if defined(TARGET_PPC64)
void helper_td (target_ulong arg1, target_ulong arg2, uint32_t flags)
{
    if (!likely(!(((int64_t)arg1 < (int64_t)arg2 && (flags & 0x10)) ||
                  ((int64_t)arg1 > (int64_t)arg2 && (flags & 0x08)) ||
                  ((int64_t)arg1 == (int64_t)arg2 && (flags & 0x04)) ||
                  ((uint64_t)arg1 < (uint64_t)arg2 && (flags & 0x02)) ||
                  ((uint64_t)arg1 > (uint64_t)arg2 && (flags & 0x01)))))
        helper_raise_exception_err(POWERPC_EXCP_PROGRAM, POWERPC_EXCP_TRAP);
}
#endif

/*****************************************************************************/
/* PowerPC 601 specific instructions (POWER bridge) */

target_ulong helper_clcs (uint32_t arg)
{
    switch (arg) {
    case 0x0CUL:
        /* Instruction cache line size */
        return env->icache_line_size;
        break;
    case 0x0DUL:
        /* Data cache line size */
        return env->dcache_line_size;
        break;
    case 0x0EUL:
        /* Minimum cache line size */
        return (env->icache_line_size < env->dcache_line_size) ?
                env->icache_line_size : env->dcache_line_size;
        break;
    case 0x0FUL:
        /* Maximum cache line size */
        return (env->icache_line_size > env->dcache_line_size) ?
                env->icache_line_size : env->dcache_line_size;
        break;
    default:
        /* Undefined */
        return 0;
        break;
    }
}

target_ulong helper_div (target_ulong arg1, target_ulong arg2)
{
    uint64_t tmp = (uint64_t)arg1 << 32 | env->spr[SPR_MQ];

    if (((int32_t)tmp == INT32_MIN && (int32_t)arg2 == (int32_t)-1) ||
        (int32_t)arg2 == 0) {
        env->spr[SPR_MQ] = 0;
        return INT32_MIN;
    } else {
        env->spr[SPR_MQ] = tmp % arg2;
        return  tmp / (int32_t)arg2;
    }
}

target_ulong helper_divo (target_ulong arg1, target_ulong arg2)
{
    uint64_t tmp = (uint64_t)arg1 << 32 | env->spr[SPR_MQ];

    if (((int32_t)tmp == INT32_MIN && (int32_t)arg2 == (int32_t)-1) ||
        (int32_t)arg2 == 0) {
        env->xer |= (1 << XER_OV) | (1 << XER_SO);
        env->spr[SPR_MQ] = 0;
        return INT32_MIN;
    } else {
        env->spr[SPR_MQ] = tmp % arg2;
        tmp /= (int32_t)arg2;
	if ((int32_t)tmp != tmp) {
            env->xer |= (1 << XER_OV) | (1 << XER_SO);
        } else {
            env->xer &= ~(1 << XER_OV);
        }
        return tmp;
    }
}

target_ulong helper_divs (target_ulong arg1, target_ulong arg2)
{
    if (((int32_t)arg1 == INT32_MIN && (int32_t)arg2 == (int32_t)-1) ||
        (int32_t)arg2 == 0) {
        env->spr[SPR_MQ] = 0;
        return INT32_MIN;
    } else {
        env->spr[SPR_MQ] = (int32_t)arg1 % (int32_t)arg2;
        return (int32_t)arg1 / (int32_t)arg2;
    }
}

target_ulong helper_divso (target_ulong arg1, target_ulong arg2)
{
    if (((int32_t)arg1 == INT32_MIN && (int32_t)arg2 == (int32_t)-1) ||
        (int32_t)arg2 == 0) {
        env->xer |= (1 << XER_OV) | (1 << XER_SO);
        env->spr[SPR_MQ] = 0;
        return INT32_MIN;
    } else {
        env->xer &= ~(1 << XER_OV);
        env->spr[SPR_MQ] = (int32_t)arg1 % (int32_t)arg2;
        return (int32_t)arg1 / (int32_t)arg2;
    }
}

#if !defined (CONFIG_USER_ONLY)
target_ulong helper_rac (target_ulong addr)
{
    mmu_ctx_t ctx;
    int nb_BATs;
    target_ulong ret = 0;

    /* We don't have to generate many instances of this instruction,
     * as rac is supervisor only.
     */
    /* XXX: FIX THIS: Pretend we have no BAT */
    nb_BATs = env->nb_BATs;
    env->nb_BATs = 0;
    if (get_physical_address(env, &ctx, addr, 0, ACCESS_INT) == 0)
        ret = ctx.raddr;
    env->nb_BATs = nb_BATs;
    return ret;
}

void helper_rfsvc (void)
{
    do_rfi(env->lr, env->ctr, 0x0000FFFF, 0);
}
#endif

/*****************************************************************************/
/* 602 specific instructions */
/* mfrom is the most crazy instruction ever seen, imho ! */
/* Real implementation uses a ROM table. Do the same */
/* Extremly decomposed:
 *                      -arg / 256
 * return 256 * log10(10           + 1.0) + 0.5
 */
#if !defined (CONFIG_USER_ONLY)
target_ulong helper_602_mfrom (target_ulong arg)
{
    if (likely(arg < 602)) {
#include "mfrom_table.c"
        return mfrom_ROM_table[arg];
    } else {
        return 0;
    }
}
#endif

/*****************************************************************************/
/* Embedded PowerPC specific helpers */

/* XXX: to be improved to check access rights when in user-mode */
target_ulong helper_load_dcr (target_ulong dcrn)
{
    target_ulong val = 0;

    if (unlikely(env->dcr_env == NULL)) {
        if (loglevel != 0) {
            fprintf(logfile, "No DCR environment\n");
        }
        helper_raise_exception_err(POWERPC_EXCP_PROGRAM,
                                   POWERPC_EXCP_INVAL | POWERPC_EXCP_INVAL_INVAL);
    } else if (unlikely(ppc_dcr_read(env->dcr_env, dcrn, &val) != 0)) {
        if (loglevel != 0) {
            fprintf(logfile, "DCR read error %d %03x\n", (int)dcrn, (int)dcrn);
        }
        helper_raise_exception_err(POWERPC_EXCP_PROGRAM,
                                   POWERPC_EXCP_INVAL | POWERPC_EXCP_PRIV_REG);
    }
    return val;
}

void helper_store_dcr (target_ulong dcrn, target_ulong val)
{
    if (unlikely(env->dcr_env == NULL)) {
        if (loglevel != 0) {
            fprintf(logfile, "No DCR environment\n");
        }
        helper_raise_exception_err(POWERPC_EXCP_PROGRAM,
                                   POWERPC_EXCP_INVAL | POWERPC_EXCP_INVAL_INVAL);
    } else if (unlikely(ppc_dcr_write(env->dcr_env, dcrn, val) != 0)) {
        if (loglevel != 0) {
            fprintf(logfile, "DCR write error %d %03x\n", (int)dcrn, (int)dcrn);
        }
        helper_raise_exception_err(POWERPC_EXCP_PROGRAM,
                                   POWERPC_EXCP_INVAL | POWERPC_EXCP_PRIV_REG);
    }
}

#if !defined(CONFIG_USER_ONLY)
void helper_40x_rfci (void)
{
    do_rfi(env->spr[SPR_40x_SRR2], env->spr[SPR_40x_SRR3],
           ~((target_ulong)0xFFFF0000), 0);
}

void helper_rfci (void)
{
    do_rfi(env->spr[SPR_BOOKE_CSRR0], SPR_BOOKE_CSRR1,
           ~((target_ulong)0x3FFF0000), 0);
}

void helper_rfdi (void)
{
    do_rfi(env->spr[SPR_BOOKE_DSRR0], SPR_BOOKE_DSRR1,
           ~((target_ulong)0x3FFF0000), 0);
}

void helper_rfmci (void)
{
    do_rfi(env->spr[SPR_BOOKE_MCSRR0], SPR_BOOKE_MCSRR1,
           ~((target_ulong)0x3FFF0000), 0);
}
#endif

/* 440 specific */
target_ulong helper_dlmzb (target_ulong high, target_ulong low, uint32_t update_Rc)
{
    target_ulong mask;
    int i;

    i = 1;
    for (mask = 0xFF000000; mask != 0; mask = mask >> 8) {
        if ((high & mask) == 0) {
            if (update_Rc) {
                env->crf[0] = 0x4;
            }
            goto done;
        }
        i++;
    }
    for (mask = 0xFF000000; mask != 0; mask = mask >> 8) {
        if ((low & mask) == 0) {
            if (update_Rc) {
                env->crf[0] = 0x8;
            }
            goto done;
        }
        i++;
    }
    if (update_Rc) {
        env->crf[0] = 0x2;
    }
 done:
    env->xer = (env->xer & ~0x7F) | i;
    if (update_Rc) {
        env->crf[0] |= xer_so;
    }
    return i;
}

/* Altivec extension helpers.  */
/* FIXME: Crufty Altivec stuff that should probably be rewritten/removed.  */
#if 0
#if defined(WORDS_BIGENDIAN)
#define HI_IDX 0
#define LO_IDX 1
#else
#define HI_IDX 1
#define LO_IDX 0
#endif

#define N_ELEMS(element) (sizeof (AVR0.element) / sizeof (AVR0.element[0]))

#define VECTOR_FOR(element)                     \
  int i;                                        \
  VECTOR_FOR_I(i, element)

#define VECTOR_FOR_I(index, element)                                    \
  for (index = 0; index < N_ELEMS(element); index++)

#if defined(WORDS_BIGENDIAN)
#define VECTOR_FOR_INORDER_I(index, element) VECTOR_FOR_I(index, element)
#else
#define VECTOR_FOR_INORDER_I(index, element)            \
  for (index = N_ELEMS(element)-1; index >= 0; index--)
#endif

/* Saturating arithmetic helpers.  */
#define SATCVT(from, to, from_type, to_type, min, max, use_min, use_max) \
  static always_inline to_type cvt##from##to (from_type x, int *sat)    \
  {                                                                     \
    to_type r;                                                          \
    if (use_min && x < min) {                                           \
      r = min;                                                          \
      *sat = 1;                                                         \
    } else if (use_max && x > max) {                                    \
      r = max;                                                          \
      *sat = 1;                                                         \
    } else {                                                            \
      r = x;                                                            \
    }                                                                   \
    return r;                                                           \
  }
SATCVT(sh, sb, int16_t, int8_t, INT8_MIN, INT8_MAX, 1, 1)
SATCVT(sw, sh, int32_t, int16_t, INT16_MIN, INT16_MAX, 1, 1)
SATCVT(sd, sw, int64_t, int32_t, INT32_MIN, INT32_MAX, 1, 1)
SATCVT(uh, ub, uint16_t, uint8_t, 0, UINT8_MAX, 0, 1)
SATCVT(uw, uh, uint32_t, uint16_t, 0, UINT16_MAX, 0, 1)
SATCVT(ud, uw, uint64_t, uint32_t, 0, UINT32_MAX, 0, 1)
SATCVT(sh, ub, int16_t, uint8_t, 0, UINT8_MAX, 1, 1)
SATCVT(sw, uh, int32_t, uint16_t, 0, UINT16_MAX, 1, 1)
SATCVT(sd, uw, int64_t, uint32_t, 0, UINT32_MAX, 1, 1)
#undef SATCVT

void do_lvsl (void)
{
  int sh = (uint32_t)T0 & 0xf;
  int i, j = sh;

  VECTOR_FOR_INORDER_I (i, u8) {
    AVR0.u8[i] = j++;
  }
}

void do_lvsr (void)
{
  int sh = (uint32_t)T0 & 0xf;
  int i, j = 0x10-sh;

  VECTOR_FOR_INORDER_I (i, u8) {
    AVR0.u8[i] = j++;
  }
}

void do_vaddcuw (void)
{
  VECTOR_FOR(u32) {
    AVR0.u32[i] = ~AVR0.u32[i] < AVR1.u32[i];
  }
}

#define VARITH_DO(name, op, element)            \
  void do_v##name (void)                        \
  {                                             \
    VECTOR_FOR (element) {                                              \
      AVR0.element[i] = AVR0.element[i] op AVR1.element[i];             \
    }                                                                   \
  }
#define VARITH(suffix, element)                 \
  VARITH_DO(add##suffix, +, element)             \
  VARITH_DO(sub##suffix, -, element)
VARITH(fp, f)
VARITH(ubm, u8)
VARITH(uhm, u16)
VARITH(uwm, u32)
#undef VARITH_DO
#undef VARITH

#define VARITHSAT_CASE(type, op, min, max, use_min, use_max, element) \
  {                                                                     \
    type result = (type)AVR0.element[i] op (type)AVR1.element[i];       \
    if (use_min && result < min) {                                      \
      result = min;                                                     \
      sat = 1;                                                          \
    } else if (use_max && result > max) {                               \
      result = max;                                                     \
      sat = 1;                                                          \
    }                                                                   \
    AVR0.element[i] = result;                                           \
  }

#define VARITHSAT_DO(name, op, min, max, use_min, use_max, element)     \
  void do_v##name (void)                                        \
  {                                                             \
    int sat = 0;                                                \
    VECTOR_FOR (element) {                                      \
      switch (sizeof(AVR0.element[0])) {                        \
      case 1: VARITHSAT_CASE(int16_t, op, min, max, use_min, use_max, element); break; \
      case 2: VARITHSAT_CASE(int32_t, op, min, max, use_min, use_max, element); break; \
      case 4: VARITHSAT_CASE(int64_t, op, min, max, use_min, use_max, element); break; \
      }                                                         \
    }                                                           \
    if (sat) {                                                  \
      env->vscr |= (1 << VSCR_SAT);                             \
    }                                                           \
  }
#define VARITHSAT_SIGNED(suffix, element, min, max)             \
  VARITHSAT_DO(adds##suffix##s, +, min, max, 1, 1, element)     \
  VARITHSAT_DO(subs##suffix##s, -, min, max, 1, 1, element)
#define VARITHSAT_UNSIGNED(suffix, element, max)                 \
  VARITHSAT_DO(addu##suffix##s, +, 0, max, 0, 1, element)        \
  VARITHSAT_DO(subu##suffix##s, -, 0, max, 1, 0, element)
VARITHSAT_SIGNED(b, s8, INT8_MIN, INT8_MAX)
VARITHSAT_SIGNED(h, s16, INT16_MIN, INT16_MAX)
VARITHSAT_SIGNED(w, s32, INT32_MIN, INT32_MAX)
VARITHSAT_UNSIGNED(b, u8, UINT8_MAX)
VARITHSAT_UNSIGNED(h, u16, UINT16_MAX)
VARITHSAT_UNSIGNED(w, u32, UINT32_MAX)
#undef VARITHSAT_CASE
#undef VARITHSAT_DO
#undef VARITHSAT_SIGNED
#undef VARITHSAT_UNSIGNED

#define VAVG_CASE(signedp, element, signed_type, unsigned_type)         \
  if (signedp) {                                                        \
    signed_type x = (signed_type)AVR0.element[i] + (signed_type)AVR1.element[i] + 1; \
    AVR0.element[i] = x >> 1;                                           \
  } else {                                                              \
    unsigned_type x = (unsigned_type)AVR0.element[i] + (unsigned_type)AVR1.element[i] + 1; \
    AVR0.element[i] = x >> 1;                                           \
  }

#define VAVG_DO(name, signedp, element)                                 \
  void do_v##name (void)                                                \
  {                                                                     \
    VECTOR_FOR (element) {                                              \
      switch (sizeof (AVR0.element[0])) {                               \
      case 1: VAVG_CASE(signedp, element, int16_t, uint16_t); break;    \
      case 2: VAVG_CASE(signedp, element, int32_t, uint32_t); break;    \
      case 4: VAVG_CASE(signedp, element, int64_t, uint64_t); break;    \
      }                                                                 \
    }                                                                   \
  }

#define VAVG(type, signed_element, unsigned_element)    \
  VAVG_DO(avgs##type, 1, signed_element)                \
  VAVG_DO(avgu##type, 0, unsigned_element)
VAVG(b, s8, u8)
VAVG(h, s16, u16)
VAVG(w, s32, u32)
#undef VAVG_CASE
#undef VAVG_DO
#undef VAVG

void do_vcmpbfp (void)
{
  int all_in = 0;
  VECTOR_FOR (f) {
    int le = AVR0.f[i] <= AVR1.f[i];
    int ge = AVR0.f[i] >= -AVR1.f[i];
    AVR0.u32[i] = ((!le) << 31) | ((!ge) << 30);
    all_in |= (!le | !ge);
  }
  T0 = (all_in == 0) << 1;
}

void do_vcfsx (void)
{
  uint32_t div = 1 << (uint32_t)T0;
  VECTOR_FOR (f) {
    AVR0.f[i] = (float)AVR1.s32[i] / div;
  }
}

void do_vcfux (void)
{
  uint32_t div = 1 << (uint32_t)T0;
  VECTOR_FOR (f) {
    AVR0.f[i] = (float)AVR1.u32[i] / div;
  }
}

#define VCMP(suffix, compare, element)                                  \
  void do_vcmp##suffix (void)                                           \
  {                                                                     \
    uint32_t ones = (sizeof (AVR0.element[0]) == 4                      \
                     ? 0xffffffff                                       \
                     : (sizeof (AVR0.element[0]) == 2                   \
                        ? 0xffff                                        \
                        : 0xff));                                       \
    uint32_t all = ones;                                                \
    uint32_t none = 0;                                                  \
    VECTOR_FOR (element) {                                              \
      uint32_t result = (AVR0.element[i] compare AVR1.element[i] ? ones : 0x0); \
      switch (sizeof (AVR0.element[0])) {                               \
      case 4: AVR0.u32[i] = result; break;                              \
      case 2: AVR0.u16[i] = result; break;                              \
      case 1: AVR0.u8[i] = result; break;                               \
      }                                                                 \
      all &= result;                                                    \
      none |= result;                                                   \
    }                                                                   \
    T0 = ((all != 0) << 3) | ((none == 0) << 1);                        \
  }
VCMP(eqfp, ==, f)
VCMP(gefp, >=, f)
VCMP(gtfp, >, f)
VCMP(equb, ==, u8)
VCMP(equh, ==, u16)
VCMP(equw, ==, u32)
VCMP(gtub, >, u8)
VCMP(gtuh, >, u16)
VCMP(gtuw, >, u32)
VCMP(gtsb, >, s8)
VCMP(gtsh, >, s16)
VCMP(gtsw, >, s32)
#undef VCMP

#define VCT(suffix, element, cvt, min, max)     \
  void do_vct##suffix (void)                    \
  {                                             \
    uint32_t uimm = (uint32_t)T0 & 0x1f;                        \
    int sat = 0;                                                \
    VECTOR_FOR (f) {                                            \
      uint32_t fi = AVR1.u32[i];                                \
      int exponent = (fi >> 23) & 0xff;                         \
      if (exponent == 255 || (exponent + uimm) <= 254) {        \
        double prod = ldexp((double)AVR1.f[i], (int)uimm);      \
        int64_t x = (int64_t)prod;                              \
        AVR0.element[i] = cvt(x, &sat);                         \
      } else {                                                  \
        if (fi >> 31) {                                         \
          AVR0.element[i] = min;                                \
        } else {                                                \
          AVR0.element[i] = max;                                \
        }                                                       \
        sat = 1;                                                \
      }                                                         \
    }                                                           \
    if (sat) {                                                  \
      env->vscr |= (1 << VSCR_SAT);                             \
    }                                                           \
  }
VCT(sxs, s32, cvtsdsw, INT32_MIN, INT32_MAX)
VCT(uxs, u32, cvtsduw, 0, UINT32_MAX)
#undef VCT

void do_vexptefp (void)
{
  /* FIXME: need to properly handle special inputs */
  VECTOR_FOR (f) {
    AVR0.f[i] = powf((float)2.0, AVR1.f[i]);
  }
}

void do_vlogefp (void)
{
  VECTOR_FOR (f) {
    AVR0.f[i] = logf(AVR1.f[i])/logf((float)2.0);
  }
}

void do_vmaddfp (void)
{
  VECTOR_FOR (f) {
    AVR0.f[i] = AVR0.f[i] * AVR2.f[i] + AVR1.f[i];
  }
}

#define VMINMAX_DO(name, compare, element)                              \
  void do_v##name (void)                                                \
  {                                                                     \
    VECTOR_FOR (element) {                                              \
      if (AVR0.element[i] compare AVR1.element[i]) {                    \
        AVR0.element[i] = AVR1.element[i];                              \
      }                                                                 \
    }                                                                   \
  }
#define VMINMAX(suffix, element)                \
  VMINMAX_DO(min##suffix, >, element)           \
  VMINMAX_DO(max##suffix, <, element)
VMINMAX(fp, f)
VMINMAX(sb, s8)
VMINMAX(sh, s16)
VMINMAX(sw, s32)
VMINMAX(ub, u8)
VMINMAX(uh, u16)
VMINMAX(uw, u32)
#undef VMINMAX_DO
#undef VMINMAX

void do_vmhaddshs (void)
{
  int sat = 0;

  VECTOR_FOR (s16) {
    int32_t prod = AVR0.s16[i] * AVR1.s16[i];
    int32_t t = (int32_t)AVR2.s16[i] + (prod >> 15);
    AVR0.s16[i] = cvtswsh (t, &sat);
  }

  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

void do_vmhraddshs (void)
{
  int sat = 0;

  VECTOR_FOR (s16) {
    int32_t prod = AVR0.s16[i] * AVR1.s16[i] + 0x00004000;
    int32_t t = (int32_t)AVR2.s16[i] + (prod >> 15);
    AVR0.s16[i] = cvtswsh (t, &sat);
  }

  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

void do_vmladduhm (void)
{
  VECTOR_FOR (s16) {
    int32_t prod = AVR0.s16[i] * AVR1.s16[i];
    AVR0.s16[i] = (int16_t) (prod + AVR2.s16[i]);
  }
}

#define VMRG_DO(name, element, highp)           \
  void do_v##name (void)                        \
  {                                             \
    ppc_avr_t result;                           \
    int i;                                      \
    size_t n_elems = N_ELEMS(element);                                  \
    for (i = 0; i < n_elems/2; i++) {                                   \
      if (highp) {                                                      \
        result.element[i*2+HI_IDX] = AVR0.element[i];                   \
        result.element[i*2+LO_IDX] = AVR1.element[i];                   \
      } else {                                                          \
        result.element[n_elems - i*2 - (1+HI_IDX)] = AVR1.element[n_elems - i - 1]; \
        result.element[n_elems - i*2 - (1+LO_IDX)] = AVR0.element[n_elems - i - 1]; \
      }                                                                 \
    }                                                                   \
    AVR0 = result;                                                      \
  }
#if defined(WORDS_BIGENDIAN)
#define MRGHI 0
#define MRGL0 1
#else
#define MRGHI 1
#define MRGLO 0
#endif
#define VMRG(suffix, element)                   \
  VMRG_DO(mrgl##suffix, element, MRGHI)         \
  VMRG_DO(mrgh##suffix, element, MRGLO)
VMRG(b, u8)
VMRG(h, u16)
VMRG(w, u32)
#undef VMRG_DO
#undef VMRG

void do_vmsummbm (void)
{
  int32_t prod[16];
  int i;

  VECTOR_FOR_I(i, s8) {
    prod[i] = (int32_t)AVR0.s8[i] * AVR1.u8[i];
  }

  VECTOR_FOR_INORDER_I(i, s32) {
    AVR0.s32[i] = AVR2.s32[i] + prod[4*i] + prod[4*i+1] + prod[4*i+2] + prod[4*i+3];
  }
}

void do_vmsumshm (void)
{
  int32_t prod[8];
  int i;

  VECTOR_FOR_I(i, s16) {
    prod[i] = AVR0.s16[i] * AVR1.s16[i];
  }

  VECTOR_FOR_INORDER_I(i, s32) {
    AVR0.s32[i] = AVR2.s32[i] + prod[2*i] + prod[2*i+1];
  }
}

void do_vmsumshs (void)
{
  int32_t prod[8];
  int i;
  int sat = 0;

  VECTOR_FOR_I (i, s16) {
    prod[i] = (int32_t)AVR0.s16[i] * AVR1.s16[i];
  }

  VECTOR_FOR_INORDER_I (i, s32) {
    int64_t t = (int64_t)AVR2.s32[i] + prod[2*i] + prod[2*i+1];
    AVR0.u32[i] = cvtsdsw(t, &sat);
  }

  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

void do_vmsumubm (void)
{
  uint16_t prod[16];
  int i;

  VECTOR_FOR_I(i, u8) {
    prod[i] = AVR0.u8[i] * AVR1.u8[i];
  }

  VECTOR_FOR_INORDER_I(i, u32) {
    AVR0.u32[i] = AVR2.u32[i] + prod[4*i] + prod[4*i+1] + prod[4*i+2] + prod[4*i+3];
  }
}

void do_vmsumuhm (void)
{
  uint32_t prod[8];
  int i;

  VECTOR_FOR_I(i, u16) {
    prod[i] = AVR0.u16[i] * AVR1.u16[i];
  }

  VECTOR_FOR_INORDER_I(i, u32) {
    AVR0.u32[i] = AVR2.u32[i] + prod[2*i] + prod[2*i+1];
  }
}

void do_vmsumuhs (void)
{
  uint32_t prod[8];
  int i;
  int sat = 0;

  VECTOR_FOR_I (i, u16) {
    prod[i] = AVR0.u16[i] * AVR1.u16[i];
  }

  VECTOR_FOR_INORDER_I (i, s32) {
    uint64_t t = (uint64_t)AVR2.u32[i] + prod[2*i] + prod[2*i+1];
    AVR0.u32[i] = cvtuduw(t, &sat);
  }

  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

#define VMUL_DO(name, mul_element, prod_element, evenp) \
  void do_v##name (void)                                \
  {                                                     \
    int i;                                              \
    VECTOR_FOR_INORDER_I(i, prod_element) {                             \
      if (evenp) {                                                      \
        AVR0.prod_element[i] = AVR0.mul_element[i*2+HI_IDX] * AVR1.mul_element[i*2+HI_IDX]; \
      } else {                                                          \
        AVR0.prod_element[i] = AVR0.mul_element[i*2+LO_IDX] * AVR1.mul_element[i*2+LO_IDX]; \
      }                                                                 \
    }                                                                   \
  }
#define VMUL(suffix, mul_element, prod_element) \
  VMUL_DO(mule##suffix, mul_element, prod_element, 1) \
  VMUL_DO(mulo##suffix, mul_element, prod_element, 0)
VMUL(sb, s8, s16)
VMUL(sh, s16, s32)
VMUL(ub, u8, u16)
VMUL(uh, u16, u32)
#undef VMUL_DO
#undef VMUL

void do_vnmsubfp (void)
{
  VECTOR_FOR (f) {
    AVR0.f[i] = -(AVR0.f[i] * AVR2.f[i] - AVR1.f[i]);
  }
}

void do_vperm (void)
{
  ppc_avr_t result;
  int i;
  VECTOR_FOR_INORDER_I (i, u8) {
    int s = AVR2.u8[i] & 0x1f;
#if defined(WORDS_BIGENDIAN)
    int index = s & 0xf;
#else
    int index = 15 - (s & 0xf);
#endif
    if (s & 0x10) {
      result.u8[i] = AVR1.u8[index];
    } else {
      result.u8[i] = AVR0.u8[index];
    }
  }
  AVR0 = result;
}

#if defined(WORDS_BIGENDIAN)
#define PKBIG 1
#else
#define PKBIG 0
#endif
void do_vpkpx (void)
{
  int i, j;
  ppc_avr_t result;
#if defined(WORDS_BIGENDIAN)
  ppc_avr_t x[2] = { AVR0, AVR1 };
#else
  ppc_avr_t x[2] = { AVR1, AVR0 };
#endif

  VECTOR_FOR_INORDER_I (i, u64) {
    VECTOR_FOR_INORDER_I (j, u32){
      uint32_t e = x[i].u32[j];
      result.u16[4*i+j] = ((e >> 9) & 0xfc00) | ((e >> 6) & 0x3e0) | ((e >> 3) & 0x1f);
    }
  }
  AVR0 = result;
}

#define VPK(suffix, from, to, cvt, dosat)       \
  void do_vpk##suffix (void)                    \
  {                                             \
    int i;                                      \
    int sat = 0;                                \
    ppc_avr_t result;                           \
    ppc_avr_t *a0 = PKBIG ? &AVR0 : &AVR1;      \
    ppc_avr_t *a1 = PKBIG ? &AVR1 : &AVR0;      \
    VECTOR_FOR_INORDER_I (i, from) {            \
      result.to[i] = cvt(a0->from[i], &sat);                \
      result.to[i+N_ELEMS(from)] = cvt(a1->from[i], &sat);  \
    }                                                   \
    AVR0 = result;                                      \
    if (dosat && sat) {                                 \
      env->vscr |= (1 << VSCR_SAT);                     \
    }                                                   \
  }
#define I(x, y) (x)
VPK(shss, s16, s8, cvtshsb, 1)
VPK(shus, s16, u8, cvtshub, 1)
VPK(swss, s32, s16, cvtswsh, 1)
VPK(swus, s32, u16, cvtswuh, 1)
VPK(uhus, u16, u8, cvtuhub, 1)
VPK(uwus, u32, u16, cvtuwuh, 1)
VPK(uhum, u16, u8, I, 0)
VPK(uwum, u32, u16, I, 0)
#undef I
#undef VPK
#undef PKBIG

void do_vrefp (void)
{
  VECTOR_FOR (f) {
    AVR0.f[i] = 1/AVR1.f[i];
  }
}

#define VRFI(suffix, func)                      \
  void do_vrfi##suffix (void)                   \
  {                                             \
    VECTOR_FOR (f) {                            \
      AVR0.f[i] = func (AVR1.f[i]);             \
    }                                           \
  }
VRFI(m, floorf)
VRFI(n, rintf)
VRFI(p, ceilf)
VRFI(z, truncf)
#undef VRFI

#define VROTATE(suffix, element)                \
  void do_vrl##suffix (void)                    \
  {                                             \
    VECTOR_FOR (element) {                      \
      unsigned int mask = ((1 << (3 + (sizeof (AVR0.element[0]) >> 1))) - 1); \
      unsigned int shift = AVR1.element[i] & mask;                      \
      AVR0.element[i] = (AVR0.element[i] << shift) | (AVR0.element[i] >> (sizeof(AVR0.element[0]) * 8 - shift)); \
    }                                                                   \
  }
VROTATE(b, u8)
VROTATE(h, u16)
VROTATE(w, u32)
#undef VROTATE

void do_vrsqrtefp (void)
{
  VECTOR_FOR (f) {
    AVR0.f[i] = 1/sqrtf(AVR1.f[i]);
  }
}

#if defined(WORDS_BIGENDIAN)
#define LEFT 0
#define RIGHT 1
#else
#define LEFT 1
#define RIGHT 0
#endif
#define VSHIFT(suffix, leftp)                   \
  void do_vs##suffix (void)                     \
  {                                             \
    int shift = AVR1.u8[LO_IDX*0x15] & 0x7;     \
    int doit = 1;                               \
    VECTOR_FOR (u8) {                           \
      doit = doit && ((AVR1.u8[i] & 0x7) == shift);     \
    }                                           \
    if (doit) {                                 \
      if (shift == 0) {                         \
        return;                                 \
      } else if (leftp) {                                               \
        uint64_t carry = AVR0.u64[LO_IDX] >> (64 - shift);              \
        AVR0.u64[HI_IDX] = (AVR0.u64[HI_IDX] << shift) | carry;         \
        AVR0.u64[LO_IDX] <<= shift;                                     \
      } else {                                                          \
        uint64_t carry = AVR0.u64[HI_IDX] << (64 - shift);              \
        AVR0.u64[LO_IDX] = (AVR0.u64[LO_IDX] >> shift) | carry;         \
        AVR0.u64[HI_IDX] >>= shift;                                     \
      }                                                                 \
    }                                                                   \
  }
VSHIFT(l, LEFT)
VSHIFT(r, RIGHT)
#undef VSHIFT
#undef LEFT
#undef RIGHT

#define VSL(suffix, element)                    \
  void do_vsl##suffix (void)                    \
  {                                             \
    VECTOR_FOR (element) {                      \
      unsigned int mask = ((1 << (3 + (sizeof (AVR0.element[0]) >> 1))) - 1); \
      unsigned int shift = AVR1.element[i] & mask;                      \
      AVR0.element[i] = AVR0.element[i] << shift;                       \
    }                                                                   \
  }
VSL(b, u8)
VSL(h, u16)
VSL(w, u32)
#undef VSL

void do_vsldoi (void)
{
  int sh = (int)T0 & 0xf;
  int i;
  ppc_avr_t result;

#if defined(WORDS_BIGENDIAN)
  VECTOR_FOR_I (i, u8) {
    int index = sh + i;
    if (index > 0xf) {
      result.u8[i] = AVR1.u8[index-0x10];
    } else {
      result.u8[i] = AVR0.u8[index];
    }
  }
#else
  VECTOR_FOR_I (i, u8) {
    int index = (16 - sh) + i;
    if (index > 0xf) {
      result.u8[i] = AVR0.u8[index-0x10];
    } else {
      result.u8[i] = AVR1.u8[index];
    }
  }
#endif
  AVR0 = result;
}

void do_vslo (void)
{
  int sh = (AVR1.u8[LO_IDX*0xf] >> 3) & 0xf;

#if defined (WORDS_BIGENDIAN)
  memmove (&AVR0.u8[0], &AVR0.u8[sh], 0x10-sh);
  memset (&AVR0.u8[16-sh], 0, sh);
#else
  memmove (&AVR0.u8[sh], &AVR0.u8[0], 0x10-sh);
  memset (&AVR0.u8[0], 0, sh);
#endif
}

/* Experimental testing shows that hardware masks the immediate.  */
#define _SPLAT_MASKED(element) ((uint32_t)T0 & (N_ELEMS(element) - 1))
#if defined(WORDS_BIGENDIAN)
#define SPLAT_ELEMENT(element) _SPLAT_MASKED(element)
#else
#define SPLAT_ELEMENT(element) (N_ELEMS(element)-1 - _SPLAT_MASKED(element))
#endif
#define VSPLT(suffix, element)                          \
  void do_vsplt##suffix (void)                          \
  {                                                     \
    uint32_t s = AVR1.element[SPLAT_ELEMENT(element)];  \
    VECTOR_FOR (element) {                              \
      AVR0.element[i] = s;                              \
    }                                                   \
  }
VSPLT(b, u8)
VSPLT(h, u16)
VSPLT(w, u32)
#undef VSPLT
#undef SPLAT_ELEMENT
#undef _SPLAT_MASKED

#define VSPLTI(suffix, element, splat_type)     \
  void do_vspltis##suffix (void)                \
  {                                             \
    splat_type x = (splat_type)T0;              \
    /* 5-bit sign extension.  */                \
    if (x & 0x10)                               \
      x -= 0x20;                                \
    VECTOR_FOR (element) {                      \
      AVR0.element[i] = x;                      \
    }                                           \
  }
VSPLTI(b, s8, int8_t)
VSPLTI(h, s16, int16_t)
VSPLTI(w, s32, int32_t)
#undef VSPLTI

#define VSR(suffix, element)                    \
  void do_vsr##suffix (void)                    \
  {                                             \
    VECTOR_FOR (element) {                      \
      unsigned int mask = ((1 << (3 + (sizeof (AVR0.element[0]) >> 1))) - 1); \
      unsigned int shift = AVR1.element[i] & mask;                      \
      AVR0.element[i] = AVR0.element[i] >> shift;                       \
    }                                                                   \
  }
VSR(ab, s8)
VSR(ah, s16)
VSR(aw, s32)
VSR(b, u8)
VSR(h, u16)
VSR(w, u32)
#undef VSR

void do_vsro (void)
{
  int sh = (AVR1.u8[LO_IDX*0xf] >> 3) & 0xf;

#if defined (WORDS_BIGENDIAN)
  memmove (&AVR0.u8[sh], &AVR0.u8[0], 0x10-sh);
  memset (&AVR0.u8[0], 0, sh);
#else
  memmove (&AVR0.u8[0], &AVR0.u8[sh], 0x10-sh);
  memset (&AVR0.u8[0x10-sh], 0, sh);
#endif
}

void do_vsubcuw (void)
{
  VECTOR_FOR(u32) {
    AVR0.u32[i] = AVR0.u32[i] >= AVR1.u32[i];
  }
}

void do_vsumsws (void)
{
  int64_t t;
  int i, upper;
  ppc_avr_t result;
  int sat = 0;

#if defined(WORDS_BIGENDIAN)
  upper = N_ELEMS(s32)-1;
#else
  upper = 0;
#endif
  t = (int64_t)AVR1.s32[upper];
  VECTOR_FOR_I (i, s32) {
    t += AVR0.s32[i];
    result.s32[i] = 0;
  }
  result.s32[upper] = cvtsdsw(t, &sat);
  AVR0 = result;

  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

void do_vsum2sws (void)
{
  int i, j, upper;
  ppc_avr_t result;
  int sat = 0;

#if defined(WORDS_BIGENDIAN)
  upper = 1;
#else
  upper = 0;
#endif
  VECTOR_FOR_I (i, u64) {
    int64_t t = (int64_t)AVR1.s32[upper+i*2];
    result.u64[i] = 0;
    VECTOR_FOR_I (j, u64) {
      t += AVR0.s32[2*i+j];
    }
    result.s32[upper+i*2] = cvtsdsw(t, &sat);
  }

  AVR0 = result;
  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

void do_vsum4sbs (void)
{
  int i, j;
  int sat = 0;

  VECTOR_FOR_I (i, s32) {
    int64_t t = (int64_t)AVR1.s32[i];
    VECTOR_FOR_I (j, s32) {
      t += AVR0.s8[4*i+j];
    }
    AVR0.s32[i] = cvtsdsw(t, &sat);
  }

  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

void do_vsum4shs (void)
{
  int sat = 0;

  VECTOR_FOR (s32) {
    int64_t t = (int64_t)AVR1.s32[i];
    t += AVR0.s16[2*i] + AVR0.s16[2*i+1];
    AVR0.s32[i] = cvtsdsw(t, &sat);
  }

  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

void do_vsum4ubs (void)
{
  int i, j;
  int sat = 0;

  VECTOR_FOR_I (i, u32) {
    uint64_t t = (uint64_t)AVR1.u32[i];
    VECTOR_FOR_I (j, u32) {
      t += AVR0.u8[4*i+j];
    }
    AVR0.u32[i] = cvtuduw(t, &sat);
  }

  if (sat) {
    env->vscr |= (1 << VSCR_SAT);
  }
}

#if defined(WORDS_BIGENDIAN)
#define UPKHI 1
#define UPKLO 0
#else
#define UPKHI 0
#define UPKLO 1
#endif
#define VUPKPX(suffix, hi)                      \
  void do_vupk##suffix (void)                   \
  {                                             \
    int i;                                      \
    VECTOR_FOR_I (i, u32) {                     \
      uint16_t e = AVR1.u16[hi ? i : i+4];      \
      uint8_t a = (e >> 15) ? 0xff : 0;         \
      uint8_t r = (e >> 10) & 0x1f;             \
      uint8_t g = (e >> 5) & 0x1f;                              \
      uint8_t b = e & 0x1f;                                     \
      AVR0.u32[i] = (a << 24) | (r << 16) | (g << 8) | b;       \
    }                                                           \
  }
VUPKPX(lpx, UPKLO)
VUPKPX(hpx, UPKHI)

#define VUPK(suffix, unpacked, packee, hi)      \
  void do_vupk##suffix (void)                   \
  {                                             \
    int i;                                      \
    ppc_avr_t result;                                                   \
    if (hi) {                                                           \
      for (i = 0; i < N_ELEMS(unpacked); i++) {                         \
        result.unpacked[i] = AVR1.packee[i];                            \
      }                                                                 \
    } else {                                                            \
      for (i = N_ELEMS(unpacked); i < N_ELEMS(packee); i++) {           \
        result.unpacked[i-N_ELEMS(unpacked)] = AVR1.packee[i];          \
      }                                                                 \
    }                                                                   \
    AVR0 = result;                                                      \
  }
VUPK(hsb, s16, s8, UPKHI)
VUPK(hsh, s32, s16, UPKHI)
VUPK(lsb, s16, s8, UPKLO)
VUPK(lsh, s32, s16, UPKLO)
#undef VUPK
#undef UPKHI
#undef UPKLO

#undef VECTOR_FOR
#undef VECTOR_FOR_I
#undef VECTOR_FOR_INORDER_I
#undef HI_IDX
#undef LO_IDX
#endif

/*****************************************************************************/
/* SPE extension helpers */
/* Use a table to make this quicker */
static uint8_t hbrev[16] = {
    0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
    0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF,
};

static always_inline uint8_t byte_reverse (uint8_t val)
{
    return hbrev[val >> 4] | (hbrev[val & 0xF] << 4);
}

static always_inline uint32_t word_reverse (uint32_t val)
{
    return byte_reverse(val >> 24) | (byte_reverse(val >> 16) << 8) |
        (byte_reverse(val >> 8) << 16) | (byte_reverse(val) << 24);
}

#define MASKBITS 16 // Random value - to be fixed (implementation dependant)
target_ulong helper_brinc (target_ulong arg1, target_ulong arg2)
{
    uint32_t a, b, d, mask;

    mask = UINT32_MAX >> (32 - MASKBITS);
    a = arg1 & mask;
    b = arg2 & mask;
    d = word_reverse(1 + word_reverse(a | ~b));
    return (arg1 & ~mask) | (d & b);
}

uint32_t helper_cntlsw32 (uint32_t val)
{
    if (val & 0x80000000)
        return clz32(~val);
    else
        return clz32(val);
}

uint32_t helper_cntlzw32 (uint32_t val)
{
    return clz32(val);
}

/* Single-precision floating-point conversions */
static always_inline uint32_t efscfsi (uint32_t val)
{
    CPU_FloatU u;

    u.f = int32_to_float32(val, &env->spe_status);

    return u.l;
}

static always_inline uint32_t efscfui (uint32_t val)
{
    CPU_FloatU u;

    u.f = uint32_to_float32(val, &env->spe_status);

    return u.l;
}

static always_inline int32_t efsctsi (uint32_t val)
{
    CPU_FloatU u;

    u.l = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float32_is_nan(u.f)))
        return 0;

    return float32_to_int32(u.f, &env->spe_status);
}

static always_inline uint32_t efsctui (uint32_t val)
{
    CPU_FloatU u;

    u.l = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float32_is_nan(u.f)))
        return 0;

    return float32_to_uint32(u.f, &env->spe_status);
}

static always_inline uint32_t efsctsiz (uint32_t val)
{
    CPU_FloatU u;

    u.l = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float32_is_nan(u.f)))
        return 0;

    return float32_to_int32_round_to_zero(u.f, &env->spe_status);
}

static always_inline uint32_t efsctuiz (uint32_t val)
{
    CPU_FloatU u;

    u.l = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float32_is_nan(u.f)))
        return 0;

    return float32_to_uint32_round_to_zero(u.f, &env->spe_status);
}

static always_inline uint32_t efscfsf (uint32_t val)
{
    CPU_FloatU u;
    float32 tmp;

    u.f = int32_to_float32(val, &env->spe_status);
    tmp = int64_to_float32(1ULL << 32, &env->spe_status);
    u.f = float32_div(u.f, tmp, &env->spe_status);

    return u.l;
}

static always_inline uint32_t efscfuf (uint32_t val)
{
    CPU_FloatU u;
    float32 tmp;

    u.f = uint32_to_float32(val, &env->spe_status);
    tmp = uint64_to_float32(1ULL << 32, &env->spe_status);
    u.f = float32_div(u.f, tmp, &env->spe_status);

    return u.l;
}

static always_inline uint32_t efsctsf (uint32_t val)
{
    CPU_FloatU u;
    float32 tmp;

    u.l = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float32_is_nan(u.f)))
        return 0;
    tmp = uint64_to_float32(1ULL << 32, &env->spe_status);
    u.f = float32_mul(u.f, tmp, &env->spe_status);

    return float32_to_int32(u.f, &env->spe_status);
}

static always_inline uint32_t efsctuf (uint32_t val)
{
    CPU_FloatU u;
    float32 tmp;

    u.l = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float32_is_nan(u.f)))
        return 0;
    tmp = uint64_to_float32(1ULL << 32, &env->spe_status);
    u.f = float32_mul(u.f, tmp, &env->spe_status);

    return float32_to_uint32(u.f, &env->spe_status);
}

#define HELPER_SPE_SINGLE_CONV(name)                                          \
uint32_t helper_e##name (uint32_t val)                                        \
{                                                                             \
    return e##name(val);                                                      \
}
/* efscfsi */
HELPER_SPE_SINGLE_CONV(fscfsi);
/* efscfui */
HELPER_SPE_SINGLE_CONV(fscfui);
/* efscfuf */
HELPER_SPE_SINGLE_CONV(fscfuf);
/* efscfsf */
HELPER_SPE_SINGLE_CONV(fscfsf);
/* efsctsi */
HELPER_SPE_SINGLE_CONV(fsctsi);
/* efsctui */
HELPER_SPE_SINGLE_CONV(fsctui);
/* efsctsiz */
HELPER_SPE_SINGLE_CONV(fsctsiz);
/* efsctuiz */
HELPER_SPE_SINGLE_CONV(fsctuiz);
/* efsctsf */
HELPER_SPE_SINGLE_CONV(fsctsf);
/* efsctuf */
HELPER_SPE_SINGLE_CONV(fsctuf);

#define HELPER_SPE_VECTOR_CONV(name)                                          \
uint64_t helper_ev##name (uint64_t val)                                       \
{                                                                             \
    return ((uint64_t)e##name(val >> 32) << 32) |                             \
            (uint64_t)e##name(val);                                           \
}
/* evfscfsi */
HELPER_SPE_VECTOR_CONV(fscfsi);
/* evfscfui */
HELPER_SPE_VECTOR_CONV(fscfui);
/* evfscfuf */
HELPER_SPE_VECTOR_CONV(fscfuf);
/* evfscfsf */
HELPER_SPE_VECTOR_CONV(fscfsf);
/* evfsctsi */
HELPER_SPE_VECTOR_CONV(fsctsi);
/* evfsctui */
HELPER_SPE_VECTOR_CONV(fsctui);
/* evfsctsiz */
HELPER_SPE_VECTOR_CONV(fsctsiz);
/* evfsctuiz */
HELPER_SPE_VECTOR_CONV(fsctuiz);
/* evfsctsf */
HELPER_SPE_VECTOR_CONV(fsctsf);
/* evfsctuf */
HELPER_SPE_VECTOR_CONV(fsctuf);

/* Single-precision floating-point arithmetic */
static always_inline uint32_t efsadd (uint32_t op1, uint32_t op2)
{
    CPU_FloatU u1, u2;
    u1.l = op1;
    u2.l = op2;
    u1.f = float32_add(u1.f, u2.f, &env->spe_status);
    return u1.l;
}

static always_inline uint32_t efssub (uint32_t op1, uint32_t op2)
{
    CPU_FloatU u1, u2;
    u1.l = op1;
    u2.l = op2;
    u1.f = float32_sub(u1.f, u2.f, &env->spe_status);
    return u1.l;
}

static always_inline uint32_t efsmul (uint32_t op1, uint32_t op2)
{
    CPU_FloatU u1, u2;
    u1.l = op1;
    u2.l = op2;
    u1.f = float32_mul(u1.f, u2.f, &env->spe_status);
    return u1.l;
}

static always_inline uint32_t efsdiv (uint32_t op1, uint32_t op2)
{
    CPU_FloatU u1, u2;
    u1.l = op1;
    u2.l = op2;
    u1.f = float32_div(u1.f, u2.f, &env->spe_status);
    return u1.l;
}

#define HELPER_SPE_SINGLE_ARITH(name)                                         \
uint32_t helper_e##name (uint32_t op1, uint32_t op2)                          \
{                                                                             \
    return e##name(op1, op2);                                                 \
}
/* efsadd */
HELPER_SPE_SINGLE_ARITH(fsadd);
/* efssub */
HELPER_SPE_SINGLE_ARITH(fssub);
/* efsmul */
HELPER_SPE_SINGLE_ARITH(fsmul);
/* efsdiv */
HELPER_SPE_SINGLE_ARITH(fsdiv);

#define HELPER_SPE_VECTOR_ARITH(name)                                         \
uint64_t helper_ev##name (uint64_t op1, uint64_t op2)                         \
{                                                                             \
    return ((uint64_t)e##name(op1 >> 32, op2 >> 32) << 32) |                  \
            (uint64_t)e##name(op1, op2);                                      \
}
/* evfsadd */
HELPER_SPE_VECTOR_ARITH(fsadd);
/* evfssub */
HELPER_SPE_VECTOR_ARITH(fssub);
/* evfsmul */
HELPER_SPE_VECTOR_ARITH(fsmul);
/* evfsdiv */
HELPER_SPE_VECTOR_ARITH(fsdiv);

/* Single-precision floating-point comparisons */
static always_inline uint32_t efststlt (uint32_t op1, uint32_t op2)
{
    CPU_FloatU u1, u2;
    u1.l = op1;
    u2.l = op2;
    return float32_lt(u1.f, u2.f, &env->spe_status) ? 4 : 0;
}

static always_inline uint32_t efststgt (uint32_t op1, uint32_t op2)
{
    CPU_FloatU u1, u2;
    u1.l = op1;
    u2.l = op2;
    return float32_le(u1.f, u2.f, &env->spe_status) ? 0 : 4;
}

static always_inline uint32_t efststeq (uint32_t op1, uint32_t op2)
{
    CPU_FloatU u1, u2;
    u1.l = op1;
    u2.l = op2;
    return float32_eq(u1.f, u2.f, &env->spe_status) ? 4 : 0;
}

static always_inline uint32_t efscmplt (uint32_t op1, uint32_t op2)
{
    /* XXX: TODO: test special values (NaN, infinites, ...) */
    return efststlt(op1, op2);
}

static always_inline uint32_t efscmpgt (uint32_t op1, uint32_t op2)
{
    /* XXX: TODO: test special values (NaN, infinites, ...) */
    return efststgt(op1, op2);
}

static always_inline uint32_t efscmpeq (uint32_t op1, uint32_t op2)
{
    /* XXX: TODO: test special values (NaN, infinites, ...) */
    return efststeq(op1, op2);
}

#define HELPER_SINGLE_SPE_CMP(name)                                           \
uint32_t helper_e##name (uint32_t op1, uint32_t op2)                          \
{                                                                             \
    return e##name(op1, op2) << 2;                                            \
}
/* efststlt */
HELPER_SINGLE_SPE_CMP(fststlt);
/* efststgt */
HELPER_SINGLE_SPE_CMP(fststgt);
/* efststeq */
HELPER_SINGLE_SPE_CMP(fststeq);
/* efscmplt */
HELPER_SINGLE_SPE_CMP(fscmplt);
/* efscmpgt */
HELPER_SINGLE_SPE_CMP(fscmpgt);
/* efscmpeq */
HELPER_SINGLE_SPE_CMP(fscmpeq);

static always_inline uint32_t evcmp_merge (int t0, int t1)
{
    return (t0 << 3) | (t1 << 2) | ((t0 | t1) << 1) | (t0 & t1);
}

#define HELPER_VECTOR_SPE_CMP(name)                                           \
uint32_t helper_ev##name (uint64_t op1, uint64_t op2)                         \
{                                                                             \
    return evcmp_merge(e##name(op1 >> 32, op2 >> 32), e##name(op1, op2));     \
}
/* evfststlt */
HELPER_VECTOR_SPE_CMP(fststlt);
/* evfststgt */
HELPER_VECTOR_SPE_CMP(fststgt);
/* evfststeq */
HELPER_VECTOR_SPE_CMP(fststeq);
/* evfscmplt */
HELPER_VECTOR_SPE_CMP(fscmplt);
/* evfscmpgt */
HELPER_VECTOR_SPE_CMP(fscmpgt);
/* evfscmpeq */
HELPER_VECTOR_SPE_CMP(fscmpeq);

/* Double-precision floating-point conversion */
uint64_t helper_efdcfsi (uint32_t val)
{
    CPU_DoubleU u;

    u.d = int32_to_float64(val, &env->spe_status);

    return u.ll;
}

uint64_t helper_efdcfsid (uint64_t val)
{
    CPU_DoubleU u;

    u.d = int64_to_float64(val, &env->spe_status);

    return u.ll;
}

uint64_t helper_efdcfui (uint32_t val)
{
    CPU_DoubleU u;

    u.d = uint32_to_float64(val, &env->spe_status);

    return u.ll;
}

uint64_t helper_efdcfuid (uint64_t val)
{
    CPU_DoubleU u;

    u.d = uint64_to_float64(val, &env->spe_status);

    return u.ll;
}

uint32_t helper_efdctsi (uint64_t val)
{
    CPU_DoubleU u;

    u.ll = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float64_is_nan(u.d)))
        return 0;

    return float64_to_int32(u.d, &env->spe_status);
}

uint32_t helper_efdctui (uint64_t val)
{
    CPU_DoubleU u;

    u.ll = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float64_is_nan(u.d)))
        return 0;

    return float64_to_uint32(u.d, &env->spe_status);
}

uint32_t helper_efdctsiz (uint64_t val)
{
    CPU_DoubleU u;

    u.ll = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float64_is_nan(u.d)))
        return 0;

    return float64_to_int32_round_to_zero(u.d, &env->spe_status);
}

uint64_t helper_efdctsidz (uint64_t val)
{
    CPU_DoubleU u;

    u.ll = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float64_is_nan(u.d)))
        return 0;

    return float64_to_int64_round_to_zero(u.d, &env->spe_status);
}

uint32_t helper_efdctuiz (uint64_t val)
{
    CPU_DoubleU u;

    u.ll = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float64_is_nan(u.d)))
        return 0;

    return float64_to_uint32_round_to_zero(u.d, &env->spe_status);
}

uint64_t helper_efdctuidz (uint64_t val)
{
    CPU_DoubleU u;

    u.ll = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float64_is_nan(u.d)))
        return 0;

    return float64_to_uint64_round_to_zero(u.d, &env->spe_status);
}

uint64_t helper_efdcfsf (uint32_t val)
{
    CPU_DoubleU u;
    float64 tmp;

    u.d = int32_to_float64(val, &env->spe_status);
    tmp = int64_to_float64(1ULL << 32, &env->spe_status);
    u.d = float64_div(u.d, tmp, &env->spe_status);

    return u.ll;
}

uint64_t helper_efdcfuf (uint32_t val)
{
    CPU_DoubleU u;
    float64 tmp;

    u.d = uint32_to_float64(val, &env->spe_status);
    tmp = int64_to_float64(1ULL << 32, &env->spe_status);
    u.d = float64_div(u.d, tmp, &env->spe_status);

    return u.ll;
}

uint32_t helper_efdctsf (uint64_t val)
{
    CPU_DoubleU u;
    float64 tmp;

    u.ll = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float64_is_nan(u.d)))
        return 0;
    tmp = uint64_to_float64(1ULL << 32, &env->spe_status);
    u.d = float64_mul(u.d, tmp, &env->spe_status);

    return float64_to_int32(u.d, &env->spe_status);
}

uint32_t helper_efdctuf (uint64_t val)
{
    CPU_DoubleU u;
    float64 tmp;

    u.ll = val;
    /* NaN are not treated the same way IEEE 754 does */
    if (unlikely(float64_is_nan(u.d)))
        return 0;
    tmp = uint64_to_float64(1ULL << 32, &env->spe_status);
    u.d = float64_mul(u.d, tmp, &env->spe_status);

    return float64_to_uint32(u.d, &env->spe_status);
}

uint32_t helper_efscfd (uint64_t val)
{
    CPU_DoubleU u1;
    CPU_FloatU u2;

    u1.ll = val;
    u2.f = float64_to_float32(u1.d, &env->spe_status);

    return u2.l;
}

uint64_t helper_efdcfs (uint32_t val)
{
    CPU_DoubleU u2;
    CPU_FloatU u1;

    u1.l = val;
    u2.d = float32_to_float64(u1.f, &env->spe_status);

    return u2.ll;
}

/* Double precision fixed-point arithmetic */
uint64_t helper_efdadd (uint64_t op1, uint64_t op2)
{
    CPU_DoubleU u1, u2;
    u1.ll = op1;
    u2.ll = op2;
    u1.d = float64_add(u1.d, u2.d, &env->spe_status);
    return u1.ll;
}

uint64_t helper_efdsub (uint64_t op1, uint64_t op2)
{
    CPU_DoubleU u1, u2;
    u1.ll = op1;
    u2.ll = op2;
    u1.d = float64_sub(u1.d, u2.d, &env->spe_status);
    return u1.ll;
}

uint64_t helper_efdmul (uint64_t op1, uint64_t op2)
{
    CPU_DoubleU u1, u2;
    u1.ll = op1;
    u2.ll = op2;
    u1.d = float64_mul(u1.d, u2.d, &env->spe_status);
    return u1.ll;
}

uint64_t helper_efddiv (uint64_t op1, uint64_t op2)
{
    CPU_DoubleU u1, u2;
    u1.ll = op1;
    u2.ll = op2;
    u1.d = float64_div(u1.d, u2.d, &env->spe_status);
    return u1.ll;
}

/* Double precision floating point helpers */
uint32_t helper_efdtstlt (uint64_t op1, uint64_t op2)
{
    CPU_DoubleU u1, u2;
    u1.ll = op1;
    u2.ll = op2;
    return float64_lt(u1.d, u2.d, &env->spe_status) ? 4 : 0;
}

uint32_t helper_efdtstgt (uint64_t op1, uint64_t op2)
{
    CPU_DoubleU u1, u2;
    u1.ll = op1;
    u2.ll = op2;
    return float64_le(u1.d, u2.d, &env->spe_status) ? 0 : 4;
}

uint32_t helper_efdtsteq (uint64_t op1, uint64_t op2)
{
    CPU_DoubleU u1, u2;
    u1.ll = op1;
    u2.ll = op2;
    return float64_eq(u1.d, u2.d, &env->spe_status) ? 4 : 0;
}

uint32_t helper_efdcmplt (uint64_t op1, uint64_t op2)
{
    /* XXX: TODO: test special values (NaN, infinites, ...) */
    return helper_efdtstlt(op1, op2);
}

uint32_t helper_efdcmpgt (uint64_t op1, uint64_t op2)
{
    /* XXX: TODO: test special values (NaN, infinites, ...) */
    return helper_efdtstgt(op1, op2);
}

uint32_t helper_efdcmpeq (uint64_t op1, uint64_t op2)
{
    /* XXX: TODO: test special values (NaN, infinites, ...) */
    return helper_efdtsteq(op1, op2);
}

/*****************************************************************************/
/* Softmmu support */
#if !defined (CONFIG_USER_ONLY)

#define MMUSUFFIX _mmu

#define SHIFT 0
#include "softmmu_template.h"

#define SHIFT 1
#include "softmmu_template.h"

#define SHIFT 2
#include "softmmu_template.h"

#define SHIFT 3
#include "softmmu_template.h"

/* try to fill the TLB and return an exception if error. If retaddr is
   NULL, it means that the function was called in C code (i.e. not
   from generated code or from helper.c) */
/* XXX: fix it to restore all registers */
void tlb_fill (target_ulong addr, int is_write, int mmu_idx, void *retaddr)
{
    TranslationBlock *tb;
    CPUState *saved_env;
    unsigned long pc;
    int ret;

    /* XXX: hack to restore env in all cases, even if not called from
       generated code */
    saved_env = env;
    env = cpu_single_env;
    ret = cpu_ppc_handle_mmu_fault(env, addr, is_write, mmu_idx, 1);
    if (unlikely(ret != 0)) {
        if (likely(retaddr)) {
            /* now we have a real cpu fault */
            pc = (unsigned long)retaddr;
            tb = tb_find_pc(pc);
            if (likely(tb)) {
                /* the PC is inside the translated code. It means that we have
                   a virtual CPU fault */
                cpu_restore_state(tb, env, pc, NULL);
            }
        }
        helper_raise_exception_err(env->exception_index, env->error_code);
    }
    env = saved_env;
}

/* Segment registers load and store */
target_ulong helper_load_sr (target_ulong sr_num)
{
    return env->sr[sr_num];
}

void helper_store_sr (target_ulong sr_num, target_ulong val)
{
    ppc_store_sr(env, sr_num, val);
}

/* SLB management */
#if defined(TARGET_PPC64)
target_ulong helper_load_slb (target_ulong slb_nr)
{
    return ppc_load_slb(env, slb_nr);
}

void helper_store_slb (target_ulong slb_nr, target_ulong rs)
{
    ppc_store_slb(env, slb_nr, rs);
}

void helper_slbia (void)
{
    ppc_slb_invalidate_all(env);
}

void helper_slbie (target_ulong addr)
{
    ppc_slb_invalidate_one(env, addr);
}

#endif /* defined(TARGET_PPC64) */

/* TLB management */
void helper_tlbia (void)
{
    ppc_tlb_invalidate_all(env);
}

void helper_tlbie (target_ulong addr)
{
    ppc_tlb_invalidate_one(env, addr);
}

/* Software driven TLBs management */
/* PowerPC 602/603 software TLB load instructions helpers */
static void do_6xx_tlb (target_ulong new_EPN, int is_code)
{
    target_ulong RPN, CMP, EPN;
    int way;

    RPN = env->spr[SPR_RPA];
    if (is_code) {
        CMP = env->spr[SPR_ICMP];
        EPN = env->spr[SPR_IMISS];
    } else {
        CMP = env->spr[SPR_DCMP];
        EPN = env->spr[SPR_DMISS];
    }
    way = (env->spr[SPR_SRR1] >> 17) & 1;
#if defined (DEBUG_SOFTWARE_TLB)
    if (loglevel != 0) {
        fprintf(logfile, "%s: EPN " ADDRX " " ADDRX " PTE0 " ADDRX
                " PTE1 " ADDRX " way %d\n",
                __func__, new_EPN, EPN, CMP, RPN, way);
    }
#endif
    /* Store this TLB */
    ppc6xx_tlb_store(env, (uint32_t)(new_EPN & TARGET_PAGE_MASK),
                     way, is_code, CMP, RPN);
}

void helper_6xx_tlbd (target_ulong EPN)
{
    do_6xx_tlb(EPN, 0);
}

void helper_6xx_tlbi (target_ulong EPN)
{
    do_6xx_tlb(EPN, 1);
}

/* PowerPC 74xx software TLB load instructions helpers */
static void do_74xx_tlb (target_ulong new_EPN, int is_code)
{
    target_ulong RPN, CMP, EPN;
    int way;

    RPN = env->spr[SPR_PTELO];
    CMP = env->spr[SPR_PTEHI];
    EPN = env->spr[SPR_TLBMISS] & ~0x3;
    way = env->spr[SPR_TLBMISS] & 0x3;
#if defined (DEBUG_SOFTWARE_TLB)
    if (loglevel != 0) {
        fprintf(logfile, "%s: EPN " ADDRX " " ADDRX " PTE0 " ADDRX
                " PTE1 " ADDRX " way %d\n",
                __func__, new_EPN, EPN, CMP, RPN, way);
    }
#endif
    /* Store this TLB */
    ppc6xx_tlb_store(env, (uint32_t)(new_EPN & TARGET_PAGE_MASK),
                     way, is_code, CMP, RPN);
}

void helper_74xx_tlbd (target_ulong EPN)
{
    do_74xx_tlb(EPN, 0);
}

void helper_74xx_tlbi (target_ulong EPN)
{
    do_74xx_tlb(EPN, 1);
}

static always_inline target_ulong booke_tlb_to_page_size (int size)
{
    return 1024 << (2 * size);
}

static always_inline int booke_page_size_to_tlb (target_ulong page_size)
{
    int size;

    switch (page_size) {
    case 0x00000400UL:
        size = 0x0;
        break;
    case 0x00001000UL:
        size = 0x1;
        break;
    case 0x00004000UL:
        size = 0x2;
        break;
    case 0x00010000UL:
        size = 0x3;
        break;
    case 0x00040000UL:
        size = 0x4;
        break;
    case 0x00100000UL:
        size = 0x5;
        break;
    case 0x00400000UL:
        size = 0x6;
        break;
    case 0x01000000UL:
        size = 0x7;
        break;
    case 0x04000000UL:
        size = 0x8;
        break;
    case 0x10000000UL:
        size = 0x9;
        break;
    case 0x40000000UL:
        size = 0xA;
        break;
#if defined (TARGET_PPC64)
    case 0x000100000000ULL:
        size = 0xB;
        break;
    case 0x000400000000ULL:
        size = 0xC;
        break;
    case 0x001000000000ULL:
        size = 0xD;
        break;
    case 0x004000000000ULL:
        size = 0xE;
        break;
    case 0x010000000000ULL:
        size = 0xF;
        break;
#endif
    default:
        size = -1;
        break;
    }

    return size;
}

/* Helpers for 4xx TLB management */
target_ulong helper_4xx_tlbre_lo (target_ulong entry)
{
    ppcemb_tlb_t *tlb;
    target_ulong ret;
    int size;

    entry &= 0x3F;
    tlb = &env->tlb[entry].tlbe;
    ret = tlb->EPN;
    if (tlb->prot & PAGE_VALID)
        ret |= 0x400;
    size = booke_page_size_to_tlb(tlb->size);
    if (size < 0 || size > 0x7)
        size = 1;
    ret |= size << 7;
    env->spr[SPR_40x_PID] = tlb->PID;
    return ret;
}

target_ulong helper_4xx_tlbre_hi (target_ulong entry)
{
    ppcemb_tlb_t *tlb;
    target_ulong ret;

    entry &= 0x3F;
    tlb = &env->tlb[entry].tlbe;
    ret = tlb->RPN;
    if (tlb->prot & PAGE_EXEC)
        ret |= 0x200;
    if (tlb->prot & PAGE_WRITE)
        ret |= 0x100;
    return ret;
}

void helper_4xx_tlbwe_hi (target_ulong entry, target_ulong val)
{
    ppcemb_tlb_t *tlb;
    target_ulong page, end;

#if defined (DEBUG_SOFTWARE_TLB)
    if (loglevel != 0) {
        fprintf(logfile, "%s entry %d val " ADDRX "\n", __func__, (int)entry, val);
    }
#endif
    entry &= 0x3F;
    tlb = &env->tlb[entry].tlbe;
    /* Invalidate previous TLB (if it's valid) */
    if (tlb->prot & PAGE_VALID) {
        end = tlb->EPN + tlb->size;
#if defined (DEBUG_SOFTWARE_TLB)
        if (loglevel != 0) {
            fprintf(logfile, "%s: invalidate old TLB %d start " ADDRX
                    " end " ADDRX "\n", __func__, (int)entry, tlb->EPN, end);
        }
#endif
        for (page = tlb->EPN; page < end; page += TARGET_PAGE_SIZE)
            tlb_flush_page(env, page);
    }
    tlb->size = booke_tlb_to_page_size((val >> 7) & 0x7);
    /* We cannot handle TLB size < TARGET_PAGE_SIZE.
     * If this ever occurs, one should use the ppcemb target instead
     * of the ppc or ppc64 one
     */
    if ((val & 0x40) && tlb->size < TARGET_PAGE_SIZE) {
        cpu_abort(env, "TLB size " TARGET_FMT_lu " < %u "
                  "are not supported (%d)\n",
                  tlb->size, TARGET_PAGE_SIZE, (int)((val >> 7) & 0x7));
    }
    tlb->EPN = val & ~(tlb->size - 1);
    if (val & 0x40)
        tlb->prot |= PAGE_VALID;
    else
        tlb->prot &= ~PAGE_VALID;
    if (val & 0x20) {
        /* XXX: TO BE FIXED */
        cpu_abort(env, "Little-endian TLB entries are not supported by now\n");
    }
    tlb->PID = env->spr[SPR_40x_PID]; /* PID */
    tlb->attr = val & 0xFF;
#if defined (DEBUG_SOFTWARE_TLB)
    if (loglevel != 0) {
        fprintf(logfile, "%s: set up TLB %d RPN " PADDRX " EPN " ADDRX
                " size " ADDRX " prot %c%c%c%c PID %d\n", __func__,
                (int)entry, tlb->RPN, tlb->EPN, tlb->size,
                tlb->prot & PAGE_READ ? 'r' : '-',
                tlb->prot & PAGE_WRITE ? 'w' : '-',
                tlb->prot & PAGE_EXEC ? 'x' : '-',
                tlb->prot & PAGE_VALID ? 'v' : '-', (int)tlb->PID);
    }
#endif
    /* Invalidate new TLB (if valid) */
    if (tlb->prot & PAGE_VALID) {
        end = tlb->EPN + tlb->size;
#if defined (DEBUG_SOFTWARE_TLB)
        if (loglevel != 0) {
            fprintf(logfile, "%s: invalidate TLB %d start " ADDRX
                    " end " ADDRX "\n", __func__, (int)entry, tlb->EPN, end);
        }
#endif
        for (page = tlb->EPN; page < end; page += TARGET_PAGE_SIZE)
            tlb_flush_page(env, page);
    }
}

void helper_4xx_tlbwe_lo (target_ulong entry, target_ulong val)
{
    ppcemb_tlb_t *tlb;

#if defined (DEBUG_SOFTWARE_TLB)
    if (loglevel != 0) {
        fprintf(logfile, "%s entry %i val " ADDRX "\n", __func__, (int)entry, val);
    }
#endif
    entry &= 0x3F;
    tlb = &env->tlb[entry].tlbe;
    tlb->RPN = val & 0xFFFFFC00;
    tlb->prot = PAGE_READ;
    if (val & 0x200)
        tlb->prot |= PAGE_EXEC;
    if (val & 0x100)
        tlb->prot |= PAGE_WRITE;
#if defined (DEBUG_SOFTWARE_TLB)
    if (loglevel != 0) {
        fprintf(logfile, "%s: set up TLB %d RPN " PADDRX " EPN " ADDRX
                " size " ADDRX " prot %c%c%c%c PID %d\n", __func__,
                (int)entry, tlb->RPN, tlb->EPN, tlb->size,
                tlb->prot & PAGE_READ ? 'r' : '-',
                tlb->prot & PAGE_WRITE ? 'w' : '-',
                tlb->prot & PAGE_EXEC ? 'x' : '-',
                tlb->prot & PAGE_VALID ? 'v' : '-', (int)tlb->PID);
    }
#endif
}

target_ulong helper_4xx_tlbsx (target_ulong address)
{
    return ppcemb_tlb_search(env, address, env->spr[SPR_40x_PID]);
}

/* PowerPC 440 TLB management */
void helper_440_tlbwe (uint32_t word, target_ulong entry, target_ulong value)
{
    ppcemb_tlb_t *tlb;
    target_ulong EPN, RPN, size;
    int do_flush_tlbs;

#if defined (DEBUG_SOFTWARE_TLB)
    if (loglevel != 0) {
        fprintf(logfile, "%s word %d entry %d value " ADDRX "\n",
                __func__, word, (int)entry, value);
    }
#endif
    do_flush_tlbs = 0;
    entry &= 0x3F;
    tlb = &env->tlb[entry].tlbe;
    switch (word) {
    default:
        /* Just here to please gcc */
    case 0:
        EPN = value & 0xFFFFFC00;
        if ((tlb->prot & PAGE_VALID) && EPN != tlb->EPN)
            do_flush_tlbs = 1;
        tlb->EPN = EPN;
        size = booke_tlb_to_page_size((value >> 4) & 0xF);
        if ((tlb->prot & PAGE_VALID) && tlb->size < size)
            do_flush_tlbs = 1;
        tlb->size = size;
        tlb->attr &= ~0x1;
        tlb->attr |= (value >> 8) & 1;
        if (value & 0x200) {
            tlb->prot |= PAGE_VALID;
        } else {
            if (tlb->prot & PAGE_VALID) {
                tlb->prot &= ~PAGE_VALID;
                do_flush_tlbs = 1;
            }
        }
        tlb->PID = env->spr[SPR_440_MMUCR] & 0x000000FF;
        if (do_flush_tlbs)
            tlb_flush(env, 1);
        break;
    case 1:
        RPN = value & 0xFFFFFC0F;
        if ((tlb->prot & PAGE_VALID) && tlb->RPN != RPN)
            tlb_flush(env, 1);
        tlb->RPN = RPN;
        break;
    case 2:
        tlb->attr = (tlb->attr & 0x1) | (value & 0x0000FF00);
        tlb->prot = tlb->prot & PAGE_VALID;
        if (value & 0x1)
            tlb->prot |= PAGE_READ << 4;
        if (value & 0x2)
            tlb->prot |= PAGE_WRITE << 4;
        if (value & 0x4)
            tlb->prot |= PAGE_EXEC << 4;
        if (value & 0x8)
            tlb->prot |= PAGE_READ;
        if (value & 0x10)
            tlb->prot |= PAGE_WRITE;
        if (value & 0x20)
            tlb->prot |= PAGE_EXEC;
        break;
    }
}

target_ulong helper_440_tlbre (uint32_t word, target_ulong entry)
{
    ppcemb_tlb_t *tlb;
    target_ulong ret;
    int size;

    entry &= 0x3F;
    tlb = &env->tlb[entry].tlbe;
    switch (word) {
    default:
        /* Just here to please gcc */
    case 0:
        ret = tlb->EPN;
        size = booke_page_size_to_tlb(tlb->size);
        if (size < 0 || size > 0xF)
            size = 1;
        ret |= size << 4;
        if (tlb->attr & 0x1)
            ret |= 0x100;
        if (tlb->prot & PAGE_VALID)
            ret |= 0x200;
        env->spr[SPR_440_MMUCR] &= ~0x000000FF;
        env->spr[SPR_440_MMUCR] |= tlb->PID;
        break;
    case 1:
        ret = tlb->RPN;
        break;
    case 2:
        ret = tlb->attr & ~0x1;
        if (tlb->prot & (PAGE_READ << 4))
            ret |= 0x1;
        if (tlb->prot & (PAGE_WRITE << 4))
            ret |= 0x2;
        if (tlb->prot & (PAGE_EXEC << 4))
            ret |= 0x4;
        if (tlb->prot & PAGE_READ)
            ret |= 0x8;
        if (tlb->prot & PAGE_WRITE)
            ret |= 0x10;
        if (tlb->prot & PAGE_EXEC)
            ret |= 0x20;
        break;
    }
    return ret;
}

target_ulong helper_440_tlbsx (target_ulong address)
{
    return ppcemb_tlb_search(env, address, env->spr[SPR_440_MMUCR] & 0xFF);
}

#endif /* !CONFIG_USER_ONLY */
