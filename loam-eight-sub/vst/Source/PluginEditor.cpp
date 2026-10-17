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
    // The highlight sits straight up, at 12 o'clock: the knob's 50% position.
    g.setGradientFill (juce::ColourGradient (panelRaised, centre.x, centre.y - capRadius * 0.5f,
                                             groove, centre.x, centre.y + capRadius * 1.3f, true));
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
LoamKnob::LoamKnob (juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                    const juce::String& labelText, juce::Colour accent, juce::String unitText, int numDecimals)
    : param (apvts.getParameter (id)), paramId (id),
      attachment (apvts, id, slider), unit (std::move (unitText)), decimals (numDecimals)
{
    addChildComponent (modButton);
    modButton.onClick = [this] { cycleBinding(); };
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setWantsKeyboardFocus (false);

    if (param != nullptr)
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
    if (! modded)
        setReadout ((float) slider.getValue());
}

void LoamKnob::setReadout (float v)
{
    juce::String text;

    if (unit == "Hz" && v >= 1000.0f)
        text = juce::String (v / 1000.0f, 1) + "k";
    else if (decimals > 0)
        text = juce::String (v, decimals) + unit;
    else
        text = juce::String (juce::roundToInt (v)) + unit;

    readout.setText (text, juce::dontSendNotification);
}

juce::Rectangle<int> LoamKnob::dialBounds() const
{
    return getLocalBounds().withTrimmedTop (14).withTrimmedBottom (16).reduced (1);
}

void LoamKnob::resized()
{
    auto area = getLocalBounds();
    label.setBounds (area.removeFromTop (14));
    readout.setBounds (area.removeFromBottom (16));
    slider.setBounds (area.reduced (1));
    modButton.setBounds (getLocalBounds().removeFromTop (16).removeFromRight (14).withTrimmedTop (2));
}

//------------------------------------------------------------------------------
void LoamKnob::ModButton::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.5f);
    const auto fill = state == 1 ? modGreen : state == 2 ? modRed : panel;
    g.setColour (fill);
    g.fillEllipse (r);
    g.setColour (state == 0 ? grooveDark : fill);
    g.drawEllipse (r, 1.5f);
}

void LoamKnob::attachMod (LoamEightSubProcessor& p)
{
    processor = &p;
    refreshMod();
}

void LoamKnob::cycleBinding()
{
    auto b = processor->getModBinding (paramId);
    if (! b.bound)
    {
        // Red at where the knob is now, green at the far end.
        const auto now = param->getValue();
        b = { true, false, now, now < 0.95f ? 1.0f : 0.0f };
    }
    else if (! b.reversed)
    {
        b = { true, true, b.green, b.red };
    }
    else
    {
        b = {};
    }
    processor->setModBinding (paramId, b);
    refreshMod();
}

void LoamKnob::refreshMod()
{
    if (processor == nullptr)
        return;

    const auto custom = processor->isCustomModActive();
    const auto b = processor->getModBinding (paramId);
    const auto nowModded = custom && b.bound;

    modButton.setVisible (custom);
    modButton.state = ! b.bound ? 0 : (b.reversed ? 2 : 1);
    modButton.repaint();

    // While the wheel drives the knob, the dial shows (and the readout says)
    // the position it's heard at; the slider underneath keeps its own value.
    slider.setVisible (! nowModded);
    const auto eff = processor->getEffectiveNormalised (paramId);
    if (nowModded != modded || nowModded != customShown || eff != shownNormalised)
    {
        modded = nowModded;
        customShown = nowModded;
        shownNormalised = eff;
        setReadout (modded ? param->convertFrom0to1 (eff) : (float) slider.getValue());
        repaint();
    }
}

juce::Point<float> LoamKnob::dotPosition (float normalised) const
{
    const auto area = dialBounds().toFloat();
    const auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 5.0f;
    const auto angle = juce::MathConstants<float>::pi * (1.25f + 1.5f * normalised);
    return area.getCentre() + juce::Point<float> (std::sin (angle), -std::cos (angle)) * radius;
}

void LoamKnob::paint (juce::Graphics& g)
{
    if (! modded)
        return;
    const auto area = dialBounds();
    getLookAndFeel().drawRotarySlider (g, area.getX(), area.getY(), area.getWidth(), area.getHeight(), shownNormalised,
                                       juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, slider);
}

