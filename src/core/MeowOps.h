// Meowcorder: operation table. One source of truth for the engine, the mockup and the manual.
// Pure C++17, no JUCE. Every operation works on the selection (or the whole sound when nothing is selected).
#pragma once
#include <cstddef>

namespace mc
{
struct OpParam
{
    const char* id; const char* name;
    float lo, hi, def; const char* unit; const char* tip;
    bool choice; const char* choices;   // '|' separated labels when choice
};

enum OpId
{
    opGain, opNormalize, opFadeIn, opFadeOut, opReverse, opInvert, opDcRemove, opSilence, opMono, opPan,
    opRate, opBitcrush, opDrive, opEq, opFilter, opDelay, opChorus, opPhaser, opReverb,
    opCompressor, opLimiter, opGate, opTrimSilence, opLoopXfade,
    opCount
};

constexpr int kMaxOpParams = 6;

struct OpInfo
{
    const char* id; const char* name; const char* group; const char* tip;
    bool changesLength;              // true: the selection can become longer or shorter
    int nParams; OpParam params[kMaxOpParams];
};

static const OpInfo kOps[opCount] = {
    { "gain", "Gain", "Level", "Makes the selection louder or quieter by a fixed amount.", false, 1, {
        { "gain", "Gain", -60, 24, 0, "dB", "Change in level. Plus 6 dB is about twice as loud.", false, "" } } },
    { "normalize", "Normalize", "Level", "Sets the level of the selection to a target. Never pushes the peak above 0 dBFS.", false, 2, {
        { "target", "Target", -36, 0, -1, "dB", "Level to reach.", false, "" },
        { "mode", "Measure", 0, 1, 0, "", "Peak: loudest sample. RMS: average loudness, then limited so the peak stays under 0 dBFS.", true, "Peak|RMS" } } },
    { "fadeIn", "Fade in", "Level", "Starts silent and rises to full level across the selection.", false, 1, {
        { "curve", "Curve", 0.25f, 4, 1, "x", "1 is straight. Above 1 stays quiet longer, below 1 comes up fast.", false, "" } } },
    { "fadeOut", "Fade out", "Level", "Falls from full level to silence across the selection.", false, 1, {
        { "curve", "Curve", 0.25f, 4, 1, "x", "1 is straight. Above 1 stays loud longer, below 1 drops fast.", false, "" } } },
    { "reverse", "Reverse", "Edit", "Plays the selection backwards.", false, 0, {} },
    { "invert", "Invert polarity", "Edit", "Flips the wave upside down. Sounds the same alone, cancels against the original.", false, 0, {} },
    { "dcRemove", "Remove DC offset", "Edit", "Centres the wave on zero. Removes a constant push that wastes headroom.", false, 0, {} },
    { "silence", "Silence", "Edit", "Replaces the selection with silence, same length.", false, 0, {} },
    { "mono", "Make mono", "Stereo", "Turns the selection into one channel on both sides.", false, 1, {
        { "mode", "Source", 0, 2, 0, "", "Sum of both sides, or only the left, or only the right.", true, "Sum|Left|Right" } } },
    { "pan", "Balance", "Stereo", "Moves the stereo image left or right. The quieter side goes down, the other stays.", false, 1, {
        { "balance", "Balance", -100, 100, 0, "%", "Negative is left, positive is right.", false, "" } } },
    { "rate", "Pitch and speed", "Time", "Plays the selection faster or slower. Pitch and length change together, like tape.", true, 2, {
        { "semis", "Pitch", -24, 24, 0, "st", "Up makes it shorter and higher, down makes it longer and lower.", false, "" },
        { "fine", "Fine", -100, 100, 0, "ct", "Fine tune in cents.", false, "" } } },
    { "bitcrush", "Bitcrusher", "Colour", "Lo-fi: fewer bits and a lower sample rate.", false, 4, {
        { "bits", "Bits", 1, 16, 8, "", "Resolution. Lower is rougher.", false, "" },
        { "down", "Downsample", 1, 64, 1, "x", "Keeps one sample out of this many and holds it.", false, "" },
        { "dither", "Dither", 0, 1, 0, "", "Adds a little noise before rounding so quiet parts stay smoother.", true, "Off|On" },
        { "mix", "Mix", 0, 100, 100, "%", "Blend with the original.", false, "" } } },
    { "drive", "Drive", "Colour", "Three analog style stages, the same as PurrOne: Drive, Transistor and Fuzz. Level matched.", false, 3, {
        { "drive", "Drive", 0, 100, 30, "%", "Symmetric clip. Odd harmonics, bright.", false, "" },
        { "transistor", "Transistor", 0, 100, 0, "%", "Asymmetric stage. Even harmonics, warm.", false, "" },
        { "fuzz", "Fuzz", 0, 100, 0, "%", "Lopsided hard fuzz.", false, "" } } },
    { "eq", "EQ", "Tone", "Low shelf at 120 Hz, a bell and a high shelf at 8 kHz.", false, 5, {
        { "lowG", "Low", -18, 18, 0, "dB", "Low shelf gain.", false, "" },
        { "midG", "Mid", -18, 18, 0, "dB", "Bell gain.", false, "" },
        { "midF", "Mid Hz", 100, 8000, 1000, "Hz", "Bell frequency.", false, "" },
        { "midQ", "Mid Q", 0.3f, 8, 1, "", "Bell width. Higher is narrower.", false, "" },
        { "highG", "High", -18, 18, 0, "dB", "High shelf gain.", false, "" } } },
    { "filter", "Filter", "Tone", "Low pass, high pass or band pass.", false, 3, {
        { "type", "Type", 0, 2, 0, "", "Which part of the sound is kept.", true, "LP|HP|BP" },
        { "cutoff", "Cutoff", 20, 20000, 1000, "Hz", "Filter frequency.", false, "" },
        { "reso", "Reso", 0.5f, 12, 0.7f, "", "Resonance at the cutoff.", false, "" } } },
    { "delay", "Delay", "Space", "Echoes. The tail can be added after the selection so it rings out.", true, 4, {
        { "time", "Time", 1, 2000, 250, "ms", "Time between echoes.", false, "" },
        { "feedback", "Feedback", 0, 95, 40, "%", "How much of each echo feeds the next one.", false, "" },
        { "mix", "Mix", 0, 100, 35, "%", "Amount of echo.", false, "" },
        { "tail", "Add tail", 0, 10, 0, "s", "Extra silence added after the selection so the echoes can finish.", false, "" } } },
    { "chorus", "Chorus", "Space", "Thickens by mixing in slightly detuned copies.", false, 3, {
        { "rate", "Rate", 0.05f, 5, 0.6f, "Hz", "Speed of the movement.", false, "" },
        { "depth", "Depth", 0, 10, 3, "ms", "Size of the movement.", false, "" },
        { "mix", "Mix", 0, 100, 50, "%", "Amount of effect.", false, "" } } },
    { "phaser", "Phaser", "Space", "Sweeping notches that move up and down.", false, 5, {
        { "rate", "Rate", 0.05f, 5, 0.4f, "Hz", "Speed of the sweep.", false, "" },
        { "depth", "Depth", 0, 100, 70, "%", "How far the sweep travels.", false, "" },
        { "feedback", "Feedback", 0, 90, 40, "%", "Sharper notches.", false, "" },
        { "stages", "Stages", 2, 12, 6, "", "More stages, more notches. Even numbers only.", false, "" },
        { "mix", "Mix", 0, 100, 50, "%", "Amount of effect.", false, "" } } },
    { "reverb", "Reverb", "Space", "A room around the sound. The tail can be added so it rings out.", true, 4, {
        { "size", "Size", 0, 100, 50, "%", "Room size.", false, "" },
        { "damp", "Damping", 0, 100, 40, "%", "Higher darkens the tail.", false, "" },
        { "mix", "Mix", 0, 100, 30, "%", "Amount of reverb.", false, "" },
        { "tail", "Add tail", 0, 10, 2, "s", "Extra silence added after the selection so the reverb can finish.", false, "" } } },
    { "compressor", "Compressor", "Dynamics", "Turns down what is above the threshold.", false, 5, {
        { "thresh", "Threshold", -60, 0, -18, "dB", "Level where it starts to work.", false, "" },
        { "ratio", "Ratio", 1, 20, 3, ":1", "How hard it pushes down.", false, "" },
        { "attack", "Attack", 0.1f, 200, 10, "ms", "How fast it reacts.", false, "" },
        { "release", "Release", 5, 2000, 120, "ms", "How fast it lets go.", false, "" },
        { "makeup", "Makeup", -12, 24, 0, "dB", "Level added after compression.", false, "" } } },
    { "limiter", "Limiter", "Dynamics", "Stops peaks from going above the ceiling. Looks ahead, so nothing gets through.", false, 2, {
        { "ceiling", "Ceiling", -24, 0, -1, "dB", "Highest level allowed.", false, "" },
        { "release", "Release", 5, 500, 60, "ms", "How fast it lets go.", false, "" } } },
    { "gate", "Noise gate", "Dynamics", "Silences what is below the threshold.", false, 5, {
        { "thresh", "Threshold", -90, 0, -50, "dB", "Level below which the sound is turned down.", false, "" },
        { "attack", "Attack", 0.1f, 100, 2, "ms", "How fast it opens.", false, "" },
        { "hold", "Hold", 0, 500, 40, "ms", "How long it stays open after the sound dips.", false, "" },
        { "release", "Release", 1, 2000, 120, "ms", "How fast it closes.", false, "" },
        { "range", "Range", -90, 0, -90, "dB", "How far it turns the sound down when closed.", false, "" } } },
    { "trimSilence", "Trim silence", "Edit", "Cuts quiet parts off the start and end.", true, 1, {
        { "thresh", "Threshold", -90, -10, -60, "dB", "Anything quieter than this counts as silence.", false, "" } } },
    { "loopXfade", "Make loop", "Time", "Blends the end into the start so the selection repeats without a click. Becomes a little shorter.", true, 1, {
        { "xfade", "Crossfade", 1, 500, 50, "ms", "Length of the blend.", false, "" } } }
};
} // namespace mc
