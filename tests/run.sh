#!/bin/sh
# Builds and runs the pure C++ core tests (no JUCE needed): sh tests/run.sh
set -e
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/meowcorder_test test_core.cpp
/tmp/meowcorder_test
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/meowcorder_dsp_test test_dsp.cpp
/tmp/meowcorder_dsp_test

# The spectral editing of the web mockup (STFT based, in docs/mockups/engine.js)
node ../tools/spectral_test.js

# New v4 DSP: resample, vocoder, pitch, warp, notes, tempo, history, pencils
node ../tools/dsp_test.js
