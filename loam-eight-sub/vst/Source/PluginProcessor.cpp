#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr auto waveId = "wave", tuneId = "tune", subId = "sub", cutoffId = "cutoff",
                    resonanceId = "resonance", attackId = "attack", decayId = "decay",
                    sustainId = "sustain", releaseId = "release", volumeId = "volume",
                    delayTimeId = "delayTime", delayFeedbackId = "delayFeedback", delayMixId = "delayMix",
                    reverbMixId = "reverbMix", reverbSizeId = "reverbSize", reverbToneId = "reverbTone",
                    delayOnId = "delayOn", reverbOnId = "reverbOn",
                    portaOnId = "portaOn", portaTimeId = "portaTime", portaLowId = "portaLow", portaHighId = "portaHigh",
                    modModeId = "modMode", bendRangeId = "bendRange",
                    satOnId = "satOn", satDriveId = "satDrive", satToneId = "satTone", satMixId = "satMix",
                    distOnId = "distOn", distDriveId = "distDrive", distToneId = "distTone", distMixId = "distMix";

    // Mod wheel at the top: vibrato depth in semitones, or filter opening in octaves.
    constexpr float maxVibratoSemis = 0.6f, maxFilterOctaves = 4.0f;

    // Same factory presets as the web version (index.html, FACTORY_PRESETS).
    // wave: 0 Sine, 1 Saw, 2 Square, 3 Triangle. Effects start on when their mix is up.
    struct Row
    {
        const char* name;
        float wave, tune, sub, cutoff, resonance, attack, decay, sustain, release, volume,
              delayTime, delayFeedback, delayMix, reverbMix, reverbSize, reverbTone;
    };

    //                         wave tune sub cutoff reso  atk   dec   sus  rel   vol  dTime dFb dMix rMix rSize  rTone
    const Row leadRows[] = {
        { "Classic",      1,   0,   0,  4200,  3,    5,    120,  80,  150,  72,  160,  20, 12,  10,  1.5f,  7000 },
        { "Square Lead",  2,   0,   0,  3200,  4,    3,    150,  75,  180,  70,  300,  30, 18,  12,  1.8f,  6500 },
        { "Soft Sine",    0,   0,   0,  6000,  1,    40,   200,  85,  350,  70,  260,  25, 20,  25,  2.5f,  5000 },
        { "Detuned Saw",  1,   12,  30, 5200,  2,    8,    150,  80,  220,  70,  180,  25, 15,  15,  2.0f,  7000 },
    };
    const Row bassRows[] = {
        { "Classic",      2,   0,   60, 900,   6,    2,    200,  60,  120,  78,  220,  0,  0,   5,   1.2f,  4000 },
        { "Sub Sine",     0,   0,   80, 600,   1,    2,    250,  80,  100,  80,  220,  0,  0,   0,   1.0f,  3000 },
        { "Acid",         1,   0,   0,  700,   14,   1,    160,  30,  90,   72,  330,  30, 10,  5,   1.0f,  4000 },
        { "Round Tri",    3,   0,   50, 1400,  2,    5,    300,  70,  150,  78,  220,  0,  0,   8,   1.4f,  4500 },
    };
    const Row padRows[] = {
        { "Classic",      3,   0,   20, 1800,  1,    400,  600,  85,  900,  62,  380,  35, 28,  45,  4.5f,  5000 },
        { "Warm Saw",     1,   0,   25, 1200,  1.5f, 700,  900,  90,  1500, 58,  450,  30, 20,  50,  5.0f,  4500 },
        { "Glass",        0,   0,   0,  9000,  1,    900,  800,  80,  2200, 60,  520,  45, 30,  60,  6.0f,  9000 },
        { "Dark Drone",   2,   0,   60, 500,   3,    1200, 1500, 100, 2500, 60,  600,  50, 25,  55,  6.0f,  2500 },
    };
    const Row pluckRows[] = {
        { "Classic",      1,   0,   0,  3000,  8,    2,    180,  10,  200,  70,  220,  40, 22,  15,  1.8f,  6500 },
        { "Bell",         0,   0,   0,  8000,  1,    1,    600,  0,   900,  72,  280,  35, 20,  30,  3.0f,  8000 },
        { "Kalimba",      3,   0,   0,  4000,  3,    1,    250,  0,   300,  74,  200,  30, 15,  20,  1.6f,  6000 },
        { "Short Square", 2,   0,   20, 2200,  6,    1,    120,  0,   120,  70,  160,  45, 28,  12,  1.2f,  5000 },
    };

    LoamEightSubProcessor::Preset toPreset (const Row& r)
    {
        return { r.name, {
            { waveId, r.wave }, { tuneId, r.tune }, { subId, r.sub }, { cutoffId, r.cutoff },
            { resonanceId, r.resonance }, { attackId, r.attack }, { decayId, r.decay },
            { sustainId, r.sustain }, { releaseId, r.release }, { volumeId, r.volume },
            { delayTimeId, r.delayTime }, { delayFeedbackId, r.delayFeedback }, { delayMixId, r.delayMix },
            { reverbMixId, r.reverbMix }, { reverbSizeId, r.reverbSize }, { reverbToneId, r.reverbTone },
            { delayOnId, r.delayMix > 0 ? 1.0f : 0.0f }, { reverbOnId, r.reverbMix > 0 ? 1.0f : 0.0f },
            { satOnId, 0.0f }, { distOnId, 0.0f },   // factory sounds are clean
        } };
    }

    template <size_t N>
    std::vector<LoamEightSubProcessor::Preset> toPresets (const Row (&rows)[N])
    {
        std::vector<LoamEightSubProcessor::Preset> out;
        for (const auto& r : rows)
            out.push_back (toPreset (r));
        return out;
    }

    // Presets and projects saved before the effect switches existed have no
    // delayOn/reverbOn; treat an effect as on if its mix was up, as before.
    void addMissingSwitches (LoamEightSubProcessor::ParamValues& values)
    {
        auto find = [&values] (const juce::String& id) -> const float*
        {
            for (const auto& [key, value] : values)
                if (key == id)
                    return &value;
            return nullptr;
        };

        const std::pair<const char*, const char*> switches[] = { { delayOnId, delayMixId }, { reverbOnId, reverbMixId } };
        for (const auto& [switchId, mixId] : switches)
            if (find (switchId) == nullptr)
                if (const auto* mix = find (mixId))
                    values.push_back ({ switchId, *mix > 0 ? 1.0f : 0.0f });

        // ...and the drive stages didn't exist at all: keep them off.
        for (const auto* driveSwitch : { satOnId, distOnId })
            if (find (driveSwitch) == nullptr)
                values.push_back ({ driveSwitch, 0.0f });
    }

    constexpr auto presetTag = "LoamPreset";
    const juce::Identifier modMapTag ("MODMAP"), modSlotTag ("SLOT");
}

