#pragma once

namespace pandoras_box {
class CombFilter {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    sr = sampleRate;
    channels = numChannels;
    const int maxDelaySamples = static_cast<int>(sr / 20.0);
    line.setMaximumDelayInSamples(maxDelaySamples);
    line.prepare({sampleRate,
                  static_cast<juce::uint32>(maxBlockSize),
                  static_cast<juce::uint32>(numChannels)});
    setParameters(200.0f, 0.8f, 0.5f);
  }

  void setParameters(float frequency, float fb, float mixLevel) {
    delaySamples = static_cast<float>(sr) / juce::jlimit(20.0f, 20000.0f, frequency);
    feedback = juce::jlimit(-0.99f, 0.99f, fb);
    mix = juce::jlimit(0.0f, 1.0f, mixLevel);
  }

  void process(juce::AudioBuffer<float>& buffer) {
    if (mix < 0.001f)
      return;

    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < channels && ch < buffer.getNumChannels(); ++ch) {
      auto* data = buffer.getWritePointer(ch);
      for (int i = 0; i < numSamples; ++i) {
        const float dry = data[i];
        const float delayed = line.popSample(ch, delaySamples);
        const float wet = dry + feedback * delayed;
        line.pushSample(ch, wet);
        data[i] = dry * (1.0f - mix) + wet * mix;
      }
    }
  }

  void reset() { line.reset(); }

private:
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> line;
  int channels = 2;
  double sr = 44100.0;
  float delaySamples = 100.0f;
  float feedback = 0.8f;
  float mix = 0.5f;
};
}  // namespace pandoras_box
