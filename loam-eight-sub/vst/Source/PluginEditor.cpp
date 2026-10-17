#include "PluginEditor.h"

using namespace LoamColours;

namespace
{
    // Layout mirrors the web panel: header, scope, presets, then the synth
    // modules on the left with effects as a tall column on the right.
    constexpr int editorWidth = 900, editorHeight = 728;
    constexpr int pad = 20, gap = 12, headerHeight = 56, scopeHeight = 132, presetsHeight = 92;
    constexpr int rowHeight = 180, fxWidth = 214, titleHeight = 26;
    constexpr int knobWidth = 60, knobHeight = 98, fxKnobHeight = 96;

    juce::Font sansFont (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }

    juce::Font monoFont (float height)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
    }

    void layoutKnobRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> knobs)
    {
        for (auto* k : knobs)
            k->setBounds (area.removeFromLeft (knobWidth).withHeight (knobHeight));
    }

    juce::String utf8 (const char* text) { return juce::String (juce::CharPointer_UTF8 (text)); }

    void setParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
    }

    const juce::Identifier presetTypeProp ("uiPresetType"), presetIndexProp ("uiPresetIndex");
}

//==============================================================================
LoamLookAndFeel::LoamLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::outlineColourId, grooveDark);
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::arrowColourId, ink);
    setColour (juce::ComboBox::focusedOutlineColourId, teal);
    setColour (juce::PopupMenu::backgroundColourId, panelRaised);
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, mustard);
    setColour (juce::PopupMenu::highlightedTextColourId, panelRaised);
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, panelRaised);
    setColour (juce::TextButton::buttonOnColourId, mustard);
    setColour (juce::TextEditor::backgroundColourId, panel);
    setColour (juce::TextEditor::outlineColourId, grooveDark);
    setColour (juce::TextEditor::focusedOutlineColourId, teal);
    setColour (juce::TextEditor::textColourId, ink);
    setColour (juce::TextEditor::highlightColourId, mustard.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, ink);
    setColour (juce::Label::textColourId, inkSoft);
}

void LoamLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto area = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 2.0f;
    const auto centre = area.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcRadius = radius - 3.0f;
    const juce::PathStrokeType arcStroke (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    // Recessed dial well with the 270-degree groove, as in the web knob.
    g.setColour (grooveDark.withAlpha (0.25f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (grooveDark);
    g.strokePath (track, arcStroke);

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (value, arcStroke);

    // Cap: soft drop shadow plus an off-centre highlight.
    const auto capRadius = radius * 0.72f;
    const auto cap = juce::Rectangle<float> (capRadius * 2.0f, capRadius * 2.0f).withCentre (centre);
    g.setColour (shadow);
    g.fillEllipse (cap.translated (0.0f, 1.5f));
    g.setGradientFill (juce::ColourGradient (panelRaised, centre.x - capRadius * 0.36f, centre.y - capRadius * 0.44f,
                                             groove, centre.x + capRadius, centre.y + capRadius, true));
    g.fillEllipse (cap);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -capRadius * 0.85f, 3.0f, capRadius * 0.55f, 1.5f);
    g.setColour (accent);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void LoamLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool isMouseOverButton, bool isButtonDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto on = button.getToggleState();
    const auto accent = button.findColour (juce::TextButton::buttonOnColourId);
    const auto alpha = button.isEnabled() ? 1.0f : 0.45f;

    auto fill = on ? accent : panel;
    if (! on && button.isEnabled() && (isMouseOverButton || isButtonDown))
        fill = fill.brighter (0.15f);

    g.setColour (fill.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour ((on ? accent : grooveDark).withMultipliedAlpha (alpha));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}

void LoamLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool)
{
    // Slide switch, like the web effect units' .fx-switch.
    const auto track = button.getLocalBounds().toFloat().withSizeKeepingCentre (32.0f, 18.0f);
    const auto on = button.getToggleState();

    g.setColour (on ? button.findColour (juce::ToggleButton::tickColourId) : grooveDark);
    g.fillRoundedRectangle (track, 9.0f);

    const auto thumb = juce::Rectangle<float> (14.0f, 14.0f)
                           .withCentre ({ on ? track.getRight() - 9.0f : track.getX() + 9.0f, track.getCentreY() });
    g.setColour (shadow);
    g.fillEllipse (thumb.translated (0.0f, 1.0f));
    g.setColour (panelRaised);
    g.fillEllipse (thumb);

    if (button.hasKeyboardFocus (true))
    {
        g.setColour (teal);
        g.drawRoundedRectangle (track.expanded (2.0f), 11.0f, 1.5f);
    }
}

juce::Font LoamLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return sansFont (11.0f, true);
}

