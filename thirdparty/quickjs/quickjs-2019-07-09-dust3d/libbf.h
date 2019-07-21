/*
 * Tiny arbitrary precision floating point library
 * 
 * Copyright (c) 2017-2018 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef LIBBF_H
#define LIBBF_H

#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__)
#define LIMB_LOG2_BITS 6
#else
#define LIMB_LOG2_BITS 5
#endif

#define LIMB_BITS (1 << LIMB_LOG2_BITS)

#if LIMB_BITS == 64
typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;
typedef int64_t slimb_t;
typedef uint64_t limb_t;
typedef uint128_t dlimb_t;
#define EXP_MIN INT64_MIN
#define EXP_MAX INT64_MAX

#else

typedef int32_t slimb_t;
typedef uint32_t limb_t;
typedef uint64_t dlimb_t;
#define EXP_MIN INT32_MIN
#define EXP_MAX INT32_MAX

#endif

/* in bits */
#define BF_EXP_BITS_MIN 3
#define BF_EXP_BITS_MAX (LIMB_BITS - 2)
#define BF_PREC_MIN 2
#define BF_PREC_MAX (((limb_t)1 << BF_EXP_BITS_MAX) - 2)
#define BF_PREC_INF (BF_PREC_MAX + 1) /* infinite precision */

#if LIMB_BITS == 64
#define BF_CHKSUM_MOD (UINT64_C(975620677) * UINT64_C(9795002197))
#else
#define BF_CHKSUM_MOD 975620677U
#endif

#define BF_EXP_ZERO EXP_MIN
#define BF_EXP_INF (EXP_MAX - 1)
#define BF_EXP_NAN EXP_MAX

/* +/-zero is represented with expn = BF_EXP_ZERO and len = 0,
   +/-infinity is represented with expn = BF_EXP_INF and len = 0,
   NaN is represented with expn = BF_EXP_NAN and len = 0 (sign is ignored)
 */
typedef struct {
    struct bf_context_t *ctx;
    int sign;
    slimb_t expn;
    limb_t len;
    limb_t *tab;
} bf_t;

typedef enum {
    BF_RNDN, /* round to nearest, ties to even */
    BF_RNDZ, /* round to zero */
    BF_RNDD, /* round to -inf */
    BF_RNDU, /* round to +inf */
    BF_RNDNA, /* round to nearest, ties away from zero */
    BF_RNDNU, /* round to nearest, ties to +inf */
    BF_RNDF, /* faithful rounding (nondeterministic, either RNDD or RNDU,
                inexact flag is always set)  */
} bf_rnd_t;

/* allow subnormal numbers (only available if the number of exponent
   bits is < BF_EXP_BITS_MAX and prec != BF_PREC_INF) */
#define BF_FLAG_SUBNORMAL (1 << 3)

#define BF_RND_MASK 0x7
#define BF_EXP_BITS_SHIFT 4
#define BF_EXP_BITS_MASK 0x3f

/* contains the rounding mode and number of exponents bits */
typedef uint32_t bf_flags_t;

typedef void *bf_realloc_func_t(void *opaque, void *ptr, size_t size);

typedef struct {
    bf_t val;
    limb_t prec;
} BFConstCache;

typedef struct bf_context_t {
    void *realloc_opaque;
    bf_realloc_func_t *realloc_func;
    BFConstCache log2_cache;
    BFConstCache pi_cache;
    struct BFNTTState *ntt_state;
} bf_context_t;

static inline int bf_get_exp_bits(bf_flags_t flags)
{
    return BF_EXP_BITS_MAX - ((flags >> BF_EXP_BITS_SHIFT) & BF_EXP_BITS_MASK);
}

static inline bf_flags_t bf_set_exp_bits(int n)
{
    return (BF_EXP_BITS_MAX - n) << BF_EXP_BITS_SHIFT;
}

/* returned status */
#define BF_ST_INVALID_OP  (1 << 0)
#define BF_ST_DIVIDE_ZERO (1 << 1)
#define BF_ST_OVERFLOW    (1 << 2)
#define BF_ST_UNDERFLOW   (1 << 3)
#define BF_ST_INEXACT     (1 << 4)
/* not used yet, indicate that a memory allocation error occured. NaN
   is returned */
#define BF_ST_MEM_ERROR   (1 << 5) 

#define BF_RADIX_MAX 36 /* maximum radix for bf_atof() and bf_ftoa() */

static inline slimb_t bf_max(slimb_t a, slimb_t b)
{
    if (a > b)
        return a;
    else
        return b;
}

static inline slimb_t bf_min(slimb_t a, slimb_t b)
{
    if (a < b)
        return a;
    else
        return b;
}

void bf_context_init(bf_context_t *s, bf_realloc_func_t *realloc_func,
                     void *realloc_opaque);
void bf_context_end(bf_context_t *s);
/* free memory allocated for the bf cache data */
void bf_clear_cache(bf_context_t *s);

