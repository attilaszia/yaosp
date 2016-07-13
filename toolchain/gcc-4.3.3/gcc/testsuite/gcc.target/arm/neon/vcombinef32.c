/* Test the `vcombinef32' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0 -mfpu=neon -mfloat-abi=softfp" } */

#include "arm_neon.h"

void test_vcombinef32 (void)
{
  float32x4_t out_float32x4_t;
  float32x2_t arg0_float32x2_t;
  float32x2_t arg1_float32x2_t;

  out_float32x4_t = vcombine_f32 (arg0_float32x2_t, arg1_float32x2_t);
}

/* { dg-final { cleanup-saved-temps } } */