void LoamKnob::paintOverChildren (juce::Graphics& g)
{
    if (! modded)
        return;
    const auto b = processor->getModBinding (paramId);
    for (const auto& [norm, colour] : { std::pair { b.red, modRed }, std::pair { b.green, modGreen } })
    {
        const auto dot = juce::Rectangle<float> (9.0f, 9.0f).withCentre (dotPosition (norm));
        g.setColour (shadow);
        g.fillEllipse (dot.translated (0.0f, 1.0f));
        g.setColour (colour);
        g.fillEllipse (dot);
        g.setColour (panelRaised);
        g.drawEllipse (dot, 1.5f);
    }
}

void LoamKnob::setLabelToggle (juce::RangedAudioParameter& toggleParam, const juce::String& offText, const juce::String& onText)
{
    labelOff = offText;
    labelOn = onText;
    labelToggle = std::make_unique<juce::ParameterAttachment> (toggleParam, [this] (float v)
    {
        labelOnState = v > 0.5f;
        label.setText (labelOnState ? labelOn : labelOff, juce::dontSendNotification);
    });
    labelToggle->sendInitialUpdate();

    label.setInterceptsMouseClicks (true, false);
    label.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    label.setTooltip ("Click to switch the tone filter: < lowpass, > highpass");
    label.addMouseListener (this, false);
}

void LoamKnob::mouseDown (const juce::MouseEvent& e)
{
    draggingDot = 0;
    if (e.eventComponent != this || ! modded)
        return;
    const auto b = processor->getModBinding (paramId);
    const auto dRed = e.position.getDistanceFrom (dotPosition (b.red));
    const auto dGreen = e.position.getDistanceFrom (dotPosition (b.green));
    if (juce::jmin (dRed, dGreen) <= 9.0f)
        draggingDot = dRed <= dGreen ? 1 : 2;
}

void LoamKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (e.eventComponent != this || draggingDot == 0)
        return;

    const auto centre = dialBounds().toFloat().getCentre();
    auto deg = juce::radiansToDegrees (std::atan2 (e.position.x - centre.x, centre.y - e.position.y));
    if (deg > 135.0f)       deg = deg > 157.5f ? -135.0f : 135.0f;    // the gap at the bottom: snap to the nearer end
    else if (deg < -135.0f) deg = deg < -157.5f ? 135.0f : -135.0f;

    auto b = processor->getModBinding (paramId);
    (draggingDot == 1 ? b.red : b.green) = juce::jlimit (0.0f, 1.0f, (deg + 135.0f) / 270.0f);
    processor->setModBinding (paramId, b);
    refreshMod();
    repaint();
}

void LoamKnob::mouseUp (const juce::MouseEvent& e)
{
    draggingDot = 0;
    if (e.eventComponent == &label && labelToggle != nullptr && label.contains (e.getEventRelativeTo (&label).getPosition()))
        labelToggle->setValueAsCompleteGesture (labelOnState ? 0.0f : 1.0f);
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
                const juce::String& mixParamId, std::array<std::unique_ptr<LoamKnob>, 3> unitKnobs, juce::Colour accent)
    : apvts (state), mixId (mixParamId), toggleAttachment (state, switchId, toggle), knobs (std::move (unitKnobs))
{
    title.setText (titleText.toUpperCase(), juce::dontSendNotification);
    title.setFont (sansFont (10.0f, true));
    title.setBorderSize ({});
    title.setMinimumHorizontalScale (0.75f);   // "REVERB" next to the switch is a tight fit
    addAndMakeVisible (title);

    toggle.setColour (juce::ToggleButton::tickColourId, accent);
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

void FxUnit::enableTypeBlind (juce::RangedAudioParameter& typeParam)
{
    typeAttachment = std::make_unique<juce::ParameterAttachment> (typeParam, [this] (float v)
    {
        pingPong = juce::roundToInt (v) == 1;
        title.setTooltip (pingPong ? "Delay type: ping-pong (click to switch)" : "Delay type: normal (click to switch)");
        repaint();
    });
    typeAttachment->sendInitialUpdate();
    title.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    title.addMouseListener (this, false);
    title.setColour (juce::Label::textColourId, juce::Colours::transparentBlack);   // drawn rolling in paintOverChildren
}

void FxUnit::mouseEnter (const juce::MouseEvent& e)
{
    if (typeAttachment != nullptr && e.eventComponent == &title)
    {
        blindTarget = true;
        startTimerHz (60);
    }
}

void FxUnit::mouseExit (const juce::MouseEvent& e)
{
    if (typeAttachment != nullptr && e.eventComponent == &title)
    {
        blindTarget = false;
        startTimerHz (60);
    }
}

void FxUnit::mouseUp (const juce::MouseEvent& e)
{
    if (typeAttachment != nullptr && e.eventComponent == &title && title.contains (e.getEventRelativeTo (&title).getPosition()))
        typeAttachment->setValueAsCompleteGesture (pingPong ? 0.0f : 1.0f);
}

void FxUnit::timerCallback()
{
    // ~250 ms each way, like the web's CSS transition.
    blind = juce::jlimit (0.0f, 1.0f, blind + (blindTarget ? 0.067f : -0.067f));
    if (blind == (blindTarget ? 1.0f : 0.0f))
        stopTimer();
    repaint (title.getBounds().expanded (8, 3));
}

void FxUnit::paintOverChildren (juce::Graphics& g)
{
    if (typeAttachment == nullptr)
        return;

    // Exactly the title's own area, so neither the plate nor the recess touches the switch.
    const auto line = title.getBounds().toFloat().reduced (0.0f, 1.0f);
    const auto eased = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * blind);   // ease in-out

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (line.toNearestInt());   // the plate disappears under the panel's edge

    // The recess under the plate, sunk into the panel, with the delay's real nature.
    if (eased > 0.0f)
    {
        g.setColour (ink.withAlpha (0.16f));
        g.fillRoundedRectangle (line, 3.0f);
        g.setGradientFill (juce::ColourGradient (shadow, 0.0f, line.getY(), shadow.withAlpha (0.0f), 0.0f, line.getY() + 4.0f, false));
        g.fillRoundedRectangle (line.withHeight (4.0f), 3.0f);
        g.setColour (ink);
        g.setFont (sansFont (7.0f, true));
        g.drawFittedText (pingPong ? "PING-PONG" : "NORMAL", line.toNearestInt(), juce::Justification::centred, 1, 0.8f);
    }

    // The plate with DELAY, sliding up and away.
    const auto plate = line.translated (0.0f, -line.getHeight() * eased);
    g.setColour (panel);
    g.fillRect (plate);
    g.setColour (inkSoft.withMultipliedAlpha (title.getAlpha()));
    g.setFont (title.getFont());
    g.drawText (title.getText(), plate, juce::Justification::centredLeft, false);
}