juce::Font LoamLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return sansFont (13.0f, true);
}

//==============================================================================
LoamKnob::LoamKnob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                    const juce::String& labelText, juce::Colour accent, juce::String unitText, int numDecimals)
    : attachment (apvts, paramId, slider), unit (std::move (unitText)), decimals (numDecimals)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setWantsKeyboardFocus (false);

    if (auto* param = apvts.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    slider.onValueChange = [this] { updateReadout(); };
    addAndMakeVisible (slider);

    label.setText (labelText.toUpperCase(), juce::dontSendNotification);
    label.setFont (sansFont (10.0f, true));
    readout.setFont (monoFont (11.0f));

    for (auto* l : { &label, &readout })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setInterceptsMouseClicks (false, false);
        l->setBorderSize ({});
        addAndMakeVisible (*l);
    }

    updateReadout();
}

void LoamKnob::updateReadout()
{
    const auto v = (float) slider.getValue();
    juce::String text;

    if (unit == "Hz" && v >= 1000.0f)
        text = juce::String (v / 1000.0f, 1) + "k";
    else if (decimals > 0)
        text = juce::String (v, decimals) + unit;
    else
        text = juce::String (juce::roundToInt (v)) + unit;

    readout.setText (text, juce::dontSendNotification);
}

void LoamKnob::resized()
{
    auto area = getLocalBounds();
    label.setBounds (area.removeFromTop (14));
    readout.setBounds (area.removeFromBottom (16));
    slider.setBounds (area.reduced (1));
}

//==============================================================================
void ScopeView::update (LoamEightSubProcessor& processor)
{
    processor.copyScope (samples.data(), (int) samples.size());

    // Start on the latest rising zero crossing that still leaves a full
    // window after it, so a steady note stands still instead of scrolling.
    const auto latestStart = (int) samples.size() - shown;
    start = latestStart;
    for (int i = latestStart; i > 0; --i)
    {
        if (samples[(size_t) i - 1] < 0.0f && samples[(size_t) i] >= 0.0f)
        {
            start = i;
            break;
        }
    }

    hasSignal = false;
    for (int i = start; i < start + shown && ! hasSignal; ++i)
        hasSignal = std::abs (samples[(size_t) i]) > 1.0e-4f;

    repaint();
}

void ScopeView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (readoutBg);
    g.fillRoundedRectangle (bounds, 6.0f);

    const auto mid = bounds.getCentreY();
    if (! hasSignal)
    {
        g.setColour (readoutInk.withAlpha (0.35f));
        g.drawHorizontalLine (juce::roundToInt (mid), bounds.getX() + 4.0f, bounds.getRight() - 4.0f);
        return;
    }

    juce::Path trace;
    const auto xStep = bounds.getWidth() / (float) (shown - 1);
    for (int i = 0; i < shown; ++i)
    {
        const auto s = juce::jlimit (-1.0f, 1.0f, samples[(size_t) (start + i)]);
        const juce::Point<float> p (bounds.getX() + (float) i * xStep, mid - s * bounds.getHeight() * 0.5f);
        if (i == 0) trace.startNewSubPath (p); else trace.lineTo (p);
    }

    // Soft glow under a crisp 2px line, like the web canvas shadowBlur.
    g.setColour (readoutInk.withAlpha (0.25f));
    g.strokePath (trace, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (readoutInk);
    g.strokePath (trace, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
FxUnit::FxUnit (juce::AudioProcessorValueTreeState& state, const juce::String& titleText, const juce::String& switchId,
                const juce::String& mixParamId, std::array<std::unique_ptr<LoamKnob>, 3> unitKnobs)
    : apvts (state), mixId (mixParamId), toggleAttachment (state, switchId, toggle), knobs (std::move (unitKnobs))
{
    title.setText (titleText.toUpperCase(), juce::dontSendNotification);
    title.setFont (sansFont (10.0f, true));
    title.setBorderSize ({});
    title.setMinimumHorizontalScale (0.75f);   // "REVERB" next to the switch is a tight fit
    addAndMakeVisible (title);

    toggle.setColour (juce::ToggleButton::tickColourId, lavender);
    toggle.setWantsKeyboardFocus (false);
    toggle.setTitle (titleText + " on");
    toggle.onClick = [this]
    {
        // Switching on with Mix at zero would change nothing audible.
        if (toggle.getToggleState())
            if (auto* mix = apvts.getRawParameterValue (mixId); mix != nullptr && mix->load() <= 0.0f)
                setParam (apvts, mixId, 25.0f);
        refreshDimming();
    };
    addAndMakeVisible (toggle);

    for (auto& k : knobs)
        addAndMakeVisible (*k);

    refreshDimming();
}

void FxUnit::refreshDimming()
{
    const auto on = toggle.getToggleState();
    for (auto& k : knobs)
        k->setAlpha (on ? 1.0f : 0.42f);
    title.setAlpha (on ? 1.0f : 0.6f);
}

void FxUnit::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (groove);
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);
}

