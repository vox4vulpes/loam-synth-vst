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
    int getNumActiveVoices() const;   // for tests

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
    bool reverbToneHighPass = false;   // reverb TONE filter: lowpass "TONE <" or highpass "TONE >"

    // Portamento: while on, notes inside [portaLow, portaHigh] share one voice
    // that glides from note to note (legato: a new key while another is held
    // changes pitch without a new attack, and releasing it glides back to the
    // key still held). Notes outside the range stay polyphonic.
    bool portaOn = false;
    float portaTimeMs = 150.0f;
    int portaLow = 60, portaHigh = 76;

    // Pitch bend (semitones, smoothed inside the engine) and vibrato depth
    // (semitones of a 5.5 Hz sine), both applied on top of every voice.
    float bendSemis = 0.0f;
    float vibratoSemis = 0.0f;

    // Drive stages after the filter, before delay and reverb: soft tanh
    // saturation, then asymmetric hard-clip distortion. Each has its own tone
    // filter on the driven signal -- a 12 dB/oct Butterworth lowpass ("TONE <")
    // or highpass ("TONE >"), the same as the web version's BiquadFilter --
    // and a dry/wet mix.
    struct Drive
    {
        bool on = false;
        float drivePct = 30.0f;
        float toneHz = 8000.0f;
        float mixPct = 50.0f;
        bool toneHighPass = false;
    };
    Drive saturation, distortion { false, 40.0f, 5000.0f, 50.0f, false };

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    struct Voice
    {
        bool active = false;
        bool keyDown = false;   // key still held (vs. kept sounding by the pedal)
        bool porta = false;     // the shared glide voice
        int note = -1;
        double phase = 0.0;
        double subPhase = 0.0;
        double freq = 0.0;
        double pitch = 0.0;         // in semitones (MIDI note + tune)
        double targetPitch = 0.0;
        double pitchStep = 0.0;     // semitones per sample while gliding
        Stage stage = Stage::Idle;
        float envLevel = 0.0f;
        float releaseStartLevel = 0.0f;
    };

    static constexpr int numVoices = 8;
    std::array<Voice, numVoices> voices;

    double sampleRate = 44100.0;
    bool sustainDown = false;
    double smoothedBend = 0.0, lfoPhase = 0.0, lastPitchOffset = 0.0;
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0, z1 = 0.0, z2 = 0.0;
        void setTone (double hz, bool highPass, double sampleRate);
        double process (double x);
    };
    Biquad satTone, distTone, reverbToneL, reverbToneR;
    float processDrive (float x, const Drive& d, bool hardClip, Biquad& tone) const;

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

    // Portamento state: keys held inside the range, most recent last. A fixed
    // array so the audio thread never allocates; the oldest key drops off if
    // more than portaStackSize are held.
    static constexpr int portaStackSize = 16;
    std::array<int, portaStackSize> portaHeld {};
    int portaHeldCount = 0;
    int portaVoice = -1;
    double lastPortaPitch = -1.0;

    bool isPortaNote (int note) const { return portaOn && note >= portaLow && note <= portaHigh; }
    void portaNoteOn (int note);
    bool portaNoteOff (int note);
    void glideTo (Voice& v, double targetPitch);
    static double pitchToFreq (double pitch) { return 440.0 * std::exp2 ((pitch - 69.0) / 12.0); }
};