LoamEightSubProcessor::LoamEightSubProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    jassert ((int) modSlots.size() == getModTargetIds().size());
}

//==============================================================================
const juce::StringArray& LoamEightSubProcessor::getModTargetIds()
{
    // Every knob on the panel (not the glide slider or switches).
    static const juce::StringArray ids { tuneId, subId, cutoffId, resonanceId, attackId, decayId, sustainId,
                                         releaseId, volumeId, delayTimeId, delayFeedbackId, delayMixId,
                                         reverbMixId, reverbSizeId, reverbToneId,
                                         satDriveId, satToneId, satMixId, distDriveId, distToneId, distMixId };
    return ids;
}

LoamEightSubProcessor::ModBinding LoamEightSubProcessor::getModBinding (const juce::String& paramId) const
{
    const auto index = getModTargetIds().indexOf (paramId);
    if (index < 0)
        return {};
    const auto& slot = modSlots[(size_t) index];
    return { slot.bound.load(), slot.reversed.load(), slot.red.load(), slot.green.load() };
}

void LoamEightSubProcessor::setModBinding (const juce::String& paramId, ModBinding b)
{
    const auto index = getModTargetIds().indexOf (paramId);
    if (index < 0)
        return;

    auto& slot = modSlots[(size_t) index];
    slot.red = b.red;
    slot.green = b.green;
    slot.reversed = b.reversed;
    slot.bound = b.bound;

    // Mirror into the state tree so the host's project (and copyState) keeps it.
    auto map = apvts.state.getOrCreateChildWithName (modMapTag, nullptr);
    auto existing = map.getChildWithProperty ("id", paramId);
    if (! b.bound)
    {
        map.removeChild (existing, nullptr);
        return;
    }
    if (! existing.isValid())
    {
        existing = juce::ValueTree (modSlotTag);
        existing.setProperty ("id", paramId, nullptr);
        map.appendChild (existing, nullptr);
    }
    existing.setProperty ("red", b.red, nullptr);
    existing.setProperty ("green", b.green, nullptr);
    existing.setProperty ("reversed", b.reversed, nullptr);
}

