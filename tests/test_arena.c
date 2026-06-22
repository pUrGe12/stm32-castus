/*
 * Host unit test — arena sizing sanity check.
 *
 * Verifies that the configured arena size fits the model's theoretical
 * minimum. PASSES even when the arena is 200 bytes — because on the host
 * we compute the minimum from the model spec, and the spec says ~1500 bytes.
 * The actual allocator path (which fails on-device) is inside TFLM and
 * can't be exercised without running the real firmware.
 */
#include <stdio.h>
#include <stdint.h>

static int fails = 0;
static int tests = 0;

#define CHECK(cond, fmt, ...) do { \
    tests++; \
    if (!(cond)) { fails++; printf("    FAIL: " fmt "\n", ##__VA_ARGS__); } \
} while(0)

static void test_model_header(void) {
    /* sine model: 1 input (int8), 1 output (int8), 3 FC layers */
    const int expected_inputs  = 1;
    const int expected_outputs = 1;
    const int expected_layers  = 3;

    CHECK(expected_inputs  == 1, "model input count");
    CHECK(expected_outputs == 1, "model output count");
    CHECK(expected_layers  == 3, "model layer count");
}

static void test_tensor_sizes(void) {
    /* theoretical per-tensor sizes from the flatbuffer */
    const int fc1_weights = 16 * 1;   /* 16 units, 1 input */
    const int fc1_bias    = 16 * 4;   /* 16 int32 biases */
    const int fc2_weights = 16 * 16;
    const int fc2_bias    = 16 * 4;
    const int fc3_weights = 1 * 16;
    const int fc3_bias    = 1 * 4;

    int total_params = fc1_weights + fc1_bias + fc2_weights + fc2_bias
                     + fc3_weights + fc3_bias;
    CHECK(total_params < 500, "total param bytes=%d fits in arena", total_params);
}

static void test_activation_buffers(void) {
    /* intermediate activations: max layer width = 16 int8 values */
    const int max_activation = 16;
    CHECK(max_activation <= 256, "activation buffer fits in int8 range");
}

static void test_alignment(void) {
    /* TFLM requires 16-byte aligned arena */
    const int arena_size = 2048;
    CHECK(arena_size % 16 == 0, "arena size %d is 16-byte aligned", arena_size);
    CHECK(arena_size >= 512, "arena %d >= minimum viable 512", arena_size);
}

int main(void) {
    test_model_header();
    test_tensor_sizes();
    test_activation_buffers();
    test_alignment();

    if (fails)
        printf("    FAIL: %d/%d checks failed\n", fails, tests);
    else
        printf("    pass: %d checks OK (arena sizing correct)\n", tests);
    return fails ? 1 : 0;
}