void FxUnit::resized()
{
    auto area = getLocalBounds().reduced (6, 8);
    auto head = area.removeFromTop (20);
    toggle.setBounds (head.removeFromRight (36));
    title.setBounds (head);
    area.removeFromTop (6);

    for (auto& k : knobs)
        k->setBounds (area.removeFromTop (fxKnobHeight).withSizeKeepingCentre (knobWidth, fxKnobHeight));
}

//==============================================================================
namespace
{
    // Tracker-style layout over two rows, same as the web version:
    //   Z-row plays the lower octave (sharps on S D G H J),
    //   Q-row the upper octave and a third up to E (sharps on 2 3 5 6 7 9 0).
    constexpr const char* keyCaps = "zsxdcvgbhnjmq2w3er5t6y7ui9o0p";
}

LoamKeyboard::LoamKeyboard (juce::MidiKeyboardState& keyboardState)
    : MidiKeyboardComponent (keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    clearKeyMappings();
    for (int i = 0; i < numKeys; ++i)
        setKeyPressForNote (juce::KeyPress (keyCaps[i], 0, 0), i);

    setScrollButtonsVisible (false);
    setBaseNote (baseNote);
}

void LoamKeyboard::setBaseNote (int midiNote)
{
    baseNote = midiNote;
    setAvailableRange (baseNote, baseNote + numKeys - 1);
    setLowestVisibleKey (baseNote);
    // Key mappings are offsets from C of this octave: note = 12 * octave + offset.
    setKeyPressBaseOctave (baseNote / 12);
    repaint();
}

bool LoamKeyboard::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
        return true;   // handled in keyStateChanged; don't let it reach the host
    return MidiKeyboardComponent::keyPressed (key);
}

bool LoamKeyboard::keyStateChanged (bool isKeyDown)
{
    const auto handled = MidiKeyboardComponent::keyStateChanged (isKeyDown);
    const auto space = juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::spaceKey);
    setSpaceDown (space);
    return handled || space;
}

void LoamKeyboard::focusLost (FocusChangeType cause)
{
    MidiKeyboardComponent::focusLost (cause);
    setSpaceDown (false);
}

void LoamKeyboard::setSpaceDown (bool down)
{
    if (down == spaceDown)
        return;
    spaceDown = down;
    if (onSustainKey)
        onSustainKey (down);
}

juce::String LoamKeyboard::capFor (int midiNoteNumber) const
{
    const auto offset = midiNoteNumber - baseNote;
    return juce::isPositiveAndBelow (offset, numKeys) ? juce::String::charToString (keyCaps[offset]).toUpperCase()
                                                       : juce::String();
}

void LoamKeyboard::drawWhiteNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                                  bool isDown, bool isOver, juce::Colour lineColour, juce::Colour)
{
    MidiKeyboardComponent::drawWhiteNote (midiNoteNumber, g, area, isDown, isOver, lineColour, juce::Colours::transparentBlack);
    g.setColour (inkSoft);
    g.setFont (monoFont (10.0f));
    g.drawText (capFor (midiNoteNumber), area.removeFromBottom (18.0f), juce::Justification::centred, false);
}

void LoamKeyboard::drawBlackNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                                  bool isDown, bool isOver, juce::Colour noteFillColour)
{
    MidiKeyboardComponent::drawBlackNote (midiNoteNumber, g, area, isDown, isOver, noteFillColour);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (monoFont (9.0f));
    g.drawText (capFor (midiNoteNumber), area.removeFromBottom (16.0f), juce::Justification::centred, false);
}

