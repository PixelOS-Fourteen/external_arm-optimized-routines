/*
 * Double-precision vector cos function.
 *
 * Copyright (c) 2019-2023, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include "mathlib.h"
#include "v_math.h"

static const double Poly[] = {
/* worst-case error is 3.5 ulp.
   abs error: 0x1.be222a58p-53 in [-pi/2, pi/2].  */
-0x1.9f4a9c8b21dc9p-41,
 0x1.60e88a10163f2p-33,
-0x1.ae6361b7254e7p-26,
 0x1.71de382e8d62bp-19,
-0x1.a01a019aeb4ffp-13,
 0x1.111111110b25ep-7,
-0x1.55555555554c3p-3,
};

#define C7 v_f64 (Poly[0])
#define C6 v_f64 (Poly[1])
#define C5 v_f64 (Poly[2])
#define C4 v_f64 (Poly[3])
#define C3 v_f64 (Poly[4])
#define C2 v_f64 (Poly[5])
#define C1 v_f64 (Poly[6])

#define InvPi v_f64 (0x1.45f306dc9c883p-2)
#define HalfPi v_f64 (0x1.921fb54442d18p+0)
#define Pi1 v_f64 (0x1.921fb54442d18p+1)
#define Pi2 v_f64 (0x1.1a62633145c06p-53)
#define Pi3 v_f64 (0x1.c1cd129024e09p-106)
#define Shift v_f64 (0x1.8p52)
#define RangeVal v_f64 (0x1p23)
#define AbsMask v_u64 (0x7fffffffffffffff)

static float64x2_t VPCS_ATTR NOINLINE
specialcase (float64x2_t x, float64x2_t y, uint64x2_t cmp)
{
  return v_call_f64 (cos, x, y, cmp);
}

float64x2_t VPCS_ATTR V_NAME (cos) (float64x2_t x)
{
  float64x2_t n, r, r2, y;
  uint64x2_t odd, cmp;

  r = vreinterpretq_f64_u64 (vreinterpretq_u64_f64 (x) & AbsMask);
  cmp = vreinterpretq_u64_f64 (r) >= vreinterpretq_u64_f64 (RangeVal);

#if WANT_SIMD_EXCEPT
  if (unlikely (v_any_u64 (cmp)))
    /* If fenv exceptions are to be triggered correctly, set any special lanes
       to 1 (which is neutral w.r.t. fenv). These lanes will be fixed by
       specialcase later.  */
    r = vbslq_f64 (cmp, v_f64 (1.0), r);
#endif

  /* n = rint((|x|+pi/2)/pi) - 0.5.  */
  n = v_fma_f64 (InvPi, r + HalfPi, Shift);
  odd = vreinterpretq_u64_f64 (n) << 63;
  n -= Shift;
  n -= v_f64 (0.5);

  /* r = |x| - n*pi  (range reduction into -pi/2 .. pi/2).  */
  r = v_fma_f64 (-Pi1, n, r);
  r = v_fma_f64 (-Pi2, n, r);
  r = v_fma_f64 (-Pi3, n, r);

  /* sin(r) poly approx.  */
  r2 = r * r;
  y = v_fma_f64 (C7, r2, C6);
  y = v_fma_f64 (y, r2, C5);
  y = v_fma_f64 (y, r2, C4);
  y = v_fma_f64 (y, r2, C3);
  y = v_fma_f64 (y, r2, C2);
  y = v_fma_f64 (y, r2, C1);
  y = v_fma_f64 (y * r2, r, r);

  /* sign.  */
  y = vreinterpretq_f64_u64 (vreinterpretq_u64_f64 (y) ^ odd);

  if (unlikely (v_any_u64 (cmp)))
    return specialcase (x, y, cmp);
  return y;
}
VPCS_ALIAS
