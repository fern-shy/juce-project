#pragma once

namespace pandoras_box {
class CompressorEffect {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    const juce::dsp::ProcessSpec spec{sampleRate,
                                      static_cast<juce::uint32>(maxBlockSize),
                                      static_cast<juce::uint32>(numChannels)};
    compressor.prepare(spec);
    compressor.setThreshold(-20.0f);
    compressor.setRatio(4.0f);
    compressor.setAttack(5.0f);
    compressor.setRelease(100.0f);

    makeupGain.prepare(spec);
    makeupGain.setGainDecibels(4.0f);
  }

  void setParameters(float threshold, float ratio, float attack,
                     float release) {
    compressor.setThreshold(juce::jlimit(-60.0f, 0.0f, threshold));
    compressor.setRatio(juce::jlimit(1.0f, 20.0f, ratio));
    compressor.setAttack(juce::jlimit(0.1f, 100.0f, attack));
    compressor.setRelease(juce::jlimit(1.0f, 500.0f, release));
  }

  void process(juce::AudioBuffer<float>& buffer) {
    auto block = juce::dsp::AudioBlock<float>(buffer);
    const juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);
    // Compression pulls the level down; add a few dB back so the effect
    // doesn't make the sound noticeably quieter.
    makeupGain.process(context);
  }

  void reset() {
    compressor.reset();
    makeupGain.reset();
  }

private:
  juce::dsp::Compressor<float> compressor;
  juce::dsp::Gain<float> makeupGain;
};
}  // namespace pandoras_box
