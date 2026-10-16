#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// v1 ships JUCE's generic parameter editor -- every APVTS parameter gets a
// working slider/combo automatically, in every host. A custom-skinned
// editor (matching the web version's panel look) is a natural follow-up,
// not a requirement for the plugin to be useful.
class LoamEightSubEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit LoamEightSubEditor (LoamEightSubProcessor& p) : GenericAudioProcessorEditor (p)
    {
        setSize (420, 560);
    }
};
