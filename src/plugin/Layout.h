// Generated from docs/mockups/Meowcorder.html (positions in the 1382 x 876 design window). Do not edit by hand.
#pragma once
namespace layout {
struct R { int x, y, w, h; };
constexpr int kW = 1382, kH = 876;
constexpr R power{1197,15,71,31};
constexpr R bZoom{1276,15,52,31};
constexpr R bLoop{63,114,33,31};
constexpr R bPlay{100,114,33,31};
constexpr R bStop{137,114,33,31};
constexpr R bRec{174,114,39,31};
constexpr R bRecOpt{212,114,18,31};
constexpr R bTake{242,125,192,19};
constexpr R bNew{63,176,33,31};
constexpr R bOpen{99,176,33,31};
constexpr R bSave{135,176,33,31};
constexpr R bUndo{183,176,33,31};
constexpr R bRedo{219,176,33,31};
constexpr R bHist{255,176,33,31};
constexpr R bCut{303,176,33,31};
constexpr R bCopy{339,176,33,31};
constexpr R bPaste{375,176,33,31};
constexpr R bDel{412,176,33,31};
constexpr R bCrop{448,176,33,31};
constexpr R bAll{496,176,33,31};
constexpr R bSnap{532,176,33,31};
constexpr R bZin{580,176,33,31};
constexpr R bZout{616,176,33,31};
constexpr R bFit{652,176,33,31};
constexpr R bSel{688,176,33,31};
constexpr R bRepeat{736,176,33,31};
constexpr R bGrab{772,176,33,31};
constexpr R bMono{820,176,33,31};
constexpr R bInput{856,176,33,31};
constexpr R bMon{892,176,33,31};
constexpr R bSpec{928,176,33,31};
constexpr R meter{1224,68,100,778};
constexpr R cCh{56,242,72,22};
constexpr R cSr{131,242,85,22};
constexpr R cBits{220,242,72,22};
constexpr R cUnit{307,242,58,22};
constexpr R cBpm{369,242,92,22};
constexpr R cSync{465,242,58,22};
constexpr R cWarp{527,242,58,22};
constexpr R live{55,272,1155,33};
constexpr R vComb{276,309,27,23};
constexpr R vPen{311,309,27,23};
constexpr R wave{55,337,1155,459};
constexpr R ruler{55,797,1155,21};
constexpr R ovw{55,818,1155,33};
constexpr R bStart{436,276,101,25};
constexpr R mFile{63,68,38,25};
constexpr R mEdit{100,68,39,25};
constexpr R mSelect{140,68,52,25};
constexpr R mView{191,68,44,25};
constexpr R mSpectral{235,68,64,25};
constexpr R mWarp{299,68,46,25};
constexpr R mPlay{345,68,41,25};
constexpr R mRecord{386,68,56,25};
constexpr R mProcess{442,68,62,25};
constexpr R mHelp{504,68,42,25};
constexpr R tNow{242,100,64,22};
constexpr R tOnInput{305,100,64,22};
constexpr R tCountIn{369,100,64,22};
constexpr R tWave{94,310,68,21};
constexpr R tPitch{221,310,47,21};
constexpr R tPitch2{221,310,47,21};
constexpr R kThreshold{618,110,37,37};
constexpr R kTempo{674,110,37,37};
constexpr R kPreroll{731,110,37,37};
constexpr R vThreshold{610,148,53,16};
constexpr R vTempo{669,148,47,16};
constexpr R vPreroll{729,148,41,16};
constexpr R bTakeBtn{242,125,192,19};
struct OpCell { int x, y, w, h; };
static const OpCell kOpCells[24] = {
    {795,68,31,29},
    {795,100,31,29},
    {829,68,31,29},
    {829,100,31,29},
    {863,68,31,29},
    {863,100,31,29},
    {897,68,31,29},
    {897,100,31,29},
    {931,100,31,29},
    {966,68,31,29},
    {966,100,31,29},
    {1000,100,31,29},
    {1034,68,31,29},
    {1034,100,31,29},
    {1068,68,31,29},
    {1068,100,31,29},
    {1102,68,31,29},
    {1102,100,31,29},
    {1137,68,31,29},
    {1137,100,31,29},
    {1171,68,31,29},
    {1171,100,31,29},
    {931,68,31,29},
    {1000,68,31,29},
};
constexpr R opCell(int op) { return R{ kOpCells[op].x, kOpCells[op].y, kOpCells[op].w, kOpCells[op].h }; }
struct OpMeta { const char* tip; const char* shortcut; const char* name; unsigned colour; };
static const OpMeta kOpMeta[24] = {
    { "Makes the selection louder or quieter by a fixed amount.", "Shift+G", "Gain", 0x02EBBA },
    { "Sets the level of the selection to a target. Never pushes the peak above 0 dBFS.", "Shift+N", "Normalize", 0x02EBBA },
    { "Starts silent and rises to full level across the selection.", "Shift+I", "Fade in", 0x02EBBA },
    { "Falls from full level to silence across the selection.", "Shift+O", "Fade out", 0x02EBBA },
    { "Plays the selection backwards.", "Shift+R", "Reverse", 0x6CC9F5 },
    { "Flips the wave upside down. Sounds the same alone, cancels against the original.", "Shift+P", "Invert polarity", 0x6CC9F5 },
    { "Centres the wave on zero. Removes a constant push that wastes headroom.", "Shift+D", "Remove DC offset", 0x6CC9F5 },
    { "Replaces the selection with silence, same length.", "Shift+S", "Silence", 0x6CC9F5 },
    { "Turns the selection into one channel on both sides.", "Shift+M", "Make mono", 0xA43DFF },
    { "Moves the stereo image left or right. The quieter side goes down, the other stays.", "Shift+B", "Balance", 0xA43DFF },
    { "Plays the selection faster or slower. Pitch and length change together, like tape.", "Shift+Y", "Pitch and speed", 0xFF9000 },
    { "Lo-fi: fewer bits and a lower sample rate.", "Shift+K", "Bitcrusher", 0xFFD100 },
    { "Three analog style stages, the same as PurrOne: Drive, Transistor and Fuzz. Level matched.", "Shift+V", "Drive", 0xFFD100 },
    { "Low shelf at 120 Hz, a bell and a high shelf at 8 kHz.", "Shift+E", "EQ", 0xFF1383 },
    { "Low pass, high pass or band pass.", "Shift+F", "Filter", 0xFF1383 },
    { "Echoes. The tail can be added after the selection so it rings out.", "Shift+L", "Delay", 0x02EBBA },
    { "Thickens by mixing in slightly detuned copies.", "Shift+H", "Chorus", 0x02EBBA },
    { "Sweeping notches that move up and down.", "Shift+A", "Phaser", 0x02EBBA },
    { "A room around the sound. The tail can be added so it rings out.", "Shift+W", "Reverb", 0x02EBBA },
    { "Turns down what is above the threshold.", "Shift+C", "Compressor", 0x02EBBA },
    { "Stops peaks from going above the ceiling. Looks ahead, so nothing gets through.", "Shift+T", "Limiter", 0x02EBBA },
    { "Silences what is below the threshold.", "Shift+U", "Noise gate", 0x02EBBA },
    { "Cuts quiet parts off the start and end.", "Shift+X", "Trim silence", 0x6CC9F5 },
    { "Blends the end into the start so the selection repeats without a click. Becomes a little shorter.", "Shift+J", "Make loop", 0xFF9000 },
};
struct Tip { const char* key; const char* text; const char* shortcut; const char* name; };
static const Tip kTips[] = {
    { "bLoop", "Repeats the playback again and again.", "L", "Loop" },
    { "bPlay", "Plays from the cursor, or the start of the selection. Pressed again while playing: stops. Change this in Settings.", "", "Play" },
    { "bStop", "Stops playing and recording.", "", "Stop" },
    { "bRec", "Records what comes in from the input. Where it starts is set by the buttons next to it.", "Enter", "Record" },
    { "bNew", "Starts with an empty sound. Undo brings the old one back.", "Alt+N", "New empty sound" },
    { "bOpen", "Opens an audio file. You can also drop a file on the display.", "Ctrl+O", "Open file" },
    { "bSave", "Saves the sound as a WAV file. In this web mockup the browser may block the download.", "Ctrl+S", "Save as WAV" },
    { "bUndo", "Nothing to undo.", "Ctrl+Z", "Undo" },
    { "bRedo", "Nothing to redo. Ctrl+Shift+Z repeats the last operation.", "Ctrl+Y", "Redo" },
    { "bHist", "Every step of this sound, with its own undo that is separate from the host. Click to jump back to any step.", "Alt+Y", "History" },
    { "bCut", "Moves the selection to the clipboard and closes the gap.", "Ctrl+X", "Cut" },
    { "bCopy", "Copies the selection to the clipboard.", "Ctrl+C", "Copy" },
    { "bPaste", "Puts the clipboard at the cursor, or over the start of the selection.", "Ctrl+V", "Paste" },
    { "bDel", "Deletes the selection and closes the gap.", "Del", "Delete" },
    { "bCrop", "Keeps only the selection and throws the rest away.", "T", "Crop" },
    { "bAll", "Selects the whole sound.", "Ctrl+A", "Select all" },
    { "bSnap", "Moves the selection edges to the nearest zero crossing, so cuts do not click.", "Z", "Snap to zero" },
    { "bZin", "Shows less of the sound, in more detail.", "+", "Zoom in" },
    { "bZout", "Shows more of the sound.", "-", "Zoom out" },
    { "bFit", "Fits the whole sound in the display.", "F", "Show everything" },
    { "bSel", "Fills the display with the selection.", "S", "Zoom to selection" },
    { "bRepeat", "Nothing to repeat yet. Apply an operation first.", "R", "Repeat last operation" },
    { "bGrab", "Keeps what you already heard, even though you were not recording. Uses the pre-roll length.", "G", "Grab the last seconds" },
    { "bMono", "What a new take is made of. Click to switch. The display shows one lane for a mono sound and two for a stereo one. Operations like Make mono change a sound you already have.", "Alt+O", "Recording in stereo" },
    { "bInput", "Where the recording comes from. In the plugin: the mixer channel it sits on. Here: a demo loop stands in, or your microphone. Click to switch.", "M", "Input: mixer channel" },
    { "bMon", "Lets you hear what comes in while you record. Mute it when the sound already plays somewhere else.", "Alt+M", "Monitoring the input" },
    { "bSpec", "Shows the sound as a picture of its frequencies, with tools to cut, erase, copy and amplify parts of it.", "", "Spectral view" },
    { "vComb", "Turn on to see more than one view at once. Overlay puts them on top of each other, Split stacks them. The view In front is the one your tools work on.", "Alt+B", "Show views together" },
    { "vPen", "Draw the wave by hand. Zoom in until you can see single samples, then drag over the wave to set them. Shift+Q turns it on or off.", "Shift+Q", "Waveform pencil" },
};
constexpr R logo{54,16,29,29};
constexpr R title{93,16,131,28};
constexpr const char* kLogoSvg = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" aria-hidden="true"><path d="M4.5 5C10 7 17 11 21 14c3-.5 7-2 11-2s8 1.500 11 2C47 11 55 7 60 5.500L61 24C61 40 48 50 32 58 16 50 3 40 3.700 24z" fill="#f4f5f7"/><path d="M11.300 25.800C20 26 25 31 27 36.500 21 37.500 15 36 12.500 31zM52.700 25.800C44 26 39 31 37 36.500 43 37.500 49 36 51.500 31z" fill="#15161a"/></svg>)SVG";
} // namespace layout
