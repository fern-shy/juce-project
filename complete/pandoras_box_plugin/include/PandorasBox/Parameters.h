#pragma once

namespace pandoras_box {
struct Parameters {
  explicit Parameters(juce::AudioProcessor&);

  juce::AudioParameterFloat& time;
  juce::AudioParameterFloat& breath;
  juce::AudioParameterFloat& order;
  juce::AudioParameterFloat& chaos;
  juce::AudioParameterFloat& space;
  juce::AudioParameterFloat& reflection;
  juce::AudioParameterFloat& fracture;
  juce::AudioParameterFloat& wrath;
  juce::AudioParameterBool& bypassed;

  JUCE_DECLARE_NON_COPYABLE(Parameters)
  JUCE_DECLARE_NON_MOVEABLE(Parameters)
};
}  // namespace pandoras_box
