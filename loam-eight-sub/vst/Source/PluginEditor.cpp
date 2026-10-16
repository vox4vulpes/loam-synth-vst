#include "PluginEditor.h"

using namespace LoamColours;

namespace
{
    constexpr int editorWidth = 760, editorHeight = 600;
    constexpr int pad = 20, gap = 12, headerHeight = 56, moduleHeight = 180, titleHeight = 26;
    constexpr int knobWidth = 64, knobHeight = 98;

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

    auto fill = on ? mustard : panel;
    if (! on && (isMouseOverButton || isButtonDown))
        fill = fill.brighter (0.15f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (on ? mustard : grooveDark);
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
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
LoamEightSubEditor::LoamEightSubEditor (LoamEightSubProcessor& p)
    : AudioProcessorEditor (p),
      audioProcessor (p),
      tune          (p.apvts, "tune",          "Tune",    mustard,  juce::String (juce::CharPointer_UTF8 ("\xc2\xa2"))),
      sub           (p.apvts, "sub",           "Sub",     mustard,  "%"),
      cutoff        (p.apvts, "cutoff",        "Cutoff",  teal,     "Hz"),
      resonance     (p.apvts, "resonance",     "Reso",    teal,     "", 1),
      attack        (p.apvts, "attack",        "Attack",  coral,    "ms"),
      decay         (p.apvts, "decay",         "Decay",   coral,    "ms"),
      sustain       (p.apvts, "sustain",       "Sustain", coral,    "%"),
      release       (p.apvts, "release",       "Release", coral,    "ms"),
      delayTime     (p.apvts, "delayTime",     "Time",    lavender, "ms"),
      delayFeedback (p.apvts, "delayFeedback", "Feedbk",  lavender, "%"),
      delayMix      (p.apvts, "delayMix",      "Mix",     lavender, "%"),
      reverbMix     (p.apvts, "reverbMix",     "Mix",     lavender, "%"),
      reverbSize    (p.apvts, "reverbSize",    "Size",    lavender, "s", 1),
      reverbTone    (p.apvts, "reverbTone",    "Tone",    lavender, "Hz"),
      volume        (p.apvts, "volume",        "Volume",  mustard,  "%"),
      keyboard      (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    // Waveform: four radio-style buttons bound to the "wave" choice parameter.
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

    for (auto* k : { &tune, &sub, &cutoff, &resonance, &attack, &decay, &sustain, &release,
                     &delayTime, &delayFeedback, &delayMix, &reverbMix, &reverbSize, &reverbTone, &volume })
        addAndMakeVisible (*k);

    for (auto [l, text] : { std::pair { &delayLabel, "DELAY" }, std::pair { &reverbLabel, "REVERB" }, std::pair { &presetLabel, "PRESET" } })
    {
        l->setText (text, juce::dontSendNotification);
        l->setFont (sansFont (10.0f, true));
        l->setBorderSize ({});
        addAndMakeVisible (*l);
    }

    // Presets live in a dropdown, same as the web version.
    presetBox.addItemList (LoamEightSubProcessor::getPresetNames(), 1);
    presetBox.setTextWhenNothingSelected (juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94 choose \xe2\x80\x94")));
    presetBox.onChange = [this]
    {
        if (const auto index = presetBox.getSelectedItemIndex(); index >= 0)
            audioProcessor.applyPreset (index);
    };
    addAndMakeVisible (presetBox);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, panelRaised);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, ink);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, grooveDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, coral.withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, mustard.withAlpha (0.3f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, shadow);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, panel);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, ink);
    keyboard.setAvailableRange (24, 108);
    keyboard.setKeyWidth (24.0f);
    keyboard.setLowestVisibleKey (48);
    addAndMakeVisible (keyboard);

    setSize (editorWidth, editorHeight);
    startTimerHz (30);
}

LoamEightSubEditor::~LoamEightSubEditor()
{
    setLookAndFeel (nullptr);
}