// A table-tennis paddle (red rubber face, wooden handle) and a ball, drawn
// small and in full colour in the unit's bottom-right corner.
void FxUnit::drawPaddle (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto unit = juce::jmin (area.getWidth() / 40.0f, area.getHeight() / 46.0f);
    const auto origin = area.getCentre() - juce::Point<float> (20.0f, 23.0f) * unit;
    auto at = [&] (float x, float y) { return origin + juce::Point<float> (x, y) * unit; };
    const auto rotation = juce::AffineTransform::rotation (-0.52f, at (18, 24).x, at (18, 24).y);

    juce::Path handle;
    handle.addRoundedRectangle (at (15, 28).x, at (15, 28).y, 6.0f * unit, 15.0f * unit, 2.5f * unit);
    g.setColour (juce::Colour (0xffb07a45));
    g.fillPath (handle, rotation);

    juce::Path rubber;
    rubber.addEllipse (at (6, 5).x, at (6, 5).y, 24.0f * unit, 26.0f * unit);
    g.setColour (driveRed);
    g.fillPath (rubber, rotation);
    g.setColour (juce::Colour (0xff7d1f19));
    g.strokePath (rubber, juce::PathStrokeType (1.0f), rotation);

    const auto ball = juce::Rectangle<float> (8.0f * unit, 8.0f * unit).withCentre (at (34, 7));
    g.setColour (juce::Colours::white);
    g.fillEllipse (ball);
    g.setColour (grooveDark);
    g.drawEllipse (ball, 1.0f);
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

    if (pingPong)
        drawPaddle (g, juce::Rectangle<float> (24.0f, 28.0f).withPosition (bounds.getRight() - 27.0f, bounds.getBottom() - 32.0f));
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

    // Octave name on every C, high on the key: shows where you are.
    if (midiNoteNumber % 12 == 0)
    {
        g.setColour (coral);
        g.setFont (monoFont (9.0f));
        g.drawText (juce::MidiMessage::getMidiNoteName (midiNoteNumber, true, true, 4),
                    area.withTrimmedTop (area.getHeight() * getBlackNoteLengthProportion() + 4.0f).withHeight (14.0f),
                    juce::Justification::centred, false);   // just below the black keys
    }

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
WheelView::WheelView (const juce::String& wheelName, std::atomic<float>& wheelValue, bool isBipolar)
    : name (wheelName), value (wheelValue), bipolar (isBipolar)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setTitle (name + " wheel");
}