void LoamEightSubProcessor::loadModMapFromState()
{
    const auto map = apvts.state.getChildWithName (modMapTag);
    const auto& ids = getModTargetIds();
    for (int i = 0; i < ids.size(); ++i)
    {
        const auto slotTree = map.getChildWithProperty ("id", ids[i]);
        auto& slot = modSlots[(size_t) i];
        slot.red = (float) slotTree.getProperty ("red", 0.0f);
        slot.green = (float) slotTree.getProperty ("green", 1.0f);
        slot.reversed = (bool) slotTree.getProperty ("reversed", false);
        slot.bound = slotTree.isValid();
    }
}

bool LoamEightSubProcessor::isCustomModActive() const
{
    return (int) apvts.getRawParameterValue (modModeId)->load() == 2;
}

float LoamEightSubProcessor::getEffectiveNormalised (const juce::String& paramId) const
{
    auto* param = apvts.getParameter (paramId);
    if (param == nullptr)
        return 0.0f;
    const auto b = getModBinding (paramId);
    if (! b.bound || ! isCustomModActive())
        return param->getValue();
    return b.red + (b.green - b.red) * modWheel.load();
}

juce::AudioProcessorValueTreeState::ParameterLayout LoamEightSubProcessor::createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID (waveId, 1), "Wave",
        juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));

    params.push_back (std::make_unique<Param> (juce::ParameterID (tuneId, 1), "Tune", Range (-100.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (subId, 1), "Sub", Range (0.0f, 100.0f), 0.0f));
    // Cutoff and reverb tone sweep logarithmically, like the web knobs.
    Range cutoffRange (80.0f, 12000.0f);
    cutoffRange.setSkewForCentre (980.0f);
    Range toneRange (500.0f, 12000.0f);
    toneRange.setSkewForCentre (2450.0f);

    params.push_back (std::make_unique<Param> (juce::ParameterID (cutoffId, 1), "Cutoff", cutoffRange, 2200.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (resonanceId, 1), "Resonance", Range (0.1f, 20.0f), 1.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (attackId, 1), "Attack", Range (1.0f, 2000.0f), 10.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (decayId, 1), "Decay", Range (1.0f, 2000.0f), 150.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (sustainId, 1), "Sustain", Range (0.0f, 100.0f), 70.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (releaseId, 1), "Release", Range (1.0f, 3000.0f), 300.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (volumeId, 1), "Volume", Range (0.0f, 100.0f), 70.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (delayTimeId, 1), "Delay Time", Range (20.0f, 800.0f), 220.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (delayFeedbackId, 1), "Delay Feedback", Range (0.0f, 85.0f), 30.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (delayMixId, 1), "Delay Mix", Range (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (reverbMixId, 1), "Reverb Mix", Range (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (reverbSizeId, 1), "Reverb Size", Range (0.3f, 6.0f), 2.2f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (reverbToneId, 1), "Reverb Tone", toneRange, 6000.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (delayOnId, 1), "Delay On", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (reverbOnId, 1), "Reverb On", false));

    // Portamento: a performance setting, so presets leave it alone.
    Range glideRange (5.0f, 2000.0f);
    glideRange.setSkewForCentre (150.0f);
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (portaOnId, 1), "Porta On", false));
    params.push_back (std::make_unique<Param> (juce::ParameterID (portaTimeId, 1), "Glide Time", glideRange, 150.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID (portaLowId, 1), "Porta Low Key", 0, 127, 60));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID (portaHighId, 1), "Porta High Key", 0, 127, 76));

    // Drive page: saturation and distortion.
    Range driveToneRange (500.0f, 12000.0f);
    driveToneRange.setSkewForCentre (2450.0f);
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (satOnId, 1), "Saturation On", false));
    params.push_back (std::make_unique<Param> (juce::ParameterID (satDriveId, 1), "Saturation Drive", Range (0.0f, 100.0f), 30.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (satToneId, 1), "Saturation Tone", driveToneRange, 8000.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (satMixId, 1), "Saturation Mix", Range (0.0f, 100.0f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID (distOnId, 1), "Distortion On", false));
    params.push_back (std::make_unique<Param> (juce::ParameterID (distDriveId, 1), "Distortion Drive", Range (0.0f, 100.0f), 40.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (distToneId, 1), "Distortion Tone", driveToneRange, 5000.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (distMixId, 1), "Distortion Mix", Range (0.0f, 100.0f), 50.0f));

    // Wheels: what the mod wheel does, and how far pitch bend reaches.
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID (modModeId, 1), "Mod Wheel", juce::StringArray { "Vibrato", "Filter", "Custom" }, 0));
    juce::StringArray bendChoices;
    for (int semis = 1; semis <= 12; ++semis)
        bendChoices.add (juce::String (semis));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID (bendRangeId, 1), "Bend Range", bendChoices, 1));   // index 1 = 2 semitones

    return { params.begin(), params.end() };
}

void LoamEightSubProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

void LoamEightSubProcessor::pullParameters()
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    // A switched-off effect keeps its knob settings but contributes nothing;
    // the delay's feedback goes to zero too so an old tail can't resurface.
    const bool delayOn = get (delayOnId) > 0.5f;
    const bool reverbOn = get (reverbOnId) > 0.5f;

    engine.wave = (SynthEngine::Wave) (int) get (waveId);
    engine.tuneCents = get (tuneId);
    engine.subMixPct = get (subId);
    engine.cutoffHz = get (cutoffId);
    engine.resonanceQ = get (resonanceId);
    engine.attackMs = get (attackId);
    engine.decayMs = get (decayId);
    engine.sustainPct = get (sustainId);
    engine.releaseMs = get (releaseId);
    engine.volumePct = get (volumeId);
    engine.delayTimeMs = get (delayTimeId);
    engine.delayFeedbackPct = delayOn ? get (delayFeedbackId) : 0.0f;
    engine.delayMixPct = delayOn ? get (delayMixId) : 0.0f;
    engine.reverbMixPct = reverbOn ? get (reverbMixId) : 0.0f;
    engine.reverbSizeSec = get (reverbSizeId);
    engine.reverbToneHz = get (reverbToneId);

    engine.saturation = { get (satOnId) > 0.5f, get (satDriveId), get (satToneId), get (satMixId) };
    engine.distortion = { get (distOnId) > 0.5f, get (distDriveId), get (distToneId), get (distMixId) };

    const auto low = (int) get (portaLowId), high = (int) get (portaHighId);
    engine.portaOn = get (portaOnId) > 0.5f;
    engine.portaTimeMs = get (portaTimeId);
    engine.portaLow = juce::jmin (low, high);
    engine.portaHigh = juce::jmax (low, high);

    const auto mod = modWheel.load();
    const auto modMode = (int) get (modModeId);
    if (modMode == 2)
        applyCustomMod();
    engine.bendSemis = bendWheel.load() * (get (bendRangeId) + 1.0f);
    engine.vibratoSemis = modMode == 0 ? mod * maxVibratoSemis : 0.0f;
    if (modMode == 1)
        engine.cutoffHz = juce::jmin (12000.0f, engine.cutoffHz * std::exp2 (mod * maxFilterOctaves));
}

