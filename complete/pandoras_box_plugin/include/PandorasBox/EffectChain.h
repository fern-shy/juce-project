#pragma once

namespace pandoras_box {
class EffectChain {
public:
  static constexpr int NUM_PARAMS = 22;

  enum ParamId {
    PitchShift,
    PitchMix,
    FilterCutoff,
    FilterResonance,
    FilterType,
    DistDrive,
    DistTone,
    DistMix,
    ReverbSize,
    ReverbDamping,
    ReverbWet,
    ReverbWidth,
    DelayTime,
    DelayFeedback,
    DelayMix,
    ChorusRate,
    ChorusDepth,
    ChorusMix,
    CrushBits,
    CrushRate,
    RingModFreq,
    RingModMix
  };

  EffectChain() { applyDefaults(); }

  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    sr = sampleRate;
    pitchShifter.prepare(sampleRate, maxBlockSize, numChannels);
    filter.prepare(sampleRate, maxBlockSize, numChannels);
    distortion.prepare(sampleRate, maxBlockSize, numChannels);
    chorus.prepare(sampleRate, maxBlockSize, numChannels);
    ringMod.prepare(sampleRate, maxBlockSize);
    bitCrusher.prepare(sampleRate);
    delay.prepare(sampleRate, maxBlockSize, numChannels);
    reverb.prepare(sampleRate, maxBlockSize, numChannels);

    for (auto& sv : smoothed)
      sv.reset(sampleRate, 0.15);
  }

  void randomize() {
    std::uniform_real_distribution<float> uniform(0.0f, 1.0f);

    targets[PitchShift].store(uniform(rng) * 24.0f - 12.0f,
                              std::memory_order_relaxed);
    targets[PitchMix].store(uniform(rng), std::memory_order_relaxed);

    targets[FilterCutoff].store(20.0f * std::pow(1000.0f, uniform(rng)),
                                std::memory_order_relaxed);
    targets[FilterResonance].store(0.1f + uniform(rng) * 9.9f,
                                  std::memory_order_relaxed);
    targets[FilterType].store(std::floor(uniform(rng) * 3.0f),
                              std::memory_order_relaxed);

    targets[DistDrive].store(uniform(rng), std::memory_order_relaxed);
    targets[DistTone].store(200.0f + uniform(rng) * 7800.0f,
                            std::memory_order_relaxed);
    targets[DistMix].store(uniform(rng), std::memory_order_relaxed);

    targets[ReverbSize].store(uniform(rng), std::memory_order_relaxed);
    targets[ReverbDamping].store(uniform(rng), std::memory_order_relaxed);
    targets[ReverbWet].store(uniform(rng), std::memory_order_relaxed);
    targets[ReverbWidth].store(uniform(rng), std::memory_order_relaxed);

    targets[DelayTime].store(0.01f + uniform(rng) * 0.99f,
                             std::memory_order_relaxed);
    targets[DelayFeedback].store(uniform(rng) * 0.95f,
                                 std::memory_order_relaxed);
    targets[DelayMix].store(uniform(rng), std::memory_order_relaxed);

    targets[ChorusRate].store(0.1f + uniform(rng) * 4.9f,
                              std::memory_order_relaxed);
    targets[ChorusDepth].store(uniform(rng), std::memory_order_relaxed);
    targets[ChorusMix].store(uniform(rng), std::memory_order_relaxed);

    targets[CrushBits].store(4.0f + uniform(rng) * 12.0f,
                             std::memory_order_relaxed);
    targets[CrushRate].store(1.0f + uniform(rng) * 49.0f,
                             std::memory_order_relaxed);

    targets[RingModFreq].store(20.0f + uniform(rng) * 1980.0f,
                               std::memory_order_relaxed);
    targets[RingModMix].store(uniform(rng), std::memory_order_relaxed);

    randomizeCount.fetch_add(1, std::memory_order_relaxed);
  }

  void process(juce::AudioBuffer<float>& buffer) {
    const auto numSamples = buffer.getNumSamples();

    std::array<float, NUM_PARAMS> p{};
    for (int i = 0; i < NUM_PARAMS; ++i) {
      smoothed[i].setTargetValue(targets[i].load(std::memory_order_relaxed));
      p[i] = smoothed[i].skip(numSamples);
    }

    pitchShifter.setParameters(p[PitchShift], p[PitchMix]);
    pitchShifter.process(buffer);

    filter.setParameters(p[FilterCutoff], p[FilterResonance],
                         static_cast<int>(std::round(p[FilterType])));
    filter.process(buffer);

    distortion.setParameters(p[DistDrive], p[DistTone], p[DistMix]);
    distortion.process(buffer);

    chorus.setParameters(p[ChorusRate], p[ChorusDepth], p[ChorusMix]);
    chorus.process(buffer);

    ringMod.setParameters(p[RingModFreq], p[RingModMix]);
    ringMod.process(buffer);

    bitCrusher.setParameters(p[CrushBits], p[CrushRate]);
    bitCrusher.process(buffer);

    delay.setParameters(p[DelayTime], p[DelayFeedback], p[DelayMix]);
    delay.process(buffer);

    reverb.setParameters(p[ReverbSize], p[ReverbDamping], p[ReverbWet],
                         p[ReverbWidth]);
    reverb.process(buffer);
  }

  void reset() {
    pitchShifter.reset();
    filter.reset();
    distortion.reset();
    chorus.reset();
    ringMod.reset();
    bitCrusher.reset();
    delay.reset();
    reverb.reset();
  }

  std::array<float, NUM_PARAMS> getParameterValues() const {
    std::array<float, NUM_PARAMS> vals{};
    for (int i = 0; i < NUM_PARAMS; ++i)
      vals[i] = targets[i].load(std::memory_order_relaxed);
    return vals;
  }

  void setParameterValues(const std::array<float, NUM_PARAMS>& vals) {
    for (int i = 0; i < NUM_PARAMS; ++i) {
      targets[i].store(vals[i], std::memory_order_relaxed);
      smoothed[i].setCurrentAndTargetValue(vals[i]);
    }
  }

  int getRandomizeCount() const {
    return randomizeCount.load(std::memory_order_relaxed);
  }

private:
  void applyDefaults() {
    const std::array<float, NUM_PARAMS> defaults = {
        0.0f,     0.0f,    20000.0f, 0.707f, 0.0f,   0.0f, 4000.0f,
        0.0f,     0.5f,    0.5f,     0.0f,   1.0f,   0.3f, 0.3f,
        0.0f,     1.0f,    0.0f,     0.0f,   16.0f,  1.0f,
        440.0f,   0.0f};
    for (int i = 0; i < NUM_PARAMS; ++i) {
      targets[i].store(defaults[i], std::memory_order_relaxed);
      smoothed[i].setCurrentAndTargetValue(defaults[i]);
    }
  }

  std::array<std::atomic<float>, NUM_PARAMS> targets{};
  std::array<juce::SmoothedValue<float>, NUM_PARAMS> smoothed;

  PitchShifter pitchShifter;
  FilterEffect filter;
  Distortion distortion;
  ChorusEffect chorus;
  RingModulator ringMod;
  BitCrusher bitCrusher;
  DelayEffect delay;
  ReverbEffect reverb;

  std::mt19937 rng{std::random_device{}()};
  std::atomic<int> randomizeCount{0};
  double sr = 44100.0;
};
}  // namespace pandoras_box
