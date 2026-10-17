// Minimal assert-based smoke test for SynthEngine. No framework: build and
// run it, non-zero exit means something broke.
//   cmake --build build --target EngineTest && ./build/EngineTest

#include <cstdio>
#include <cmath>
#include "../Source/SynthEngine.h"

static float rms (const juce::AudioBuffer<float>& buffer, int channel, int start, int num)
{
    double sum = 0.0;
    const auto* data = buffer.getReadPointer (channel, start);
    for (int i = 0; i < num; ++i)
        sum += (double) data[i] * (double) data[i];
    return (float) std::sqrt (sum / num);
}

int main()
{
    SynthEngine engine;
    engine.prepare (44100.0, 512);
    engine.delayMixPct = 0.0f;
    engine.reverbMixPct = 0.0f;

    const int blockSize = 512;
    juce::AudioBuffer<float> buffer (2, blockSize);

    // Note on: after the attack/decay settle, the output should be audibly
    // non-silent.
    engine.noteOn (69, 1.0f); // A4
    for (int block = 0; block < 10; ++block)
        engine.render (buffer, 0, blockSize);

    const float soundingLevel = rms (buffer, 0, 0, blockSize);
    if (soundingLevel < 0.01f)
    {
        std::fprintf (stderr, "FAIL: expected audible output after note-on, got RMS=%f\n", soundingLevel);
        return 1;
    }
    std::printf ("ok: sounding RMS=%f\n", soundingLevel);

    // Note off: after release has had time to finish, output should be
    // back to silence.
    engine.noteOff (69);
    for (int block = 0; block < 60; ++block) // release default 300ms << 60*512/44100 ~= 697ms
        engine.render (buffer, 0, blockSize);

    const float silentLevel = rms (buffer, 0, 0, blockSize);
    if (silentLevel > 0.001f)
    {
        std::fprintf (stderr, "FAIL: expected silence after release, got RMS=%f\n", silentLevel);
        return 1;
    }
    std::printf ("ok: released RMS=%f\n", silentLevel);

    // Sustain pedal: a note released while the pedal is down keeps sounding,
    // and lifting the pedal lets it release to silence.
    engine.noteOn (64, 1.0f);
    for (int block = 0; block < 10; ++block)
        engine.render (buffer, 0, blockSize);
    engine.setSustain (true);
    engine.noteOff (64);
    for (int block = 0; block < 60; ++block)
        engine.render (buffer, 0, blockSize);

    const float sustainedLevel = rms (buffer, 0, 0, blockSize);
    if (sustainedLevel < 0.01f)
    {
        std::fprintf (stderr, "FAIL: expected the pedal to hold the note, got RMS=%f\n", sustainedLevel);
        return 1;
    }
    std::printf ("ok: held by pedal RMS=%f\n", sustainedLevel);

    engine.setSustain (false);
    for (int block = 0; block < 60; ++block)
        engine.render (buffer, 0, blockSize);

    const float afterPedalLevel = rms (buffer, 0, 0, blockSize);
    if (afterPedalLevel > 0.001f)
    {
        std::fprintf (stderr, "FAIL: expected silence after lifting the pedal, got RMS=%f\n", afterPedalLevel);
        return 1;
    }
    std::printf ("ok: pedal lifted RMS=%f\n", afterPedalLevel);

    // Portamento: keys inside the range share one gliding voice.
    {
        SynthEngine porta;
        porta.prepare (44100.0, 512);
        porta.wave = SynthEngine::Wave::Sine;
        porta.cutoffHz = 12000.0f;
        porta.portaOn = true;
        porta.portaLow = 60;
        porta.portaHigh = 72;
        porta.portaTimeMs = 100.0f;

        // Rising zero crossings of the left channel over several blocks -> Hz.
        auto measureHz = [&porta] (int blocks)
        {
            juce::AudioBuffer<float> b (2, 512);
            int crossings = 0;
            float prev = 0.0f;
            for (int block = 0; block < blocks; ++block)
            {
                porta.render (b, 0, 512);
                const auto* d = b.getReadPointer (0);
                for (int i = 0; i < 512; ++i)
                {
                    if (prev < 0.0f && d[i] >= 0.0f) ++crossings;
                    prev = d[i];
                }
            }
            return crossings * 44100.0 / (blocks * 512.0);
        };

        porta.noteOn (60, 1.0f);
        measureHz (20);
        porta.noteOn (72, 1.0f);              // legato while 60 is held
        measureHz (20);                       // ~230 ms: glide done
        const auto upperHz = measureHz (20);
        if (porta.getNumActiveVoices() != 1 || std::abs (upperHz - 523.25) > 15.0)
        {
            std::fprintf (stderr, "FAIL: legato glide, voices=%d Hz=%f\n", porta.getNumActiveVoices(), upperHz);
            return 1;
        }
        std::printf ("ok: legato glide to C5, one voice, %.1f Hz\n", upperHz);

        porta.noteOn (40, 1.0f);              // outside the range: its own voice
        if (porta.getNumActiveVoices() != 2)
        {
            std::fprintf (stderr, "FAIL: note outside range should add a voice, voices=%d\n", porta.getNumActiveVoices());
            return 1;
        }
        porta.noteOff (40);

        porta.noteOff (72);                   // glide back to the held C4
        measureHz (60);                       // let the low note release fully
        porta.portaOn = false;                // measure only the glide voice now
        const auto backHz = measureHz (20);
        if (std::abs (backHz - 261.63) > 10.0)
        {
            std::fprintf (stderr, "FAIL: releasing the top key should glide back to C4, Hz=%f\n", backHz);
            return 1;
        }
        std::printf ("ok: glided back to C4, %.1f Hz\n", backHz);
    }

    std::printf ("EngineTest passed.\n");
    return 0;
}
