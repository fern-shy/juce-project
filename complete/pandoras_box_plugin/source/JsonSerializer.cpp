namespace {
struct SerializableParameters {
  float time = 0.5f;
  float breath = 0.5f;
  float order = 0.5f;
  float chaos = 0.5f;
  float space = 0.5f;
  float reflection = 0.5f;
  float fracture = 0.5f;
  float wrath = 0.5f;
  bool bypassed = false;
  int paramSeed = 0;
  int orderSeed = 0;

  static constexpr auto marshallingVersion = 5;

  template <typename Archive, typename T>
  static void serialise(Archive& archive, T& p) {
    using namespace juce;

    std::string pluginName = FERNSHY_PANDORAS_BOX_PLUGIN_NAME;

    archive(named("pluginName", pluginName));

    archive(named("time", p.time),
            named("breath", p.breath),
            named("order", p.order),
            named("chaos", p.chaos),
            named("space", p.space),
            named("reflection", p.reflection),
            named("fracture", p.fracture),
            named("wrath", p.wrath),
            named("bypassed", p.bypassed),
            named("paramSeed", p.paramSeed),
            named("orderSeed", p.orderSeed));
  }
};

struct SerializableParametersV4 {
  float time = 0.5f;
  float breath = 0.5f;
  float order = 0.5f;
  float chaos = 0.5f;
  float space = 0.5f;
  float reflection = 0.5f;
  float fracture = 0.5f;
  float wrath = 0.5f;
  bool bypassed = false;

  static constexpr auto marshallingVersion = 4;

  template <typename Archive, typename T>
  static void serialise(Archive& archive, T& p) {
    using namespace juce;
    std::string pluginName = FERNSHY_PANDORAS_BOX_PLUGIN_NAME;
    archive(named("pluginName", pluginName),
            named("time", p.time),
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

SerializableParameters from(const pandoras_box::Parameters& p, int paramSeed,
                            int orderSeed) {
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
      .paramSeed = paramSeed,
      .orderSeed = orderSeed,
  };
}

SerializableParameters migrate(const SerializableParametersV4& p) {
  return {
      .time = p.time,
      .breath = p.breath,
      .order = p.order,
      .chaos = p.chaos,
      .space = p.space,
      .reflection = p.reflection,
      .fracture = p.fracture,
      .wrath = p.wrath,
      .bypassed = p.bypassed,
      .paramSeed = 0,
      .orderSeed = 0,
  };
}
}  // namespace

namespace pandoras_box {
void JsonSerializer::serialize(const Parameters& parameters, int paramSeed,
                               int orderSeed, juce::OutputStream& output) {
  const auto json =
      juce::ToVar::convert(from(parameters, paramSeed, orderSeed));

  if (!json.has_value()) {
    juce::Logger::writeToLog("Pandoras Box: failed to serialize plugin state");
    jassertfalse;
    return;
  }

  juce::JSON::writeToStream(output, *json,
                            juce::JSON::FormatOptions{}
                                .withSpacing(juce::JSON::Spacing::multiLine)
                                .withMaxDecimalPlaces(2));
}

juce::Result JsonSerializer::deserialize(juce::InputStream& input,
                                         Parameters& parameters,
                                         int& paramSeed, int& orderSeed) {
  juce::var parsedResult;
  auto parsingResult =
      juce::JSON::parse(input.readEntireStreamAsString(), parsedResult);

  if (parsingResult.failed()) {
    return parsingResult;
  }

  if (!parsedResult.isObject()) {
    return juce::Result::fail("plugin state must be a JSON object");
  }

  const auto pluginName =
      parsedResult.getProperty("pluginName", juce::var{}).toString();
  if (pluginName != FERNSHY_PANDORAS_BOX_PLUGIN_NAME) {
    return juce::Result::fail("plugin state belongs to a different plugin");
  }

  const auto versionValue =
      parsedResult.getProperty("__version__", juce::var{});
  if (!versionValue.isInt() && !versionValue.isInt64()) {
    return juce::Result::fail("plugin state has no valid version");
  }

  const auto version = static_cast<int>(versionValue);
  SerializableParameters decoded;

  if (version == SerializableParameters::marshallingVersion) {
    const auto converted =
        juce::FromVar::convert<SerializableParameters>(parsedResult);
    if (!converted.has_value()) {
      return juce::Result::fail(
          "failed to parse version 5 plugin state");
    }
    decoded = *converted;
  } else if (version == SerializableParametersV4::marshallingVersion) {
    const auto converted =
        juce::FromVar::convert<SerializableParametersV4>(parsedResult);
    if (!converted.has_value()) {
      return juce::Result::fail(
          "failed to parse version 4 plugin state");
    }
    decoded = migrate(*converted);
  } else {
    return juce::Result::fail("unsupported plugin state version");
  }

  const auto validMacro = [](float value) {
    return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
  };
  if (!validMacro(decoded.time) || !validMacro(decoded.breath) ||
      !validMacro(decoded.order) || !validMacro(decoded.chaos) ||
      !validMacro(decoded.space) || !validMacro(decoded.reflection) ||
      !validMacro(decoded.fracture) || !validMacro(decoded.wrath)) {
    return juce::Result::fail(
        "plugin state contains an invalid macro value");
  }

  parameters.time = decoded.time;
  parameters.breath = decoded.breath;
  parameters.order = decoded.order;
  parameters.chaos = decoded.chaos;
  parameters.space = decoded.space;
  parameters.reflection = decoded.reflection;
  parameters.fracture = decoded.fracture;
  parameters.wrath = decoded.wrath;
  parameters.bypassed = decoded.bypassed;
  paramSeed = decoded.paramSeed;
  orderSeed = decoded.orderSeed;

  return juce::Result::ok();
}
}  // namespace pandoras_box
