// Renders the real editor window to a PNG (no host, no display needed) so it can be compared with the mockup.
// Usage: meowcorder_snapshot out.png [sel | spec]
#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    MeowcorderProcessor p; p.prepareToPlay (44100.0, 512);
    // a demo sound: eight decaying hits, like the mockup's demo loop
    mc::Audio a; a.sr = 44100.0; a.resize ((size_t) (4.0 * 44100.0)); mc::Rng rng;
    for (size_t i = 0; i < a.frames(); ++i)
    {
        const double t = (double) i / a.sr, ph = std::fmod (t, 0.5); const float env = (float) std::exp (-ph * 7.0); const float n = rng.bi() * 0.25f;
        const float v = env * (0.8f * (float) std::sin (2 * 3.14159265 * (90.0 + 400.0 * std::exp (-ph * 30.0)) * ph) + n);
        a.ch[0][i] = v; a.ch[1][i] = v * 0.97f;
    }
    if (argc > 2 && std::string (argv[2]) == "pitch")        // a short melody instead of the drums
    {
        const double notes[] = { 220.0, 261.63, 329.63, 293.66, 220.0 };
        for (size_t i = 0; i < a.frames(); ++i) { const double t = (double) i / a.sr; const int k = juce::jlimit (0, 4, (int) (t / 0.8)); const double lt = t - k * 0.8; const float env = (float) (std::min (1.0, lt * 40.0) * std::exp (-lt * 1.2));
                                                  a.ch[0][i] = a.ch[1][i] = 0.45f * env * (float) (std::sin (2 * 3.14159265 * notes[k] * t) + 0.3 * std::sin (4 * 3.14159265 * notes[k] * t)); }
    }
    p.ed.load (a); p.audioChanged(); p.soundName = "demo loop, not saved";
    if (argc > 2 && std::string (argv[2]) == "sel") p.ed.setSelection ((size_t) (0.9 * 44100.0), (size_t) (2.1 * 44100.0));
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    if (argc > 2 && std::string (argv[2]) == "pitch") static_cast<MeowcorderEditor*> (ed.get())->showView (2);
    if (argc > 2 && std::string (argv[2]) == "spec") static_cast<MeowcorderEditor*> (ed.get())->showView (1);
    ed->setVisible (true); ed->setBounds (0, 0, ed->getWidth(), ed->getHeight());
    juce::MessageManager::getInstance()->runDispatchLoopUntil (400);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File out (argc > 1 ? argv[1] : "snapshot.png"); out.deleteFile(); juce::FileOutputStream os (out); juce::PNGImageFormat png; png.writeImageToStream (img, os);
    std::printf ("wrote %s (%d x %d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    return 0;
}
