// Meowcorder plugin: glue between the host and the pure C++ engine in src/core.
// Audio thread: copies the input into a lock-free FIFO, plays a published snapshot of the sound, never allocates.
// Worker thread: feeds the engine's recorder (pre-roll, threshold start). Message thread: all editing (mc::Editor).
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include "../core/MeowCore.h"
#include "../core/MeowOps.h"

class MeowcorderProcessor : public juce::AudioProcessor, private juce::Thread
{
public:
    enum StartMode { startNow = 0, startOnInput = 1, startCountIn = 2 };
    enum TakeMode { takeReplace = 0, takeAppend = 1, takePunch = 2, takeInsert = 3 };   // same numbers as mc::Editor::takeRecording
    enum RecState { recOff = 0, recArmed = 1, recRecording = 2, recCountIn = 3 };

    MeowcorderProcessor();
    ~MeowcorderProcessor() override;

    // ---- AudioProcessor
    const juce::String getName() const override { return "Meowcorder"; }
    void prepareToPlay (double sampleRate, int maxBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- message thread only
    mc::Editor ed;
    float opParams[mc::opCount][mc::kMaxOpParams] = {};   // last used settings of every operation
    int exportBits = 24;                                  // 16, 24 or 32 (float)
    int takeMode = takeAppend;
    int uiScalePercent = 100;
    bool infoBubbles = true;                              // hover tips on or off
    int startSel = 1;                                     // 0 now, 1 on input, 2 count-in
    bool monoTakes = false;                               // a new take is made mono (left plus right)
    uint64_t version = 1;                                 // bumped on every change of the sound, the display uses it to redraw
    juce::String soundName { "new sound, not saved" };
    bool dirty = false;

    void audioChanged();                                  // call after every change to ed.audio: publishes it for playback
    bool startRecording (StartMode);
    void stopRecording();
    void cancelRecording();
    bool grabLastSeconds();                               // keeps what was heard before, as long as the pre-roll
    RecState recState() const { return (RecState) recStateA.load(); }
    double recordedSeconds() const { return (double) recFramesA.load() / std::max (1.0, hostSr); }
    void play();                                          // from the cursor, or the start of the selection
    void stopPlayback();
    void startPreview (const mc::Audio& processed, size_t from, size_t to);   // plays a processed copy, the real sound is not touched
    void endPreview();                                                         // stops it and puts the real sound back for playback
    bool isPreviewing() const { return previewing.load(); }
    bool isPlaying() const { return playing.load(); }
    double playheadFrames() const { return playPos.load(); }
    void setPlayhead (double frames) { playPos.store (frames); }
    bool loadFile (const juce::File&, juce::String& error);
    bool saveWav (const juce::File&, int bits, juce::String& error) const;
    bool saveWavRange (const juce::File&, size_t a, size_t b, int bits) const;
    double hostSampleRate() const { return hostSr; }
    static void resampleTo (mc::Audio&, double newSr);
    static void toMono (mc::Audio&);

    // ---- meters, written by the audio thread, read by the editor
    std::atomic<float> inPeak[2] { { 0.f }, { 0.f } }, outPeak[2] { { 0.f }, { 0.f } };
    static constexpr int kLive = 512;                     // about 10 seconds of input level history
    std::array<std::atomic<float>, kLive> livePeaks {};
    std::atomic<int> liveHead { 0 };

private:
    void run() override;                                  // worker: drains the FIFO into the recorder
    void drainFifo();                                     // call with recMutex held
    void publish();

    juce::AbstractFifo fifo { 1 << 17 };
    std::vector<float> fifoL, fifoR;
    std::mutex recMutex; mc::Recorder recorder;
    std::atomic<int> recStateA { recOff }; std::atomic<size_t> recFramesA { 0 };
    std::atomic<float>* pThreshold = nullptr; std::atomic<float>* pPreroll = nullptr; std::atomic<float>* pMonitor = nullptr; std::atomic<float>* pLoop = nullptr;

    juce::SpinLock snapLock; std::shared_ptr<const mc::Audio> snap; std::deque<std::shared_ptr<const mc::Audio>> keepAlive;
    std::atomic<bool> countInActive { false }, countInArm { false }; std::atomic<double> countInPos { 0.0 }, countInTotal { 0.0 }, countInBeat { 0.0 };
    std::atomic<float>* pTempo = nullptr;
    std::atomic<bool> previewing { false };
    std::atomic<bool> playing { false }; std::atomic<double> playPos { 0.0 }, playStart { 0.0 }, playEnd { 0.0 };
    double hostSr = 48000.0; float liveAcc = 0.f; int liveCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeowcorderProcessor)
};