//==============================================================================
LoamEightSubEditor::LoamEightSubEditor (LoamEightSubProcessor& p)
    : AudioProcessorEditor (p),
      audioProcessor (p),
      tune      (p.apvts, "tune",      "Tune",    mustard, utf8 ("\xc2\xa2")),
      sub       (p.apvts, "sub",       "Sub",     mustard, "%"),
      cutoff    (p.apvts, "cutoff",    "Cutoff",  teal,    "Hz"),
      resonance (p.apvts, "resonance", "Reso",    teal,    "", 1),
      attack    (p.apvts, "attack",    "Attack",  coral,   "ms"),
      decay     (p.apvts, "decay",     "Decay",   coral,   "ms"),
      sustain   (p.apvts, "sustain",   "Sustain", coral,   "%"),
      release   (p.apvts, "release",   "Release", coral,   "ms"),
      volume    (p.apvts, "volume",    "Volume",  mustard, "%"),
      delayUnit (p.apvts, "Delay", "delayOn", "delayMix",
                 { std::make_unique<LoamKnob> (p.apvts, "delayTime", "Time", lavender, "ms"),
                   std::make_unique<LoamKnob> (p.apvts, "delayFeedback", "Feedbk", lavender, "%"),
                   std::make_unique<LoamKnob> (p.apvts, "delayMix", "Mix", lavender, "%") }),
      reverbUnit (p.apvts, "Reverb", "reverbOn", "reverbMix",
                  { std::make_unique<LoamKnob> (p.apvts, "reverbMix", "Mix", lavender, "%"),
                    std::make_unique<LoamKnob> (p.apvts, "reverbSize", "Size", lavender, "s", 1),
                    std::make_unique<LoamKnob> (p.apvts, "reverbTone", "Tone", lavender, "Hz") }),
      keyboard (p.keyboardState)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (scope);

    // ---- presets
    const auto typeNames = [] { auto names = LoamEightSubProcessor::getPresetTypeNames(); names.add ("User"); return names; }();
    for (int i = 0; i < (int) presetTypeButtons.size(); ++i)
    {
        auto& b = presetTypeButtons[(size_t) i];
        b.setButtonText (typeNames[i].toUpperCase());
        b.setColour (juce::TextButton::buttonOnColourId, sage);
        b.onClick = [this, i] { selectPresetType (i, true); };
        addAndMakeVisible (b);
    }

    presetPrev.onClick = [this] { loadPresetVersion (presetIndex < 0 ? 0 : presetIndex - 1); };
    presetNext.onClick = [this] { loadPresetVersion (presetIndex + 1); };
    presetSave.onClick = [this] { saveUserPreset(); };
    presetDelete.onClick = [this] { deleteUserPreset(); };
    presetBox.onChange = [this]
    {
        if (const auto index = presetBox.getSelectedItemIndex(); index >= 0 && index != presetIndex)
            loadPresetVersion (index);
    };

    presetName.setTextToShowWhenEmpty ("Preset name", inkSoft);
    presetName.setInputRestrictions (32);
    presetName.setFont (sansFont (13.0f));
    presetName.setIndents (8, 6);
    presetName.onReturnKey = [this] { saveUserPreset(); };

    for (auto [l, text] : { std::pair { &typeLabel, "TYPE" }, std::pair { &versionLabel, "VERSION" },
                            std::pair { &saveLabel, "SAVE AS USER PRESET" } })
    {
        l->setText (text, juce::dontSendNotification);
        l->setFont (sansFont (10.0f, true));
        l->setBorderSize ({});
        addAndMakeVisible (*l);
    }
    presetStatus.setFont (monoFont (11.0f));
    presetStatus.setJustificationType (juce::Justification::centredRight);
    presetStatus.setBorderSize ({});
    addAndMakeVisible (presetStatus);

    for (auto* c : std::initializer_list<juce::Component*> { &presetPrev, &presetNext, &presetSave, &presetDelete, &presetBox, &presetName })
        addAndMakeVisible (*c);

    // ---- waveform: four radio-style buttons bound to the "wave" choice parameter
    const char* waveNames[] = { "SIN", "SAW", "SQR", "TRI" };
    for (int i = 0; i < (int) waveButtons.size(); ++i)
    {
        waveButtons[(size_t) i].setButtonText (waveNames[i]);
        waveButtons[(size_t) i].onClick = [this, i] { waveAttachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (waveButtons[(size_t) i]);
    }

    waveAttachment = std::make_unique<juce::ParameterAttachment> (
        *p.apvts.getParameter ("wave"),
        [this] (float v)
        {
            for (int i = 0; i < (int) waveButtons.size(); ++i)
                waveButtons[(size_t) i].setToggleState (i == juce::roundToInt (v), juce::dontSendNotification);
        });
    waveAttachment->sendInitialUpdate();

    for (auto* k : { &tune, &sub, &cutoff, &resonance, &attack, &decay, &sustain, &release, &volume })
        addAndMakeVisible (*k);
    addAndMakeVisible (delayUnit);
    addAndMakeVisible (reverbUnit);

    // ---- keyboard
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, panelRaised);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, ink);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, grooveDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, coral.withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, mustard.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, shadow);
    keyboard.onSustainKey = [this] (bool down) { audioProcessor.uiSustain = down; };
    addAndMakeVisible (keyboard);

    octaveDown.onClick = [this] { setOctave (octaveShift - 1); };
    octaveUp.onClick = [this] { setOctave (octaveShift + 1); };
    octaveLabel.setText ("OCTAVE", juce::dontSendNotification);
    octaveLabel.setFont (monoFont (10.0f));
    octaveLabel.setJustificationType (juce::Justification::centred);
    octaveLabel.setColour (juce::Label::textColourId, ink);
    addAndMakeVisible (octaveDown);
    addAndMakeVisible (octaveLabel);
    addAndMakeVisible (octaveUp);

    // Buttons shouldn't steal focus from the keyboard, or computer keys stop playing.
    for (auto& b : presetTypeButtons) b.setWantsKeyboardFocus (false);
    for (auto& b : waveButtons) b.setWantsKeyboardFocus (false);
    for (auto* b : { &presetPrev, &presetNext, &presetSave, &presetDelete, &octaveDown, &octaveUp })
        b->setWantsKeyboardFocus (false);
    presetBox.setWantsKeyboardFocus (false);

    // Restore the preset selection shown last time this plugin's editor was open.
    const auto& state = p.apvts.state;
    presetType = juce::jlimit (0, userType, (int) state.getProperty (presetTypeProp, 0));
    presetIndex = (int) state.getProperty (presetIndexProp, -1);
    presetNames = presetNamesFor (presetType);
    if (presetIndex >= presetNames.size())
        presetIndex = -1;
    refreshPresetControls();

    setSize (editorWidth, editorHeight);
    startTimerHz (30);
}

