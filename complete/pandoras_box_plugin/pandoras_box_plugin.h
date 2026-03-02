/*
==============================================================================

BEGIN_JUCE_MODULE_DECLARATION

   ID:            pandoras_box_plugin
   vendor:        FernShy
   version:       1.0.0
   name:          Pandoras Box Plugin
   description:   Core of the Pandoras Box chaos generator plugin
   dependencies:  juce_audio_utils, juce_dsp

   website:       https://fernshy.com
   license:       MIT

END_JUCE_MODULE_DECLARATION

==============================================================================
*/

#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include <functional>
#include <ranges>
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <span>

#include "include/PandorasBox/detail/StridedQueue.h"

#include "include/PandorasBox/Parameters.h"
#include "include/PandorasBox/CustomLookAndFeel.h"
#include "include/PandorasBox/JsonSerializer.h"
#include "include/PandorasBox/LfoVisualizer.h"
#include "include/PandorasBox/SampleFifo.h"
#include "include/PandorasBox/Tremolo.h"
#include "include/PandorasBox/BypassTransitionSmoother.h"
#include "include/PandorasBox/PluginProcessor.h"
#include "include/PandorasBox/MessageOnClick.h"
#include "include/PandorasBox/PluginEditor.h"
