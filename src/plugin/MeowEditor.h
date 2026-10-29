// Meowcorder window, built from docs/mockups/Meowcorder.html: wood cheeks, menus with shortcuts, transport and record controls,
// IN and OUT meters, stereo waveform with selection, ruler and playhead, spectral view, process panels for all 24 operations.
// Not built yet (see ROADMAP): the spectral tools, the pitch piano roll, warp and the clickable channel, rate and bit depth info.
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