LoamEightSubEditor::~LoamEightSubEditor()
{
    audioProcessor.uiSustain = false;
    setLookAndFeel (nullptr);
}

//==============================================================================
juce::StringArray LoamEightSubEditor::presetNamesFor (int type) const
{
    if (type == userType)
        return audioProcessor.getUserPresetNames();

    juce::StringArray names;
    for (const auto& preset : LoamEightSubProcessor::getFactoryPresets (type))
        names.add (preset.name);
    return names;
}

void LoamEightSubEditor::selectPresetType (int type, bool loadFirst)
{
    presetType = type;
    presetIndex = -1;
    presetNames = presetNamesFor (type);
    setStatus ({});

    if (loadFirst && ! presetNames.isEmpty())
        loadPresetVersion (0);
    else
        refreshPresetControls();
}

void LoamEightSubEditor::loadPresetVersion (int index)
{
    if (presetNames.isEmpty())
        return;

    presetIndex = (index % presetNames.size() + presetNames.size()) % presetNames.size();

    if (presetType == userType)
    {
        if (! audioProcessor.loadUserPreset (presetNames[presetIndex]))
            setStatus ("Couldn't read that preset file");
    }
    else
    {
        audioProcessor.applyPresetValues (LoamEightSubProcessor::getFactoryPresets (presetType)[(size_t) presetIndex].values);
    }

    refreshPresetControls();
}

void LoamEightSubEditor::refreshPresetControls()
{
    deleteArmed = false;
    presetDelete.setButtonText ("DELETE");

    for (int i = 0; i < (int) presetTypeButtons.size(); ++i)
        presetTypeButtons[(size_t) i].setToggleState (i == presetType, juce::dontSendNotification);

    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < presetNames.size(); ++i)
        presetBox.addItem (juce::String (i + 1) + utf8 (" \xc2\xb7 ") + presetNames[i], i + 1);

    presetBox.setTextWhenNothingSelected (presetNames.isEmpty() && presetType == userType
                                              ? utf8 ("\xe2\x80\x94 no saved presets \xe2\x80\x94")
                                              : utf8 ("\xe2\x80\x94 choose \xe2\x80\x94"));
    presetBox.setSelectedId (presetIndex + 1, juce::dontSendNotification);

    const auto hasPresets = ! presetNames.isEmpty();
    presetBox.setEnabled (hasPresets);
    presetPrev.setEnabled (hasPresets);
    presetNext.setEnabled (hasPresets);
    presetDelete.setEnabled (presetType == userType && presetIndex >= 0);

    audioProcessor.apvts.state.setProperty (presetTypeProp, presetType, nullptr);
    audioProcessor.apvts.state.setProperty (presetIndexProp, presetIndex, nullptr);
}

