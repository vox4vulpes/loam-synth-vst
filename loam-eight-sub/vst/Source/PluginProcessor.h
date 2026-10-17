#pragma once

#include <array>
#include <atomic>
#include <utility>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SynthEngine.h"

class LoamEightSubProcessor : public juce::AudioProcessor
{
public:
    LoamEightSubProcessor();
    ~LoamEightSubProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Loam Eight Sub"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;   // on-screen keyboard in the editor
    std::atomic<int> lastNote { -1 };        // for the editor's NOTE readout
    std::atomic<bool> uiSustain { false };   // Space on the on-screen keyboard acts as the pedal
    std::atomic<bool> sustainActive { false };   // pedal state shown by the editor's SUSTAIN light

    // Wheels: written by MIDI (pitch wheel, CC 1) on the audio thread and by the
    // on-screen wheels on the message thread; whichever moved last wins.
    std::atomic<float> bendWheel { 0.0f };   // -1 .. 1
    std::atomic<float> modWheel { 0.0f };    //  0 .. 1

    //==============================================================================
    // Presets: factory ones grouped by type with several versions each (same
    // as the web version), plus user presets saved as XML files on disk.
    using ParamValues = std::vector<std::pair<juce::String, float>>;

    struct Preset
    {
        juce::String name;
        ParamValues values;
    };

    static const juce::StringArray& getPresetTypeNames();          // Lead, Bass, Pad, Pluck
    static const std::vector<Preset>& getFactoryPresets (int typeIndex);
    void applyPresetValues (const ParamValues&);

    static juce::File getUserPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);

    //==============================================================================
    // Oscilloscope tap: the audio thread writes the mono output into a ring,
    // the editor copies the most recent samples out on its timer.
    //==============================================================================
    // CUST mod wheel: any knob can follow the wheel from a "red" position (wheel
    // down) to a "green" one (wheel up), both normalised 0..1 along the knob.
    // Kept in atomics for the audio thread and mirrored into the state tree
    // (so projects and presets save them).
    struct ModBinding
    {
        bool bound = false;
        bool reversed = false;   // the assign button's third state; red/green already swapped
        float red = 0.0f, green = 1.0f;
    };

    static const juce::StringArray& getModTargetIds();
    ModBinding getModBinding (const juce::String& paramId) const;
    void setModBinding (const juce::String& paramId, ModBinding);
    // The value a knob is currently heard at: its own, or the wheel's position
    // between its dots when CUST drives it. Normalised 0..1.
    float getEffectiveNormalised (const juce::String& paramId) const;
    bool isCustomModActive() const;

    static constexpr int scopeSize = 4096;
    void copyScope (float* dest, int numSamples) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void pullParameters();
    void pushToScope (const juce::AudioBuffer<float>&);

    SynthEngine engine;
    bool midiSustain = false;   // CC 64 from the host / MIDI input

    struct ModSlot
    {
        std::atomic<bool> bound { false }, reversed { false };
        std::atomic<float> red { 0.0f }, green { 1.0f };
    };
    std::array<ModSlot, 21> modSlots;
    void loadModMapFromState();
    void applyCustomMod();

    std::array<std::atomic<float>, scopeSize> scopeRing {};
    std::atomic<int> scopeWritePos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamEightSubProcessor)
};
