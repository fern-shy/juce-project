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
  return 0.0;
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
}

bool PluginProcessor::hasEditor() const {
  return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
  return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
  juce::MemoryOutputStream outputStream{destData, true};
  JsonSerializer::serialize(parameters, outputStream);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
  juce::MemoryInputStream inputStream{data, static_cast<size_t>(sizeInBytes),
                                      false};
  const auto result = JsonSerializer::deserialize(inputStream, parameters);

  if (result.failed()) {
    DBG(result.getErrorMessage());
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
}  // namespace pandoras_box

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new pandoras_box::PluginProcessor();
}