void LoamEightSubEditor::saveUserPreset()
{
    auto name = presetName.getText().trim();
    if (name.isEmpty())
        name = (presetType == userType && presetIndex >= 0) ? presetNames[presetIndex]
                                                            : "User " + juce::String (audioProcessor.getUserPresetNames().size() + 1);

    const auto existed = audioProcessor.getUserPresetNames().contains (name, true);
    if (! audioProcessor.saveUserPreset (name))
    {
        setStatus ("Couldn't save " + utf8 ("\xe2\x80\x9c") + name + utf8 ("\xe2\x80\x9d"));
        return;
    }

    presetType = userType;
    presetNames = presetNamesFor (userType);
    presetIndex = juce::jmax (0, presetNames.indexOf (name, true));
    presetName.clear();
    refreshPresetControls();
    setStatus ((existed ? "Updated " : "Saved ") + utf8 ("\xe2\x80\x9c") + name + utf8 ("\xe2\x80\x9d"));
    keyboard.grabKeyboardFocus();
}

void LoamEightSubEditor::deleteUserPreset()
{
    if (presetType != userType || presetIndex < 0)
        return;

    const auto name = presetNames[presetIndex];

    // Two-step delete: the first click arms the button, the second deletes.
    if (! deleteArmed)
    {
        deleteArmed = true;
        presetDelete.setButtonText ("SURE?");
        setStatus ("Click again to delete " + utf8 ("\xe2\x80\x9c") + name + utf8 ("\xe2\x80\x9d"));
        juce::Timer::callAfterDelay (3000, [safe = juce::Component::SafePointer<LoamEightSubEditor> (this)]
        {
            if (safe != nullptr && safe->deleteArmed)
            {
                safe->refreshPresetControls();
                safe->setStatus ({});
            }
        });
        return;
    }

    const auto deleted = audioProcessor.deleteUserPreset (name);
    presetNames = presetNamesFor (userType);
    presetIndex = -1;
    refreshPresetControls();
    setStatus ((deleted ? "Deleted " : "Couldn't delete ") + utf8 ("\xe2\x80\x9c") + name + utf8 ("\xe2\x80\x9d"));
}

void LoamEightSubEditor::setStatus (const juce::String& text)
{
    presetStatus.setText (text, juce::dontSendNotification);
}

void LoamEightSubEditor::setOctave (int shift)
{
    octaveShift = juce::jlimit (-2, 2, shift);
    keyboard.setBaseNote (48 + 12 * octaveShift);
    repaint (headerArea);
}

//==============================================================================
void LoamEightSubEditor::timerCallback()
{
    scope.update (audioProcessor);

    // Effect switches can also change from host automation or a preset.
    delayUnit.refreshDimming();
    reverbUnit.refreshDimming();

    if (const auto note = audioProcessor.lastNote.load(); note != shownNote)
    {
        shownNote = note;
        repaint (headerArea);
    }

    if (const auto pedal = audioProcessor.sustainActive.load(); pedal != shownSustain)
    {
        shownSustain = pedal;
        repaint (sustainLight);
    }

    // Let computer keys play straight away, without clicking the keyboard first.
    if (! keyboardFocused && keyboard.isShowing())
    {
        keyboard.grabKeyboardFocus();
        keyboardFocused = true;
    }
}

void LoamEightSubEditor::paintModule (juce::Graphics& g, const Module& m) const
{
    const auto bounds = m.bounds.toFloat();

    // Coloured top edge (the web module's border-top) peeking out above the face.
    g.setColour (m.accent);
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (panelRaised);
    g.fillRoundedRectangle (bounds.withTrimmedTop (3.0f), 12.0f);
    g.setColour (grooveDark.withAlpha (0.6f));
    g.drawRoundedRectangle (bounds.withTrimmedTop (3.0f), 12.0f, 1.0f);

    g.setColour (m.accent);
    g.setFont (sansFont (14.0f, true).withExtraKerningFactor (0.12f));
    g.drawText (m.title.toUpperCase(), m.bounds.reduced (12, 0).withTrimmedTop (8).withHeight (16),
                juce::Justification::centredLeft);
}

