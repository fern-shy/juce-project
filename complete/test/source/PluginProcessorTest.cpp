#include <pandoras_box_plugin/pandoras_box_plugin.h>
#include <gtest/gtest.h>
#include <cmath>
#include <cstring>
#include <vector>

namespace pandoras_box {
namespace {
constexpr double sampleRate = 48000.0;
constexpr int renderSamples = 4096;

juce::MemoryBlock makeState(int paramSeed, int orderSeed,
                            bool bypassed = false) {
  PluginProcessor source;
  source.getParameterRefs().bypassed = bypassed;
  juce::MemoryBlock state;
  juce::MemoryOutputStream output{state, false};
  JsonSerializer::serialize(source.getParameterRefs(), paramSeed, orderSeed,
                            output);
  output.flush();
  return state;
}

std::vector<float> render(const juce::MemoryBlock& state, int blockSize) {
  PluginProcessor processor;
  processor.prepareToPlay(sampleRate, blockSize);
  processor.setStateInformation(state.getData(),
                                static_cast<int>(state.getSize()));

  std::vector<float> result(static_cast<size_t>(renderSamples * 2));
  juce::MidiBuffer midi;

  for (int offset = 0; offset < renderSamples; offset += blockSize) {
    const int samples = juce::jmin(blockSize, renderSamples - offset);
    juce::AudioBuffer<float> block(2, samples);

    for (int channel = 0; channel < 2; ++channel) {
      auto* data = block.getWritePointer(channel);
      for (int i = 0; i < samples; ++i) {
        const auto phase =
            static_cast<float>(offset + i) * 440.0f /
            static_cast<float>(sampleRate);
        data[i] = 0.1f *
                  std::sin(juce::MathConstants<float>::twoPi * phase +
                           static_cast<float>(channel) * 0.2f);
      }
    }

    processor.processBlock(block, midi);
    for (int channel = 0; channel < 2; ++channel) {
      const auto* data = block.getReadPointer(channel);
      for (int i = 0; i < samples; ++i) {
        result[static_cast<size_t>((offset + i) * 2 + channel)] = data[i];
      }
    }
  }

  return result;
}
}  // namespace

TEST(PluginProcessor, StateRoundTripPreservesRandomizationSeeds) {
  PluginProcessor source;
  source.prepareToPlay(sampleRate, 128);
  source.randomizeBoth();

  juce::MemoryBlock state;
  source.getStateInformation(state);

  PluginProcessor restored;
  restored.prepareToPlay(sampleRate, 128);
  restored.setStateInformation(state.getData(),
                               static_cast<int>(state.getSize()));

  juce::MemoryBlock restoredState;
  restored.getStateInformation(restoredState);
  EXPECT_EQ(state.getSize(), restoredState.getSize());
  EXPECT_EQ(std::memcmp(state.getData(), restoredState.getData(),
                        state.getSize()),
            0);
}

TEST(PluginProcessor, FixedStateRendersDeterministically) {
  const auto state = makeState(1234567, 7654321);
  const auto first = render(state, 128);
  const auto second = render(state, 128);
  ASSERT_EQ(first.size(), second.size());

  for (size_t i = 0; i < first.size(); ++i)
    EXPECT_FLOAT_EQ(first[i], second[i]);
}

TEST(PluginProcessor, RenderIsStableAcrossBlockSizes) {
  const auto state = makeState(1234567, 7654321);
  const auto smallBlocks = render(state, 64);
  const auto largeBlocks = render(state, 512);
  ASSERT_EQ(smallBlocks.size(), largeBlocks.size());

  for (size_t i = 0; i < smallBlocks.size(); ++i)
    EXPECT_NEAR(smallBlocks[i], largeBlocks[i], 1.0e-4f);
}

TEST(PluginProcessor, RandomizedStateReportsNonZeroTailImmediately) {
  PluginProcessor processor;
  processor.prepareToPlay(sampleRate, 128);
  const auto state = makeState(1234567, 7654321);
  processor.setStateInformation(state.getData(),
                                static_cast<int>(state.getSize()));
  EXPECT_GE(processor.getTailLengthSeconds(), 30.0);
}

TEST(PluginProcessor, DefaultStateReportsNoTail) {
  PluginProcessor processor;
  processor.prepareToPlay(sampleRate, 128);
  const auto state = makeState(0, 0);
  processor.setStateInformation(state.getData(),
                                static_cast<int>(state.getSize()));
  EXPECT_DOUBLE_EQ(processor.getTailLengthSeconds(), 0.0);
}

TEST(PluginProcessor, RapidRandomizationAlwaysProducesFiniteAudio) {
  PluginProcessor processor;
  processor.prepareToPlay(sampleRate, 256);
  juce::AudioBuffer<float> block(2, 256);
  juce::MidiBuffer midi;

  for (int iteration = 0; iteration < 100; ++iteration) {
    processor.randomizeBoth();
    block.clear();
    block.setSample(0, 0, 0.25f);
    block.setSample(1, 0, -0.25f);
    processor.processBlock(block, midi);

    for (int channel = 0; channel < block.getNumChannels(); ++channel) {
      const auto* data = block.getReadPointer(channel);
      for (int i = 0; i < block.getNumSamples(); ++i)
        EXPECT_TRUE(std::isfinite(data[i]));
    }
  }
}

TEST(PluginProcessor, SupportsAndProcessesMono) {
  PluginProcessor processor;
  juce::AudioProcessor::BusesLayout monoLayout;
  monoLayout.inputBuses.add(juce::AudioChannelSet::mono());
  monoLayout.outputBuses.add(juce::AudioChannelSet::mono());
  ASSERT_TRUE(processor.isBusesLayoutSupported(monoLayout));
  ASSERT_TRUE(processor.setBusesLayout(monoLayout));

  processor.prepareToPlay(sampleRate, 128);
  const auto state = makeState(24680, 13579);
  processor.setStateInformation(state.getData(),
                                static_cast<int>(state.getSize()));

  juce::AudioBuffer<float> block(1, 128);
  block.clear();
  block.setSample(0, 0, 0.25f);
  juce::MidiBuffer midi;
  processor.processBlock(block, midi);

  for (int i = 0; i < block.getNumSamples(); ++i)
    EXPECT_TRUE(std::isfinite(block.getSample(0, i)));
}
}  // namespace pandoras_box
