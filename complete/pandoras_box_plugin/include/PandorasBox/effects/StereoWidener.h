#pragma once

namespace pandoras_box {
// Stereo field widener + constant-rate stereo detuner.
// Width is handled with Mid/Side scaling. Detune applies a constant (non-LFO)
// micro pitch-shift in opposite directions per channel, which spreads the
// stereo image without the periodic "wobble" of a chorus.
class StereoWidener {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    juce::ignoreUnused(maxBlockSize, numChannels);
    sr = sampleRate;
    grainSize = static_cast<int>(sr * 0.05);
    bufSize = grainSize * 4;
    for (auto& b : circBuf)
      b.assign(static_cast<size_t>(bufSize), 0.0f);
    reset();
  }

  void setParameters(float widthAmount, float detuneCents, float mixLevel) {
    width = juce::jlimit(1.0f, 3.0f, widthAmount);
    detune = juce::jlimit(0.0f, 50.0f, detuneCents);
    mix = juce::jlimit(0.0f, 1.0f, mixLevel);
  }

  void process(juce::AudioBuffer<float>& buffer) {
    if (mix < 0.001f || bufSize == 0 || buffer.getNumChannels() < 2)
      return;

    const auto numSamples = buffer.getNumSamples();
    const auto gs = static_cast<float>(grainSize);

    const float ratio[2] = {std::pow(2.0f, detune / 1200.0f),
                            std::pow(2.0f, -detune / 1200.0f)};

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i) {
      const float dryL = left[i];
      const float dryR = right[i];

      circBuf[0][static_cast<size_t>(writeIdx)] = dryL;
      circBuf[1][static_cast<size_t>(writeIdx)] = dryR;

      float wet[2] = {};
      for (int ch = 0; ch < 2; ++ch) {
        const float drift = 1.0f - ratio[ch];
        for (int g = 0; g < 2; ++g) {
          auto phase = grainOffset[ch][g] / gs;
          phase = phase - std::floor(phase);
          if (phase < 0.0f)
            phase += 1.0f;

          const auto window =
              0.5f * (1.0f - std::cos(phase * juce::MathConstants<float>::twoPi));

          const auto readPos =
              static_cast<float>(writeIdx) - grainOffset[ch][g];

          wet[ch] += interpolate(circBuf[ch], readPos) * window;

          grainOffset[ch][g] += drift;
          if (grainOffset[ch][g] >= gs)
            grainOffset[ch][g] -= gs;
          else if (grainOffset[ch][g] < 0.0f)
            grainOffset[ch][g] += gs;
        }
      }

      const float l = dryL * (1.0f - mix) + wet[0] * mix;
      const float r = dryR * (1.0f - mix) + wet[1] * mix;

      const float mid = 0.5f * (l + r);
      const float side = 0.5f * (l - r) * width;

      left[i] = mid + side;
      right[i] = mid - side;

      writeIdx = (writeIdx + 1) % bufSize;
    }
  }

  void reset() {
    for (auto& b : circBuf)
      std::fill(b.begin(), b.end(), 0.0f);
    writeIdx = 0;
    for (int ch = 0; ch < 2; ++ch) {
      grainOffset[ch][0] = 0.0f;
      grainOffset[ch][1] = static_cast<float>(grainSize) * 0.5f;
    }
  }

private:
  float interpolate(const std::vector<float>& buf, float pos) const {
    auto p0 = static_cast<int>(std::floor(pos));
    const auto frac = pos - std::floor(pos);
    p0 = ((p0 % bufSize) + bufSize) % bufSize;
    const auto p1 = (p0 + 1) % bufSize;
    return buf[static_cast<size_t>(p0)] * (1.0f - frac) +
           buf[static_cast<size_t>(p1)] * frac;
  }

  std::vector<float> circBuf[2];
  int bufSize = 0;
  int writeIdx = 0;
  float grainOffset[2][2] = {};
  int grainSize = 2048;
  double sr = 44100.0;
  float width = 1.5f;
  float detune = 10.0f;
  float mix = 0.0f;
};
}  // namespace pandoras_box
