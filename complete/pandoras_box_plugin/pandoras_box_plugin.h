/*
==============================================================================

BEGIN_JUCE_MODULE_DECLARATION

   ID:            pandoras_box_plugin
   vendor:        FernShy
   version:       1.0.1
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
#include <random>

#include "include/PandorasBox/detail/StridedQueue.h"

#include "include/PandorasBox/Parameters.h"
#include "include/PandorasBox/JsonSerializer.h"
#include "include/PandorasBox/BypassTransitionSmoother.h"

#include "include/PandorasBox/effects/BitCrusher.h"
#include "include/PandorasBox/effects/ChorusEffect.h"
#include "include/PandorasBox/effects/CombFilter.h"
#include "include/PandorasBox/effects/CompressorEffect.h"
#include "include/PandorasBox/effects/DelayEffect.h"
#include "include/PandorasBox/effects/Distortion.h"
#include "include/PandorasBox/effects/FilterEffect.h"
#include "include/PandorasBox/effects/PitchShifter.h"
#include "include/PandorasBox/effects/ReverbEffect.h"
#include "include/PandorasBox/effects/RingModulator.h"
#include "include/PandorasBox/effects/StereoWidener.h"
#include "include/PandorasBox/effects/WaveFolder.h"
#include "include/PandorasBox/EffectChain.h"

#include "include/PandorasBox/PluginProcessor.h"
#include "include/PandorasBox/PluginEditor.h"