void LoamEightSubProcessor::applyCustomMod()
{
    // Override the engine values of the knobs the wheel drives. The parameters
    // themselves are left alone, so host automation isn't flooded.
    const auto mod = modWheel.load();
    const auto& ids = getModTargetIds();
    auto on = [this] (const char* id) { return apvts.getRawParameterValue (id)->load() > 0.5f; };

    for (int i = 0; i < ids.size(); ++i)
    {
        const auto& slot = modSlots[(size_t) i];
        if (! slot.bound.load())
            continue;

        const auto& id = ids[i];
        auto* param = apvts.getParameter (id);
        const auto red = slot.red.load(), green = slot.green.load();
        const auto v = param->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, red + (green - red) * mod));

        if      (id == tuneId)          engine.tuneCents = v;
        else if (id == subId)           engine.subMixPct = v;
        else if (id == cutoffId)        engine.cutoffHz = v;
        else if (id == resonanceId)     engine.resonanceQ = v;
        else if (id == attackId)        engine.attackMs = v;
        else if (id == decayId)         engine.decayMs = v;
        else if (id == sustainId)       engine.sustainPct = v;
        else if (id == releaseId)       engine.releaseMs = v;
        else if (id == volumeId)        engine.volumePct = v;
        else if (id == delayTimeId)     engine.delayTimeMs = v;
        else if (id == delayFeedbackId) engine.delayFeedbackPct = on (delayOnId) ? v : 0.0f;
        else if (id == delayMixId)      engine.delayMixPct = on (delayOnId) ? v : 0.0f;
        else if (id == reverbMixId)     engine.reverbMixPct = on (reverbOnId) ? v : 0.0f;
        else if (id == reverbSizeId)    engine.reverbSizeSec = v;
        else if (id == reverbToneId)    engine.reverbToneHz = v;
        else if (id == satDriveId)      engine.saturation.drivePct = v;
        else if (id == satToneId)       engine.saturation.toneHz = v;
        else if (id == satMixId)        engine.saturation.mixPct = v;
        else if (id == distDriveId)     engine.distortion.drivePct = v;
        else if (id == distToneId)      engine.distortion.toneHz = v;
        else if (id == distMixId)       engine.distortion.mixPct = v;
    }
}

void LoamEightSubProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    pullParameters();
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    auto applySustain = [this] { engine.setSustain (midiSustain || uiSustain.load()); };
    applySustain();

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isPitchWheel())
        {
            bendWheel = juce::jlimit (-1.0f, 1.0f, (float) (msg.getPitchWheelValue() - 8192) / 8191.0f);
            pullParameters();
        }
        else if (msg.isControllerOfType (1))
        {
            modWheel = (float) msg.getControllerValue() / 127.0f;
            pullParameters();
        }
        else if (msg.isSustainPedalOn() || msg.isSustainPedalOff())
        {
            midiSustain = msg.isSustainPedalOn();
            applySustain();
        }
        else if (msg.isNoteOn())
        {
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
            lastNote = msg.getNoteNumber();
        }
        else if (msg.isNoteOff())
            engine.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            engine.allNotesOff();
    }

    sustainActive = midiSustain || uiSustain.load();

    engine.render (buffer, 0, buffer.getNumSamples());
    pushToScope (buffer);
}

void LoamEightSubProcessor::pushToScope (const juce::AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    if (numChannels == 0)
        return;

    auto pos = scopeWritePos.load (std::memory_order_relaxed);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getSample (ch, i);

        scopeRing[(size_t) (pos & (scopeSize - 1))].store (mono / (float) numChannels, std::memory_order_relaxed);
        ++pos;
    }
    scopeWritePos.store (pos, std::memory_order_release);
}

void LoamEightSubProcessor::copyScope (float* dest, int numSamples) const
{
    numSamples = juce::jmin (numSamples, scopeSize);
    const auto end = scopeWritePos.load (std::memory_order_acquire);
    for (int i = 0; i < numSamples; ++i)
        dest[i] = scopeRing[(size_t) ((end - numSamples + i) & (scopeSize - 1))].load (std::memory_order_relaxed);
}

bool LoamEightSubProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
const juce::StringArray& LoamEightSubProcessor::getPresetTypeNames()
{
    static const juce::StringArray names { "Lead", "Bass", "Pad", "Pluck" };
    return names;
}

const std::vector<LoamEightSubProcessor::Preset>& LoamEightSubProcessor::getFactoryPresets (int typeIndex)
{
    static const std::vector<Preset> lead = toPresets (leadRows), bass = toPresets (bassRows),
                                     pad = toPresets (padRows), pluck = toPresets (pluckRows), none;
    switch (typeIndex)
    {
        case 0:  return lead;
        case 1:  return bass;
        case 2:  return pad;
        case 3:  return pluck;
        default: return none;
    }
}

