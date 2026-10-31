# Meowcorder roadmap

Done (v0.1)
- C++ core with 619 checks: recorder with pre-roll, editor with undo, 24 operations, WAV in and out, spectrogram, hit detection

Next
1. Interactive mockup, Edison style: menu bar with shortcuts shown, record from the input with start options (now, on input, count-in with click), spectral view with tools (mouse, selector, scissors, cutter, eraser, copy, paste, amp). Done; the JavaScript engine matches the C++ core on all 24 operations. Spectral editing exists in JavaScript only (tools/spectral_test.js); the C++ port comes with the plugin.
2. JUCE plugin and standalone: first build done (CI green on Linux and Windows, window seen via headless snapshot). Written and compiling, but not yet clicked through by a person: history list, spectral tools, pitch piano roll, layers, warp, waveform pencil, live strip, clickable channel / rate / bit depth chips. Still to do: test every function by hand, test in Bitwig and Ableton, FLAC/OGG import check, installers, release. Record from the track or input and drag the result back into the host is in
3. Real file formats beyond WAV (FLAC, OGG) through the host or a small library
4. Manual with interface tour, installers (Windows, macOS, Flatpak), release workflow
5. Test in Bitwig and Ableton

   v4 mockup: own Edison-style history (undo, redo, repeat, jump to any step), process popups with scope and mix, waveform / spectral / pitch views (solo, overlay or split, front view gets the edits), timing with bpm and host sync, warp modes, transpose and formants, piano roll with draggable notes, clickable channel / rate / bit depth info, configurable Play keys, spectral pencil and waveform pencil. New DSP is JavaScript only for now (tools/dsp_test.js); the C++ port comes with the plugin.

Known limits
- Reverb, compressor, limiter and gate are simple and not tuned by ear
- Warp preview renders the sound first, so long sounds may pause for a moment
- Name is not final
