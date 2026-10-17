#pragma once

#include <array>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Palette lifted from the web version's :root tokens, so the plugin and
// the browser synth read as the same instrument.
namespace LoamColours
{
    inline const juce::Colour panel       { 0xffe9e1cc };
    inline const juce::Colour panelRaised { 0xfff2ecd9 };
    inline const juce::Colour groove      { 0xffc7bb9c };
    inline const juce::Colour grooveDark  { 0xffa99c79 };
    inline const juce::Colour ink         { 0xff2a241d };
    inline const juce::Colour inkSoft     { 0x9e2a241d };
    inline const juce::Colour mustard     { 0xffc08f17 };
    inline const juce::Colour teal        { 0xff336f6b };
    inline const juce::Colour coral       { 0xffb85540 };
    inline const juce::Colour lavender    { 0xff8a78a3 };
    inline const juce::Colour sage        { 0xff5f7344 };
    inline const juce::Colour readoutBg   { 0xff171308 };
    inline const juce::Colour readoutInk  { 0xfff0a830 };
    inline const juce::Colour shadow      { 0x591e190f };
}

class LoamLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LoamLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

// One knob cell as in the web panel: label above, dial, value readout below.
class LoamKnob : public juce::Component
{
public:
    LoamKnob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
              const juce::String& labelText, juce::Colour accent, juce::String unit, int decimals = 0);

    void resized() override;

private:
    void updateReadout();

    juce::Slider slider;
    juce::Label label, readout;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    juce::String unit;
    int decimals;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamKnob)
};

// The amber scope screen; the editor feeds it fresh samples on its timer.
class ScopeView : public juce::Component
{
public:
    void update (LoamEightSubProcessor&);
    void paint (juce::Graphics&) override;

private:
    static constexpr int shown = 2048;   // same window as the web scope's analyser
    std::array<float, LoamEightSubProcessor::scopeSize> samples {};
    int start = 0;
    bool hasSignal = false;
};

// One vertical effect unit: name, on/off slide switch, knobs stacked below.
// Off dims the knobs but leaves them adjustable.
class FxUnit : public juce::Component
{
public:
    FxUnit (juce::AudioProcessorValueTreeState&, const juce::String& title, const juce::String& switchId,
            const juce::String& mixId, std::array<std::unique_ptr<LoamKnob>, 3> knobs);

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshDimming();

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String mixId;
    juce::Label title;
    juce::ToggleButton toggle;
    juce::AudioProcessorValueTreeState::ButtonAttachment toggleAttachment;
    std::array<std::unique_ptr<LoamKnob>, 3> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxUnit)
};

// MIDI keyboard that labels each key with the computer key that plays it.
class LoamKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit LoamKeyboard (juce::MidiKeyboardState&);

    void setBaseNote (int midiNote);
    int getBaseNote() const { return baseNote; }
    static constexpr int numKeys = 29;   // C to E two octaves up

    void drawWhiteNote (int midiNoteNumber, juce::Graphics&, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int midiNoteNumber, juce::Graphics&, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour noteFillColour) override;

private:
    juce::String capFor (int midiNoteNumber) const;
    int baseNote = 48;
};

class LoamEightSubEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LoamEightSubEditor (LoamEightSubProcessor&);
    ~LoamEightSubEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Module
    {
        juce::String title;
        juce::Colour accent;
        juce::Rectangle<int> bounds;
    };

    void timerCallback() override;
    void paintModule (juce::Graphics&, const Module&) const;
    void paintReadout (juce::Graphics&, juce::Rectangle<float>, const juce::String& label, const juce::String& value) const;

    // presets
    static constexpr int userType = 4;
    juce::StringArray presetNamesFor (int type) const;
    void selectPresetType (int type, bool loadFirst);
    void loadPresetVersion (int index);
    void refreshPresetControls();
    void saveUserPreset();
    void deleteUserPreset();
    void setStatus (const juce::String&);

    void setOctave (int shift);

    LoamEightSubProcessor& audioProcessor;
    LoamLookAndFeel lnf;

    ScopeView scope;

    std::array<juce::TextButton, 5> presetTypeButtons;
    juce::TextButton presetPrev { "<" }, presetNext { ">" }, presetSave { "SAVE" }, presetDelete { "DELETE" };
    juce::ComboBox presetBox;
    juce::TextEditor presetName;
    juce::Label typeLabel, versionLabel, saveLabel, presetStatus;
    int presetType = 0, presetIndex = -1;
    juce::StringArray presetNames;
    bool deleteArmed = false;

    std::array<juce::TextButton, 4> waveButtons;
    std::unique_ptr<juce::ParameterAttachment> waveAttachment;

    LoamKnob tune, sub, cutoff, resonance, attack, decay, sustain, release, volume;
    FxUnit delayUnit, reverbUnit;

    LoamKeyboard keyboard;
    juce::TextButton octaveDown { "-" }, octaveUp { "+" };
    juce::Label octaveLabel;
    int octaveShift = 0;
    bool keyboardFocused = false;

    Module presetsModule { "Presets",    LoamColours::sage,     {} };
    Module osc           { "Oscillator", LoamColours::mustard,  {} };
    Module filter        { "Filter",     LoamColours::teal,     {} };
    Module env           { "Envelope",   LoamColours::coral,    {} };
    Module fx            { "Effects",    LoamColours::lavender, {} };
    Module out           { "Output",     LoamColours::mustard,  {} };

    juce::Rectangle<int> headerArea, scopeBay, keyboardBay;
    int shownNote = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamEightSubEditor)
};
