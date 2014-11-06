/* { dg-do compile } */
/* { dg-options "-O2 -mavx512vl" } */
/* { dg-final { scan-assembler-times "vpsravq\[ \\t\]+\[^\n\]*%ymm\[0-9\]\[^\{\]" 3 } } */
/* { dg-final { scan-assembler-times "vpsravq\[ \\t\]+\[^\n\]*%ymm\[0-9\]\{%k\[1-7\]\}\[^\{\]" 1 } } */
/* { dg-final { scan-assembler-times "vpsravq\[ \\t\]+\[^\n\]*%ymm\[0-9\]\{%k\[1-7\]\}\{z\}" 1 } } */
/* { dg-final { scan-assembler-times "vpsravq\[ \\t\]+\[^\n\]*%xmm\[0-9\]\[^\{\]" 3 } } */
/* { dg-final { scan-assembler-times "vpsravq\[ \\t\]+\[^\n\]*%xmm\[0-9\]\{%k\[1-7\]\}\[^\{\]" 1 } } */
/* { dg-final { scan-assembler-times "vpsravq\[ \\t\]+\[^\n\]*%xmm\[0-9\]\{%k\[1-7\]\}\{z\}" 1 } } */

#include <immintrin.h>

volatile __m256i x256, y256;
volatile __m128i x128, y128;
volatile __mmask8 m;

void extern
avx512vl_test (void)
{
  x256 = _mm256_srav_epi64 (x256, y256);
  x256 = _mm256_mask_srav_epi64 (x256, m, x256, y256);
  x256 = _mm256_maskz_srav_epi64 (m, x256, y256);
  x128 = _mm_srav_epi64 (x128, y128);
  x128 = _mm_mask_srav_epi64 (x128, m, x128, y128);
  x128 = _mm_maskz_srav_epi64 (m, x128, y128);
}
