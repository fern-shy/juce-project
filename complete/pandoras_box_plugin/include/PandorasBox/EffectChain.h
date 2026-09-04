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
      groupOrder[static_cast<size_t>(i)] = i;
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
    limiter.reset();

    dryBuffer.setSize(numChannels, maxBlockSize);

    // Apply either the factory defaults or state queued before prepare().
    stateChanged.store(true, std::memory_order_relaxed);
    applyPendingState();
  }

  // --- Randomization methods ---
  //
  // Randomization is fully deterministic given a seed. The eye buttons pick a
  // fresh seed; the concrete parameter values are (re)generated from it. Only
  // the seeds are persisted (see get/setState), so reloading a project or
  // rendering offline reproduces the exact same sound as what was heard live.

  void randomizeParams() {
    paramSeed.store(nextSeed(), std::memory_order_relaxed);
    paramsChanged.store(true, std::memory_order_release);
  }

  void randomizeOrder() {
    orderSeed.store(nextSeed(), std::memory_order_relaxed);
    orderChanged.store(true, std::memory_order_release);
  }

  void randomizeBoth() {
    paramSeed.store(nextSeed(), std::memory_order_relaxed);
    orderSeed.store(nextSeed(), std::memory_order_relaxed);
    paramsChanged.store(true, std::memory_order_release);
    orderChanged.store(true, std::memory_order_release);
  }

  // --- State (for persistence) ---

  [[nodiscard]] int getParamSeed() const noexcept {
    return paramSeed.load(std::memory_order_acquire);
  }

  [[nodiscard]] int getOrderSeed() const noexcept {
    return orderSeed.load(std::memory_order_acquire);
  }

  [[nodiscard]] double getTailLengthSeconds() const noexcept {
    if (paramSeed.load(std::memory_order_acquire) == 0)
      return 0.0;

    // State changes are applied on the next audio block. Until then, report a
    // safe baseline rather than telling the host there is no tail.
    return juce::jmax(30.0,
                      tailLengthSeconds.load(std::memory_order_relaxed));
  }

  void setState(int newParamSeed, int newOrderSeed) {
    // Only atomics cross from the host/message thread to the audio thread.
    // Parameter structures and effect instances are mutated exclusively by
    // applyPendingState() on the processing thread.
    paramSeed.store(newParamSeed, std::memory_order_relaxed);
    orderSeed.store(newOrderSeed, std::memory_order_relaxed);
    stateChanged.store(true, std::memory_order_release);
  }

  // --- Process ---

  void process(juce::AudioBuffer<float>& buffer,
               float timeMacro, float breathMacro,
               float orderMacro, float chaosMacro,
               float spaceMacro, float reflectionMacro,
               float fractureMacro, float wrathMacro) {
    applyPendingState();

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
  void applyPendingState() {
    if (stateChanged.exchange(false, std::memory_order_acq_rel)) {
      generateOrder(orderSeed.load(std::memory_order_acquire));
      const auto seed = paramSeed.load(std::memory_order_acquire);
      if (seed == 0)
        resetParamsToDefaults();
      else
        generateParams(seed);

      applyParamsToEffects();
      updateTailLength();
      reset();
      return;
    }

    if (orderChanged.exchange(false, std::memory_order_acq_rel)) {
      generateOrder(orderSeed.load(std::memory_order_acquire));
    }

    if (paramsChanged.exchange(false, std::memory_order_acq_rel)) {
      const auto seed = paramSeed.load(std::memory_order_acquire);
      if (seed == 0)
        resetParamsToDefaults();
      else
        generateParams(seed);

      applyParamsToEffects();
      updateTailLength();
    }
  }

  void resetParamsToDefaults() {
    delayParams = {};
    breathActive = false;
    reverbParams = {};
    compParams = {};
    chaosParams = {};
    spaceParams = {};
    reflectionParams = {};
    fractureParams = {};
    wrathParams = {};
    destroyActive = false;
    destroyParams = {};
  }

  // Non-zero random seed for a fresh randomization. The source of new seeds
  // need not be deterministic -- only regeneration *from* a seed must be.
  static int nextSeed() {
    auto& source = juce::Random::getSystemRandom();
    int seed = 0;
    while (seed == 0)
      seed = source.nextInt();
    return seed;
  }

  // Deterministic, platform-independent value generation. juce::Random has a
  // fixed algorithm, so a given seed always yields the same parameter set on
  // every machine and in both real-time and offline rendering.
  void generateParams(int seed) {
    juce::Random r(static_cast<juce::int64>(seed));

    // Time: 3 delays with individual params
    for (int d = 0; d < 3; ++d) {
      auto& delay = delayParams[static_cast<size_t>(d)];
      delay.time = 0.01f + r.nextFloat() * 0.99f;
      delay.feedback = r.nextFloat() * 0.90f;
      delay.mix = r.nextFloat();
    }

    // Breath: reverb (20% chance of being active)
    // Bias toward large rooms and low damping for long, spacious tails.
    breathActive = (r.nextFloat() < 0.2f);
    if (breathActive) {
      reverbParams.size = 0.55f + r.nextFloat() * 0.45f;
      reverbParams.damping = r.nextFloat() * 0.5f;
      reverbParams.wet = 0.3f + r.nextFloat() * 0.7f;
      reverbParams.width = 0.6f + r.nextFloat() * 0.4f;
    }

    // Order: compressor
    compParams.threshold = -60.0f + r.nextFloat() * 50.0f;
    compParams.ratio = 1.0f + r.nextFloat() * 19.0f;
    compParams.attack = 0.1f + r.nextFloat() * 99.9f;
    compParams.release = 1.0f + r.nextFloat() * 499.0f;

    // Chaos: pitch + ring + chorus + filter
    chaosParams.pitchSemitones = r.nextFloat() * 24.0f - 12.0f;
    chaosParams.pitchMix = r.nextFloat();
    chaosParams.ringFreq = 20.0f + r.nextFloat() * 1980.0f;
    chaosParams.ringMix = r.nextFloat();
    chaosParams.chorusRate = 0.1f + r.nextFloat() * 4.9f;
    chaosParams.chorusDepth = r.nextFloat();
    chaosParams.chorusMix = r.nextFloat();
    chaosParams.filterCutoff = 20.0f * std::pow(1000.0f, r.nextFloat());
    chaosParams.filterResonance = 0.1f + r.nextFloat() * 9.9f;
    chaosParams.filterType = r.nextInt(3);

    // Space: stereo widener + detuner
    spaceParams.width = 1.3f + r.nextFloat() * 1.2f;
    spaceParams.detune = 5.0f + r.nextFloat() * 25.0f;
    spaceParams.mix = 0.4f + r.nextFloat() * 0.6f;

    // Reflection: octave layering
    reflectionParams.upperSemitones = 5.0f + r.nextFloat() * 7.0f;
    reflectionParams.upperMix = 0.3f + r.nextFloat() * 0.7f;
    reflectionParams.lowerSemitones = -(5.0f + r.nextFloat() * 7.0f);
    reflectionParams.lowerMix = 0.3f + r.nextFloat() * 0.7f;

    // Fracture: comb filter bank
    for (int i = 0; i < 3; ++i) {
      auto& fracture = fractureParams[static_cast<size_t>(i)];
      fracture.frequency = 80.0f + r.nextFloat() * 1920.0f;
      fracture.feedback = 0.7f + r.nextFloat() * 0.28f;
      fracture.mix = 0.2f + r.nextFloat() * 0.8f;
    }

    // Wrath: coloured wavefolder
    wrathParams.drive = 3.0f + r.nextFloat() * 9.0f;
    wrathParams.folds = 2 + r.nextInt(4);
    wrathParams.symmetry = r.nextFloat();
    wrathParams.colour = 400.0f + r.nextFloat() * 2600.0f;
    wrathParams.mix = 0.5f + r.nextFloat() * 0.5f;

    // 10% chance of distortion/bitcrusher
    destroyActive = (r.nextFloat() < 0.1f);
    if (destroyActive) {
      destroyParams.drive = r.nextFloat();
      destroyParams.tone = 200.0f + r.nextFloat() * 7800.0f;
      destroyParams.distMix = r.nextFloat();
      destroyParams.bits = 4.0f + r.nextFloat() * 12.0f;
      destroyParams.crushRate = 1.0f + r.nextFloat() * 49.0f;
    }
  }

  void generateOrder(int seed) {
    for (int i = 0; i < NUM_GROUPS; ++i)
      groupOrder[static_cast<size_t>(i)] = i;

    // seed 0 == identity order (never randomized)
    if (seed != 0) {
      juce::Random r(static_cast<juce::int64>(seed));
      for (int i = NUM_GROUPS - 1; i > 0; --i) {
        const int j = r.nextInt(i + 1);
        std::swap(groupOrder[static_cast<size_t>(i)],
                  groupOrder[static_cast<size_t>(j)]);
      }
    }
  }

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
    for (size_t i = 0; i < combFilters.size(); ++i)
      combFilters[i].setParameters(fractureParams[i].frequency,
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

  void updateTailLength() {
    constexpr double silenceLevel = 0.001;  // -60 dB
    double seconds = 0.0;

    for (const auto& delay : delayParams) {
      if (delay.mix > 0.001f && delay.feedback > 0.001f) {
        seconds += static_cast<double>(delay.time) *
                   std::log(silenceLevel) /
                   std::log(static_cast<double>(delay.feedback));
      }
    }

    if (breathActive && reverbParams.wet > 0.001f)
      seconds = juce::jmax(seconds, 2.0 + 18.0 * reverbParams.size);

    for (const auto& comb : fractureParams) {
      if (comb.mix > 0.001f && comb.feedback > 0.001f) {
        const auto delaySeconds = 1.0 / static_cast<double>(comb.frequency);
        const auto decay = delaySeconds * std::log(silenceLevel) /
                           std::log(static_cast<double>(comb.feedback));
        seconds = juce::jmax(seconds, decay);
      }
    }

    // Avoid pathological host render extensions while still preserving the
    // long feedback/reverb tails that define the effect.
    tailLengthSeconds.store(juce::jlimit(0.0, 120.0, seconds),
                            std::memory_order_relaxed);
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
  std::atomic<int> paramSeed{0};
  std::atomic<int> orderSeed{0};
  std::atomic<double> tailLengthSeconds{0.0};
  std::atomic<bool> stateChanged{true};
  double sr = 44100.0;
};
}  // namespace pandoras_box
