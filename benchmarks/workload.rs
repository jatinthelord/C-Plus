use std::env;

#[inline]
fn mix_value(mut value: u64) -> u64 {
    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    value.wrapping_mul(2_685_821_657_736_338_717)
}

fn main() {
    let rounds = env::args().nth(1).and_then(|v| v.parse().ok()).unwrap_or(25_000_000_u64);
    let mut state = 88_172_645_463_325_252_u64;
    let mut checksum = 0_u64;
    for index in 0..rounds {
        state = mix_value(state.wrapping_add(index));
        checksum ^= state;
    }
    println!("{checksum}");
}
