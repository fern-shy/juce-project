#pragma once

namespace pandoras_box {
class JsonSerializer {
public:
  static void serialize(const Parameters&, int paramSeed, int orderSeed,
                        juce::OutputStream&);

  /** @return Error message on failure; empty string otherwise.
   *           In case of error, no parameters are updated. */
  static juce::Result deserialize(juce::InputStream&, Parameters&,
                                  int& paramSeed, int& orderSeed);
};
}  // namespace pandoras_box
