#include "MeowProcessor.h"
#include "MeowEditor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout MeowProcessor::makeLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "thresh", 1 }, "Record threshold", NormalisableRange<float>(-60.f, 0.f, 0.1f), -40.f, AudioParameterFloatAttributes().withLabel("dB")));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "bpm", 1 }, "Click tempo", NormalisableRange<float>(40.f, 240.f, 1.f), 120.f, AudioParameterFloatAttributes().withLabel("bpm")));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { "preroll", 1 }, "Pre-roll", NormalisableRange<float>(0.f, 2000.f, 1.f), 300.f, AudioParameterFloatAttributes().withLabel("ms")));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { "monitor", 1 }, "Monitor input", true));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { "loop", 1 }, "Loop playback", false));
    l.add(std::make_unique<AudioParameterChoice>(ParameterID { "take", 1 }, "Take goes", StringArray { "Replace sound", "Append", "Punch selection", "Insert at selection" }, 0));
    return l;
}

MeowProcessor::MeowProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "MEOWCORDER", makeLayout())
{
    fifoBuf[0].assign(kFifo, 0.f); fifoBuf[1].assign(kFifo, 0.f); drainL.resize(4096); drainR.resize(4096);
    pThresh = apvts.getRawParameterValue("thresh"); pPre = apvts.getRawParameterValue("preroll"); pBpm = apvts.getRawParameterValue("bpm"); pMonitor = apvts.getRawParameterValue("monitor");
    pLoop = apvts.getRawParameterValue("loop"); pTake = apvts.getRawParameterValue("take");
    fm.registerBasicFormats();
    recorder.prepare(sr, 30.0);
    startTimerHz(30);
}
MeowProcessor::~MeowProcessor() { stopTimer(); }

bool MeowProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    return o == juce::AudioChannelSet::stereo() && (i == juce::AudioChannelSet::stereo() || i == juce::AudioChannelSet::mono());
}

void MeowProcessor::prepareToPlay(double rate, int)
{
    const juce::ScopedLock sl(lock);
    if (rate != sr) { sr = rate; recorder.prepare(sr, 30.0); }
}

void MeowProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd; const int n = buf.getNumSamples(); const int nIn = getTotalNumInputChannels();
    if (buf.getNumChannels() < 2) return;
    float* L = buf.getWritePointer(0); float* R = buf.getWritePointer(1);
    if (nIn < 2) std::memcpy(R, L, sizeof(float) * (size_t) n);   // mono input on both sides
    hostBpm = (double) pBpm->load();   // the Tempo knob, used when the host gives no tempo
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) if (*b > 20.0) hostBpm = *b;

    // input to the recorder FIFO and the IN meter (before anything is added to the output)
    float pk = 0.f; for (int i = 0; i < n; ++i) pk = std::max(pk, std::max(std::fabs(L[i]), std::fabs(R[i])));
    inPeak = pk;
    { int s1, z1, s2, z2; fifo.prepareToWrite(n, s1, z1, s2, z2);
      if (z1 + z2 >= n) { std::memcpy(&fifoBuf[0][(size_t) s1], L, sizeof(float) * (size_t) z1); std::memcpy(&fifoBuf[1][(size_t) s1], R, sizeof(float) * (size_t) z1);
                         if (z2 > 0) { std::memcpy(&fifoBuf[0][(size_t) s2], L + z1, sizeof(float) * (size_t) z2); std::memcpy(&fifoBuf[1][(size_t) s2], R + z1, sizeof(float) * (size_t) z2); }
                         fifo.finishedWrite(n); } }
    const int64_t w0 = written.load(); written = w0 + n;

    if (pMonitor->load() < 0.5f) { std::fill(L, L + n, 0.f); std::fill(R, R + n, 0.f); }

    // playback of the published sound
    if (pubFlag.exchange(false)) { const juce::SpinLock::ScopedTryLockType t(playLock); if (t.isLocked()) curAudio = pubAudio; else pubFlag = true; }
    if (playing.load() && curAudio && curAudio->frames() > 1)
    {
        const auto& a = *curAudio; const double ratio = a.sr / getSampleRate(); double pos = playPos.load(); const double from = playFrom.load(), to = std::min(playTo.load(), (double) a.frames() - 1.0);
        const bool loop = pLoop->load() > 0.5f;
        for (int i = 0; i < n; ++i)
        {
            if (pos >= to) { if (loop && to > from + 1.0) pos = from; else { playing = false; break; } }
            const size_t i0 = (size_t) pos; const float f = (float) (pos - (double) i0); const size_t i1 = std::min(i0 + 1, a.frames() - 1);
            L[i] += a.ch[0][i0] * (1.f - f) + a.ch[0][i1] * f; R[i] += a.ch[1][i0] * (1.f - f) + a.ch[1][i1] * f; pos += ratio;
        }
        playPos = pos;
    }

    // count-in click: four beats, accent on the first
    int left = countInLeft.load();
    if (left > 0)
    {
        const double beat = 60.0 / hostBpm.load() * getSampleRate(); const int total = countInTotal.load();
        for (int i = 0; i < n && left > 0; ++i, --left)
        {
            const int idx = total - left; const double pos = std::fmod((double) idx, beat);
            if (pos < 1.0) { clickLeft = (int) (0.03 * getSampleRate()); clickPhase = 0.f; clickFreq = (idx < (int) beat) ? 1500.f : 1000.f; }
            if (clickLeft > 0) { const float env = (float) clickLeft / (float) (0.03 * getSampleRate()); const float s = std::sin(clickPhase) * env * 0.4f; clickPhase += 6.2831853f * clickFreq / (float) getSampleRate(); L[i] += s; R[i] += s; --clickLeft; }
        }
        countInLeft = left;
        if (left == 0) { countInDoneAt = w0 + n; countInDone = true; }
    }

    float po = 0.f; for (int i = 0; i < n; ++i) po = std::max(po, std::max(std::fabs(L[i]), std::fabs(R[i])));
    outPeak = po;
}

