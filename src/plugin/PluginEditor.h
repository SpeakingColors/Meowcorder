// Meowcorder editor: the approved mockup (docs/mockups/Meowcorder.html) ported natively in JUCE.
// Positions and icons are generated from the mockup (Layout.h, Icons.h), see tools/gen_layout.py.
#pragma once
#include "PluginProcessor.h"

class MeowcorderEditor : public juce::AudioProcessorEditor
{
public:
    explicit MeowcorderEditor (MeowcorderProcessor&);
    ~MeowcorderEditor() override;
    void resized() override;
    void setScalePercent (int percent);
    void showView (int mode);              // 0 waveform, 1 spectral (used by the snapshot tool)

    class Content;                         // everything on screen, laid out in the 1382 x 876 design window

private:
    MeowcorderProcessor& proc;
    std::unique_ptr<Content> content;
    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeowcorderEditor)
};
