#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sr850 {
struct WavImpulse {
  unsigned sample_rate = 0;
  std::vector<float> taps;
};
// Offline/control-thread loader. Never call from the ASIO callback.
// Supports mono PCM16 or IEEE float32 RIFF/WAVE; rejects unknown formats,
// stereo files, truncated chunks, and nonfinite samples.
inline WavImpulse load_mono_wav_impulse(const std::string& path,
                                        unsigned required_sample_rate,
                                        std::size_t max_taps = 262144) {
  std::ifstream file(path, std::ios::binary);
  if (!file) throw std::runtime_error("cannot open WAV");
  auto u16 = [](const unsigned char* p) -> unsigned {
    return unsigned(p[0]) | (unsigned(p[1]) << 8);
  };
  auto u32 = [](const unsigned char* p) -> std::uint32_t {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
  };
  unsigned char header[12]{};
  if (!file.read(reinterpret_cast<char*>(header), 12) ||
      std::memcmp(header, "RIFF", 4) || std::memcmp(header + 8, "WAVE", 4))
    throw std::runtime_error("not RIFF WAVE");
  const std::uint64_t riff_end = 8ULL + u32(header + 4);
  if (riff_end < 12) throw std::runtime_error("invalid RIFF size");
  std::uint64_t offset = 12;
  bool have_fmt = false, have_data = false;
  unsigned format = 0, channels = 0, bits = 0, rate = 0;
  std::vector<unsigned char> samples;
  while (offset + 8 <= riff_end) {
    unsigned char chunk[8]{};
    if (!file.read(reinterpret_cast<char*>(chunk), 8))
      throw std::runtime_error("truncated WAV chunk header");
    offset += 8;
    const std::uint32_t size = u32(chunk + 4);
    if (std::uint64_t(size) > riff_end - offset)
      throw std::runtime_error("WAV chunk exceeds RIFF");
    if (!std::memcmp(chunk, "fmt ", 4)) {
      if (have_fmt || size < 16 || size > 4096)
        throw std::runtime_error("invalid WAV fmt chunk");
      std::vector<unsigned char> fmt(size);
      if (!file.read(reinterpret_cast<char*>(fmt.data()), size))
        throw std::runtime_error("truncated WAV fmt");
      format = u16(fmt.data());
      channels = u16(fmt.data() + 2);
      rate = u32(fmt.data() + 4);
      bits = u16(fmt.data() + 14);
      have_fmt = true;
    } else if (!std::memcmp(chunk, "data", 4)) {
      if (have_data || size > max_taps * 4ULL)
        throw std::runtime_error("invalid WAV data size");
      samples.resize(size);
      if (!file.read(reinterpret_cast<char*>(samples.data()), size))
        throw std::runtime_error("truncated WAV samples");
      have_data = true;
    } else {
      file.seekg(size, std::ios::cur);
      if (!file) throw std::runtime_error("truncated WAV chunk");
    }
    offset += size;
    if (size & 1U) {
      if (offset >= riff_end || !file.seekg(1, std::ios::cur))
        throw std::runtime_error("missing WAV padding");
      ++offset;
    }
  }
  if (!have_fmt || !have_data || channels != 1 ||
      rate != required_sample_rate || !rate ||
      !((format == 1 && bits == 16) || (format == 3 && bits == 32)))
    throw std::runtime_error("unsupported WAV impulse format/rate");
  const std::size_t bytes = bits / 8;
  if (samples.empty() || samples.size() % bytes ||
      samples.size() / bytes > max_taps)
    throw std::runtime_error("invalid WAV impulse length");
  WavImpulse result;
  result.sample_rate = rate;
  result.taps.reserve(samples.size() / bytes);
  for (std::size_t i = 0; i < samples.size(); i += bytes) {
    const unsigned char* p = samples.data() + i;
    float value;
    if (format == 1) {
      const auto pcm = static_cast<std::int16_t>(u16(p));
      value = float(pcm) / 32768.0f;
    } else {
      const std::uint32_t raw = u32(p);
      std::memcpy(&value, &raw, sizeof(value));
      if (!std::isfinite(value)) throw std::runtime_error("nonfinite WAV tap");
    }
    result.taps.push_back(value);
  }
  return result;
}
} // namespace sr850
