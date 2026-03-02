#pragma once

namespace pandoras_box {
class ReverbEffect {
public:
  void prepare(double sampleRate, int /*maxBlockSize*/, int /*numChannels*/) {
    reverb.setSampleRate(sampleRate);
    reverb.reset();
  }

  void setParameters(float roomSize, float damping, float wet, float width) {
    juce::Reverb::Parameters params;
    params.roomSize = juce::jlimit(0.0f, 1.0f, roomSize);
    params.damping = juce::jlimit(0.0f, 1.0f, damping);
    params.wetLevel = juce::jlimit(0.0f, 1.0f, wet);
    params.dryLevel = 1.0f - params.wetLevel;
    params.width = juce::jlimit(0.0f, 1.0f, width);
    params.freezeMode = 0.0f;
    reverb.setParameters(params);
  }

  void process(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() >= 2)
      reverb.processStereo(buffer.getWritePointer(0),
                           buffer.getWritePointer(1),
                           buffer.getNumSamples());
    else if (buffer.getNumChannels() == 1)
      reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
  }

  void reset() { reverb.reset(); }

private:
  juce::Reverb reverb;
};
}  // namespace pandoras_box