static inline void *bf_realloc(bf_context_t *s, void *ptr, size_t size)
{
    return s->realloc_func(s->realloc_opaque, ptr, size);
}

void bf_init(bf_context_t *s, bf_t *r);

static inline void bf_delete(bf_t *r)
{
    bf_context_t *s = r->ctx;
    /* we accept to delete a zeroed bf_t structure */
    if (s) {
        bf_realloc(s, r->tab, 0);
    }
}

static inline void bf_neg(bf_t *r)
{
    r->sign ^= 1;
}

static inline int bf_is_finite(const bf_t *a)
{
    return (a->expn < BF_EXP_INF);
}

static inline int bf_is_nan(const bf_t *a)
{
    return (a->expn == BF_EXP_NAN);
}

static inline int bf_is_zero(const bf_t *a)
{
    return (a->expn == BF_EXP_ZERO);
}

void bf_set_ui(bf_t *r, uint64_t a);
void bf_set_si(bf_t *r, int64_t a);
void bf_set_nan(bf_t *r);
void bf_set_zero(bf_t *r, int is_neg);
void bf_set_inf(bf_t *r, int is_neg);
void bf_set(bf_t *r, const bf_t *a);
void bf_move(bf_t *r, bf_t *a);
int bf_get_float64(const bf_t *a, double *pres, bf_rnd_t rnd_mode);
void bf_set_float64(bf_t *a, double d);

int bf_cmpu(const bf_t *a, const bf_t *b);
int bf_cmp_full(const bf_t *a, const bf_t *b);
int bf_cmp_eq(const bf_t *a, const bf_t *b);
int bf_cmp_le(const bf_t *a, const bf_t *b);
int bf_cmp_lt(const bf_t *a, const bf_t *b);
int bf_add(bf_t *r, const bf_t *a, const bf_t *b, limb_t prec, bf_flags_t flags);
int bf_sub(bf_t *r, const bf_t *a, const bf_t *b, limb_t prec, bf_flags_t flags);
int bf_add_si(bf_t *r, const bf_t *a, int64_t b1, limb_t prec, bf_flags_t flags);
int bf_mul(bf_t *r, const bf_t *a, const bf_t *b, limb_t prec, bf_flags_t flags);
int bf_mul_ui(bf_t *r, const bf_t *a, uint64_t b1, limb_t prec, bf_flags_t flags);
int bf_mul_si(bf_t *r, const bf_t *a, int64_t b1, limb_t prec, 
              bf_flags_t flags);
int bf_mul_2exp(bf_t *r, slimb_t e, limb_t prec, bf_flags_t flags);
int bf_div(bf_t *r, const bf_t *a, const bf_t *b, limb_t prec, bf_flags_t flags);
#define BF_DIVREM_EUCLIDIAN BF_RNDF
int bf_divrem(bf_t *q, bf_t *r, const bf_t *a, const bf_t *b,
              limb_t prec, bf_flags_t flags, int rnd_mode);
int bf_fmod(bf_t *r, const bf_t *a, const bf_t *b, limb_t prec,
            bf_flags_t flags);
int bf_remainder(bf_t *r, const bf_t *a, const bf_t *b, limb_t prec,
                 bf_flags_t flags);
int bf_remquo(slimb_t *pq, bf_t *r, const bf_t *a, const bf_t *b, limb_t prec,
              bf_flags_t flags);
int bf_pow_ui(bf_t *r, const bf_t *a, limb_t b, limb_t prec,
              bf_flags_t flags);
int bf_pow_ui_ui(bf_t *r, limb_t a1, limb_t b, limb_t prec, bf_flags_t flags);
int bf_rint(bf_t *r, limb_t prec, bf_flags_t flags);
int bf_round(bf_t *r, limb_t prec, bf_flags_t flags);
int bf_sqrtrem(bf_t *r, bf_t *rem1, const bf_t *a);
int bf_sqrt(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);
slimb_t bf_get_exp_min(const bf_t *a);
void bf_logic_or(bf_t *r, const bf_t *a, const bf_t *b);
void bf_logic_xor(bf_t *r, const bf_t *a, const bf_t *b);
void bf_logic_and(bf_t *r, const bf_t *a, const bf_t *b);

