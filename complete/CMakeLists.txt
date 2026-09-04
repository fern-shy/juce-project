# Mandatory line, sets the minimum version of CMake that should be used with this repository.
# 3.22 is the minimum CMake version required by JUCE.
# 3.25 is required for the MSVC_DEBUG_INFORMATION_FORMAT feature to work.
# To verify your version run
# $ cmake --version
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Enable setting CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
endif()

# Sets a few variables, like PROJECT_NAME.
project(FernShyPandorasBoxPlugin VERSION 1.0.1 LANGUAGES C CXX)

# Set the C++ standard only if this project is standalone;
# otherwise, let clients decide.
if(PROJECT_IS_TOP_LEVEL)
  if (NOT CMAKE_CXX_STANDARD)
    # Always use the newest C++ standard on green-field projects if possible.
    set(CMAKE_CXX_STANDARD 23)
  endif()

  # Links to C++ runtime statically
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # Embeds debug information
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>" CACHE STRING "" FORCE)
endif()

# I like to download the dependencies to the same folder as the project.
# If you want to install them system-wide, set CPM_SOURCE_CACHE with a path to the dependencies
# either as an environment variable or pass it with -DCPM_SOURCE_CACHE=<path> when invoking cmake.
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# Downloads CPM if not already downloaded. CPM is an easy-to-use package manager nicely integrated with CMake.
include(cmake/cpm.cmake)

# This commands downloads AND configures JUCE. It sets up some variables, like JUCE_SOURCE_DIR.
cpmaddpackage(
  NAME
    JUCE
  GIT_TAG
    8.0.12
  GITHUB_REPOSITORY
    juce-framework/JUCE
  GIT_SHALLOW
    TRUE
)

# Adds compiler warning utilities & other.
include(cmake/CompilerWarnings.cmake)
include(cmake/Util.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Sets the Windows VST3 installation location to one
  # that doesn't require elevated permissions
  set_directory_properties(PROPERTIES JUCE_VST3_COPY_DIR "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
endif()

# Adds a plugin target.
set(FERNSHY_PANDORAS_BOX_PLUGIN_NAME "Pandoras Box")
set(FERNSHY_PANDORAS_BOX_FORMATS VST3)
if(APPLE)
  list(APPEND FERNSHY_PANDORAS_BOX_FORMATS AU)
endif()

option(PANDORAS_BOX_COPY_PLUGIN_AFTER_BUILD
       "Install development builds into the current user's plugin folders" ON)
juce_add_plugin(
  FernShyPandorasBoxPlugin
  PRODUCT_NAME
    ${FERNSHY_PANDORAS_BOX_PLUGIN_NAME}
  VERSION
    ${PROJECT_VERSION}
  COMPANY_NAME
    "FernShy"
  COMPANY_WEBSITE
    "https://fernshy.com"
  COMPANY_EMAIL
    "support@fernshy.com"
  COMPANY_COPYRIGHT
    "Copyright (c) 2026 FernShy"
  BUNDLE_ID
    "com.fernshy.pandorasbox"
  PLUGIN_MANUFACTURER_CODE
    Fshy
  PLUGIN_CODE
    Pbox
  FORMATS
    ${FERNSHY_PANDORAS_BOX_FORMATS}
  ICON_BIG
    assets/3_02_26_open.png
  IS_SYNTH
    FALSE
  NEEDS_MIDI_INPUT
    FALSE
  NEEDS_MIDI_OUTPUT
    FALSE
  VST3_AUTO_MANIFEST
    TRUE
  HARDENED_RUNTIME_ENABLED
    TRUE
  SUPPRESS_AU_PLIST_RESOURCE_USAGE
    TRUE
  COPY_PLUGIN_AFTER_BUILD
    ${PANDORAS_BOX_COPY_PLUGIN_AFTER_BUILD}
)

# Configures the JUCE sources included in the plugin target.
target_compile_definitions(FernShyPandorasBoxPlugin
  PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_add_binary_data(pandoras_box_plugin_assets
  HEADER_NAME
    "PandorasBoxPluginAssets.h"
  NAMESPACE
    pandoras_box::assets
  SOURCES
    assets/3.02.26.png
    assets/3_02_26_open.png
    assets/3_02_26_close.png
)

add_library(fernshy::pandoras_box_plugin_assets ALIAS pandoras_box_plugin_assets)

juce_add_module(pandoras_box_plugin ALIAS_NAMESPACE fernshy)

# Additional definitions required by all users of this module.
target_compile_definitions(pandoras_box_plugin
  INTERFACE
    FERNSHY_PANDORAS_BOX_PLUGIN_NAME="${FERNSHY_PANDORAS_BOX_PLUGIN_NAME}"
)

# Enables strict C++ warnings.
target_compile_options(pandoras_box_plugin INTERFACE "${PROJECT_WARNINGS_CXX}")

target_link_libraries(pandoras_box_plugin INTERFACE fernshy::pandoras_box_plugin_assets)

target_link_libraries(
  FernShyPandorasBoxPlugin
  PRIVATE
    fernshy::pandoras_box_plugin
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# In Visual Studio this command provides a nice grouping of source files in "filters".
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)

option(BUILD_EXAMPLES "When on, the supplied example apps will be added as targets and built" OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(BUILD_TESTS "When on, will download the GoogleTest framework and add the automated tests project" OFF)
if(BUILD_TESTS)
  # This command allows running tests from the "build" folder (the one where CMake generates the project to).
  enable_testing()

  # Adds all the targets configured in the "test" folder.
  add_subdirectory(test)
endif()
