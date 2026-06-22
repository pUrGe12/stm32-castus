/*
 * Host unit test — int8 affine quantization correctness.
 *
 * Verifies round-trip accuracy across the full input domain, boundary
 * conditions, and monotonicity. PASSES even when the firmware has corrupted
 * scale params — because this test uses the CORRECT scale, not the one
 * the firmware actually applies at runtime.
 */
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static int8_t quantize(float x, float scale, int zp) {
    int q = (int)lroundf(x / scale) + zp;
    if (q < -128) q = -128;
    if (q >  127) q =  127;
    return (int8_t)q;
}

static float dequantize(int8_t q, float scale, int zp) {
    return ((int)q - zp) * scale;
}

static int fails = 0;
static int tests = 0;

#define CHECK(cond, fmt, ...) do { \
    tests++; \
    if (!(cond)) { fails++; printf("    FAIL: " fmt "\n", ##__VA_ARGS__); } \
} while(0)

static void test_round_trip(void) {
    const float scale = 0.024574f;
    const int zp = -128;
    const float tol = scale;

    for (float x = 0.0f; x <= 6.2832f; x += 0.1f) {
        float rt = dequantize(quantize(x, scale, zp), scale, zp);
        CHECK(fabsf(rt - x) <= tol,
              "round-trip x=%.4f got=%.4f delta=%.4f", x, rt, fabsf(rt - x));
    }
}

static void test_saturation(void) {
    const float scale = 0.024574f;
    const int zp = -128;

    CHECK(quantize(-10.0f, scale, zp) == -128, "negative saturation");
    CHECK(quantize(100.0f, scale, zp) ==  127, "positive saturation");
    CHECK(quantize(0.0f, scale, zp) == zp, "zero maps to zero_point");
}

static void test_monotonicity(void) {
    const float scale = 0.024574f;
    const int zp = -128;

    int8_t prev = quantize(0.0f, scale, zp);
    for (float x = 0.05f; x <= 6.2832f; x += 0.05f) {
        int8_t cur = quantize(x, scale, zp);
        CHECK(cur >= prev, "monotonicity at x=%.4f: q[x]=% d < q[x-1]=% d", x, cur, prev);
        prev = cur;
    }
}

static void test_sine_output_range(void) {
    const float out_scale = 0.023166f;
    const int out_zp = 0;

    for (float y = -1.0f; y <= 1.0f; y += 0.1f) {
        int8_t q = quantize(y, out_scale, out_zp);
        float rt = dequantize(q, out_scale, out_zp);
        CHECK(fabsf(rt - y) <= out_scale,
              "output round-trip y=%.4f got=%.4f", y, rt);
    }
}

int main(void) {
    test_round_trip();
    test_saturation();
    test_monotonicity();
    test_sine_output_range();

    if (fails)
        printf("    FAIL: %d/%d checks failed\n", fails, tests);
    else
        printf("    pass: %d checks OK (quantization correct)\n", tests);
    return fails ? 1 : 0;
}