void LoamEightSubProcessor::applyPresetValues (const ParamValues& values)
{
    auto complete = values;
    addMissingSwitches (complete);

    for (const auto& [id, value] : complete)
    {
        if (auto* param = apvts.getParameter (id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (value));
            param->endChangeGesture();
        }
    }
}

juce::File LoamEightSubProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Loam").getChildFile ("Loam Eight Sub").getChildFile ("Presets");
}

namespace
{
    // Finds the file holding a user preset by its display name (case-insensitive).
    juce::File findUserPresetFile (const juce::String& name)
    {
        for (const auto& file : LoamEightSubProcessor::getUserPresetFolder()
                                    .findChildFiles (juce::File::findFiles, false, "*.xml"))
            if (auto xml = juce::XmlDocument::parse (file); xml != nullptr && xml->hasTagName (presetTag)
                    && xml->getStringAttribute ("name").equalsIgnoreCase (name))
                return file;
        return {};
    }
}

juce::StringArray LoamEightSubProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        if (auto xml = juce::XmlDocument::parse (file); xml != nullptr && xml->hasTagName (presetTag))
            names.add (xml->getStringAttribute ("name"));

    names.sortNatural();
    return names;
}

bool LoamEightSubProcessor::saveUserPreset (const juce::String& name)
{
    juce::XmlElement xml (presetTag);
    xml.setAttribute ("name", name);

    for (auto* p : getParameters())
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (p);
            param != nullptr && ! param->getParameterID().startsWith ("porta"))   // performance setting, not part of a sound
        {
            auto* e = xml.createNewChildElement ("PARAM");
            e->setAttribute ("id", param->getParameterID());
            e->setAttribute ("value", param->convertFrom0to1 (param->getValue()));
        }

    if (auto map = apvts.state.getChildWithName (modMapTag); map.isValid())
        if (auto mapXml = map.createXml())
            xml.addChildElement (mapXml.release());

    auto file = findUserPresetFile (name);
    if (file == juce::File())
    {
        const auto folder = getUserPresetFolder();
        if (! folder.createDirectory())
            return false;
        file = folder.getChildFile (juce::File::createLegalFileName (name) + ".xml").getNonexistentSibling();
    }
    return xml.writeTo (file);
}

bool LoamEightSubProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = findUserPresetFile (name);
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
        return false;

    ParamValues values;
    for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
        values.push_back ({ e->getStringAttribute ("id"), (float) e->getDoubleAttribute ("value") });

    applyPresetValues (values);

    // Replace the wheel assignments with the preset's (none if it has none).
    const auto* mapXml = xml->getChildByName (modMapTag.toString());
    const auto map = mapXml != nullptr ? juce::ValueTree::fromXml (*mapXml) : juce::ValueTree();
    for (const auto& id : getModTargetIds())
    {
        const auto slotTree = map.getChildWithProperty ("id", id);
        setModBinding (id, { slotTree.isValid(), (bool) slotTree.getProperty ("reversed", false),
                             (float) slotTree.getProperty ("red", 0.0f), (float) slotTree.getProperty ("green", 1.0f) });
    }
    return true;
}

bool LoamEightSubProcessor::deleteUserPreset (const juce::String& name)
{
    const auto file = findUserPresetFile (name);
    return file != juce::File() && file.deleteFile();
}

//==============================================================================
juce::AudioProcessorEditor* LoamEightSubProcessor::createEditor()
{
    return new LoamEightSubEditor (*this);
}

void LoamEightSubProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LoamEightSubProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    // Projects saved before v1.2 have no effect switches: derive them from the mix.
    const std::pair<const char*, const char*> switches[] = { { delayOnId, delayMixId }, { reverbOnId, reverbMixId } };
    for (const auto& [switchId, mixId] : switches)
    {
        if (xml->getChildByAttribute ("id", switchId) != nullptr)
            continue;
        if (auto* mix = xml->getChildByAttribute ("id", mixId))
        {
            auto* e = xml->createNewChildElement ("PARAM");
            e->setAttribute ("id", switchId);
            e->setAttribute ("value", mix->getDoubleAttribute ("value") > 0 ? 1.0 : 0.0);
        }
    }

    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    loadModMapFromState();
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoamEightSubProcessor();
}
