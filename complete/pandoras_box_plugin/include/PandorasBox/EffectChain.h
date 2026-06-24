#pragma once

namespace pandoras_box {
class EffectChain {
public:
  static constexpr int NUM_GROUPS = 8;

  enum GroupId {
    GrpTime = 0,
    GrpBreath,
    GrpOrder,
    GrpChaos,
    GrpSpace,
    GrpReflection,
    GrpFracture,
    GrpWrath
  };

  EffectChain() {
    for (int i = 0; i < NUM_GROUPS; ++i)
      groupOrder[i] = i;
    pendingGroupOrder = groupOrder;
  }

  void prepare(double sampleRate, int maxBlockSize, int numChannels) {
    sr = sampleRate;

    for (auto& d : delays)
      d.prepare(sampleRate, maxBlockSize, numChannels);
    reverb.prepare(sampleRate, maxBlockSize, numChannels);
    compressor.prepare(sampleRate, maxBlockSize, numChannels);
    pitchShifter.prepare(sampleRate, maxBlockSize, numChannels);
    ringMod.prepare(sampleRate, maxBlockSize);
    chaosChorus.prepare(sampleRate, maxBlockSize, numChannels);
    filter.prepare(sampleRate, maxBlockSize, numChannels);
    distortion.prepare(sampleRate, maxBlockSize, numChannels);
    bitCrusher.prepare(sampleRate);

    // Space
    stereoWidener.prepare(sampleRate, maxBlockSize, numChannels);

    // Reflection
    for (auto& p : reflectionPitch)
      p.prepare(sampleRate, maxBlockSize, numChannels);

    // Fracture
    for (auto& c : combFilters)
      c.prepare(sampleRate, maxBlockSize, numChannels);

    // Wrath
    waveFolder.prepare(sampleRate, maxBlockSize, numChannels);

    limiter.prepare({sampleRate,
                     static_cast<juce::uint32>(maxBlockSize),
                     static_cast<juce::uint32>(numChannels)});
    limiter.setThreshold(-1.0f);
    limiter.setRelease(50.0f);

    dryBuffer.setSize(numChannels, maxBlockSize);
  }

  // --- Randomization methods ---

