#pragma once

namespace pandoras_box {
class FilterEffect {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    sr = sampleRate;
    filter.prepare({sampleRate,
                    static_cast<juce::uint32>(maxBlockSize),
                    static_cast<juce::uint32>(numChannels)});
    filter.setCutoffFrequency(juce::jmin(20000.0f, maxCutoff()));
    filter.setResonance(0.707f);
  }

  void setParameters(float cutoff, float resonance, int type) {
    filter.setCutoffFrequency(juce::jlimit(20.0f, maxCutoff(), cutoff));
    filter.setResonance(juce::jlimit(0.1f, 10.0f, resonance));

    using Type = juce::dsp::StateVariableTPTFilterType;
    switch (type) {
      case 1:
        filter.setType(Type::bandpass);
        break;
      case 2:
        filter.setType(Type::highpass);
        break;
      default:
        filter.setType(Type::lowpass);
        break;
    }
  }

  void process(juce::AudioBuffer<float>& buffer) {
    auto block = juce::dsp::AudioBlock<float>(buffer);
    filter.process(juce::dsp::ProcessContextReplacing<float>(block));
  }

  void reset() { filter.reset(); }

private:
  // Keep the cutoff safely below Nyquist to avoid TPT filter instability
  // (and JUCE asserts) at low sample rates.
  [[nodiscard]] float maxCutoff() const {
    return juce::jmin(20000.0f, static_cast<float>(sr) * 0.49f);
  }

  juce::dsp::StateVariableTPTFilter<float> filter;
  double sr = 44100.0;
};
}  // namespace pandoras_box
