#include "PluginProcessor.h"
#ifndef MEOW_NO_EDITOR
 #include "PluginEditor.h"
#endif

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using P = juce::AudioProcessorValueTreeState;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    // Stable IDs: never rename these, old projects keep loading.
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "threshold", 1 }, "Threshold", juce::NormalisableRange<float> (-80.f, 0.f, 0.1f), -40.f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "preroll", 1 }, "Pre-roll", juce::NormalisableRange<float> (0.f, 3000.f, 1.f, 0.4f), 200.f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tempo", 1 }, "Tempo", juce::NormalisableRange<float> (40.f, 240.f, 0.1f), 120.f, juce::AudioParameterFloatAttributes().withLabel ("bpm")));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "monitor", 1 }, "Monitor input", true));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "loop", 1 }, "Loop", false));
    (void) sizeof (P);
    return l;
}
}

MeowcorderProcessor::MeowcorderProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread ("Meowcorder recorder"),
      apvts (*this, nullptr, "STATE", makeLayout())
{
    pThreshold = apvts.getRawParameterValue ("threshold"); pPreroll = apvts.getRawParameterValue ("preroll");
    pMonitor = apvts.getRawParameterValue ("monitor");     pLoop = apvts.getRawParameterValue ("loop");
    pTempo = apvts.getRawParameterValue ("tempo");
    fifoL.assign ((size_t) fifo.getTotalSize(), 0.f); fifoR.assign ((size_t) fifo.getTotalSize(), 0.f);
    for (int o = 0; o < mc::opCount; ++o) mc::defaultParams (o, opParams[o]);
    for (auto& p : livePeaks) p.store (0.f);
    recorder.prepare (hostSr);
    publish();
    startThread (juce::Thread::Priority::normal);
}

MeowcorderProcessor::~MeowcorderProcessor() { stopThread (2000); }

void MeowcorderProcessor::prepareToPlay (double sr, int)
{
    hostSr = sr > 0 ? sr : 48000.0;
    { std::lock_guard<std::mutex> g (recMutex); recorder.prepare (hostSr); recStateA = recOff; recFramesA = 0; }
    fifo.reset();
}

bool MeowcorderProcessor::hasEditor() const
{
#ifdef MEOW_NO_EDITOR
    return false;
#else
    return true;
#endif
}

juce::AudioProcessorEditor* MeowcorderProcessor::createEditor()
{
#ifdef MEOW_NO_EDITOR
    return nullptr;
#else
    return new MeowcorderEditor (*this);
#endif
}

