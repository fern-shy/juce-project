#pragma once

namespace pandoras_box {
class PluginEditor : public juce::AudioProcessorEditor {
public:
  explicit PluginEditor(PluginProcessor&);
  ~PluginEditor() override;

  void resized() override;

private:
  PluginProcessor& processorRef;

  juce::ImageComponent background;

  // Left eye: randomize parameters
  juce::ImageButton paramsEye;
  // Center eye: randomize both (placeholder until custom art)
  juce::ImageButton bothEye;
  // Right eye: randomize order (mirrored)
  juce::ImageButton orderEye;

  juce::ToggleButton bypassButton{"BYPASS"};
  juce::ButtonParameterAttachment bypassAttachment;

  static juce::Image flipHorizontal(const juce::Image& src);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
}  // namespace pandoras_box
