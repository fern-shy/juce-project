
namespace pandoras_box {
namespace {
auto& addParameterToProcessor(juce::AudioProcessor& processor, auto parameter) {
  auto& result = *parameter;
  processor.addParameter(parameter.release());
  return result;
}

juce::AudioParameterFloat& createMacroParameter(
    juce::AudioProcessor& processor,
    const juce::String& id, const juce::String& name) {
  constexpr auto versionHint = 2;
  return addParameterToProcessor(
      processor,
      std::make_unique<juce::AudioParameterFloat>(
          juce::ParameterID{id, versionHint}, name,
          juce::NormalisableRange<float>{0.0f, 1.0f, 0.01f}, 0.5f));
}

juce::AudioParameterBool& createBypassedParameter(
    juce::AudioProcessor& processor) {
  constexpr auto versionHint = 1;
  return addParameterToProcessor(
      processor,
      std::make_unique<juce::AudioParameterBool>(
          juce::ParameterID{"bypassed", versionHint}, "Bypass", false));
}
}  // namespace

Parameters::Parameters(juce::AudioProcessor& processor)
    : time{createMacroParameter(processor, "macro.time", "Time")},
      breath{createMacroParameter(processor, "macro.breath", "Breath")},
      order{createMacroParameter(processor, "macro.order", "Order")},
      chaos{createMacroParameter(processor, "macro.chaos", "Chaos")},
      space{createMacroParameter(processor, "macro.space", "Space")},
      reflection{createMacroParameter(processor, "macro.reflection", "Reflection")},
      fracture{createMacroParameter(processor, "macro.fracture", "Fracture")},
      wrath{createMacroParameter(processor, "macro.wrath", "Wrath")},
      bypassed{createBypassedParameter(processor)} {}
}  // namespace pandoras_box