void MeowProcessor::timerCallback()
{
    // drain what the audio thread collected into the recorder
    const juce::ScopedLock sl(lock);
    for (;;)
    {
        const int ready = fifo.getNumReady(); if (ready <= 0) break; const int n = std::min(ready, (int) drainL.size());
        int s1, z1, s2, z2; fifo.prepareToRead(n, s1, z1, s2, z2);
        std::memcpy(drainL.data(), &fifoBuf[0][(size_t) s1], sizeof(float) * (size_t) z1); std::memcpy(drainR.data(), &fifoBuf[1][(size_t) s1], sizeof(float) * (size_t) z1);
        if (z2 > 0) { std::memcpy(drainL.data() + z1, &fifoBuf[0][(size_t) s2], sizeof(float) * (size_t) z2); std::memcpy(drainR.data() + z1, &fifoBuf[1][(size_t) s2], sizeof(float) * (size_t) z2); }
        fifo.finishedRead(n); recorder.process(drainL.data(), drainR.data(), (size_t) n); drained += n;
    }
    if (countInDone.exchange(false))
    {
        const double extraMs = std::max<int64_t>(0, drained - countInDoneAt.load()) * 1000.0 / sr;
        recorder.arm(mc::Recorder::Immediate, pThresh->load(), (double) pPre->load() + extraMs);
    }
}

void MeowProcessor::startRecord(int mode)
{
    const juce::ScopedLock sl(lock);
    pendingTake = juce::roundToInt(pTake->load());
    if (mode == RecNow) recorder.arm(mc::Recorder::Immediate, pThresh->load(), pPre->load());
    else if (mode == RecOnInput) recorder.arm(mc::Recorder::Threshold, pThresh->load(), pPre->load());
    else { const int total = (int) std::llround(4.0 * 60.0 / hostBpm.load() * sr); countInTotal = total; countInLeft = total; stopPlay(); }
}
bool MeowProcessor::isRecording() const { const juce::ScopedLock sl(lock); return recorder.isRecording() || recorder.isArmed() || countInLeft.load() > 0; }
bool MeowProcessor::isArmedOnly() const { const juce::ScopedLock sl(lock); return !recorder.isRecording() && (recorder.isArmed() || countInLeft.load() > 0); }
double MeowProcessor::recordedSeconds() const { const juce::ScopedLock sl(lock); return (double) recorder.recordedFrames() / sr; }
void MeowProcessor::stopRecord()
{
    mc::Audio a; { const juce::ScopedLock sl(lock); countInLeft = 0; a = recorder.stop(); }
    if (a.frames() > 0) { doc.takeRecording(a, pendingTake); changed(); }
}

