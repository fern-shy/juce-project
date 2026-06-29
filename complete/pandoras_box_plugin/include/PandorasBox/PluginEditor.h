#pragma once

namespace pandoras_box {
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer {
public:
  explicit PluginEditor(PluginProcessor&);
  ~PluginEditor() override;

  void resized() override;

private:
  void timerCallback() override;

  PluginProcessor& processorRef;

  juce::ImageComponent background;

  // Volume-reactive glow drawn behind the eyes.
  EyeGlow glow;

  // Left eye: randomize parameters
  juce::ImageButton paramsEye;
  // Center eye: randomize both (placeholder until custom art)
  juce::ImageButton bothEye;
  // Right eye: randomize order (mirrored)
  juce::ImageButton orderEye;

  juce::ToggleButton bypassButton{"BYPASS"};
  juce::ButtonParameterAttachment bypassAttachment;

  // Resting (un-pulsed) eye bounds, used as the base for the pulse animation.
  juce::Rectangle<int> paramsEyeBase;
  juce::Rectangle<int> bothEyeBase;
  juce::Rectangle<int> orderEyeBase;

  // Smoothed level driving the glow/pulse (fast attack, slow release).
  float displayLevel = 0.f;

  static juce::Image flipHorizontal(const juce::Image& src);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
}  // namespace pandoras_box
