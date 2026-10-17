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
    inline const juce::Colour modRed      { 0xffd33b2c };
    inline const juce::Colour modGreen    { 0xff3a9a4a };
    inline const juce::Colour driveRed    { 0xffb8322a };   // the Effects module on its Drive page
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
// In the CUST mod-wheel mode it also gets an assign button and, when assigned,
// red/green dots on its ring that set where the wheel takes it.
class LoamKnob : public juce::Component
{
public:
    LoamKnob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
              const juce::String& labelText, juce::Colour accent, juce::String unit, int decimals = 0);

    void resized() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void attachMod (LoamEightSubProcessor&);
    void refreshMod();   // called on the editor's timer

private:
    // Small round button: off -> on (green) -> reversed (red) -> off.
    struct ModButton : public juce::Component
    {
        std::function<void()> onClick;
        int state = 0;   // 0 off, 1 on, 2 reversed
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent& e) override { if (contains (e.getPosition()) && onClick) onClick(); }
    };

    void updateReadout();
    void setReadout (float value);
    void cycleBinding();
    juce::Point<float> dotPosition (float normalised) const;
    juce::Rectangle<int> dialBounds() const;

    LoamEightSubProcessor* processor = nullptr;
    juce::RangedAudioParameter* param = nullptr;
    juce::String paramId;
    ModButton modButton;
    bool customShown = false, modded = false;
    float shownNormalised = 0.0f;
    int draggingDot = 0;   // 0 none, 1 red, 2 green

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
            const juce::String& mixId, std::array<std::unique_ptr<LoamKnob>, 3> knobs,
            juce::Colour accent = LoamColours::lavender);

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshDimming();
    const std::array<std::unique_ptr<LoamKnob>, 3>& getKnobs() const { return knobs; }

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

    // Space works as a sustain pedal while the keyboard has focus.
    std::function<void (bool)> onSustainKey;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;
    void focusLost (FocusChangeType) override;
    int getBaseNote() const { return baseNote; }
    static constexpr int numKeys = 29;   // C to E two octaves up

    void drawWhiteNote (int midiNoteNumber, juce::Graphics&, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int midiNoteNumber, juce::Graphics&, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour noteFillColour) override;

private:
    juce::String capFor (int midiNoteNumber) const;
    void setSpaceDown (bool);
    int baseNote = 48;
    bool spaceDown = false;
};

// A pitch or mod wheel beside the keys. Drag it up and down; the pitch wheel
// springs back to the centre on release, the mod wheel stays put. It also
// follows the hardware wheels, since both write the same value.
class WheelView : public juce::Component
{
public:
    WheelView (const juce::String& name, std::atomic<float>& value, bool bipolar);

    // Optional big label in the middle of the wheel (the pitch wheel shows its bend range).
    juce::String centreText;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void refresh();

private:
    juce::Rectangle<float> slot() const;
    void setFromY (float y);

    juce::String name;
    std::atomic<float>& value;
    bool bipolar;            // pitch: -1..1 around a centre detent; mod: 0..1
    float shown = 0.0f;
};

// Strip under the keyboard showing which keys PORTA glides on. Drag either
// end (or click a key) to move the nearer end of the range.
class PortaRangeStrip : public juce::Component
{
public:
    PortaRangeStrip (juce::AudioProcessorValueTreeState&, LoamKeyboard&);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    int noteAt (float x) const;
    void moveEnd (int note);

    juce::AudioProcessorValueTreeState& apvts;
    LoamKeyboard& keyboard;
    juce::RangedAudioParameter* lowParam = nullptr;
    juce::RangedAudioParameter* highParam = nullptr;
    bool draggingLow = false;
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
    std::vector<LoamKnob*> allKnobs();
    FxUnit delayUnit, reverbUnit, satUnit, distUnit;

    // Effects pages, flipped with the arrows in the module's title row.
    static constexpr int numFxPages = 2;
    int fxPage = 0;
    juce::TextButton fxPrev { "<" }, fxNext { ">" };
    juce::Label fxPageName;
    void setFxPage (int page);

    LoamKeyboard keyboard;
    juce::TextButton octaveDown { "-" }, octaveUp { "+" };
    juce::Label octaveLabel;
    int octaveShift = 0;

    juce::Label portaLabel, glideLabel, glideReadout;
    juce::ToggleButton portaToggle;
    juce::AudioProcessorValueTreeState::ButtonAttachment portaToggleAttachment;
    juce::Slider glideSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment glideAttachment;
    PortaRangeStrip portaStrip;
    bool portaShown = false;

    // Wheels: + / - around the pitch wheel set the bend range (shown on the
    // wheel); the button above the mod wheel steps VIB -> FILT -> CUST.
    WheelView pitchWheel, modWheel;
    juce::TextButton bendUp { "+" }, bendDown { "-" }, modModeButton;
    juce::Label modWheelLabel;
    std::unique_ptr<juce::ParameterAttachment> modModeAttachment, bendRangeAttachment;
    int modMode = 0;
    void stepBendRange (int delta);
    bool keyboardFocused = false;

    Module presetsModule { "Presets",    LoamColours::sage,     {} };
    Module osc           { "Oscillator", LoamColours::mustard,  {} };
    Module filter        { "Filter",     LoamColours::teal,     {} };
    Module env           { "Envelope",   LoamColours::coral,    {} };
    Module fx            { "Effects",    LoamColours::lavender, {} };
    Module out           { "Output",     LoamColours::mustard,  {} };

    juce::Rectangle<int> headerArea, scopeBay, keyboardBay, sustainLight;
    int shownNote = -2;
    bool shownSustain = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamEightSubEditor)
};
