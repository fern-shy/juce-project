#pragma once

namespace pandoras_box {
class Distortion {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    sr = sampleRate;
    toneFilter.prepare({sampleRate,
                        static_cast<juce::uint32>(maxBlockSize),
                        static_cast<juce::uint32>(numChannels)});
    toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    toneFilter.setCutoffFrequency(juce::jmin(4000.0f, maxCutoff()));
  }

  void setParameters(float driveLevel, float toneFreq, float mixLevel) {
    drive = driveLevel;
    mix = mixLevel;
    toneFilter.setCutoffFrequency(juce::jlimit(200.0f, maxCutoff(), toneFreq));
  }

  void process(juce::AudioBuffer<float>& buffer) {
    if (mix < 0.001f)
      return;

    const auto numSamples = buffer.getNumSamples();
    const auto numCh = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i) {
      for (int ch = 0; ch < numCh; ++ch) {
        const auto dry = buffer.getSample(ch, i);
        const auto gained = dry * (1.0f + drive * 20.0f);
        const auto clipped = std::tanh(gained);
        buffer.setSample(ch, i, dry * (1.0f - mix) + clipped * mix);
      }
    }

    auto block = juce::dsp::AudioBlock<float>(buffer);
    toneFilter.process(juce::dsp::ProcessContextReplacing<float>(block));
  }

  void reset() { toneFilter.reset(); }

private:
  [[nodiscard]] float maxCutoff() const {
    return juce::jmin(8000.0f, static_cast<float>(sr) * 0.49f);
  }

  juce::dsp::StateVariableTPTFilter<float> toneFilter;
  double sr = 44100.0;
  float drive = 0.0f;
  float mix = 0.0f;
};
}  // namespace pandoras_box