// ---------------------------------------------------------------- audio thread
void MeowcorderProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buf.getNumSamples(), chans = buf.getNumChannels(); if (n == 0 || chans == 0) return;
    const float* inL = buf.getReadPointer (0); const float* inR = chans > 1 ? buf.getReadPointer (1) : inL;

    float pl = 0.f, pr = 0.f;
    for (int i = 0; i < n; ++i) { pl = std::max (pl, std::fabs (inL[i])); pr = std::max (pr, std::fabs (inR[i])); }
    inPeak[0].store (std::max (pl, inPeak[0].load() * 0.92f)); inPeak[1].store (std::max (pr, inPeak[1].load() * 0.92f));

    // record feed: a lock-free copy, the worker thread does the rest
    {
        int s1, n1, s2, n2; fifo.prepareToWrite (n, s1, n1, s2, n2);
        if (n1 > 0) { std::memcpy (fifoL.data() + s1, inL, (size_t) n1 * sizeof (float)); std::memcpy (fifoR.data() + s1, inR, (size_t) n1 * sizeof (float)); }
        if (n2 > 0) { std::memcpy (fifoL.data() + s2, inL + n1, (size_t) n2 * sizeof (float)); std::memcpy (fifoR.data() + s2, inR + n1, (size_t) n2 * sizeof (float)); }
        fifo.finishedWrite (n1 + n2);
    }
    // live level history for the strip above the display
    liveAcc = std::max (liveAcc, std::max (pl, pr)); liveCount += n;
    if (liveCount >= (int) (hostSr / 50.0)) { const int h = liveHead.load(); livePeaks[(size_t) (h % kLive)].store (liveAcc); liveHead.store ((h + 1) % kLive); liveAcc = 0.f; liveCount = 0; }

    // monitoring: what you hear while recording. Muted: only the playback is heard.
    if (pMonitor->load() < 0.5f) buf.clear();

    // count-in: a click on every beat (4/4, the first one higher), heard whatever the monitor says
    if (countInActive.load())
    {
        double pos = countInPos.load(); const double total = countInTotal.load(), beat = countInBeat.load(), clickLen = 0.03 * hostSr;
        float* cL = buf.getWritePointer (0); float* cR = chans > 1 ? buf.getWritePointer (1) : nullptr;
        for (int i = 0; i < n && pos < total; ++i, pos += 1.0)
        {
            const double within = std::fmod (pos, beat); if (within >= clickLen) continue;
            const bool first = pos < beat; const float env = (float) std::pow (1.0 - within / clickLen, 2.0);
            const float v = 0.35f * env * (float) std::sin (2.0 * juce::MathConstants<double>::pi * (first ? 1500.0 : 1000.0) * within / hostSr);
            cL[i] += v; if (cR) cR[i] += v;
        }
        countInPos.store (pos);
        if (pos >= total) { countInActive.store (false); countInArm.store (true); }
    }

    if (playing.load())
    {
        std::shared_ptr<const mc::Audio> a;
        { juce::SpinLock::ScopedTryLockType tl (snapLock); if (tl.isLocked()) a = snap; }
        if (a && a->frames() > 1)
        {
            double pos = playPos.load(); const double end = std::min (playEnd.load(), (double) a->frames() - 1.0), start = playStart.load(), ratio = a->sr / hostSr;
            const bool loop = pLoop->load() > 0.5f;
            float* oL = buf.getWritePointer (0); float* oR = chans > 1 ? buf.getWritePointer (1) : nullptr;
            for (int i = 0; i < n; ++i)
            {
                if (pos >= end) { if (loop && end > start + 1.0) pos = start; else { playing.store (false); pos = end; break; } }
                const size_t i0 = (size_t) pos; const float fr = (float) (pos - (double) i0); const size_t i1 = std::min (i0 + 1, a->frames() - 1);
                oL[i] += a->ch[0][i0] + (a->ch[0][i1] - a->ch[0][i0]) * fr;
                if (oR) oR[i] += a->ch[1][i0] + (a->ch[1][i1] - a->ch[1][i0]) * fr;
                pos += ratio;
            }
            playPos.store (pos);
        }
        a.reset();   // the editor keeps the last snapshots alive, so this never frees memory on the audio thread
    }

    float ol = buf.getMagnitude (0, 0, n), orr = chans > 1 ? buf.getMagnitude (1, 0, n) : ol;
    outPeak[0].store (std::max (ol, outPeak[0].load() * 0.92f)); outPeak[1].store (std::max (orr, outPeak[1].load() * 0.92f));
}

// ---------------------------------------------------------------- worker thread
void MeowcorderProcessor::drainFifo()
{
    int ready = fifo.getNumReady(); if (ready <= 0) return;
    int s1, n1, s2, n2; fifo.prepareToRead (ready, s1, n1, s2, n2);
    if (n1 > 0) recorder.process (fifoL.data() + s1, fifoR.data() + s1, (size_t) n1);
    if (n2 > 0) recorder.process (fifoL.data() + s2, fifoR.data() + s2, (size_t) n2);
    fifo.finishedRead (n1 + n2);
    recFramesA.store (recorder.recordedFrames());
    recStateA.store (countInActive.load() ? recCountIn : (recorder.isRecording() ? recRecording : (recorder.isArmed() ? recArmed : recOff)));
}

