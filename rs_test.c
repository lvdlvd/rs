#include "rs.h"
#include <stdio.h>
#include <string.h>

enum {
    N    = 96,
    K    = 64,
    NSYM = (N - K),
    T    = (NSYM / 2), // max correctable errors 
};

static uint8_t orig[N];
static uint8_t buf[N];
static uint8_t enc_work[RS_ENCODE_WORKSPACE(N, K)];
static uint8_t dec_scratch[RS_DECODE_SCRATCH(N, K)];

int main(void) {
    // Fill payload with 32, 33, ..., 95.
    for (int i = 0; i < K; i++) {
        orig[i] = (uint8_t)(32 + i);
    }

    // Initialize encoder and encode.
    rs_encode_init(K, N, enc_work, sizeof enc_work);
    memcpy(orig, orig, K); // payload already set
    memcpy(buf, orig, K);
    rs_encode(buf, K, N, enc_work);
    // Save the full encoded codeword.
    memcpy(orig, buf, N);

    int pass = 0, fail = 0;

    // Test corrupting 0, 1, 2, ... T+1 bytes.
    for (int nerr = 0; nerr <= T + 1; nerr++) {
        memcpy(buf, orig, N);

        // Corrupt nerr bytes at evenly spaced positions.
        for (int e = 0; e < nerr; e++) {
            int pos = (e * N) / (nerr > 0 ? nerr : 1);
            buf[pos] = 128;
        }

        int rc = rs_decode(buf, K, N, dec_scratch, sizeof dec_scratch);

        if (nerr <= T) {
            // Should succeed and reconstruct original.
            if (rc < 0) {
                printf("FAIL: %2d errors: decode returned -1 (expected success)\n", nerr);
                fail++;
            } else if (memcmp(buf, orig, N) != 0) {
                printf("FAIL: %2d errors: decode returned %d but data mismatch\n", nerr, rc);
                fail++;
            } else {
                printf("  ok: %2d errors: corrected %d\n", nerr, rc);
                pass++;
            }
        } else {
            // Beyond correction capacity — decode should report failure.
            if (rc >= 0 && memcmp(buf, orig, N) == 0) {
                // Unlikely but acceptable: it might still succeed by luck.
                printf("  ok: %2d errors: unexpectedly corrected (lucky)\n", nerr);
                pass++;
            } else if (rc < 0) {
                printf("  ok: %2d errors: correctly reported uncorrectable\n", nerr);
                pass++;
            } else {
                printf("FAIL: %2d errors: decode returned %d but data is wrong (silent miscorrection)\n", nerr, rc);
                fail++;
            }
        }
    }

    printf("\n%d passed, %d failed\n", pass, fail);
    return fail > 0 ? 1 : 0;
}
