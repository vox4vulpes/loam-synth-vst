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

    LoamEightSubProcessor& audioProcessor;
    LoamLookAndFeel lnf;

    std::array<juce::TextButton, 4> waveButtons;
    std::unique_ptr<juce::ParameterAttachment> waveAttachment;

    LoamKnob tune, sub, cutoff, resonance,
             attack, decay, sustain, release,
             delayTime, delayFeedback, delayMix,
             reverbMix, reverbSize, reverbTone,
             volume;

    juce::Label delayLabel, reverbLabel, presetLabel;
    juce::ComboBox presetBox;
    juce::MidiKeyboardComponent keyboard;

    Module osc    { "Oscillator", LoamColours::mustard,  {} };
    Module filter { "Filter",     LoamColours::teal,     {} };
    Module env    { "Envelope",   LoamColours::coral,    {} };
    Module fx     { "Effects",    LoamColours::lavender, {} };
    Module out    { "Output",     LoamColours::mustard,  {} };

    juce::Rectangle<int> headerArea, keyboardBay;
    int shownNote = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamEightSubEditor)
};