void MeowcorderProcessor::run()
{
    while (! threadShouldExit())
    {
        if (countInArm.exchange (false))
        {
            std::lock_guard<std::mutex> g (recMutex); drainFifo();
            recorder.arm (mc::Recorder::Immediate, pThreshold->load(), 0.0);            // the count-in is over: record from this moment on
            recFramesA.store (recorder.recordedFrames()); recStateA.store (recRecording);
        }
        { std::lock_guard<std::mutex> g (recMutex); drainFifo(); }
        wait (4);
    }
}

// ---------------------------------------------------------------- message thread
void MeowcorderProcessor::publish()
{
    auto s = std::make_shared<const mc::Audio> (ed.audio);
    { juce::SpinLock::ScopedLockType l (snapLock); snap = s; }
    keepAlive.push_back (s); while (keepAlive.size() > 4) keepAlive.pop_front();
}

void MeowcorderProcessor::audioChanged()
{
    publish(); dirty = true; ++version;
    const double n = (double) ed.audio.frames(); if (playPos.load() > n) playPos.store (n);
}

bool MeowcorderProcessor::startRecording (StartMode m)
{
    stopPlayback();
    if (m == startCountIn)
    {
        const double beat = 60.0 / (double) std::max (20.f, pTempo->load()) * hostSr;
        countInBeat.store (beat); countInTotal.store (4.0 * beat); countInPos.store (0.0); countInArm.store (false); countInActive.store (true);
        recFramesA.store (0); recStateA.store (recCountIn);
        return true;
    }
    std::lock_guard<std::mutex> g (recMutex);
    drainFifo();
    recorder.arm (m == startNow ? mc::Recorder::Immediate : mc::Recorder::Threshold, pThreshold->load(), (double) pPreroll->load());
    recFramesA.store (recorder.recordedFrames());
    recStateA.store (recorder.isRecording() ? recRecording : recArmed);
    return true;
}

void MeowcorderProcessor::stopRecording()
{
    if (recStateA.load() == recCountIn) { cancelRecording(); return; }
    mc::Audio rec;
    { std::lock_guard<std::mutex> g (recMutex); drainFifo(); rec = recorder.stop(); recStateA.store (recOff); recFramesA.store (0); }
    if (rec.frames() == 0) return;
    rec.sr = hostSr; if (monoTakes) toMono (rec);
    if (ed.audio.frames() > 0 && takeMode != takeReplace && std::fabs (ed.audio.sr - rec.sr) > 0.5) resampleTo (rec, ed.audio.sr);
    ed.takeRecording (rec, takeMode);
    audioChanged();
}

void MeowcorderProcessor::cancelRecording()
{
    countInActive.store (false); countInArm.store (false);
    std::lock_guard<std::mutex> g (recMutex); drainFifo(); (void) recorder.stop(); recStateA.store (recOff); recFramesA.store (0);
}

bool MeowcorderProcessor::grabLastSeconds()
{
    mc::Audio a;
    { std::lock_guard<std::mutex> g (recMutex); drainFifo(); a = recorder.captureLast ((double) pPreroll->load() * 0.001); }
    if (a.frames() == 0) return false;
    a.sr = hostSr; if (monoTakes) toMono (a);
    if (ed.audio.frames() > 0 && takeMode != takeReplace && std::fabs (ed.audio.sr - a.sr) > 0.5) resampleTo (a, ed.audio.sr);
    ed.takeRecording (a, takeMode); audioChanged();
    return true;
}

void MeowcorderProcessor::play()
{
    if (ed.audio.frames() < 2) return;
    const double n = (double) ed.audio.frames();
    const double start = ed.hasSelection() ? (double) ed.selA : (double) std::min (ed.cursor, ed.audio.frames() > 0 ? ed.audio.frames() - 1 : 0);
    const double end = ed.hasSelection() ? (double) ed.selB : n;
    playStart.store (ed.hasSelection() ? start : 0.0); playEnd.store (end); playPos.store (start);
    playing.store (true);
}

