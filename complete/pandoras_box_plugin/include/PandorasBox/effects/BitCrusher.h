#pragma once

namespace pandoras_box {
class BitCrusher {
public:
  void prepare(double /*sampleRate*/) {
    for (auto& h : holdSample)
      h = 0.0f;
    holdCounter = 0.0f;
  }

  void setParameters(float bits, float rateReduce) {
    bitDepth = juce::jlimit(4.0f, 16.0f, bits);
    rateReduction = juce::jlimit(1.0f, 50.0f, rateReduce);
  }

  void process(juce::AudioBuffer<float>& buffer) {
    const auto effectiveRate = static_cast<int>(std::round(rateReduction));
    if (effectiveRate <= 1 && bitDepth >= 15.9f)
      return;

    const auto levels =
        std::pow(2.0f, static_cast<float>(static_cast<int>(bitDepth)) - 1.0f);
    const auto numSamples = buffer.getNumSamples();
    const auto numCh = std::min(buffer.getNumChannels(), 2);

    for (int i = 0; i < numSamples; ++i) {
      holdCounter += 1.0f;
      const bool updateSample =
          holdCounter >= static_cast<float>(effectiveRate);
      if (updateSample)
        holdCounter = 0.0f;

      for (int ch = 0; ch < numCh; ++ch) {
        if (updateSample) {
          const auto sample = buffer.getSample(ch, i);
          holdSample[ch] = std::round(sample * levels) / levels;
        }
        buffer.setSample(ch, i, holdSample[ch]);
      }
    }
  }

  void reset() {
    for (auto& h : holdSample)
      h = 0.0f;
    holdCounter = 0.0f;
  }

private:
  float holdSample[2] = {};
  float holdCounter = 0.0f;
  float bitDepth = 16.0f;
  float rateReduction = 1.0f;
};
}  // namespace pandoras_box