juce::Rectangle<float> WheelView::slot() const
{
    return getLocalBounds().toFloat();
}

void WheelView::setFromY (float y)
{
    const auto s = slot();
    const auto t = juce::jlimit (0.0f, 1.0f, 1.0f - (y - s.getY()) / s.getHeight());
    value = bipolar ? t * 2.0f - 1.0f : t;
    refresh();
}

void WheelView::mouseDown (const juce::MouseEvent& e) { setFromY (e.position.y); }
void WheelView::mouseDrag (const juce::MouseEvent& e) { setFromY (e.position.y); }

void WheelView::mouseUp (const juce::MouseEvent&)
{
    if (bipolar)
    {
        value = 0.0f;   // a pitch wheel springs back to the centre
        refresh();
    }
}

void WheelView::refresh()
{
    if (const auto v = value.load(); v != shown)
    {
        shown = v;
        repaint();
    }
}

void WheelView::paint (juce::Graphics& g)
{
    const auto s = slot();
    g.setColour (ink.withAlpha (0.35f));
    g.fillRoundedRectangle (s, 5.0f);

    // The wheel face, with ridges that roll as it turns.
    const auto face = s.reduced (3.0f);
    g.setGradientFill (juce::ColourGradient (groove, face.getX(), 0.0f, grooveDark, face.getRight(), 0.0f, false));
    g.fillRoundedRectangle (face, 4.0f);

    const auto t = bipolar ? (shown + 1.0f) * 0.5f : shown;
    const auto y = face.getBottom() - t * face.getHeight();
    g.setColour (ink.withAlpha (0.18f));
    for (float ridge = std::fmod (y - face.getY(), 6.0f); ridge < face.getHeight(); ridge += 6.0f)
        g.drawHorizontalLine (juce::roundToInt (face.getY() + ridge), face.getX() + 2.0f, face.getRight() - 2.0f);

    if (bipolar)
    {
        g.setColour (ink.withAlpha (0.35f));
        g.drawHorizontalLine (juce::roundToInt (face.getCentreY()), face.getX(), face.getRight());
    }
    g.setColour (bipolar ? coral : teal);
    g.fillRoundedRectangle ({ face.getX(), y - 2.0f, face.getWidth(), 4.0f }, 2.0f);

    if (centreText.isNotEmpty())
    {
        const auto badge = juce::Rectangle<float> (face.getWidth() - 2.0f, 16.0f).withCentre (face.getCentre());
        g.setColour (panelRaised.withAlpha (0.9f));
        g.fillRoundedRectangle (badge, 3.0f);
        g.setColour (ink);
        g.setFont (monoFont (11.0f));
        g.drawText (centreText, badge, juce::Justification::centred, false);
    }
}

//==============================================================================
namespace
{
    // Same drawing as the web versions (loam-daboom / loam-eight-sub index.html).
    constexpr const char* egretSvg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24"> <path d="M14.5 4.2c1.6-.3 2.8.6 2.8 1.8 0 .9-.6 1.5-1.4 1.7-.4 1.7-.2 3.4.6 4.8 1 1.8.5 4-1.6 5.2-1.9 1.1-4.6 1-6.4-.4-1.3-1-1.9-2.4-1.7-3.6 2.2.8 4.1.4 5.4-.9.9-1 .9-2.4.3-3.8-.6-1.4-.4-3.2 2-4.8z" fill="#fbf8ef" stroke="#2a241d" stroke-width=".8" stroke-linejoin="round"/> <path d="M17.2 5.4l4.6 1.1-4.4.6z" fill="#2a241d"/> <circle cx="15.6" cy="5.2" r=".45" fill="#2a241d"/> <path d="M14.2 4.4c-1.2-.6-2-.3-2.6.4" fill="none" stroke="#2a241d" stroke-width=".5"/> <path d="M10.8 17.6l-.6 4.2M12.6 17.7l.4 4.1" stroke="#2a241d" stroke-width=".9" stroke-linecap="round"/> <path d="M9.3 21.9h1.8M12.3 21.9h1.8" stroke="#e8c32a" stroke-width="1.4" stroke-linecap="round"/> </svg>)svg";

    juce::String subtitleText()  { return utf8 ("\xe5\xb0\x8f\xe9\xb7\xba \xc2\xb7 POLYPHONIC VOICE SYNTHESIZER"); }
    juce::String egretNameText() { return utf8 ("\xe3\x82\xb3\xe3\x82\xb5\xe3\x82\xae \xc2\xb7 KOSAGI \xc2\xb7 LITTLE EGRET"); }
}