void LoamEightSubEditor::paintReadout (juce::Graphics& g, juce::Rectangle<float> window,
                                       const juce::String& label, const juce::String& value) const
{
    g.setColour (readoutBg);
    g.fillRoundedRectangle (window, 6.0f);
    window.reduce (10.0f, 0.0f);
    g.setColour (readoutInk.withAlpha (0.55f));
    g.setFont (monoFont (9.0f).withExtraKerningFactor (0.1f));
    g.drawText (label, window.removeFromLeft (32.0f), juce::Justification::centredLeft, false);
    g.setColour (readoutInk);
    g.setFont (monoFont (15.0f));
    g.drawText (value, window, juce::Justification::centredLeft, false);
}

void LoamEightSubEditor::paint (juce::Graphics& g)
{
    g.fillAll (panel);

    // Brand block: LOAM / EIGHT / SUB, as in the web header.
    auto brand = headerArea.toFloat();
    const auto brandFont = sansFont (30.0f, true);
    const auto variantFont = sansFont (18.0f, true).withExtraKerningFactor (0.04f);
    auto titleRow = brand.removeFromTop (32.0f);

    const std::pair<juce::String, juce::Colour> words[] = { { "LOAM", ink }, { "EIGHT", coral } };
    for (const auto& [text, colour] : words)
    {
        g.setFont (brandFont);
        g.setColour (colour);
        const auto w = juce::GlyphArrangement::getStringWidth (brandFont, text);
        g.drawText (text, titleRow.removeFromLeft (w + 8.0f), juce::Justification::bottomLeft, false);
    }
    g.setFont (variantFont);
    g.setColour (inkSoft);
    g.drawText ("SUB", titleRow.withTrimmedBottom (2.0f), juce::Justification::bottomLeft, false);

    g.setFont (monoFont (10.0f).withExtraKerningFactor (0.14f));
    g.drawText ("POLYPHONIC VOICE SYNTHESIZER", brand.removeFromTop (18.0f), juce::Justification::centredLeft, false);

    // Amber OCT and NOTE readout windows on the right, as in the web header.
    auto readouts = headerArea.toFloat();
    const auto noteText = shownNote >= 0 ? juce::MidiMessage::getMidiNoteName (shownNote, true, true, 4)
                                         : utf8 ("\xe2\x80\x94");
    paintReadout (g, readouts.removeFromRight (110.0f).withSizeKeepingCentre (110.0f, 28.0f), "NOTE", noteText);
    readouts.removeFromRight (10.0f);
    paintReadout (g, readouts.removeFromRight (80.0f).withSizeKeepingCentre (80.0f, 28.0f), "OCT", juce::String (3 + octaveShift));

    // Recessed bays behind the scope and the keyboard, like the web wells.
    g.setColour (grooveDark);
    g.fillRoundedRectangle (scopeBay.toFloat(), 10.0f);
    g.fillRoundedRectangle (keyboardBay.toFloat(), 10.0f);

    // SUSTAIN light in the keyboard bay: lit while the pedal (or Space) is down.
    auto light = sustainLight.toFloat();
    const auto led = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ light.getRight() - 6.0f, light.getCentreY() });
    if (shownSustain)
    {
        g.setColour (coral.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
    }
    g.setColour (shownSustain ? coral : ink.withAlpha (0.25f));
    g.fillEllipse (led);
    g.setColour (ink);
    g.setFont (monoFont (10.0f).withExtraKerningFactor (0.12f));
    g.drawText ("SUSTAIN", light.withTrimmedRight (16.0f), juce::Justification::centredRight, false);
    g.setColour (readoutInk.withAlpha (0.5f));
    g.setFont (monoFont (9.0f).withExtraKerningFactor (0.15f));
    g.drawText ("SCOPE", scopeBay.reduced (12, 0).removeFromBottom (16), juce::Justification::centredRight, false);

    for (const auto* m : { &presetsModule, &osc, &filter, &env, &fx, &out })
        paintModule (g, *m);
}

