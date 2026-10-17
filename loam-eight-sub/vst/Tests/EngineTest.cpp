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

    // Pitch bend: +2 semitones on A4 should land on B4.
    {
        SynthEngine bend;
        bend.prepare (44100.0, 512);
        bend.wave = SynthEngine::Wave::Sine;
        bend.cutoffHz = 12000.0f;
        bend.noteOn (69, 1.0f);
        bend.bendSemis = 2.0f;

        juce::AudioBuffer<float> b (2, 512);
        for (int block = 0; block < 10; ++block)
            bend.render (b, 0, 512);

        int crossings = 0;
        float prev = 0.0f;
        for (int block = 0; block < 20; ++block)
        {
            bend.render (b, 0, 512);
            for (int i = 0; i < 512; ++i)
            {
                const auto x = b.getSample (0, i);
                if (prev < 0.0f && x >= 0.0f) ++crossings;
                prev = x;
            }
        }
        const auto hz = crossings * 44100.0 / (20 * 512.0);
        if (std::abs (hz - 493.88) > 12.0)
        {
            std::fprintf (stderr, "FAIL: bend +2 on A4 should give ~494 Hz, got %f\n", hz);
            return 1;
        }
        std::printf ("ok: bent A4 up 2 semitones, %.1f Hz\n", hz);
    }

    // Drive stages: switching saturation or distortion on changes the sound,
    // and the output stays finite and within range.
    {
        auto renderRms = [] (bool sat, bool dist)
        {
            SynthEngine e;
            e.prepare (44100.0, 512);
            e.saturation = { sat, 100.0f, 12000.0f, 100.0f };
            e.distortion = { dist, 100.0f, 12000.0f, 100.0f };
            e.noteOn (57, 1.0f);
            juce::AudioBuffer<float> b (2, 512);
            for (int block = 0; block < 10; ++block)
                e.render (b, 0, 512);
            const auto peak = b.getMagnitude (0, 0, 512);
            return std::isfinite (peak) && peak < 2.0f ? rms (b, 0, 0, 512) : -1.0f;
        };
        const auto clean = renderRms (false, false), saturated = renderRms (true, false), distorted = renderRms (false, true);
        if (clean <= 0.0f || saturated <= 0.0f || distorted <= 0.0f
            || std::abs (saturated - clean) < 1.0e-3f || std::abs (distorted - clean) < 1.0e-3f)
        {
            std::fprintf (stderr, "FAIL: drive stages, RMS clean=%f sat=%f dist=%f\n", clean, saturated, distorted);
            return 1;
        }
        std::printf ("ok: drive stages, RMS clean=%.3f sat=%.3f dist=%.3f\n", clean, saturated, distorted);

        // TONE > (highpass) at 2 kHz on a low sine should leave much less than TONE < (lowpass).
        auto toneRms = [] (bool highPass)
        {
            SynthEngine e;
            e.prepare (44100.0, 512);
            e.wave = SynthEngine::Wave::Sine;
            e.cutoffHz = 12000.0f;
            e.saturation = { true, 0.0f, 2000.0f, 100.0f, highPass };
            e.noteOn (45, 1.0f);   // A2, 110 Hz
            juce::AudioBuffer<float> b (2, 512);
            for (int block = 0; block < 10; ++block)
                e.render (b, 0, 512);
            return rms (b, 0, 0, 512);
        };
        const auto lowPassed = toneRms (false), highPassed = toneRms (true);
        if (! (highPassed < lowPassed * 0.1f))
        {
            std::fprintf (stderr, "FAIL: TONE highpass should cut a 110 Hz sine, RMS lp=%f hp=%f\n", lowPassed, highPassed);
            return 1;
        }
        std::printf ("ok: TONE < keeps 110 Hz (RMS %.3f), TONE > cuts it (RMS %.4f)\n", lowPassed, highPassed);
    }

    // Reverb TONE >: a highpass at 8 kHz on the reverb of a low note leaves far less than TONE <.
    {
        auto reverbRms = [] (bool highPass)
        {
            SynthEngine e;
            e.prepare (44100.0, 512);
            e.wave = SynthEngine::Wave::Sine;
            e.reverbMixPct = 100.0f;
            e.reverbToneHz = 8000.0f;
            e.reverbToneHighPass = highPass;
            e.noteOn (45, 1.0f);
            juce::AudioBuffer<float> b (2, 512);
            for (int block = 0; block < 20; ++block)
                e.render (b, 0, 512);
            e.noteOff (45);
            for (int block = 0; block < 30; ++block)   // dry note gone after its 300 ms release: tail only
                e.render (b, 0, 512);
            return rms (b, 0, 0, 512);
        };
        const auto lp = reverbRms (false), hp = reverbRms (true);
        if (! (lp > 1.0e-4f && hp < lp * 0.2f))
        {
            std::fprintf (stderr, "FAIL: reverb TONE, tail RMS lp=%f hp=%f\n", lp, hp);
            return 1;
        }
        std::printf ("ok: reverb tail TONE < RMS %.4f, TONE > RMS %.5f\n", lp, hp);
    }

    // Ping-pong delay: a short blip's first echo is on the left, the second on the right.
    {
        SynthEngine e;
        e.prepare (44100.0, 512);
        e.attackMs = 1.0f; e.decayMs = 10.0f; e.sustainPct = 0.0f; e.releaseMs = 1.0f;
        e.delayTimeMs = 100.0f; e.delayFeedbackPct = 60.0f; e.delayMixPct = 100.0f;
        e.delayPingPong = true;

        juce::AudioBuffer<float> out (2, 44100 / 2);   // 500 ms
        juce::AudioBuffer<float> block (2, 512);
        e.noteOn (69, 1.0f);
        for (int pos = 0; pos < out.getNumSamples(); pos += 512)
        {
            if (pos == 512) e.noteOff (69);
            const auto n = juce::jmin (512, out.getNumSamples() - pos);
            e.render (block, 0, n);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, block, ch, 0, n);
        }
        const auto first = 4410, second = 8820, len = 1500;   // echoes at 100 ms and 200 ms
        const auto l1 = rms (out, 0, first, len), r1 = rms (out, 1, first, len);
        const auto l2 = rms (out, 0, second, len), r2 = rms (out, 1, second, len);
        if (! (l1 > 4.0f * r1 && r2 > 4.0f * l2))
        {
            std::fprintf (stderr, "FAIL: ping-pong echoes, 1st L=%f R=%f, 2nd L=%f R=%f\n", l1, r1, l2, r2);
            return 1;
        }
        std::printf ("ok: ping-pong, 1st echo left (L %.3f / R %.4f), 2nd right (L %.4f / R %.3f)\n", l1, r1, l2, r2);
    }

    std::printf ("EngineTest passed.\n");
    return 0;
}