bool MeowProcessor::grabLast(double seconds)
{
    mc::Audio a; { const juce::ScopedLock sl(lock); a = recorder.captureLast(seconds); }
    if (a.frames() < 2) return false; doc.takeRecording(a, juce::roundToInt(pTake->load())); changed(); return true;
}

void MeowProcessor::play()
{
    if (doc.audio.frames() < 2) return;
    double from = doc.hasSelection() ? (double) doc.selA : (double) doc.cursor, to = doc.hasSelection() ? (double) doc.selB : (double) doc.audio.frames();
    if (to - from < 2.0) { from = 0.0; to = (double) doc.audio.frames(); }
    playRange((size_t) from, (size_t) to, false);
}

void MeowProcessor::refreshTempo()
{
    if (tempoVer == ver.load()) return; tempoVer = ver.load(); tempoV = 0; hitsV.clear();
    if (doc.audio.frames() < 4 || doc.audio.frames() > (size_t) (doc.audio.sr * 900.0)) return;   // very long sounds: no automatic tempo
    hitsV = mc::detectTransients(doc.audio, 0.6); tempoV = mc::detectBpm(doc.audio);
}
double MeowProcessor::detectedBpm() { refreshTempo(); return tempoV; }
const std::vector<size_t>& MeowProcessor::hits() { refreshTempo(); return hitsV; }

mc::WarpOpts MeowProcessor::warpOpts(size_t a, size_t b)
{
    const double sb = sampleBpm(); mc::WarpOpts o; o.ratio = warp.sync && sb > 0 ? hostBpm.load() / sb : 1.0; o.mode = warp.mode; o.semis = warp.semis; o.cents = warp.cents; o.formant = warp.keepFormants; o.fshift = warp.fshift;
    for (size_t h : hits()) if (h > a && h < b) o.hits.push_back(h - a);
    return o;
}

void MeowProcessor::playRange(size_t a, size_t b, bool forceWarp)
{
    if (doc.audio.frames() < 2 || b <= a) return;
    std::shared_ptr<const mc::Audio> pub; double from = (double) a, to = (double) b;
    if (forceWarp || warpActive())
    {
        const auto o = warpOpts(a, b); const std::string key = std::to_string(ver.load()) + ":" + std::to_string(a) + ":" + std::to_string(b) + ":" + std::to_string(o.ratio) + ":" + std::to_string(o.mode) + ":" + std::to_string(o.semis) + ":" + std::to_string(o.cents) + ":" + std::to_string((int) o.formant) + ":" + std::to_string(o.fshift);
        if (key != warpKey || !warpCache) { warpCache = std::make_shared<const mc::Audio>(mc::warpAudio(doc.audio.slice(a, b), o)); warpKey = key; }
        pub = warpCache; from = 0.0; to = (double) pub->frames(); mapA = (double) a; mapRatio = o.ratio > 0 ? o.ratio : 1.0;
    }
    else { pub = std::make_shared<const mc::Audio>(doc.audio); mapA = 0.0; mapRatio = 1.0; }
    if (pub->frames() < 2) return;
    { const juce::SpinLock::ScopedLockType l(playLock); pubAudio = pub; }
    playFrom = from; playTo = to; playPos = from; pubFlag = true; playing = true;
}

bool MeowProcessor::bakeWarp(bool allOverride)
{
    if (!warpActive() && !(warp.keepFormants && warp.fshift != 0)) return false;
    const bool all = allOverride || warp.all || !doc.hasSelection(); const size_t s = all ? 0 : doc.selA, e = all ? doc.audio.frames() : doc.selB; if (e <= s) return false;
    stopPlay(); const auto o = warpOpts(s, e); const double st = warp.semis + warp.cents / 100.0; size_t outLen = 0;
    std::string label = "Warp"; if (warp.sync) label += " to " + juce::String(hostBpm.load(), 1).toStdString() + " bpm"; if (st != 0) label += juce::String(st > 0 ? " +" : " ").toStdString() + juce::String(st, 2).toStdString() + " st";
    doc.transform(label, [&](mc::Audio& a) { const auto w = mc::warpAudio(a.slice(s, e), o); outLen = w.frames(); a.splice(s, e, w); });
    if (all) doc.setCursor(0); else doc.setSelection(s, s + outLen);
    if (warp.sync) warp.bpm = 0; resetWarp(); warpCache.reset(); changed(); return true;
}

