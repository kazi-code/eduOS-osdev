#pragma once

#include <stdint.h>
#include <stdlib.h>

struct chacha20_state {
    uint32_t n[16];
};

struct chacha20_state init(
    const char key[static 32], const char nonce[static 12], uint32_t count);

void chacha20_keystream(struct chacha20_state *state, char *buffer, size_t len);