  void randomizeParams() {
    std::uniform_real_distribution<float> u(0.0f, 1.0f);

    // Time: 3 delays with individual params
    for (int d = 0; d < 3; ++d) {
      delayParams[d].time = 0.01f + u(rng) * 0.99f;
      delayParams[d].feedback = u(rng) * 0.90f;
      delayParams[d].mix = u(rng);
    }

    // Breath: reverb (20% chance of being active)
    // Bias toward large rooms and low damping for long, spacious tails.
    breathActive = (u(rng) < 0.2f);
    if (breathActive) {
      reverbParams.size = 0.55f + u(rng) * 0.45f;
      reverbParams.damping = u(rng) * 0.5f;
      reverbParams.wet = 0.3f + u(rng) * 0.7f;
      reverbParams.width = 0.6f + u(rng) * 0.4f;
    }

    // Order: compressor
    compParams.threshold = -60.0f + u(rng) * 50.0f;
    compParams.ratio = 1.0f + u(rng) * 19.0f;
    compParams.attack = 0.1f + u(rng) * 99.9f;
    compParams.release = 1.0f + u(rng) * 499.0f;

    // Chaos: pitch + ring + chorus + filter
    chaosParams.pitchSemitones = u(rng) * 24.0f - 12.0f;
    chaosParams.pitchMix = u(rng);
    chaosParams.ringFreq = 20.0f + u(rng) * 1980.0f;
    chaosParams.ringMix = u(rng);
    chaosParams.chorusRate = 0.1f + u(rng) * 4.9f;
    chaosParams.chorusDepth = u(rng);
    chaosParams.chorusMix = u(rng);
    chaosParams.filterCutoff = 20.0f * std::pow(1000.0f, u(rng));
    chaosParams.filterResonance = 0.1f + u(rng) * 9.9f;
    chaosParams.filterType = static_cast<int>(std::floor(u(rng) * 3.0f));

    // Space: stereo widener + detuner
    spaceParams.width = 1.3f + u(rng) * 1.2f;
    spaceParams.detune = 5.0f + u(rng) * 25.0f;
    spaceParams.mix = 0.4f + u(rng) * 0.6f;

    // Reflection: octave layering
    reflectionParams.upperSemitones = 5.0f + u(rng) * 7.0f;
    reflectionParams.upperMix = 0.3f + u(rng) * 0.7f;
    reflectionParams.lowerSemitones = -(5.0f + u(rng) * 7.0f);
    reflectionParams.lowerMix = 0.3f + u(rng) * 0.7f;

    // Fracture: comb filter bank
    for (int i = 0; i < 3; ++i) {
      fractureParams[i].frequency = 80.0f + u(rng) * 1920.0f;
      fractureParams[i].feedback = 0.7f + u(rng) * 0.28f;
      fractureParams[i].mix = 0.2f + u(rng) * 0.8f;
    }

    // Wrath: coloured wavefolder
    wrathParams.drive = 3.0f + u(rng) * 9.0f;
    wrathParams.folds = 2 + static_cast<int>(std::floor(u(rng) * 4.0f));
    wrathParams.symmetry = u(rng);
    wrathParams.colour = 400.0f + u(rng) * 2600.0f;
    wrathParams.mix = 0.5f + u(rng) * 0.5f;

    // 10% chance of distortion/bitcrusher
    destroyActive = (u(rng) < 0.1f);
    if (destroyActive) {
      destroyParams.drive = u(rng);
      destroyParams.tone = 200.0f + u(rng) * 7800.0f;
      destroyParams.distMix = u(rng);
      destroyParams.bits = 4.0f + u(rng) * 12.0f;
      destroyParams.crushRate = 1.0f + u(rng) * 49.0f;
    }

    paramsChanged.store(true, std::memory_order_release);
  }

  void randomizeOrder() {
    std::array<int, NUM_GROUPS> newOrder = pendingGroupOrder;
    for (int i = NUM_GROUPS - 1; i > 0; --i) {
      std::uniform_int_distribution<int> dist(0, i);
      std::swap(newOrder[static_cast<size_t>(i)],
                newOrder[static_cast<size_t>(dist(rng))]);
    }
    pendingGroupOrder = newOrder;
    orderChanged.store(true, std::memory_order_release);
  }

  void randomizeBoth() {
    randomizeParams();
    randomizeOrder();
  }

  // --- Process ---

