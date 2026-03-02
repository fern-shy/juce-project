#pragma once

namespace pandoras_box {
class Distortion {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    toneFilter.prepare({sampleRate,
                        static_cast<juce::uint32>(maxBlockSize),
                        static_cast<juce::uint32>(numChannels)});
    toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    toneFilter.setCutoffFrequency(4000.0f);
  }

  void setParameters(float driveLevel, float toneFreq, float mixLevel) {
    drive = driveLevel;
    mix = mixLevel;
    toneFilter.setCutoffFrequency(juce::jlimit(200.0f, 8000.0f, toneFreq));
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
  juce::dsp::StateVariableTPTFilter<float> toneFilter;
  float drive = 0.0f;
  float mix = 0.0f;
};
}  // namespace pandoras_box
