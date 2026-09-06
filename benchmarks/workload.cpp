#include <cstdint>
#include <cstdio>
#include <cstdlib>

static std::uint64_t mix_value(std::uint64_t value) {
    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    return value * UINT64_C(2685821657736338717);
}

int main(int argc, char **argv) {
    std::uint64_t rounds = UINT64_C(25000000);
    if (argc > 1) rounds = std::strtoull(argv[1], nullptr, 10);
    std::uint64_t state = UINT64_C(88172645463325252);
    std::uint64_t checksum = 0;
    for (std::uint64_t index = 0; index < rounds; ++index) {
        state = mix_value(state + index);
        checksum ^= state;
    }
    std::printf("%llu\n", static_cast<unsigned long long>(checksum));
    return 0;
}