void MeowcorderProcessor::stopPlayback() { playing.store (false); }

void MeowcorderProcessor::toMono (mc::Audio& a)
{
    for (size_t i = 0; i < a.frames(); ++i) a.ch[0][i] = a.ch[1][i] = 0.5f * (a.ch[0][i] + a.ch[1][i]);
}

void MeowcorderProcessor::resampleTo (mc::Audio& a, double newSr)
{
    if (a.frames() == 0 || newSr <= 0 || std::fabs (a.sr - newSr) < 0.5) { a.sr = newSr > 0 ? newSr : a.sr; return; }
    const double ratio = a.sr / newSr; const size_t outN = std::max<size_t> (1, (size_t) std::llround ((double) a.frames() / ratio));
    for (int c = 0; c < 2; ++c)
    {
        std::vector<float> o (outN + 8, 0.f); juce::LagrangeInterpolator li;
        li.process (ratio, a.ch[c].data(), o.data(), (int) outN); o.resize (outN); a.ch[c] = std::move (o);
    }
    a.sr = newSr;
}

bool MeowcorderProcessor::loadFile (const juce::File& f, juce::String& error)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (! r) { error = "Could not read " + f.getFileName() + ". Supported: WAV, AIFF, FLAC, OGG."; return false; }
    if (r->lengthInSamples <= 0 || r->lengthInSamples > (juce::int64) r->sampleRate * 60 * 30) { error = "The file is empty or longer than 30 minutes."; return false; }
    const int n = (int) r->lengthInSamples; juce::AudioBuffer<float> b ((int) std::max<unsigned> (2u, r->numChannels), n);
    r->read (&b, 0, n, 0, true, true);
    mc::Audio a; a.sr = r->sampleRate; a.ch[0].assign (b.getReadPointer (0), b.getReadPointer (0) + n);
    if (r->numChannels > 1) a.ch[1].assign (b.getReadPointer (1), b.getReadPointer (1) + n); else a.ch[1] = a.ch[0];
    ed.load (a); soundName = f.getFileName(); audioChanged(); dirty = false; return true;
}

bool MeowcorderProcessor::saveWavRange (const juce::File& f, size_t a, size_t b, int bits) const
{
    const mc::Audio part = ed.audio.slice (a, b); if (part.frames() == 0) return false;
    const auto bytes = mc::encodeWav (part, bits);
    return f.replaceWithData (bytes.data(), bytes.size());
}

bool MeowcorderProcessor::saveWav (const juce::File& f, int bits, juce::String& error) const
{
    if (ed.audio.frames() == 0) { error = "There is no sound to save."; return false; }
    if (! saveWavRange (f, 0, ed.audio.frames(), bits)) { error = "Could not write " + f.getFullPathName(); return false; }
    return true;
}

// ---------------------------------------------------------------- state
void MeowcorderProcessor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState();
    st.setProperty ("exportBits", exportBits, nullptr); st.setProperty ("takeMode", takeMode, nullptr); st.setProperty ("uiScale", uiScalePercent, nullptr); st.setProperty ("startSel", startSel, nullptr); st.setProperty ("monoTakes", monoTakes, nullptr);
    st.setProperty ("version", 1, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, d);
}

void MeowcorderProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto st = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (st);
    const int eb = (int) st.getProperty ("exportBits", 24); exportBits = (eb == 16 || eb == 24 || eb == 32) ? eb : 24;
    takeMode = juce::jlimit (0, 3, (int) st.getProperty ("takeMode", (int) takeAppend));
    uiScalePercent = juce::jlimit (50, 200, (int) st.getProperty ("uiScale", 100));
    startSel = juce::jlimit (0, 2, (int) st.getProperty ("startSel", 1)); monoTakes = (bool) st.getProperty ("monoTakes", false);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MeowcorderProcessor(); }
