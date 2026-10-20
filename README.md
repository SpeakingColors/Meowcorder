# Meowcorder

A free audio recorder and editor for Linux first (also Windows and macOS): record from a track or input, cut and process the sound, drop it back. Made for quick sound design in the middle of a project. Part of the Studio 87 cat plugin suite.

## Status
Version 0.1 (early): the engine is done (pure C++, 619 passing checks). The plugin, the window and the installers are not built yet. See [ROADMAP](ROADMAP.md).

## What the engine does
- Recorder that is always listening: immediate or threshold start, pre-roll (the sound just before you pressed record), capture of the last seconds without recording
- Editor with selection, cursor, undo and redo, cut, copy, paste, delete, crop, snap to zero crossing, punch in, insert and append recordings
- 24 operations on the selection (or the whole sound): gain, normalize, fades, reverse, invert, DC remove, silence, make mono, balance, pitch and speed, bitcrusher, Drive (Drive, Transistor, Fuzz as in PurrOne), EQ, filter, delay, chorus, phaser, reverb, compressor, limiter, noise gate, trim silence, make loop
- Repeat last operation
- WAV in (8, 16, 24, 32 bit, float 32 and 64) and out (16, 24, 32 float)
- Peak overview, spectrogram and hit detection for the display

## Build and test the core
```
sh tests/run.sh
```

## Support
Free and open source. If you like it, you can support it on [Ko-fi](https://ko-fi.com/studio87). Nothing is locked and nothing is collected.

## License
AGPL-3.0-or-later, see [LICENSE](LICENSE).
