#pragma once

namespace pandoras_box {
class PitchShifter {
public:
  void prepare(double sampleRate, int /*maxBlockSize*/, int /*numChannels*/) {
    sr = sampleRate;
    grainSize = static_cast<int>(sr * 0.04);
    bufSize = grainSize * 4;
    for (auto& b : circBuf)
      b.assign(static_cast<size_t>(bufSize), 0.0f);
    writeIdx = 0;
    grainOffset[0] = 0.0f;
    grainOffset[1] = static_cast<float>(grainSize) * 0.5f;
  }

  void setParameters(float semitones, float mixLevel) {
    pitchSemitones = semitones;
    mix = mixLevel;
  }

  void process(juce::AudioBuffer<float>& buffer) {
    if (bufSize == 0 || mix < 0.001f)
      return;

    const auto ratio = std::pow(2.0f, pitchSemitones / 12.0f);
    const auto drift = 1.0f - ratio;
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = std::min(buffer.getNumChannels(), 2);
    const auto gs = static_cast<float>(grainSize);

    for (int i = 0; i < numSamples; ++i) {
      for (int ch = 0; ch < numCh; ++ch)
        circBuf[ch][writeIdx] = buffer.getSample(ch, i);

      float wet[2] = {};

      for (int g = 0; g < 2; ++g) {
        auto phase = grainOffset[g] / gs;
        phase = phase - std::floor(phase);
        if (phase < 0.0f)
          phase += 1.0f;

        const auto window =
            0.5f *
            (1.0f -
             std::cos(phase * juce::MathConstants<float>::twoPi));

        const auto readPos =
            static_cast<float>(writeIdx) - grainOffset[g];

        for (int ch = 0; ch < numCh; ++ch)
          wet[ch] += interpolate(circBuf[ch], readPos) * window;

        grainOffset[g] += drift;

        if (grainOffset[g] >= gs)
          grainOffset[g] -= gs;
        else if (grainOffset[g] < 0.0f)
          grainOffset[g] += gs;
      }

      for (int ch = 0; ch < numCh; ++ch) {
        const auto dry = buffer.getSample(ch, i);
        buffer.setSample(ch, i, dry * (1.0f - mix) + wet[ch] * mix);
      }

      writeIdx = (writeIdx + 1) % bufSize;
    }
  }

  void reset() {
    for (auto& b : circBuf)
      std::fill(b.begin(), b.end(), 0.0f);
    writeIdx = 0;
    grainOffset[0] = 0.0f;
    grainOffset[1] = static_cast<float>(grainSize) * 0.5f;
  }

private:
  float interpolate(const std::vector<float>& buf, float pos) const {
    auto p0 = static_cast<int>(std::floor(pos));
    const auto frac = pos - std::floor(pos);
    p0 = ((p0 % bufSize) + bufSize) % bufSize;
    const auto p1 = (p0 + 1) % bufSize;
    return buf[p0] * (1.0f - frac) + buf[p1] * frac;
  }

  std::vector<float> circBuf[2];
  int bufSize = 0;
  int writeIdx = 0;
  float grainOffset[2] = {};
  int grainSize = 2048;
  double sr = 44100.0;
  float pitchSemitones = 0.0f;
  float mix = 0.0f;
};
}  // namespace pandoras_box
