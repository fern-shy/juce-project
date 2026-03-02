#pragma once
#include <juce_core/juce_core.h>

namespace pandoras_box {
inline std::string getFileOutputPath(juce::StringRef fileName) {
  return juce::File::getSpecialLocation(
             juce::File::SpecialLocationType::currentExecutableFile)
      .getParentDirectory()
      .getChildFile(fileName)
      .getFullPathName()
      .toStdString();
}
}  // namespace pandoras_box