EgretView::EgretView()
{
    // createFromImageData recognises SVG text, and exists in every JUCE version.
    drawable = juce::Drawable::createFromImageData (egretSvg, std::strlen (egretSvg));
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle ("Kosagi, the little egret");
    setTooltip (utf8 ("\xe5\xb0\x8f\xe9\xb7\xba"));
}

void EgretView::paint (juce::Graphics& g)
{
    if (drawable != nullptr)
        drawable->drawWithin (g, getLocalBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
}

//==============================================================================
PortaRangeStrip::PortaRangeStrip (juce::AudioProcessorValueTreeState& state, LoamKeyboard& kb)
    : apvts (state), keyboard (kb),
      lowParam (state.getParameter ("portaLow")), highParam (state.getParameter ("portaHigh"))
{
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
}

int PortaRangeStrip::noteAt (float x) const
{
    // Probe the keyboard just below the black keys' tips so either colour can be picked.
    const auto kx = x + (float) (getX() - keyboard.getX());
    return keyboard.getNoteAndVelocityAtPosition ({ kx, (float) keyboard.getHeight() * 0.4f }).note;
}

void PortaRangeStrip::moveEnd (int note)
{
    if (note < 0)
        return;

    auto low = (int) lowParam->convertFrom0to1 (lowParam->getValue());
    auto high = (int) highParam->convertFrom0to1 (highParam->getValue());
    if (draggingLow) low = note; else high = note;
    if (low > high)
    {
        std::swap (low, high);
        draggingLow = ! draggingLow;   // keep dragging the same edge after crossing over
    }
    lowParam->setValueNotifyingHost (lowParam->convertTo0to1 ((float) low));
    highParam->setValueNotifyingHost (highParam->convertTo0to1 ((float) high));
    repaint();
}

void PortaRangeStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto note = noteAt (e.position.x);
    if (note < 0)
        return;

    const auto low = (int) lowParam->convertFrom0to1 (lowParam->getValue());
    const auto high = (int) highParam->convertFrom0to1 (highParam->getValue());
    draggingLow = std::abs (note - low) <= std::abs (note - high);

    lowParam->beginChangeGesture();
    highParam->beginChangeGesture();
    moveEnd (note);
}

void PortaRangeStrip::mouseDrag (const juce::MouseEvent& e)
{
    moveEnd (noteAt (e.position.x));
}

void PortaRangeStrip::mouseUp (const juce::MouseEvent&)
{
    lowParam->endChangeGesture();
    highParam->endChangeGesture();
}

