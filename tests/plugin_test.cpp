// Headless checks of the plugin glue: record (now, on input, pre-roll), edit, undo, play back, files, state.
// No window and no host. Run: build the MeowcorderPluginTest target and execute it.
#include "plugin/PluginProcessor.h"
#include <cstdio>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf ("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)

static void setParam (MeowcorderProcessor& p, const char* id, float v) { auto* prm = p.apvts.getParameter (id); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); }

static void feed (MeowcorderProcessor& p, float amp, int seconds100, float freq = 440.f, double sr = 48000.0)
{
    juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; static double ph = 0;
    for (int blk = 0; blk < seconds100; ++blk)
    {
        for (int i = 0; i < 480; ++i) { const float v = amp * (float) std::sin (ph); ph += 2 * juce::MathConstants<double>::pi * freq / sr; b.setSample (0, i, v); b.setSample (1, i, v); }
        p.processBlock (b, m);
        juce::Thread::sleep (2);
    }
    juce::Thread::sleep (40);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    MeowcorderProcessor p; p.prepareToPlay (48000.0, 480);

    // 1. record "now": 0.5 s of sine, plus the pre-roll that was already heard
    feed (p, 0.5f, 20);                                   // 0.2 s heard, not recording
    p.takeMode = MeowcorderProcessor::takeReplace;
    CHECK (p.startRecording (MeowcorderProcessor::startNow));
    feed (p, 0.5f, 50);                                   // 0.5 s recorded
    CHECK (p.recState() == MeowcorderProcessor::recRecording);
    p.stopRecording();
    const double sec = (double) p.ed.audio.frames() / 48000.0;
    CHECK (sec > 0.6 && sec < 0.8);                       // 0.5 s + about 0.2 s of pre-roll (default 200 ms)
    CHECK (p.ed.audio.peak (0, p.ed.audio.frames()) > 0.45f);
    CHECK (p.recState() == MeowcorderProcessor::recOff);

    // 2. record on input: silence first, nothing may be recorded until the level passes the threshold
    p.takeMode = MeowcorderProcessor::takeReplace;
    CHECK (p.startRecording (MeowcorderProcessor::startOnInput));
    feed (p, 0.0f, 20);
    CHECK (p.recState() == MeowcorderProcessor::recArmed);
    CHECK (p.recordedSeconds() < 1e-6);
    feed (p, 0.5f, 30);
    CHECK (p.recState() == MeowcorderProcessor::recRecording);
    p.stopRecording();
    CHECK (p.ed.audio.frames() > 0 && p.ed.audio.frames() < (size_t) (48000 * 0.6));   // started at the sound, not at the silence

    // 3. take modes: append makes it longer
    const size_t before = p.ed.audio.frames();
    p.takeMode = MeowcorderProcessor::takeAppend;
    p.startRecording (MeowcorderProcessor::startNow); feed (p, 0.3f, 20); p.stopRecording();
    CHECK (p.ed.audio.frames() > before);

    // 4. edit and undo
    p.ed.setSelection (0, p.ed.audio.frames()); const float pk0 = p.ed.audio.peak (0, p.ed.audio.frames());
    float prm[mc::kMaxOpParams]; mc::defaultParams (mc::opGain, prm); prm[0] = -6.0f;
    CHECK (p.ed.apply (mc::opGain, prm)); p.audioChanged();
    CHECK (p.ed.audio.peak (0, p.ed.audio.frames()) < pk0 * 0.6f);
    p.ed.undo(); p.audioChanged();
    CHECK (std::fabs (p.ed.audio.peak (0, p.ed.audio.frames()) - pk0) < 1e-6f);

    // 5. playback comes out of the processor
    p.ed.setCursor (15000);                               // after the silent pre-roll at the start of the take
    p.play(); CHECK (p.isPlaying());
    juce::AudioBuffer<float> out (2, 480); juce::MidiBuffer m; out.clear();
    setParam (p, "monitor", 0.f);                         // muted input: only playback is heard
    p.processBlock (out, m);
    CHECK (out.getMagnitude (0, 480) > 0.05f);
    for (int i = 0; i < 480; ++i) CHECK (std::isfinite (out.getSample (0, i)));
    p.stopPlayback();

    // 6. WAV round trip
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("meowcorder_test.wav");
    juce::String err; CHECK (p.saveWav (f, 24, err));
    MeowcorderProcessor q; q.prepareToPlay (48000.0, 480);
    CHECK (q.loadFile (f, err)); CHECK (q.ed.audio.frames() == p.ed.audio.frames());
    CHECK (std::fabs (q.ed.audio.peak (0, q.ed.audio.frames()) - p.ed.audio.peak (0, p.ed.audio.frames())) < 1e-3f);
    f.deleteFile();
    CHECK (! q.loadFile (juce::File ("/definitely/not/here.wav"), err));

    // 7. state keeps parameters and settings, ignores nonsense
    setParam (p, "threshold", -30.f); p.exportBits = 16; p.uiScalePercent = 125;
    juce::MemoryBlock mb; p.getStateInformation (mb);
    MeowcorderProcessor r; r.setStateInformation (mb.getData(), (int) mb.getSize());
    CHECK (std::fabs (r.apvts.getRawParameterValue ("threshold")->load() - (-30.f)) < 0.2f); CHECK (r.exportBits == 16); CHECK (r.uiScalePercent == 125);
    r.setStateInformation ("garbage", 7); CHECK (r.exportBits == 16);

    // 8. count-in: four clicks are heard, then recording starts by itself and records what comes in afterwards
    setParam (p, "tempo", 240.f); setParam (p, "monitor", 0.f); p.takeMode = MeowcorderProcessor::takeReplace;           // 240 bpm: the count-in is 1 s
    CHECK (p.startRecording (MeowcorderProcessor::startCountIn)); CHECK (p.recState() == MeowcorderProcessor::recCountIn);
    {
        juce::AudioBuffer<float> o (2, 480); juce::MidiBuffer mm; float e = 0;
        for (int k = 0; k < 5; ++k) { o.clear(); p.processBlock (o, mm); e += o.getMagnitude (0, 480); }
        CHECK (e > 0.1f);                                                                                                // the click is audible even with the monitor off
    }
    feed (p, 0.0f, 80);                                                                                                  // 0.8 s: still counting
    CHECK (p.recState() == MeowcorderProcessor::recCountIn);
    feed (p, 0.0f, 30);                                                                                                  // the count-in ends, recording starts
    CHECK (p.recState() == MeowcorderProcessor::recRecording);
    feed (p, 0.5f, 50); p.stopRecording();
    CHECK (p.ed.audio.frames() > (size_t) (48000 * 0.4) && p.ed.audio.frames() < (size_t) (48000 * 0.9));                // only what came after the count-in
    CHECK (p.startRecording (MeowcorderProcessor::startCountIn)); p.stopRecording(); CHECK (p.recState() == MeowcorderProcessor::recOff);   // stopping during the count-in cancels

    // 9. mono takes: left plus right
    { mc::Audio a; a.sr = 48000.0; a.resize (4); a.ch[0][0] = 0.5f; a.ch[1][0] = -0.1f; a.ch[0][1] = 1.f; a.ch[1][1] = 1.f; MeowcorderProcessor::toMono (a);
      CHECK (std::fabs (a.ch[0][0] - 0.2f) < 1e-6f && a.ch[0][0] == a.ch[1][0] && std::fabs (a.ch[0][1] - 1.f) < 1e-6f); }

    if (! fails) std::puts ("meowcorder_plugin_test: all passed");
    return fails ? 1 : 0;
}
