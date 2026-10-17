#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Plain-C++ DSP core for Loam Eight Sub, independent of juce::AudioProcessor
// so it can be built and exercised by a lightweight test target with no
// GUI/windowing dependencies. Mirrors the signal path of the web version:
// oscillator (+ sub) -> shared lowpass filter -> per-voice ADSR -> delay
// and reverb sends -> output.
class SynthEngine
{
public:
    enum class Wave { Sine, Saw, Square, Triangle };

    void prepare (double newSampleRate, int samplesPerBlock);
    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();
    // Sustain pedal (MIDI CC 64): while down, released keys keep sounding;
    // lifting it releases every note whose key is no longer held.
    void setSustain (bool down);
    void render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    // Parameters -- set these from the processor every block before calling render().
    Wave wave = Wave::Saw;
    float tuneCents = 0.0f;
    float subMixPct = 0.0f;
    float cutoffHz = 2200.0f;
    float resonanceQ = 1.0f;
    float attackMs = 10.0f;
    float decayMs = 150.0f;
    float sustainPct = 70.0f;
    float releaseMs = 300.0f;
    float volumePct = 70.0f;
    float delayTimeMs = 220.0f;
    float delayFeedbackPct = 30.0f;
    float delayMixPct = 0.0f;
    float reverbMixPct = 0.0f;
    float reverbSizeSec = 2.2f;
    float reverbToneHz = 6000.0f;

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    struct Voice
    {
        bool active = false;
        bool keyDown = false;   // key still held (vs. kept sounding by the pedal)
        int note = -1;
        double phase = 0.0;
        double subPhase = 0.0;
        double freq = 0.0;
        Stage stage = Stage::Idle;
        float envLevel = 0.0f;
        float releaseStartLevel = 0.0f;
    };

    static constexpr int numVoices = 8;
    std::array<Voice, numVoices> voices;

    double sampleRate = 44100.0;
    bool sustainDown = false;

    // Shared lowpass filter (RBJ biquad), recalculated once per block.
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;
    void updateFilterCoefficients();

    juce::dsp::DelayLine<float> delayLine { 48000 };
    juce::dsp::Reverb reverb;
    std::vector<float> reverbScratchL, reverbScratchR;

    Voice* findVoiceForNote (int note);
    Voice* allocateVoice (int note);
    float oscillatorSample (Wave w, double ph) const;
    void advanceEnvelope (Voice& v);
    static void startRelease (Voice& v);
};
