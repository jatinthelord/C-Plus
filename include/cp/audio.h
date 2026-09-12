#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace cp {
inline float midi_to_hz(int note) {
  return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

inline std::vector<float> sine_wave(float hz, float seconds, int sample_rate) {
  const int samples = static_cast<int>(seconds * static_cast<float>(sample_rate));
  std::vector<float> out(static_cast<std::size_t>(std::max(0, samples)));
  const float step = 6.28318530718f * hz / static_cast<float>(sample_rate);
  float phase = 0;
  for (float &sample : out) {
    sample = std::sin(phase);
    phase += step;
  }
  return out;
}

inline std::int16_t float_to_pcm16(float sample) {
  if (sample > 1)
    sample = 1;
  if (sample < -1)
    sample = -1;
  return static_cast<std::int16_t>(sample * 32767.0f);
}
} // namespace cp
