// Meowcorder window, built from docs/mockups/Meowcorder.html: wood cheeks, menus with shortcuts, transport and record controls,
// IN and OUT meters, stereo waveform with selection, ruler and playhead, spectral view, process panels for all 24 operations.
// Also: history list, spectral tools, pitch piano roll, layers (overlay or split), warp panel, waveform pencil, live input strip, clickable channel, rate, bit depth and time unit chips.
#pragma once
#include "MeowProcessor.h"

class MeowEditor : public juce::AudioProcessorEditor, private juce::Timer, public juce::DragAndDropContainer
{
public:
    explicit MeowEditor(MeowProcessor&);
    ~MeowEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    void timerCallback() override;
};
