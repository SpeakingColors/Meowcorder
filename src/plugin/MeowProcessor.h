// Meowcorder plugin (VST3 and standalone). The editing engine is src/core (pure C++); this file connects it to JUCE.
// Thread rules: the audio thread only copies the input into a FIFO, plays a published copy of the sound, makes the count-in click
// and measures levels. The recorder, the editor model and all files live on the message thread (a 30 Hz timer drains the FIFO).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include "../core/MeowCore.h"

class MeowProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    enum RecMode { RecNow = 0, RecOnInput = 1, RecCountIn = 2 };
    enum TakeMode { TakeReplace = 0, TakeAppend = 1, TakePunch = 2, TakeInsert = 3 };

    MeowProcessor();
    ~MeowProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Meowcorder"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- message thread only ----
    mc::Editor doc;                 // the sound, selection, undo
    int version() const { return ver.load(); }
    double currentBpm() const { return hostBpm.load(); }
    void changed() { ++ver; }       // call after any change to doc
    bool loadFile(const juce::File&);
    bool saveFile(const juce::File&, bool flac);
    std::atomic<int> exportBits { 24 }; std::atomic<bool> exportDither { false };   // export word length (8, 16, 24, 32 float) and dither when rounding
    juce::File exportTempWav();     // for dragging the sound into the host
    void newSound();

    void startRecord(int mode);     // RecMode
    void stopRecord();              // puts the take into the sound
    bool isRecording() const;       // recording or armed
    bool isArmedOnly() const;
    bool isCountingIn() const { return countInLeft.load() > 0; }

    void play();                    // selection if any, else from the cursor to the end
    void stopPlay() { playing = false; }
    bool isPlaying() const { return playing.load(); }
    double playPosFrames() const { return playPos.load(); }   // in frames of the sound
    double recordedSeconds() const;

    // levels, 0..1, written by the audio thread
    std::atomic<float> inPeak { 0.f }, outPeak { 0.f };

    juce::AudioProcessorValueTreeState apvts;

private:
    void timerCallback() override;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

    std::atomic<int> ver { 0 };
    juce::CriticalSection lock;     // recorder and its settings
    mc::Recorder recorder; double sr = 48000.0; int64_t drained = 0; int pendingTake = TakeReplace;

    // FIFO audio thread -> message thread
    static constexpr int kFifo = 1 << 18;
    juce::AbstractFifo fifo { kFifo }; std::vector<float> fifoBuf[2]; std::atomic<int64_t> written { 0 };
    std::vector<float> drainL, drainR;

    // playback
    juce::SpinLock playLock; std::shared_ptr<const mc::Audio> pubAudio; std::shared_ptr<const mc::Audio> curAudio; std::atomic<bool> pubFlag { false };
    std::atomic<bool> playing { false }; std::atomic<double> playPos { 0.0 }, playFrom { 0.0 }, playTo { 0.0 };

    // count-in click
    std::atomic<int> countInLeft { 0 }, countInTotal { 0 }; std::atomic<bool> countInDone { false }; std::atomic<int64_t> countInDoneAt { 0 };
    std::atomic<double> hostBpm { 120.0 }; int clickLeft = 0; float clickPhase = 0.f, clickFreq = 1000.f;

    std::atomic<float>* pBpm = nullptr; std::atomic<float>* pThresh = nullptr; std::atomic<float>* pPre = nullptr; std::atomic<float>* pMonitor = nullptr; std::atomic<float>* pLoop = nullptr; std::atomic<float>* pTake = nullptr;
    juce::AudioFormatManager fm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeowProcessor)
};
