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

    std::printf ("EngineTest passed.\n");
    return 0;
}
