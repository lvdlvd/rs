# rs — Minimal Reed-Solomon for embedded use

A minimal Reed-Solomon encoding/decoding library in C (rs.h + rs.c), intended for sending packets over unreliable links on microcontrollers. No dynamic memory allocation — the caller provides all workspace.

## GF(2^8) arithmetic

Primitive polynomial: x^8 + x^4 + x^3 + x^2 + 1 (0x11d), generator element α = 2.

Three `static const` lookup tables:
- `gf_exp[255]` — powers of α
- `gf_log[256]` — discrete logarithm
- `gf_inv[256]` — multiplicative inverse

Total static data: 767 bytes of flash. `gf_mul()` uses `log[a] + log[b]` with a conditional subtract instead of doubling the exp table.

## API

```c
// Workspace sizing macros
#define RS_ENCODE_WORKSPACE(N, K)  ((N) - (K) + 1)
#define RS_DECODE_SCRATCH(N, K)    (4 * ((N) - (K)))

// Precompute generator polynomial (call once per K, N pair)
void rs_encode_init(uint8_t K, uint8_t N, uint8_t* work, size_t work_len);

// Systematic encoding: fills buf[K:N] with parity bytes
void rs_encode(uint8_t* buf, uint8_t K, uint8_t N, uint8_t* work);

// Decode and correct errors; returns error count or -1
int rs_decode(uint8_t* buf, uint8_t K, uint8_t N, uint8_t* scratch, size_t scratch_len);
```

Constraints: 0 < K < N ≤ 255. Can correct up to (N-K)/2 errors.

## Implementation

- **rs_encode_init** — builds generator polynomial g(x) = (x - α¹)···(x - α^(N-K)) into the workspace
- **rs_encode** — systematic encoding via polynomial long division (shift-register style)
- **rs_decode** — classic syndrome-based decoder:
  1. Syndrome computation
  2. Berlekamp-Massey for error locator σ(x)
  3. Chien search for error positions
  4. Forney algorithm for error values

The encode workspace is reusable across calls with the same (K, N). The decode scratch is temporary and needs no initialization.
