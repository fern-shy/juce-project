#pragma once

namespace pandoras_box {
class RingModulator {
public:
  void prepare(double sampleRate, int maxBlockSize) {
    const juce::dsp::ProcessSpec spec{
        sampleRate, static_cast<juce::uint32>(maxBlockSize), 1u};
    oscillator.prepare(spec);
    oscillator.setFrequency(440.0f);
  }

  void setParameters(float freq, float mixLevel) {
    oscillator.setFrequency(juce::jlimit(20.0f, 2000.0f, freq));
    mix = mixLevel;
  }

  void process(juce::AudioBuffer<float>& buffer) {
    if (mix < 0.001f)
      return;

    const auto numSamples = buffer.getNumSamples();
    const auto numCh = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i) {
      const auto modValue = oscillator.processSample(0.0f);
      for (int ch = 0; ch < numCh; ++ch) {
        const auto dry = buffer.getSample(ch, i);
        const auto wet = dry * modValue;
        buffer.setSample(ch, i, dry * (1.0f - mix) + wet * mix);
      }
    }
  }

  void reset() { oscillator.reset(); }

private:
  juce::dsp::Oscillator<float> oscillator{
      [](float phase) { return std::sin(phase); }};
  float mix = 0.0f;
};
}  // namespace pandoras_box
