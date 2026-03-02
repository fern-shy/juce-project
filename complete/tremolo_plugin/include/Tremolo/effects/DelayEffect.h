#pragma once

namespace tremolo {
class DelayEffect {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    sr = sampleRate;
    const auto maxDelaySamples = static_cast<int>(sr * 2.0);
    delayLine.setMaximumDelayInSamples(maxDelaySamples);
    delayLine.prepare({sampleRate,
                       static_cast<juce::uint32>(maxBlockSize),
                       static_cast<juce::uint32>(numChannels)});
    delayLine.reset();
  }

  void setParameters(float time, float fb, float mixLevel) {
    delayTime = time;
    feedback = juce::jlimit(0.0f, 0.95f, fb);
    mix = mixLevel;
  }

  void process(juce::AudioBuffer<float>& buffer) {
    if (mix < 0.001f)
      return;

    const auto delaySamples = delayTime * static_cast<float>(sr);
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = std::min(buffer.getNumChannels(), 2);

    for (int i = 0; i < numSamples; ++i) {
      for (int ch = 0; ch < numCh; ++ch) {
        const auto dry = buffer.getSample(ch, i);
        const auto delayed = delayLine.popSample(ch, delaySamples);
        delayLine.pushSample(ch, dry + delayed * feedback);
        buffer.setSample(ch, i, dry * (1.0f - mix) + delayed * mix);
      }
    }
  }

  void reset() { delayLine.reset(); }

private:
  juce::dsp::DelayLine<float,
                        juce::dsp::DelayLineInterpolationTypes::Linear>
      delayLine{192000};
  double sr = 44100.0;
  float delayTime = 0.3f;
  float feedback = 0.3f;
  float mix = 0.0f;
};
}  // namespace tremolo