  void process(juce::AudioBuffer<float>& buffer,
               float timeMacro, float breathMacro,
               float orderMacro, float chaosMacro,
               float spaceMacro, float reflectionMacro,
               float fractureMacro, float wrathMacro) {
    if (orderChanged.load(std::memory_order_acquire)) {
      groupOrder = pendingGroupOrder;
      orderChanged.store(false, std::memory_order_relaxed);
    }

    if (paramsChanged.load(std::memory_order_acquire)) {
      applyParamsToEffects();
      paramsChanged.store(false, std::memory_order_relaxed);
    }

    for (int slot = 0; slot < NUM_GROUPS; ++slot) {
      float macro = 0.0f;
      switch (groupOrder[static_cast<size_t>(slot)]) {
        case GrpTime:
          macro = timeMacro;
          processGroup(buffer, macro, [this](juce::AudioBuffer<float>& b) {
            for (auto& d : delays)
              d.process(b);
          });
          break;
        case GrpBreath:
          if (!currentBreathActive)
            break;
          macro = breathMacro;
          processGroup(buffer, macro, [this](juce::AudioBuffer<float>& b) {
            reverb.process(b);
          });
          break;
        case GrpOrder:
          macro = orderMacro;
          processGroup(buffer, macro, [this](juce::AudioBuffer<float>& b) {
            compressor.process(b);
          });
          break;
        case GrpChaos:
          macro = chaosMacro;
          processGroup(buffer, macro, [this](juce::AudioBuffer<float>& b) {
            pitchShifter.process(b);
            ringMod.process(b);
            chaosChorus.process(b);
            filter.process(b);
          });
          break;
        case GrpSpace:
          macro = spaceMacro;
          processGroup(buffer, macro, [this](juce::AudioBuffer<float>& b) {
            stereoWidener.process(b);
          });
          break;
        case GrpReflection:
          macro = reflectionMacro;
          processGroup(buffer, macro, [this](juce::AudioBuffer<float>& b) {
            for (auto& p : reflectionPitch)
              p.process(b);
          });
          break;
        case GrpFracture:
          macro = fractureMacro;
          processGroup(buffer, macro, [this](juce::AudioBuffer<float>& b) {
            for (auto& c : combFilters)
              c.process(b);
          });
          break;
        case GrpWrath:
          macro = wrathMacro;
          processGroup(buffer, macro, [this](juce::AudioBuffer<float>& b) {
            waveFolder.process(b);
          });
          break;
      }
    }

    // Destroy effects (10% chance, applied at the end if active)
    if (currentDestroyActive) {
      distortion.process(buffer);
      bitCrusher.process(buffer);
    }

    // Safety limiter -- prevent clipping beyond 0 dBFS
    auto block = juce::dsp::AudioBlock<float>(buffer);
    limiter.process(juce::dsp::ProcessContextReplacing<float>(block));
  }

  void reset() {
    for (auto& d : delays)
      d.reset();
    reverb.reset();
    compressor.reset();
    pitchShifter.reset();
    ringMod.reset();
    chaosChorus.reset();
    filter.reset();
    distortion.reset();
    bitCrusher.reset();

    stereoWidener.reset();
    for (auto& p : reflectionPitch)
      p.reset();
    for (auto& c : combFilters)
      c.reset();
    waveFolder.reset();
    limiter.reset();
  }

private:
  template <typename ProcessFn>
  void processGroup(juce::AudioBuffer<float>& buffer, float macro,
                    ProcessFn fn) {
    if (macro < 0.001f)
      return;

    const auto numCh = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
      dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    fn(buffer);

    if (macro < 0.999f) {
      for (int ch = 0; ch < numCh; ++ch) {
        auto* wet = buffer.getWritePointer(ch);
        const auto* dry = dryBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
          wet[i] = dry[i] * (1.0f - macro) + wet[i] * macro;
      }
    }
  }

  void applyParamsToEffects() {
    // Time
    for (int d = 0; d < 3; ++d)
      delays[static_cast<size_t>(d)].setParameters(
          delayParams[static_cast<size_t>(d)].time,
          delayParams[static_cast<size_t>(d)].feedback,
          delayParams[static_cast<size_t>(d)].mix);

    // Breath
    currentBreathActive = breathActive;
    if (currentBreathActive)
      reverb.setParameters(reverbParams.size, reverbParams.damping,
                           reverbParams.wet, reverbParams.width);

    // Order
    compressor.setParameters(compParams.threshold, compParams.ratio,
                             compParams.attack, compParams.release);

    // Chaos
    pitchShifter.setParameters(chaosParams.pitchSemitones,
                               chaosParams.pitchMix);
    ringMod.setParameters(chaosParams.ringFreq, chaosParams.ringMix);
    chaosChorus.setParameters(chaosParams.chorusRate, chaosParams.chorusDepth,
                              chaosParams.chorusMix);
    filter.setParameters(chaosParams.filterCutoff,
                         chaosParams.filterResonance,
                         chaosParams.filterType);

    // Space
    stereoWidener.setParameters(spaceParams.width, spaceParams.detune,
                                spaceParams.mix);

    // Reflection
    reflectionPitch[0].setParameters(reflectionParams.upperSemitones,
                                     reflectionParams.upperMix);
    reflectionPitch[1].setParameters(reflectionParams.lowerSemitones,
                                     reflectionParams.lowerMix);

    // Fracture
    for (int i = 0; i < 3; ++i)
      combFilters[i].setParameters(
          fractureParams[i].frequency,
          fractureParams[i].feedback,
          fractureParams[i].mix);

    // Wrath
    waveFolder.setParameters(wrathParams.drive, wrathParams.folds,
                             wrathParams.symmetry, wrathParams.colour,
                             wrathParams.mix);

    // Destroy
    currentDestroyActive = destroyActive;
    if (currentDestroyActive) {
      distortion.setParameters(destroyParams.drive, destroyParams.tone,
                               destroyParams.distMix);
      bitCrusher.setParameters(destroyParams.bits, destroyParams.crushRate);
    }
  }

