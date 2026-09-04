#include <pandoras_box_plugin/pandoras_box_plugin.h>
#include <gtest/gtest.h>
#include <cmath>

namespace pandoras_box {
namespace {
void fillImpulse(juce::AudioBuffer<float>& buffer) {
  buffer.clear();
  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    buffer.setSample(channel, 0, channel == 0 ? 0.25f : -0.25f);
}

void processAllMacros(EffectChain& chain, juce::AudioBuffer<float>& buffer) {
  chain.process(buffer, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f);
}
}  // namespace

TEST(EffectChain, SameSeedsProduceSameFirstBlock) {
  EffectChain first;
  EffectChain second;
  first.prepare(48000.0, 256, 2);
  second.prepare(48000.0, 256, 2);
  first.setState(1234567, 7654321);
  second.setState(1234567, 7654321);

  juce::AudioBuffer<float> firstBuffer(2, 256);
  juce::AudioBuffer<float> secondBuffer(2, 256);
  fillImpulse(firstBuffer);
  fillImpulse(secondBuffer);
  processAllMacros(first, firstBuffer);
  processAllMacros(second, secondBuffer);

  for (int channel = 0; channel < 2; ++channel)
    for (int sample = 0; sample < 256; ++sample)
      EXPECT_FLOAT_EQ(firstBuffer.getSample(channel, sample),
                      secondBuffer.getSample(channel, sample));
}

TEST(EffectChain, SeedZeroRestoresFactoryDefaults) {
  EffectChain reused;
  reused.prepare(48000.0, 256, 2);
  reused.setState(1234567, 7654321);
  juce::AudioBuffer<float> discarded(2, 256);
  fillImpulse(discarded);
  processAllMacros(reused, discarded);
  reused.reset();
  reused.setState(0, 0);

  EffectChain fresh;
  fresh.prepare(48000.0, 256, 2);

  juce::AudioBuffer<float> reusedBuffer(2, 256);
  juce::AudioBuffer<float> freshBuffer(2, 256);
  fillImpulse(reusedBuffer);
  fillImpulse(freshBuffer);
  processAllMacros(reused, reusedBuffer);
  processAllMacros(fresh, freshBuffer);

  for (int channel = 0; channel < 2; ++channel)
    for (int sample = 0; sample < 256; ++sample)
      EXPECT_FLOAT_EQ(reusedBuffer.getSample(channel, sample),
                      freshBuffer.getSample(channel, sample));
}

TEST(EffectChain, FactoryFractureSettingsAreDry) {
  EffectChain fractureEnabled;
  EffectChain fractureDisabled;
  fractureEnabled.prepare(48000.0, 64, 2);
  fractureDisabled.prepare(48000.0, 64, 2);
  juce::AudioBuffer<float> enabledBuffer(2, 64);
  juce::AudioBuffer<float> disabledBuffer(2, 64);
  fillImpulse(enabledBuffer);
  fillImpulse(disabledBuffer);

  fractureEnabled.process(enabledBuffer, 0.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, 0.0f, 1.0f, 0.0f);
  fractureDisabled.process(disabledBuffer, 0.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 0.0f, 0.0f);

  for (int channel = 0; channel < 2; ++channel)
    for (int sample = 0; sample < enabledBuffer.getNumSamples(); ++sample)
      EXPECT_FLOAT_EQ(enabledBuffer.getSample(channel, sample),
                      disabledBuffer.getSample(channel, sample));
}

TEST(EffectChain, RandomizedOutputRemainsFinite) {
  EffectChain chain;
  chain.prepare(192000.0, 512, 2);
  chain.setState(-123456, 987654);
  juce::AudioBuffer<float> buffer(2, 512);
  fillImpulse(buffer);
  processAllMacros(chain, buffer);

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
      EXPECT_TRUE(std::isfinite(buffer.getSample(channel, sample)));
}
}  // namespace pandoras_box
