#pragma once

namespace tremolo {
class ChorusEffect {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    chorus.prepare({sampleRate,
                    static_cast<juce::uint32>(maxBlockSize),
                    static_cast<juce::uint32>(numChannels)});
    chorus.reset();
  }

  void setParameters(float rate, float depth, float mixLevel) {
    chorus.setRate(juce::jlimit(0.1f, 5.0f, rate));
    chorus.setDepth(juce::jlimit(0.0f, 1.0f, depth));
    chorus.setMix(juce::jlimit(0.0f, 1.0f, mixLevel));
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(-0.2f);
  }

  void process(juce::AudioBuffer<float>& buffer) {
    auto block = juce::dsp::AudioBlock<float>(buffer);
    chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
  }

  void reset() { chorus.reset(); }

private:
  juce::dsp::Chorus<float> chorus;
};
}  // namespace tremolo
