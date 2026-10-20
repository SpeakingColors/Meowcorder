# Meowcorder roadmap

Done (v0.1)
- C++ core with 619 checks: recorder with pre-roll, editor with undo, 24 operations, WAV in and out, spectrogram, hit detection

Next
1. Interactive mockup, Edison style: menu bar with shortcuts shown, record from the input with start options (now, on input, count-in with click), spectral view with tools (mouse, selector, scissors, cutter, eraser, copy, paste, amp). Done; the JavaScript engine matches the C++ core on all 24 operations. Spectral editing exists in JavaScript only (tools/spectral_test.js); the C++ port comes with the plugin.
2. JUCE plugin and standalone: window built from the mockup, record from the track or input, drag the result back into the host
3. Real file formats beyond WAV (FLAC, OGG) through the host or a small library
4. Manual with interface tour, installers (Windows, macOS, Flatpak), release workflow
5. Test in Bitwig and Ableton

Known limits
- Reverb, compressor, limiter and gate are simple and not tuned by ear
- Pitch and speed changes pitch and length together (like tape); no time stretch yet
- Name is not final
