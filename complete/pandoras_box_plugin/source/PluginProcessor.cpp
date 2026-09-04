namespace pandoras_box {
PluginProcessor::PluginProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

const juce::String PluginProcessor::getName() const {
  return FERNSHY_PANDORAS_BOX_PLUGIN_NAME;
}

bool PluginProcessor::acceptsMidi() const {
  return false;
}

bool PluginProcessor::producesMidi() const {
  return false;
}

bool PluginProcessor::isMidiEffect() const {
  return false;
}

double PluginProcessor::getTailLengthSeconds() const {
  return effectChain.getTailLengthSeconds();
}

int PluginProcessor::getNumPrograms() {
  return 1;
}

int PluginProcessor::getCurrentProgram() {
  return 0;
}

void PluginProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String PluginProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void PluginProcessor::changeProgramName(int index,
                                        const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

void PluginProcessor::prepareToPlay(double sampleRate,
                                    int expectedMaxFramesPerBlock) {
  currentSampleRate = sampleRate;

  const auto numChannels = static_cast<uint32_t>(
      juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels()));

  effectChain.prepare(sampleRate, expectedMaxFramesPerBlock,
                      static_cast<int>(numChannels));

  bypassTransitionSmoother.prepare(
      {.sampleRate = sampleRate,
       .maximumBlockSize = static_cast<uint32_t>(expectedMaxFramesPerBlock),
       .numChannels = numChannels});
}

void PluginProcessor::releaseResources() {
  effectChain.reset();
  bypassTransitionSmoother.reset();
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }

  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) {
    return false;
  }

  return true;
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;
  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (const auto channelToClear :
       std::views::iota(totalNumInputChannels, totalNumOutputChannels)) {
    buffer.clear(channelToClear, 0, buffer.getNumSamples());
  }

  const auto bypassedAndNotTransitioning =
      parameters.bypassed.get() && !bypassTransitionSmoother.isTransitioning();

  bypassTransitionSmoother.setBypass(parameters.bypassed);

  if (bypassedAndNotTransitioning) {
    updateOutputLevel(buffer);
    return;
  }

  bypassTransitionSmoother.setDryBuffer(buffer);

  effectChain.process(buffer,
                      parameters.time.get(),
                      parameters.breath.get(),
                      parameters.order.get(),
                      parameters.chaos.get(),
                      parameters.space.get(),
                      parameters.reflection.get(),
                      parameters.fracture.get(),
                      parameters.wrath.get());

  bypassTransitionSmoother.mixToWetBuffer(buffer);

  updateOutputLevel(buffer);
}

bool PluginProcessor::hasEditor() const {
  return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
  return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::MemoryOutputStream outputStream{destData, true};
  JsonSerializer::serialize(parameters, effectChain.getParamSeed(),
                            effectChain.getOrderSeed(), outputStream);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
  juce::MemoryInputStream inputStream{data, static_cast<size_t>(sizeInBytes),
                                      false};
  int paramSeed = 0;
  int orderSeed = 0;
  const auto result =
      JsonSerializer::deserialize(inputStream, parameters, paramSeed, orderSeed);

  if (result.failed()) {
    juce::Logger::writeToLog("Pandoras Box: state restore failed: " +
                             result.getErrorMessage());
  } else {
    // Reproduce the exact randomized state that was saved.
    effectChain.setState(paramSeed, orderSeed);
  }

  bypassTransitionSmoother.setBypassForced(parameters.bypassed);
}

Parameters& PluginProcessor::getParameterRefs() noexcept {
  return parameters;
}

juce::AudioProcessorParameter* PluginProcessor::getBypassParameter()
    const noexcept {
  return &parameters.bypassed;
}

void PluginProcessor::randomizeParams() {
  effectChain.randomizeParams();
}

void PluginProcessor::randomizeOrder() {
  effectChain.randomizeOrder();
}

void PluginProcessor::randomizeBoth() {
  effectChain.randomizeBoth();
}

double PluginProcessor::getSampleRateThreadSafe() const noexcept {
  return currentSampleRate;
}

float PluginProcessor::getOutputLevel() const noexcept {
  return outputLevel.load(std::memory_order_relaxed);
}

void PluginProcessor::updateOutputLevel(
    const juce::AudioBuffer<float>& buffer) noexcept {
  const auto numChannels = buffer.getNumChannels();
  const auto numSamples = buffer.getNumSamples();

  if (numChannels <= 0 || numSamples <= 0) {
    return;
  }

  float sum = 0.f;
  for (int channel = 0; channel < numChannels; ++channel) {
    sum += buffer.getRMSLevel(channel, 0, numSamples);
  }

  outputLevel.store(sum / static_cast<float>(numChannels),
                    std::memory_order_relaxed);
}
}  // namespace pandoras_box

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new pandoras_box::PluginProcessor();
}