void PortaRangeStrip::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (ink.withAlpha (0.12f));
    g.fillRoundedRectangle (bounds, 4.0f);

    const auto low = (int) lowParam->convertFrom0to1 (lowParam->getValue());
    const auto high = (int) highParam->convertFrom0to1 (highParam->getValue());
    const auto first = keyboard.getBaseNote(), last = first + LoamKeyboard::numKeys - 1;
    const auto label = juce::MidiMessage::getMidiNoteName (low, true, true, 4) + utf8 ("â")
                     + juce::MidiMessage::getMidiNoteName (high, true, true, 4);

    g.setFont (monoFont (9.0f).withExtraKerningFactor (0.08f));

    if (high < first || low > last)
    {
        // Range is outside the visible octaves: point at it.
        g.setColour (ink.withAlpha (0.7f));
        g.drawText ((high < first ? utf8 ("â PORTA ") : juce::String ("PORTA ")) + label
                        + (low > last ? utf8 (" â¶") : juce::String()),
                    bounds.reduced (6.0f, 0.0f), high < first ? juce::Justification::centredLeft : juce::Justification::centredRight);
        return;
    }

    const auto dx = (float) (keyboard.getX() - getX());
    const auto left = keyboard.getRectangleForKey (juce::jmax (low, first)).getX() + dx;
    const auto right = keyboard.getRectangleForKey (juce::jmin (high, last)).getRight() + dx;
    const auto bar = juce::Rectangle<float> (left, bounds.getY() + 2.0f, right - left, bounds.getHeight() - 4.0f);

    g.setColour (coral.withAlpha (0.3f));
    g.fillRoundedRectangle (bar, 3.0f);
    g.setColour (coral);
    g.drawRoundedRectangle (bar, 3.0f, 1.0f);
    for (auto x : { bar.getX() + 1.0f, bar.getRight() - 5.0f })
        g.fillRoundedRectangle ({ x, bar.getY() + 2.0f, 4.0f, bar.getHeight() - 4.0f }, 2.0f);

    g.setColour (ink);
    g.drawText ("PORTA " + label, bar, juce::Justification::centred, false);
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
                 { std::make_unique<LoamKnob> (p.apvts, "delayMix", "Mix", lavender, "%"),
                   std::make_unique<LoamKnob> (p.apvts, "delayTime", "Time", lavender, "ms"),
                   std::make_unique<LoamKnob> (p.apvts, "delayFeedback", "Feedbk", lavender, "%") }),
      reverbUnit (p.apvts, "Reverb", "reverbOn", "reverbMix",
                  { std::make_unique<LoamKnob> (p.apvts, "reverbMix", "Mix", lavender, "%"),
                    std::make_unique<LoamKnob> (p.apvts, "reverbSize", "Size", lavender, "s", 1),
                    std::make_unique<LoamKnob> (p.apvts, "reverbTone", "Tone", lavender, "Hz") }),
      satUnit (p.apvts, "Sat", "satOn", "satMix",
               { std::make_unique<LoamKnob> (p.apvts, "satDrive", "Drive", driveRed, "%"),
                 std::make_unique<LoamKnob> (p.apvts, "satTone", "Tone", driveRed, "Hz"),
                 std::make_unique<LoamKnob> (p.apvts, "satMix", "Mix", driveRed, "%") }, driveRed),
      distUnit (p.apvts, "Dist", "distOn", "distMix",
                { std::make_unique<LoamKnob> (p.apvts, "distDrive", "Drive", driveRed, "%"),
                  std::make_unique<LoamKnob> (p.apvts, "distTone", "Tone", driveRed, "Hz"),
                  std::make_unique<LoamKnob> (p.apvts, "distMix", "Mix", driveRed, "%") }, driveRed),
      keyboard (p.keyboardState),
      portaToggleAttachment (p.apvts, "portaOn", portaToggle),
      glideAttachment (p.apvts, "portaTime", glideSlider),
      portaStrip (p.apvts, keyboard),
      pitchWheel ("PITCH", p.bendWheel, true),
      modWheel ("MOD", p.modWheel, false)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (scope);

    brandSub = subtitleText();
    egret.onClick = [this]
    {
        brandSub = egretNameText();
        repaint (headerArea);
        juce::Timer::callAfterDelay (2600, [safe = juce::Component::SafePointer<LoamEightSubEditor> (this)]
        {
            if (safe != nullptr)
            {
                safe->brandSub = subtitleText();
                safe->repaint (safe->headerArea);
            }
        });
    };
    addAndMakeVisible (egret);

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
    for (auto* k : allKnobs())
        k->attachMod (p);

    delayUnit.enableTypeBlind (*p.apvts.getParameter ("delayMode"));

    // TONE on the drive units and the reverb: click the label to flip lowpass / highpass.
    satUnit.getKnobs()[1]->setLabelToggle (*p.apvts.getParameter ("satToneHp"), "TONE <", "TONE >");
    distUnit.getKnobs()[1]->setLabelToggle (*p.apvts.getParameter ("distToneHp"), "TONE <", "TONE >");
    reverbUnit.getKnobs()[2]->setLabelToggle (*p.apvts.getParameter ("reverbToneHp"), "TONE <", "TONE >");

    addAndMakeVisible (delayUnit);
    addAndMakeVisible (reverbUnit);
    addChildComponent (satUnit);
    addChildComponent (distUnit);

    for (auto* b : { &fxPrev, &fxNext })
    {
        b->setWantsKeyboardFocus (false);
        b->setColour (juce::TextButton::buttonOnColourId, lavender);
        addAndMakeVisible (*b);
    }
    fxPageName.setFont (monoFont (10.0f).withExtraKerningFactor (0.12f));
    fxPageName.setColour (juce::Label::textColourId, lavender);
    fxPageName.setJustificationType (juce::Justification::centred);
    fxPageName.setBorderSize ({});
    addAndMakeVisible (fxPageName);
    fxPrev.setTitle ("Previous effects page");
    fxNext.setTitle ("Next effects page");
    fxPrev.onClick = [this] { setFxPage (fxPage - 1); };
    fxNext.onClick = [this] { setFxPage (fxPage + 1); };
    setFxPage (0);

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

    // ---- portamento: switch + glide time above the keys, range strip below them
    for (auto [l, text] : { std::pair { &portaLabel, "PORTA" }, std::pair { &glideLabel, "GLIDE" } })
    {
        l->setText (text, juce::dontSendNotification);
        l->setFont (monoFont (10.0f).withExtraKerningFactor (0.12f));
        l->setColour (juce::Label::textColourId, ink);
        l->setJustificationType (juce::Justification::centredRight);
        l->setBorderSize ({});
        addAndMakeVisible (*l);
    }
    portaToggle.setColour (juce::ToggleButton::tickColourId, coral);
    portaToggle.setWantsKeyboardFocus (false);
    portaToggle.setTitle ("Portamento on");
    addAndMakeVisible (portaToggle);

    glideSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    glideSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    glideSlider.setColour (juce::Slider::backgroundColourId, ink.withAlpha (0.2f));
    glideSlider.setColour (juce::Slider::trackColourId, coral);
    glideSlider.setColour (juce::Slider::thumbColourId, panelRaised);
    glideSlider.setWantsKeyboardFocus (false);
    glideSlider.setDoubleClickReturnValue (true, 150.0);
    glideSlider.onValueChange = [this] { glideReadout.setText (juce::String (juce::roundToInt (glideSlider.getValue())) + " ms", juce::dontSendNotification); };
    glideSlider.onValueChange();
    addAndMakeVisible (glideSlider);
    glideReadout.setFont (monoFont (10.0f));
    glideReadout.setColour (juce::Label::textColourId, ink);
    glideReadout.setBorderSize ({});
    addAndMakeVisible (glideReadout);
    addChildComponent (portaStrip);

    // ---- wheels: bend range on + / - around the pitch wheel, mod mode above the mod wheel
    addAndMakeVisible (pitchWheel);
    addAndMakeVisible (modWheel);

    bendRangeAttachment = std::make_unique<juce::ParameterAttachment> (
        *p.apvts.getParameter ("bendRange"),
        [this] (float index)
        {
            pitchWheel.centreText = juce::String (juce::roundToInt (index) + 1);   // choice index 0 = 1 semitone
            pitchWheel.repaint();
        });
    bendRangeAttachment->sendInitialUpdate();
    bendUp.setTitle ("Wider pitch bend range");
    bendDown.setTitle ("Narrower pitch bend range");
    bendUp.onClick = [this] { stepBendRange (1); };
    bendDown.onClick = [this] { stepBendRange (-1); };

    modModeAttachment = std::make_unique<juce::ParameterAttachment> (
        *p.apvts.getParameter ("modMode"),
        [this] (float v)
        {
            const char* names[] = { "VIB", "FILT", "CUST" };
            modMode = juce::jlimit (0, 2, juce::roundToInt (v));
            modModeButton.setButtonText (names[modMode]);
        });
    modModeAttachment->sendInitialUpdate();
    modModeButton.setTitle ("Mod wheel mode: vibrato, filter or custom");
    modModeButton.setColour (juce::TextButton::buttonOnColourId, teal);
    modModeButton.setToggleState (true, juce::dontSendNotification);   // always shown lit, in the wheel's colour
    modModeButton.onClick = [this] { modModeAttachment->setValueAsCompleteGesture ((float) ((modMode + 1) % 3)); };

    modWheelLabel.setText ("MOD", juce::dontSendNotification);
    modWheelLabel.setFont (monoFont (8.5f));
    modWheelLabel.setColour (juce::Label::textColourId, ink);
    modWheelLabel.setJustificationType (juce::Justification::centred);
    modWheelLabel.setBorderSize ({});

    for (auto* c : std::initializer_list<juce::Component*> { &bendUp, &bendDown, &modModeButton, &modWheelLabel })
    {
        c->setWantsKeyboardFocus (false);
        addAndMakeVisible (*c);
    }

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
std::vector<LoamKnob*> LoamEightSubEditor::allKnobs()
{
    std::vector<LoamKnob*> list { &tune, &sub, &cutoff, &resonance, &attack, &decay, &sustain, &release, &volume };
    for (auto* unit : { &delayUnit, &reverbUnit, &satUnit, &distUnit })
        for (auto& k : unit->getKnobs())
            list.push_back (k.get());
    return list;
}

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
        audioProcessor.applyFactoryPreset (presetType, presetIndex);
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

