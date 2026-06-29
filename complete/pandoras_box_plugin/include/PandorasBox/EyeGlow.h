#pragma once

namespace pandoras_box {
// Transparent overlay drawn behind the eyes. Renders a soft radial halo at each
// eye centre whose size/brightness tracks the output level, producing a
// volume-reactive strobe/glow. Purely cosmetic and never intercepts the mouse.
class EyeGlow : public juce::Component {
public:
  EyeGlow() { setInterceptsMouseClicks(false, false); }

  void setSpots(juce::Point<float> left, juce::Point<float> centre,
                juce::Point<float> right, float radius) {
    spots[0] = left;
    spots[1] = centre;
    spots[2] = right;
    baseRadius = radius;
    repaint();
  }

  void setLevel(float newLevel) {
    level = juce::jlimit(0.f, 1.f, newLevel);
    repaint();
  }

  void setGlowColour(juce::Colour colour) { glowColour = colour; }

  void paint(juce::Graphics& g) override {
    if (level <= 0.001f || baseRadius <= 0.f) {
      return;
    }

    for (const auto& centre : spots) {
      const float radius = baseRadius * (0.6f + level * 1.4f);
      const float alpha = juce::jlimit(0.f, 0.85f, level * 0.85f);

      juce::ColourGradient gradient{
          glowColour.withAlpha(alpha), centre.x, centre.y,
          glowColour.withAlpha(0.f), centre.x + radius, centre.y, true};
      g.setGradientFill(gradient);
      g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.f,
                    radius * 2.f);
    }
  }

private:
  std::array<juce::Point<float>, 3> spots{};
  float baseRadius = 0.f;
  float level = 0.f;
  juce::Colour glowColour{0xFFFF2A2A};
};
}  // namespace pandoras_box