void MeowProcessor::newSound() { stopPlay(); doc.load(mc::Audio()); changed(); }

bool MeowProcessor::loadFile(const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> r(fm.createReaderFor(f)); if (!r || r->lengthInSamples < 1 || r->sampleRate <= 0) return false;
    const juce::int64 n = std::min<juce::int64>(r->lengthInSamples, (juce::int64) (r->sampleRate * 60.0 * 20.0));
    juce::AudioBuffer<float> b((int) std::max<unsigned>(2, r->numChannels), (int) n); b.clear(); r->read(&b, 0, (int) n, 0, true, true);
    mc::Audio a; a.sr = r->sampleRate; a.resize((size_t) n);
    for (int c = 0; c < 2; ++c) { const float* s = b.getReadPointer(r->numChannels >= 2 ? c : 0); std::copy(s, s + n, a.ch[c].begin()); }
    stopPlay(); doc.load(a); changed(); return true;
}

bool MeowProcessor::saveFile(const juce::File& f, bool flac)
{
    if (doc.audio.frames() == 0) return false;
    int bits = exportBits.load(); mc::Audio a = doc.audio; if (bits < 24) mc::quantize(a, bits, exportDither.load());
    if (!flac)
    {
        const auto wav = mc::encodeWav(a, bits); f.deleteFile(); return f.replaceWithData(wav.data(), wav.size());
    }
    bits = std::min(bits, 24);   // FLAC holds up to 24 bit integers
    juce::AudioBuffer<float> b(2, (int) a.frames()); for (int c = 0; c < 2; ++c) std::copy(a.ch[c].begin(), a.ch[c].end(), b.getWritePointer(c));
    f.deleteFile(); std::unique_ptr<juce::FileOutputStream> os(f.createOutputStream()); if (!os) return false;
    juce::FlacAudioFormat fmt; std::unique_ptr<juce::AudioFormatWriter> w(fmt.createWriterFor(os.get(), a.sr, 2, bits, {}, 0)); if (!w) return false;
    os.release(); return w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
}

juce::File MeowProcessor::exportTempWav()
{
    auto f = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("Meowcorder").getChildFile("Meowcorder sound.wav"); f.getParentDirectory().createDirectory();
    return saveFile(f, false) ? f : juce::File();
}

// state: the parameters and, if it is not huge, the sound itself (32 bit float WAV)
void MeowProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream o(dest, false);
    auto xml = apvts.copyState().createXml(); const juce::String xs = xml ? xml->toString() : juce::String();
    o.writeInt(0x4d454f57); o.writeString(xs);
    const bool withSound = doc.audio.frames() > 0 && doc.audio.frames() * 8 < 120u * 1024u * 1024u;
    if (withSound) { const auto wav = mc::encodeWav(doc.audio, 32); o.writeInt64((juce::int64) wav.size()); o.write(wav.data(), wav.size()); } else o.writeInt64(0);
}
void MeowProcessor::setStateInformation(const void* data, int size)
{
    juce::MemoryInputStream in(data, (size_t) size, false); if (in.readInt() != 0x4d454f57) return;
    const juce::String xs = in.readString(); if (auto xml = juce::parseXML(xs)) apvts.replaceState(juce::ValueTree::fromXml(*xml));
    const juce::int64 len = in.readInt64();
    if (len > 44 && len <= in.getNumBytesRemaining()) { std::vector<uint8_t> wav((size_t) len); in.read(wav.data(), (int) len); mc::Audio a; if (mc::decodeWav(wav.data(), wav.size(), a)) { doc.load(a); changed(); } }
}

juce::AudioProcessorEditor* MeowProcessor::createEditor() { return new MeowEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MeowProcessor(); }