void LoamEightSubEditor::setFxPage (int page)
{
    // Page 0: time effects (delay, reverb). Page 1: drive (saturation, distortion).
    fxPage = (page % numFxPages + numFxPages) % numFxPages;
    delayUnit.setVisible (fxPage == 0);
    reverbUnit.setVisible (fxPage == 0);
    satUnit.setVisible (fxPage == 1);
    distUnit.setVisible (fxPage == 1);
    fxPageName.setText (fxPage == 0 ? "TIME" : "DRIVE", juce::dontSendNotification);

    // Drive gets a red look; Time keeps the module's lavender.
    fx.accent = fxPage == 1 ? driveRed : lavender;
    fxPageName.setColour (juce::Label::textColourId, fx.accent);
    for (auto* b : { &fxPrev, &fxNext })
        b->setColour (juce::TextButton::buttonOnColourId, fx.accent);
    repaint (fx.bounds);
}

void LoamEightSubEditor::stepBendRange (int delta)
{
    auto* param = audioProcessor.apvts.getParameter ("bendRange");
    const auto index = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    bendRangeAttachment->setValueAsCompleteGesture ((float) juce::jlimit (0, 11, index + delta));
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
    for (auto* unit : { &delayUnit, &reverbUnit, &satUnit, &distUnit })
        unit->refreshDimming();

    if (const auto note = audioProcessor.lastNote.load(); note != shownNote)
    {
        shownNote = note;
        repaint (headerArea);
    }

    // PORTA shows its range strip (and makes room for it) only while it's on.
    if (portaToggle.getToggleState() != portaShown)
        resized();
    if (portaShown)
        portaStrip.repaint();
    pitchWheel.refresh();
    modWheel.refresh();
    for (auto* k : allKnobs())
        k->refreshMod();

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
    g.drawText (brandSub, brand.removeFromTop (18.0f), juce::Justification::centredLeft, false);

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

    // The egret stands just after the subtitle, under the brand name.
    const auto subWidth = juce::GlyphArrangement::getStringWidth (monoFont (10.0f).withExtraKerningFactor (0.14f), subtitleText());
    egret.setBounds (headerArea.getX() + juce::roundToInt (subWidth) + 8, headerArea.getY() + 30, 22, 22);

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
    const auto leftUnit = fxArea.removeFromLeft (unitWidth), rightUnit = fxArea.removeFromRight (unitWidth);
    delayUnit.setBounds (leftUnit);
    reverbUnit.setBounds (rightUnit);
    satUnit.setBounds (leftUnit);
    distUnit.setBounds (rightUnit);

    auto fxTitleRow = fx.bounds.reduced (12, 0).withTrimmedTop (7).withHeight (18);
    fxNext.setBounds (fxTitleRow.removeFromRight (22));
    fxPageName.setBounds (fxTitleRow.removeFromRight (50));
    fxPrev.setBounds (fxTitleRow.removeFromRight (22));

    // ---- keyboard with octave buttons above it
    auto kb = keyboardBay.reduced (8);
    auto octaveRow = kb.removeFromTop (24);
    octaveDown.setBounds (octaveRow.removeFromLeft (28));
    octaveLabel.setBounds (octaveRow.removeFromLeft (64));
    octaveUp.setBounds (octaveRow.removeFromLeft (28));
    sustainLight = octaveRow.removeFromRight (90);
    octaveRow.removeFromLeft (12);
    portaLabel.setBounds (octaveRow.removeFromLeft (40));
    portaToggle.setBounds (octaveRow.removeFromLeft (36));
    octaveRow.removeFromLeft (6);
    glideLabel.setBounds (octaveRow.removeFromLeft (38));
    glideSlider.setBounds (octaveRow.removeFromLeft (80).reduced (4, 0));
    glideReadout.setBounds (octaveRow.removeFromLeft (50));
    kb.removeFromTop (6);

    // Pitch and mod wheels to the left of the keys, like on a hardware keyboard:
    // + / wheel / - for pitch, mode button / wheel / MOD for the mod wheel.
    auto wheels = kb.removeFromLeft (70);
    auto pitchColumn = wheels.removeFromLeft (32), modColumn = wheels.removeFromRight (32);
    bendUp.setBounds (pitchColumn.removeFromTop (18));
    bendDown.setBounds (pitchColumn.removeFromBottom (18));
    pitchWheel.setBounds (pitchColumn.reduced (5, 3));
    modModeButton.setBounds (modColumn.removeFromTop (18));
    modWheelLabel.setBounds (modColumn.removeFromBottom (18));
    modWheel.setBounds (modColumn.reduced (5, 3));
    kb.removeFromLeft (8);

    portaShown = portaToggle.getToggleState();
    portaStrip.setVisible (portaShown);
    glideSlider.setAlpha (portaShown ? 1.0f : 0.45f);
    glideReadout.setAlpha (portaShown ? 1.0f : 0.45f);
    if (portaShown)
    {
        portaStrip.setBounds (kb.removeFromBottom (18));
        kb.removeFromBottom (4);
    }
    keyboard.setBounds (kb);
    keyboard.setKeyWidth ((float) kb.getWidth() / 17.0f);   // 17 white keys, C to E two octaves up
}
