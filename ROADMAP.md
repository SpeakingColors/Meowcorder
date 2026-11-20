# Meowcorder roadmap

Done (v0.1)
- C++ core with 619 checks: recorder with pre-roll, editor with undo, 24 operations, WAV in and out, spectrogram, hit detection

Next
1. Interactive mockup, Edison style: menu bar with shortcuts shown, record from the input with start options (now, on input, count-in with click), spectral view with tools (mouse, selector, scissors, cutter, eraser, copy, paste, amp). Done; the JavaScript engine matches the C++ core on all 24 operations. Spectral editing exists in JavaScript only (tools/spectral_test.js); the C++ port comes with the plugin.
2. JUCE plugin and standalone: window built from the mockup, record from the track or input. Done in 0.2 for the main window: record, waveform, edit, 24 operations, play, files. Done in 0.2.1: count-in click, mono takes, history list, tempo detection. Done in 0.3: spectral view and its box tools (C++ port of the mockup, same results). Done in 0.4: pitch view (notes found with YIN, drag a note to retune it), warp and transpose with four methods and formant keeping (C++ port of the mockup DSP, 662 engine checks). Done in 0.5: the waveform pencil (Shift+Q, one stroke is one undo step). Still to do: drag the result back into the host, spectral eraser and pencil, overlay and split views, host tempo sync, the settings page, the process popups with scope and mix, host tempo sync
3. Real file formats beyond WAV (FLAC, OGG) through the host or a small library
4. Manual with interface tour, installers (Windows, macOS, Flatpak), release workflow
5. Test in Bitwig and Ableton

   v4 mockup: own Edison-style history (undo, redo, repeat, jump to any step), process popups with scope and mix, waveform / spectral / pitch views (solo, overlay or split, front view gets the edits), timing with bpm and host sync, warp modes, transpose and formants, piano roll with draggable notes, clickable channel / rate / bit depth info, configurable Play keys, spectral pencil and waveform pencil. New DSP is JavaScript only for now (tools/dsp_test.js); the C++ port comes with the plugin.

Known limits
- The plugin 0.2 has been built and checked headless (record, edit, play, files, state) but not yet tried in a real DAW; Bitwig and Ableton tests are item 5
- The sound you record or open lives in the plugin while it is open and is not saved inside the project, save it as a WAV to keep it
- Every edit copies the sound for playback, so very long sounds (many minutes) edit slowly
- Reverb, compressor, limiter and gate are simple and not tuned by ear
- Warp preview renders the sound first, so long sounds may pause for a moment
- Name is not final
