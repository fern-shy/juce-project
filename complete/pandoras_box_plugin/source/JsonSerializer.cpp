namespace {
struct SerializableParameters {
  float time;
  float breath;
  float order;
  float chaos;
  float space;
  float reflection;
  float fracture;
  float wrath;
  bool bypassed;

  static constexpr auto marshallingVersion = 4;

  template <typename Archive, typename T>
  static void serialise(Archive& archive, T& p) {
    using namespace juce;

    if (archive.getVersion() != 4) {
      return;
    }

    std::string pluginName = FERNSHY_PANDORAS_BOX_PLUGIN_NAME;

    archive(named("pluginName", pluginName));

    if (pluginName != FERNSHY_PANDORAS_BOX_PLUGIN_NAME) {
      return;
    }

    archive(named("time", p.time),
            named("breath", p.breath),
            named("order", p.order),
            named("chaos", p.chaos),
            named("space", p.space),
            named("reflection", p.reflection),
            named("fracture", p.fracture),
            named("wrath", p.wrath),
            named("bypassed", p.bypassed));
  }
};

SerializableParameters from(const pandoras_box::Parameters& p) {
  return {
      .time = p.time.get(),
      .breath = p.breath.get(),
      .order = p.order.get(),
      .chaos = p.chaos.get(),
      .space = p.space.get(),
      .reflection = p.reflection.get(),
      .fracture = p.fracture.get(),
      .wrath = p.wrath.get(),
      .bypassed = p.bypassed.get(),
  };
}
}  // namespace

namespace pandoras_box {
void JsonSerializer::serialize(const Parameters& parameters,
                               juce::OutputStream& output) {
  const auto json = juce::ToVar::convert(from(parameters));

  if (!json.has_value()) {
    return;
  }

  juce::JSON::writeToStream(output, *json,
                            juce::JSON::FormatOptions{}
                                .withSpacing(juce::JSON::Spacing::multiLine)
                                .withMaxDecimalPlaces(2));
}

juce::Result JsonSerializer::deserialize(juce::InputStream& input,
                                         Parameters& parameters) {
  juce::var parsedResult;
  auto parsingResult =
      juce::JSON::parse(input.readEntireStreamAsString(), parsedResult);

  if (parsingResult.failed()) {
    return parsingResult;
  }

  const auto parsedParameters =
      juce::FromVar::convert<SerializableParameters>(parsedResult);

  if (!parsedParameters.has_value()) {
    return juce::Result::fail(
        "failed to parse parameters from JSON representation");
  }

  parameters.time = parsedParameters->time;
  parameters.breath = parsedParameters->breath;
  parameters.order = parsedParameters->order;
  parameters.chaos = parsedParameters->chaos;
  parameters.space = parsedParameters->space;
  parameters.reflection = parsedParameters->reflection;
  parameters.fracture = parsedParameters->fracture;
  parameters.wrath = parsedParameters->wrath;
  parameters.bypassed = parsedParameters->bypassed;

  return juce::Result::ok();
}
}  // namespace pandoras_box
