#pragma once

namespace pandoras_box {
// Aggressive wavefolder with asymmetric drive + tanh saturation, followed by a
// mid-focused bandpass "color" filter. The bandpass gives the gritty,
// band-limited "playing through a car / phone speaker" character rather than
// behaving like a plain gain stage.
class WaveFolder {
public:
  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    colourFilter.prepare({sampleRate,
                          static_cast<juce::uint32>(maxBlockSize),
                          static_cast<juce::uint32>(numChannels)});
    colourFilter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    colourFilter.setCutoffFrequency(1000.0f);
    colourFilter.setResonance(0.9f);

    dryCopy.setSize(numChannels, maxBlockSize);
  }

  void setParameters(float driveLevel, int numFolds, float sym,
                     float colourFreq, float mixLevel) {
    drive = juce::jlimit(2.0f, 16.0f, driveLevel);
    folds = juce::jlimit(1, 6, numFolds);
    symmetry = juce::jlimit(0.0f, 1.0f, sym);
    mix = juce::jlimit(0.0f, 1.0f, mixLevel);
    colourFilter.setCutoffFrequency(juce::jlimit(200.0f, 3500.0f, colourFreq));
  }

  void process(juce::AudioBuffer<float>& buffer) {
    if (mix < 0.001f)
      return;

    const auto numCh = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    const float offset = (symmetry - 0.5f) * 0.8f;

    for (int ch = 0; ch < numCh; ++ch)
      dryCopy.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Distorted, full-range signal (harsh, harmonically rich).
    for (int ch = 0; ch < numCh; ++ch) {
      auto* data = buffer.getWritePointer(ch);
      for (int i = 0; i < numSamples; ++i) {
        float x = data[i] * drive + offset;
        for (int f = 0; f < folds; ++f)
          x = fold(x);
        data[i] = std::tanh(x);
      }
    }

    // Band-limit the distortion -> "car speaker" color.
    auto block = juce::dsp::AudioBlock<float>(buffer);
    colourFilter.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Blend coloured distortion back with the clean signal.
    for (int ch = 0; ch < numCh; ++ch) {
      auto* wet = buffer.getWritePointer(ch);
      const auto* dry = dryCopy.getReadPointer(ch);
      for (int i = 0; i < numSamples; ++i)
        wet[i] = dry[i] * (1.0f - mix) + wet[i] * mix;
    }
  }

  void reset() { colourFilter.reset(); }

private:
  static float fold(float x) {
    return 4.0f *
           (std::abs(0.25f * x + 0.25f - std::round(0.25f * x + 0.25f)) - 0.25f);
  }

  juce::dsp::StateVariableTPTFilter<float> colourFilter;
  juce::AudioBuffer<float> dryCopy;
  float drive = 2.0f;
  int folds = 1;
  float symmetry = 0.5f;
  float mix = 0.5f;
};
}  // namespace pandoras_box