void LoamEightSubEditor::resized()
{
    auto area = getLocalBounds().reduced (pad);
    headerArea = area.removeFromTop (headerHeight);
    area.removeFromTop (gap);

    scopeBay = area.removeFromTop (scopeHeight);
    scope.setBounds (scopeBay.reduced (10).withTrimmedBottom (14));
    area.removeFromTop (gap);

    presetsModule.bounds = area.removeFromTop (presetsHeight);
    area.removeFromTop (gap);

    // Main area: effects column on the right, two rows of modules on the left.
    fx.bounds = area.removeFromRight (fxWidth);
    area.removeFromRight (gap);
    auto row1 = area.removeFromTop (rowHeight);
    area.removeFromTop (gap);
    auto row2 = area;

    osc.bounds = row1.removeFromLeft (168);
    row1.removeFromLeft (gap);
    filter.bounds = row1.removeFromLeft (144);
    row1.removeFromLeft (gap);
    env.bounds = row1;

    out.bounds = row2.removeFromLeft (120);
    row2.removeFromLeft (gap);
    keyboardBay = row2;

    auto content = [] (const Module& m) { return m.bounds.reduced (12).withTrimmedTop (titleHeight); };

    // ---- presets: TYPE | VERSION | SAVE AS USER PRESET, status in the title row
    presetStatus.setBounds (presetsModule.bounds.reduced (12, 0).withTrimmedTop (8).withHeight (16).withTrimmedLeft (140));
    auto presetsArea = content (presetsModule);
    auto group = [&presetsArea] (int width, juce::Label& label)
    {
        auto g = presetsArea.removeFromLeft (width);
        presetsArea.removeFromLeft (22);
        label.setBounds (g.removeFromTop (14));
        g.removeFromTop (2);
        return g;
    };

    auto typeRow = group (5 * 50 + 4 * 4, typeLabel);
    for (auto& b : presetTypeButtons)
    {
        b.setBounds (typeRow.removeFromLeft (50));
        typeRow.removeFromLeft (4);
    }

    auto versionRow = group (26 + 4 + 190 + 4 + 26, versionLabel);
    presetPrev.setBounds (versionRow.removeFromLeft (26));
    presetNext.setBounds (versionRow.removeFromRight (26));
    presetBox.setBounds (versionRow.reduced (4, 0));

    auto saveRow = group (presetsArea.getWidth(), saveLabel);
    presetDelete.setBounds (saveRow.removeFromRight (64));
    saveRow.removeFromRight (4);
    presetSave.setBounds (saveRow.removeFromRight (56));
    saveRow.removeFromRight (4);
    presetName.setBounds (saveRow);

    // ---- oscillator, filter, envelope, output
    auto oscArea = content (osc);
    auto waveRow = oscArea.removeFromTop (26);
    const auto buttonWidth = waveRow.getWidth() / (int) waveButtons.size();
    for (auto& b : waveButtons)
        b.setBounds (waveRow.removeFromLeft (buttonWidth).reduced (2, 0));
    oscArea.removeFromTop (6);
    layoutKnobRow (oscArea.withSizeKeepingCentre (2 * knobWidth, oscArea.getHeight()), { &tune, &sub });

    auto filterArea = content (filter);
    layoutKnobRow (filterArea.withSizeKeepingCentre (2 * knobWidth, filterArea.getHeight()), { &cutoff, &resonance });

    auto envArea = content (env);
    layoutKnobRow (envArea.withSizeKeepingCentre (4 * knobWidth, envArea.getHeight()), { &attack, &decay, &sustain, &release });

    auto outArea = content (out);
    layoutKnobRow (outArea.withSizeKeepingCentre (knobWidth, outArea.getHeight()), { &volume });

    // ---- effects: two vertical units side by side
    auto fxArea = content (fx);
    const auto unitWidth = (fxArea.getWidth() - 10) / 2;
    delayUnit.setBounds (fxArea.removeFromLeft (unitWidth));
    reverbUnit.setBounds (fxArea.removeFromRight (unitWidth));

    // ---- keyboard with octave buttons above it
    auto kb = keyboardBay.reduced (8);
    auto octaveRow = kb.removeFromTop (24);
    octaveDown.setBounds (octaveRow.removeFromLeft (28));
    octaveLabel.setBounds (octaveRow.removeFromLeft (64));
    octaveUp.setBounds (octaveRow.removeFromLeft (28));
    sustainLight = octaveRow.removeFromRight (90);
    kb.removeFromTop (6);
    keyboard.setBounds (kb);
    keyboard.setKeyWidth ((float) kb.getWidth() / 17.0f);   // 17 white keys, C to E two octaves up
}