  // --- Parameter storage ---

  struct DelayParams {
    float time = 0.3f, feedback = 0.3f, mix = 0.0f;
  };
  std::array<DelayParams, 3> delayParams{};

  bool breathActive = false;
  bool currentBreathActive = false;
  struct ReverbParams {
    float size = 0.5f, damping = 0.5f, wet = 0.0f, width = 1.0f;
  };
  ReverbParams reverbParams;

  struct CompressorParams {
    float threshold = -20.0f, ratio = 4.0f, attack = 5.0f, release = 100.0f;
  };
  CompressorParams compParams;

  struct ChaosParams {
    float pitchSemitones = 0.0f, pitchMix = 0.0f;
    float ringFreq = 440.0f, ringMix = 0.0f;
    float chorusRate = 1.0f, chorusDepth = 0.0f, chorusMix = 0.0f;
    float filterCutoff = 20000.0f, filterResonance = 0.707f;
    int filterType = 0;
  };
  ChaosParams chaosParams;

  struct SpaceParams {
    float width = 1.5f, detune = 10.0f, mix = 0.0f;
  };
  SpaceParams spaceParams;

  struct ReflectionParams {
    float upperSemitones = 12.0f, upperMix = 0.0f;
    float lowerSemitones = -12.0f, lowerMix = 0.0f;
  };
  ReflectionParams reflectionParams;

  struct FractureParams {
    float frequency = 200.0f, feedback = 0.8f, mix = 0.0f;
  };
  std::array<FractureParams, 3> fractureParams{};

  struct WrathParams {
    float drive = 2.0f;
    int folds = 1;
    float symmetry = 0.5f, colour = 1000.0f, mix = 0.0f;
  };
  WrathParams wrathParams;

  bool destroyActive = false;
  bool currentDestroyActive = false;
  struct DestroyParams {
    float drive = 0.0f, tone = 4000.0f, distMix = 0.0f;
    float bits = 16.0f, crushRate = 1.0f;
  };
  DestroyParams destroyParams;

  std::atomic<bool> paramsChanged{false};

  // --- Routing ---
  std::array<int, NUM_GROUPS> groupOrder{0, 1, 2, 3, 4, 5, 6, 7};
  std::array<int, NUM_GROUPS> pendingGroupOrder{0, 1, 2, 3, 4, 5, 6, 7};
  std::atomic<bool> orderChanged{false};

  // --- Effects ---
  std::array<DelayEffect, 3> delays;
  ReverbEffect reverb;
  CompressorEffect compressor;
  PitchShifter pitchShifter;
  RingModulator ringMod;
  ChorusEffect chaosChorus;
  FilterEffect filter;
  Distortion distortion;
  BitCrusher bitCrusher;

  StereoWidener stereoWidener;
  std::array<PitchShifter, 2> reflectionPitch;
  std::array<CombFilter, 3> combFilters;
  WaveFolder waveFolder;

  juce::dsp::Limiter<float> limiter;

  juce::AudioBuffer<float> dryBuffer;
  std::mt19937 rng{std::random_device{}()};
  double sr = 44100.0;
};
}  // namespace pandoras_box
