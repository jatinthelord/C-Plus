#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t mix_value(uint64_t value) {
    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    return value * UINT64_C(2685821657736338717);
}

int main(int argc, char **argv) {
    uint64_t rounds = UINT64_C(25000000);
    if (argc > 1) rounds = strtoull(argv[1], NULL, 10);
    uint64_t state = UINT64_C(88172645463325252);
    uint64_t checksum = 0;
    for (uint64_t index = 0; index < rounds; ++index) {
        state = mix_value(state + index);
        checksum ^= state;
    }
    printf("%llu\n", (unsigned long long)checksum);
    return 0;
}