void LoamEightSubEditor::timerCallback()
{
    if (const auto note = audioProcessor.lastNote.load(); note != shownNote)
    {
        shownNote = note;
        repaint (headerArea);
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

    // Amber NOTE readout window on the right.
    auto window = headerArea.toFloat().removeFromRight (110.0f).withSizeKeepingCentre (110.0f, 28.0f);
    g.setColour (readoutBg);
    g.fillRoundedRectangle (window, 6.0f);
    window.reduce (10.0f, 0.0f);
    g.setColour (readoutInk.withAlpha (0.55f));
    g.setFont (monoFont (9.0f).withExtraKerningFactor (0.1f));
    g.drawText ("NOTE", window.removeFromLeft (32.0f), juce::Justification::centredLeft, false);
    g.setColour (readoutInk);
    g.setFont (monoFont (15.0f));
    const auto noteText = shownNote >= 0 ? juce::MidiMessage::getMidiNoteName (shownNote, true, true, 4)
                                         : juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94"));
    g.drawText (noteText, window, juce::Justification::centredLeft, false);

    for (const auto* m : { &osc, &filter, &env, &fx, &out })
        paintModule (g, *m);

    // Recessed bay behind the keyboard, like the web scope/keybed wells.
    g.setColour (grooveDark);
    g.fillRoundedRectangle (keyboardBay.toFloat(), 10.0f);
}

void LoamEightSubEditor::resized()
{
    auto area = getLocalBounds().reduced (pad);
    headerArea = area.removeFromTop (headerHeight);
    area.removeFromTop (gap);

    // Row 1: Oscillator | Filter | Envelope
    auto row1 = area.removeFromTop (moduleHeight);
    area.removeFromTop (gap);
    osc.bounds = row1.removeFromLeft (2 * knobWidth + 48);
    row1.removeFromLeft (gap);
    filter.bounds = row1.removeFromLeft (2 * knobWidth + 24);
    row1.removeFromLeft (gap);
    env.bounds = row1;

    // Row 2: Effects | Output
    auto row2 = area.removeFromTop (moduleHeight);
    area.removeFromTop (gap);
    fx.bounds = row2.removeFromLeft (6 * knobWidth + 3 * 24);
    row2.removeFromLeft (gap);
    out.bounds = row2;

    keyboardBay = area;
    keyboard.setBounds (keyboardBay.reduced (8));

    auto content = [] (const Module& m) { return m.bounds.reduced (12).withTrimmedTop (titleHeight); };

    auto oscArea = content (osc);
    auto waveRow = oscArea.removeFromTop (26);
    const auto buttonWidth = waveRow.getWidth() / (int) waveButtons.size();
    for (auto& b : waveButtons)
        b.setBounds (waveRow.removeFromLeft (buttonWidth).reduced (2, 0));
    oscArea.removeFromTop (6);
    layoutKnobRow (oscArea, { &tune, &sub });

    layoutKnobRow (content (filter), { &cutoff, &resonance });

    auto envArea = content (env);
    layoutKnobRow (envArea.withSizeKeepingCentre (4 * knobWidth, envArea.getHeight()), { &attack, &decay, &sustain, &release });

    auto fxArea = content (fx);
    auto delayArea = fxArea.removeFromLeft (3 * knobWidth);
    fxArea.removeFromLeft (fxArea.getWidth() - 3 * knobWidth);
    delayLabel.setBounds (delayArea.removeFromTop (14));
    reverbLabel.setBounds (fxArea.removeFromTop (14));
    layoutKnobRow (delayArea.withTrimmedTop (4), { &delayTime, &delayFeedback, &delayMix });
    layoutKnobRow (fxArea.withTrimmedTop (4), { &reverbMix, &reverbSize, &reverbTone });

    auto outArea = content (out);
    layoutKnobRow (outArea.removeFromLeft (knobWidth), { &volume });
    outArea.removeFromLeft (gap);
    auto presetArea = outArea.withSizeKeepingCentre (outArea.getWidth(), 14 + 4 + 30);
    presetLabel.setBounds (presetArea.removeFromTop (14));
    presetArea.removeFromTop (4);
    presetBox.setBounds (presetArea);
}
