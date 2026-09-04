#include <pandoras_box_plugin/pandoras_box_plugin.h>
#include <gtest/gtest.h>

namespace pandoras_box {
namespace {
juce::Result deserializeText(const juce::String& text, Parameters& parameters,
                             int& paramSeed, int& orderSeed) {
  juce::MemoryInputStream input{text.toRawUTF8(),
                                static_cast<size_t>(text.getNumBytesAsUTF8()),
                                false};
  return JsonSerializer::deserialize(input, parameters, paramSeed, orderSeed);
}
}  // namespace

TEST(JsonSerializer, RoundTripPreservesMacrosBypassAndSeeds) {
  PluginProcessor source;
  auto& sourceParams = source.getParameterRefs();
  sourceParams.time = 0.11f;
  sourceParams.breath = 0.22f;
  sourceParams.order = 0.33f;
  sourceParams.chaos = 0.44f;
  sourceParams.space = 0.55f;
  sourceParams.reflection = 0.66f;
  sourceParams.fracture = 0.77f;
  sourceParams.wrath = 0.88f;
  sourceParams.bypassed = true;

  juce::MemoryBlock block;
  juce::MemoryOutputStream output{block, false};
  JsonSerializer::serialize(sourceParams, 123456, -987654, output);
  output.flush();

  PluginProcessor destination;
  auto& destinationParams = destination.getParameterRefs();
  int paramSeed = 0;
  int orderSeed = 0;
  juce::MemoryInputStream input{block.getData(), block.getSize(), false};

  ASSERT_TRUE(JsonSerializer::deserialize(
                  input, destinationParams, paramSeed, orderSeed)
                  .wasOk());
  EXPECT_FLOAT_EQ(destinationParams.time.get(), 0.11f);
  EXPECT_FLOAT_EQ(destinationParams.breath.get(), 0.22f);
  EXPECT_FLOAT_EQ(destinationParams.order.get(), 0.33f);
  EXPECT_FLOAT_EQ(destinationParams.chaos.get(), 0.44f);
  EXPECT_FLOAT_EQ(destinationParams.space.get(), 0.55f);
  EXPECT_FLOAT_EQ(destinationParams.reflection.get(), 0.66f);
  EXPECT_FLOAT_EQ(destinationParams.fracture.get(), 0.77f);
  EXPECT_FLOAT_EQ(destinationParams.wrath.get(), 0.88f);
  EXPECT_TRUE(destinationParams.bypassed.get());
  EXPECT_EQ(paramSeed, 123456);
  EXPECT_EQ(orderSeed, -987654);
}

TEST(JsonSerializer, MigratesVersionFourWithDefaultSeeds) {
  const juce::String state = R"({
    "__version__": 4,
    "pluginName": "Pandoras Box",
    "time": 0.1,
    "breath": 0.2,
    "order": 0.3,
    "chaos": 0.4,
    "space": 0.5,
    "reflection": 0.6,
    "fracture": 0.7,
    "wrath": 0.8,
    "bypassed": true
  })";

  PluginProcessor processor;
  int paramSeed = 99;
  int orderSeed = 100;
  ASSERT_TRUE(deserializeText(state, processor.getParameterRefs(),
                              paramSeed, orderSeed)
                  .wasOk());
  EXPECT_FLOAT_EQ(processor.getParameterRefs().wrath.get(), 0.8f);
  EXPECT_TRUE(processor.getParameterRefs().bypassed.get());
  EXPECT_EQ(paramSeed, 0);
  EXPECT_EQ(orderSeed, 0);
}

TEST(JsonSerializer, RejectsInvalidStateWithoutChangingValues) {
  PluginProcessor processor;
  auto& parameters = processor.getParameterRefs();
  parameters.time = 0.25f;
  parameters.bypassed = true;
  int paramSeed = 111;
  int orderSeed = 222;

  const juce::String wrongVersion = R"({
    "__version__": 999,
    "pluginName": "Pandoras Box"
  })";
  EXPECT_TRUE(deserializeText(wrongVersion, parameters,
                              paramSeed, orderSeed)
                  .failed());
  EXPECT_FLOAT_EQ(parameters.time.get(), 0.25f);
  EXPECT_TRUE(parameters.bypassed.get());
  EXPECT_EQ(paramSeed, 111);
  EXPECT_EQ(orderSeed, 222);

  const juce::String wrongPlugin = R"({
    "__version__": 5,
    "pluginName": "Another Plugin"
  })";
  EXPECT_TRUE(deserializeText(wrongPlugin, parameters,
                              paramSeed, orderSeed)
                  .failed());
  EXPECT_FLOAT_EQ(parameters.time.get(), 0.25f);
  EXPECT_EQ(paramSeed, 111);
  EXPECT_EQ(orderSeed, 222);
}

TEST(JsonSerializer, RejectsOutOfRangeMacroWithoutChangingValues) {
  PluginProcessor processor;
  auto& parameters = processor.getParameterRefs();
  parameters.time = 0.25f;
  int paramSeed = 111;
  int orderSeed = 222;

  const juce::String state = R"({
    "__version__": 5,
    "pluginName": "Pandoras Box",
    "time": 2.0,
    "breath": 0.5,
    "order": 0.5,
    "chaos": 0.5,
    "space": 0.5,
    "reflection": 0.5,
    "fracture": 0.5,
    "wrath": 0.5,
    "bypassed": false,
    "paramSeed": 1,
    "orderSeed": 2
  })";

  EXPECT_TRUE(deserializeText(state, parameters, paramSeed, orderSeed).failed());
  EXPECT_FLOAT_EQ(parameters.time.get(), 0.25f);
  EXPECT_EQ(paramSeed, 111);
  EXPECT_EQ(orderSeed, 222);
}
}  // namespace pandoras_box
