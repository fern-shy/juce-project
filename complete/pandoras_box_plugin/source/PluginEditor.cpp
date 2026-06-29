namespace pandoras_box {

juce::Image PluginEditor::flipHorizontal(const juce::Image& src) {
  const int w = src.getWidth();
  const int h = src.getHeight();
  juce::Image flipped(src.getFormat(), w, h, true);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      flipped.setPixelAt(w - 1 - x, y, src.getPixelAt(x, y));
  return flipped;
}

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p),
      bypassAttachment{p.getParameterRefs().bypassed, bypassButton} {
  setSize(400, 400);

  background.setImage(juce::ImageCache::getFromMemory(
      assets::_3_02_26_png, assets::_3_02_26_pngSize));
  background.setInterceptsMouseClicks(false, false);
  addAndMakeVisible(background);

  // Glow sits above the background but below the eyes, so the halos read as
  // light bleeding out from behind each eye.
  glow.setGlowColour(juce::Colour{0xFFFF2A2A});
  addAndMakeVisible(glow);

  auto openImg = juce::ImageCache::getFromMemory(
      assets::_3_02_26_open_png, assets::_3_02_26_open_pngSize);
  auto closedImg = juce::ImageCache::getFromMemory(
      assets::_3_02_26_close_png, assets::_3_02_26_close_pngSize);

  auto openMirror = flipHorizontal(openImg);
  auto closedMirror = flipHorizontal(closedImg);

  // Left eye: randomize parameters
  paramsEye.setImages(false, true, true,
                      openImg, 1.0f, {},
                      openImg, 0.85f, {},
                      closedImg, 1.0f, {});
  paramsEye.onClick = [this]() { processorRef.randomizeParams(); };
  addAndMakeVisible(paramsEye);

  // Center eye: randomize both (larger, uses same art scaled up for now)
  bothEye.setImages(false, true, true,
                    openImg, 1.0f, {},
                    openImg, 0.85f, {},
                    closedImg, 1.0f, {});
  bothEye.onClick = [this]() { processorRef.randomizeBoth(); };
  addAndMakeVisible(bothEye);

  // Right eye: randomize order (mirrored)
  orderEye.setImages(false, true, true,
                     openMirror, 1.0f, {},
                     openMirror, 0.85f, {},
                     closedMirror, 1.0f, {});
  orderEye.onClick = [this]() { processorRef.randomizeOrder(); };
  addAndMakeVisible(orderEye);

  bypassButton.setColour(juce::ToggleButton::textColourId,
                          juce::Colour{0xFF5A2020});
  bypassButton.setColour(juce::ToggleButton::tickColourId,
                          juce::Colour{0xFF8B0000});
  bypassButton.setColour(juce::ToggleButton::tickDisabledColourId,
                          juce::Colour{0xFF5A2020});
  bypassButton.onClick = [this]() {
    bypassButton.setButtonText(bypassButton.getToggleState() ? "BYPASSED"
                                                             : "BYPASS");
  };
  bypassButton.onClick();
  addAndMakeVisible(bypassButton);

  startTimerHz(60);
}

PluginEditor::~PluginEditor() {
  stopTimer();
}

void PluginEditor::resized() {
  const auto bounds = getLocalBounds();
  background.setBounds(bounds);
  glow.setBounds(bounds);

  const int smallEye = 120;
  const int bigEye = 160;
  const int eyeY = bounds.getCentreY() - bigEye / 2;
  const int totalWidth = smallEye + 16 + bigEye + 16 + smallEye;
  const int startX = bounds.getCentreX() - totalWidth / 2;

  // Left eye (params) — vertically centered with center eye
  paramsEyeBase = {startX, eyeY + (bigEye - smallEye) / 2, smallEye, smallEye};

  // Center eye (both) — larger
  bothEyeBase = {startX + smallEye + 16, eyeY, bigEye, bigEye};

  // Right eye (order) — mirrored
  orderEyeBase = {startX + smallEye + 16 + bigEye + 16,
                  eyeY + (bigEye - smallEye) / 2, smallEye, smallEye};

  paramsEye.setBounds(paramsEyeBase);
  bothEye.setBounds(bothEyeBase);
  orderEye.setBounds(orderEyeBase);

  glow.setSpots(paramsEyeBase.getCentre().toFloat(),
                bothEyeBase.getCentre().toFloat(),
                orderEyeBase.getCentre().toFloat(),
                static_cast<float>(smallEye) * 0.55f);

  // Bypass bottom-right
  bypassButton.setBounds(bounds.getWidth() - 100, bounds.getHeight() - 30,
                          90, 24);
}

void PluginEditor::timerCallback() {
  // Map the raw RMS (typically small) into a usable 0..1 range, then apply a
  // fast attack / slow release so the glow strobes with transients.
  constexpr float levelGain = 4.0f;
  constexpr float releaseCoeff = 0.86f;

  const float raw =
      juce::jlimit(0.f, 1.f, processorRef.getOutputLevel() * levelGain);
  displayLevel = juce::jmax(raw, displayLevel * releaseCoeff);

  glow.setLevel(displayLevel);

  // Subtle pulse: scale each eye around its resting centre.
  const float scale = 1.0f + displayLevel * 0.14f;
  const auto pulse = [scale](juce::Rectangle<int> base) {
    return juce::Rectangle<int>{
        0, 0, juce::roundToInt(static_cast<float>(base.getWidth()) * scale),
        juce::roundToInt(static_cast<float>(base.getHeight()) * scale)}
        .withCentre(base.getCentre());
  };

  paramsEye.setBounds(pulse(paramsEyeBase));
  bothEye.setBounds(pulse(bothEyeBase));
  orderEye.setBounds(pulse(orderEyeBase));
}
}  // namespace pandoras_box