/* additional flags for bf_atof */
/* do not accept hex radix prefix (0x or 0X) if radix = 0 or radix = 16 */
#define BF_ATOF_NO_HEX       (1 << 16)
/* accept binary (0b or 0B) or octal (0o or 0O) radix prefix if radix = 0 */
#define BF_ATOF_BIN_OCT      (1 << 17)
/* Only accept integers (no decimal point, no exponent, no infinity nor NaN */
#define BF_ATOF_INT_ONLY     (1 << 18)
/* Do not accept radix prefix after sign  */
#define BF_ATOF_NO_PREFIX_AFTER_SIGN (1 << 19)
/* Do not parse NaN and parse case sensitive 'Infinity' */
#define BF_ATOF_JS_QUIRKS    (1 << 20)
/* Do not round integers to the indicated precision */
#define BF_ATOF_INT_PREC_INF (1 << 21)
/* Support legacy octal syntax for well formed numbers */
#define BF_ATOF_LEGACY_OCTAL (1 << 22)
/* accept _ between digits as a digit separator */
#define BF_ATOF_UNDERSCORE_SEP (1 << 23)
/* if a 'n' suffix is present, force integer parsing (XXX: remove) */
#define BF_ATOF_INT_N_SUFFIX   (1 << 24)
/* if set return NaN if empty number string (instead of 0) */
#define BF_ATOF_NAN_IF_EMPTY   (1 << 25)
/* only accept decimal floating point if radix = 0 */
#define BF_ATOF_ONLY_DEC_FLOAT (1 << 26)

/* one more return flag: indicate that the parsed number is an integer
   (only set when the flags BF_ATOF_INT_PREC_INF or
   BF_ATOF_INT_N_SUFFIX are used) */
#define BF_ATOF_ST_INTEGER   (1 << 5)

int bf_atof(bf_t *a, const char *str, const char **pnext, int radix,
            limb_t prec, bf_flags_t flags);
/* this version accepts prec = BF_PREC_INF and returns the radix
   exponent */
int bf_atof2(bf_t *r, slimb_t *pexponent,
             const char *str, const char **pnext, int radix,
             limb_t prec, bf_flags_t flags);
int bf_mul_pow_radix(bf_t *r, const bf_t *T, limb_t radix,
                     slimb_t expn, limb_t prec, bf_flags_t flags);

#define BF_FTOA_FORMAT_MASK (3 << 16)
/* fixed format: prec significant digits rounded with (flags &
   BF_RND_MASK). Exponential notation is used if too many zeros are
   needed. */
#define BF_FTOA_FORMAT_FIXED (0 << 16)
/* fractional format: prec digits after the decimal point rounded with
   (flags & BF_RND_MASK) */
#define BF_FTOA_FORMAT_FRAC  (1 << 16)
/* free format: use as many digits as necessary so that bf_atof()
   return the same number when using precision 'prec', rounding to
   nearest and the subnormal+exponent configuration of 'flags'. The
   result is meaningful only if 'a' is already rounded to the wanted
   precision.
   
   Infinite precision (BF_PREC_INF) is supported when the radix is a
   power of two. */
#define BF_FTOA_FORMAT_FREE  (2 << 16)
/* same as BF_FTOA_FORMAT_FREE but uses the minimum number of digits
   (takes more computation time). */
#define BF_FTOA_FORMAT_FREE_MIN (3 << 16)

/* force exponential notation for fixed or free format */
#define BF_FTOA_FORCE_EXP    (1 << 20)
/* add 0x prefix for base 16, 0o prefix for base 8 or 0b prefix for
   base 2 if non zero value */
#define BF_FTOA_ADD_PREFIX   (1 << 21)
#define BF_FTOA_JS_QUIRKS    (1 << 22)

size_t bf_ftoa(char **pbuf, const bf_t *a, int radix, limb_t prec,
               bf_flags_t flags);

/* modulo 2^n instead of saturation. NaN and infinity return 0 */
#define BF_GET_INT_MOD (1 << 0) 
int bf_get_int32(int *pres, const bf_t *a, int flags);
int bf_get_int64(int64_t *pres, const bf_t *a, int flags);

/* the following functions are exported for testing only. */
void bf_print_str(const char *str, const bf_t *a);
void bf_resize(bf_t *r, limb_t len);
int bf_get_fft_size(int *pdpl, int *pnb_mods, limb_t len);
void bf_recip(bf_t *r, const bf_t *a, limb_t prec);
void bf_rsqrt(bf_t *a, const bf_t *x, limb_t prec);
int bf_normalize_and_round(bf_t *r, limb_t prec1, bf_flags_t flags);
int bf_can_round(const bf_t *a, slimb_t prec, bf_rnd_t rnd_mode, slimb_t k);
slimb_t bf_mul_log2_radix(slimb_t a1, unsigned int radix, int is_inv,
                          int is_ceil1);

/* transcendental functions */
int bf_const_log2(bf_t *T, limb_t prec, bf_flags_t flags);
int bf_const_pi(bf_t *T, limb_t prec, bf_flags_t flags);
int bf_exp(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);
int bf_log(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);
#define BF_POW_JS_QUICKS (1 << 16)
int bf_pow(bf_t *r, const bf_t *x, const bf_t *y, limb_t prec, bf_flags_t flags);
int bf_cos(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);
int bf_sin(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);
int bf_tan(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);
int bf_atan(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);
int bf_atan2(bf_t *r, const bf_t *y, const bf_t *x,
             limb_t prec, bf_flags_t flags);
int bf_asin(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);
int bf_acos(bf_t *r, const bf_t *a, limb_t prec, bf_flags_t flags);

#endif /* LIBBF_H */
