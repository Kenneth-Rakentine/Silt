// The full parameter list, shared by the app and tests. IDs match the browser lab's patch keys,
// so patch JSON moves between the lab and the app unchanged. Newer sections add their own IDs.
#pragma once
#include "SiltParams.h"
#include <cstring>
#include <cstddef>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace silt {

enum class Kind { Cont, Int, Toggle, Choice };

struct PDef {
    const char* id; const char* label; float min, max, def; bool log; Kind kind; const char* fmt; const char* group; const char* choices;
};

// Which page of the app a group belongs to.
inline const char* tabOf(const std::string& group) {
    static const char* fb[] = { "Bus 1", "Bus 2", "Bus 3" };
    for (auto* g : fb) if (group == g) return "Feedback";
    static const char* mem[] = { "Sediment", "Erosion", "Regeneration", "Microcuts" };
    for (auto* g : mem) if (group == g) return "Memory";
    static const char* mv[] = { "Clock", "LFOs and dust", "Mod matrix", "Sequencer", "Playhead 1", "Playhead 2", "Steps" };
    for (auto* g : mv) if (group == g) return "Movement";
    if (group.rfind("Catch", 0) == 0) return "Catch";
    if (group == "Bench") return "Bench";
    return "Voice";
}

// Modulation sources, in matrix order. Values: bipolar sources run -1..1, unipolar 0..1.
inline const std::vector<std::string>& modSourceNames() {
    static const std::vector<std::string> s = { "None", "LFO 1", "LFO 2", "Clock osc A", "Clock osc B", "Clock pulse", "Random (per pulse)", "Dust",
        "Sequencer value", "Sequencer gate", "Velocity", "Note", "Mod wheel", "Voice level", "Voice random", "Coherence", "Sediment", "Erosion",
        "Feedback bus 1", "Feedback bus 2", "Feedback bus 3", "Output level", "Catch output" };
    return s;
}
enum ModSrc { MS_NONE, MS_LFO1, MS_LFO2, MS_CLKA, MS_CLKB, MS_PULSE, MS_RAND, MS_DUST, MS_SEQVAL, MS_SEQGATE, MS_VEL, MS_NOTE, MS_WHEEL,
              MS_VLEVEL, MS_VRAND, MS_COH, MS_SED, MS_ERO, MS_FB1, MS_FB2, MS_FB3, MS_OUT, MS_CATCH, MS_COUNT };
inline bool modSourceIsVoice(int s) { return s == MS_VEL || s == MS_NOTE || s == MS_VLEVEL || s == MS_VRAND || s == MS_COH; }

namespace detail {
inline std::deque<std::string>& pool() { static std::deque<std::string> p; return p; }
inline const char* keep(const std::string& s) { pool().push_back(s); return pool().back().c_str(); }
}

inline const std::vector<PDef>& params() {
    static const std::vector<PDef> table = [] {
        using detail::keep;
        std::vector<PDef> v = {

    // Layer A
    { "aOn", "Layer on", 0, 1, 1, false, Kind::Toggle, "", "Layer A", "" },
    { "aMat", "Material", 0, 1, 0, false, Kind::Choice, "", "Layer A", "Wavetable|Noise" },
    { "aBank", "Bank", 0, 5, 0, false, Kind::Choice, "", "Layer A", "Classic|Hollow 90s|Reeds and brass|Vocal|Glass|Bitmask" },
    { "aPos", "Position", 0, 1, 3.f / 7.f, false, Kind::Cont, "posA", "Layer A", "" },
    { "aForm", "Formant", 0.25f, 32, 1, true, Kind::Cont, "ratio", "Layer A", "" },
    { "aCyc", "Cycles", 0.25f, 8, 1, true, Kind::Cont, "num", "Layer A", "" },
    { "aWin", "Window", 0, 1, 0, false, Kind::Cont, "win", "Layer A", "" },
    { "aPhase", "Phase", 0, 1, 0, false, Kind::Cont, "pct", "Layer A", "" },
    { "aN", "Emitters", 1, 6, 1, false, Kind::Int, "n", "Layer A", "" },
    { "aDet", "Detune spread", 0, 100, 6, false, Kind::Cont, "cents", "Layer A", "" },
    { "aWander", "Wander", 0, 1, 0, false, Kind::Cont, "pct", "Layer A", "" },
    { "aPan", "Stereo spread", 0, 1, 0.5f, false, Kind::Cont, "pct", "Layer A", "" },
    { "aLvl", "Level", 0, 1, 0.8f, false, Kind::Cont, "pct", "Layer A", "" },
    // Layer B
    { "bOn", "Layer on", 0, 1, 0, false, Kind::Toggle, "", "Layer B", "" },
    { "bSync", "Sync to A", 0, 1, 0, false, Kind::Toggle, "", "Layer B", "" },
    { "bMat", "Material", 0, 1, 0, false, Kind::Choice, "", "Layer B", "Wavetable|Noise" },
    { "bBank", "Bank", 0, 5, 0, false, Kind::Choice, "", "Layer B", "Classic|Hollow 90s|Reeds and brass|Vocal|Glass|Bitmask" },
    { "bPos", "Position", 0, 1, 3.f / 7.f, false, Kind::Cont, "posB", "Layer B", "" },
    { "bForm", "Formant", 0.25f, 32, 1, true, Kind::Cont, "ratio", "Layer B", "" },
    { "bCyc", "Cycles", 0.25f, 8, 1, true, Kind::Cont, "num", "Layer B", "" },
    { "bWin", "Window", 0, 1, 0, false, Kind::Cont, "win", "Layer B", "" },
    { "bPhase", "Phase", 0, 1, 0, false, Kind::Cont, "pct", "Layer B", "" },
    { "bN", "Emitters", 1, 6, 1, false, Kind::Int, "n", "Layer B", "" },
    { "bDet", "Detune spread", 0, 100, 6, false, Kind::Cont, "cents", "Layer B", "" },
    { "bWander", "Wander", 0, 1, 0, false, Kind::Cont, "pct", "Layer B", "" },
    { "bPan", "Stereo spread", 0, 1, 0.5f, false, Kind::Cont, "pct", "Layer B", "" },
    { "bLvl", "Level", 0, 1, 0.6f, false, Kind::Cont, "pct", "Layer B", "" },
    { "bSemi", "Pitch", -24, 24, 0, false, Kind::Int, "semi", "Layer B", "" },
    { "bFine", "Fine", -50, 50, 0, false, Kind::Cont, "cents", "Layer B", "" },
    // Voice
    { "pmAmt", "FM (B into A)", 0, 1, 0, false, Kind::Cont, "pct", "Voice", "" },
    { "formTrack", "Formant track", 0, 1, 1, false, Kind::Cont, "pct", "Voice", "" },
    { "wanderRate", "Wander rate", 0, 1, 0.3f, false, Kind::Cont, "pct", "Voice", "" },
    { "derez", "Derez", 0, 1, 0, false, Kind::Cont, "pct", "Voice", "" },
    { "drift", "Analog drift", 0, 1, 0.25f, false, Kind::Cont, "pct", "Voice", "" },
    { "resTune", "Resonator tune", 0.25f, 8, 1, true, Kind::Cont, "x", "Voice", "" },
    { "resDecay", "Resonator decay", 0.01f, 4, 0.5f, true, Kind::Cont, "time", "Voice", "" },
    { "strike", "Strike", 0, 1, 0.3f, false, Kind::Cont, "pct", "Voice", "" },
    // Filter
    { "fMode", "Mode", 0, 3, 0, false, Kind::Choice, "", "Filter and VCA", "Ladder LP 24|State variable LP 12|State variable BP|State variable HP" },
    { "fCut", "Cutoff", -5, 5, 0, false, Kind::Cont, "oct", "Filter and VCA", "" },
    { "reso", "Resonance", 0, 1, 0.2f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    { "drive", "Drive", 0, 1, 0.2f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    { "heat", "VCA heat", 0, 1, 0.25f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    { "keytrack", "Key tracking", 0, 1, 0.5f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    { "velCut", "Velocity > cutoff", 0, 1, 0.3f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    // Macros
    { "mass", "Mass", -1, 1, 0, false, Kind::Cont, "bi", "Macros", "" },
    { "grainM", "Grain", -1, 1, 0, false, Kind::Cont, "bi", "Macros", "" },
    { "allure", "Flux", 0, 1, 0, false, Kind::Cont, "pct", "Macros", "" },
    { "vol", "Volume", 0, 2.5f, 0.9f, false, Kind::Cont, "pct", "Macros", "" },
    // Articulation
    { "atk", "Attack", 0.001f, 8, 0.005f, true, Kind::Cont, "time", "Articulation", "" },
    { "dec", "Decay", 0.001f, 8, 0.6f, true, Kind::Cont, "time", "Articulation", "" },
    { "rel", "Release", 0.001f, 8, 0.5f, true, Kind::Cont, "time", "Articulation", "" },
    { "curve", "Curve", -1, 1, 0.5f, false, Kind::Cont, "bi", "Articulation", "" },
    { "tail", "Tail", 0, 10, 0, false, Kind::Cont, "tail", "Articulation", "" },
    { "loopTime", "Loop time", 0.02f, 8, 1.5f, true, Kind::Cont, "time", "Articulation", "" },
    { "loop", "Loop continuant", 0, 1, 0, false, Kind::Toggle, "", "Articulation", "" },
    // Motion
    { "auth", "Authority", 0, 1, 0, false, Kind::Cont, "pct", "Motion", "" },
    { "stiff", "Stiffness", 0.2f, 30, 6, true, Kind::Cont, "hz", "Motion", "" },
    { "visc", "Viscosity", 0.05f, 3, 0.8f, true, Kind::Cont, "num", "Motion", "" },
    { "temp", "Temperature", 0, 1, 0, false, Kind::Cont, "pct", "Motion", "" },
    { "poly", "Voices", 1, 6, 6, false, Kind::Int, "n", "Motion", "" },
    { "velAmp", "Velocity > level", 0, 1, 0.5f, false, Kind::Cont, "pct", "Motion", "" },
    // Vocoder
    { "vocOn", "Vocoder on", 0, 1, 0, false, Kind::Toggle, "", "Material vocoder", "" },
    { "vocAuto", "Auto-level", 0, 1, 1, false, Kind::Toggle, "", "Material vocoder", "" },
    { "vocBody", "Body", 0, 4, 1, false, Kind::Choice, "", "Material vocoder", "Voices|Noise|Dust (crackle)|Droplets (water)|Feedback" },
    { "vocSkin", "Skin", 0, 2, 0, false, Kind::Choice, "", "Material vocoder", "Voices|Feedback|Fixed (bands only)" },
    { "vocMix", "Mix", 0, 1, 0.5f, false, Kind::Cont, "pct", "Material vocoder", "" },
    { "vocShift", "Shift", -1.5f, 1.5f, 0, false, Kind::Cont, "oct", "Material vocoder", "" },
    { "vocSpread", "Band spacing", 0.5f, 1.6f, 1, false, Kind::Cont, "x", "Material vocoder", "" },
    { "vocQ", "Band width (Q)", 0.7f, 20, 5, true, Kind::Cont, "num", "Material vocoder", "" },
    { "vocResp", "Response", 0, 1, 0.2f, false, Kind::Cont, "pct", "Material vocoder", "" },
    { "scrapeRate", "Scrape rate", 0.05f, 400, 1, true, Kind::Cont, "hz", "Material vocoder", "" },
    { "scrapeDepth", "Scrape depth", 0, 1, 0, false, Kind::Cont, "pct", "Material vocoder", "" },
    { "dustRate", "Dust density", 5, 3000, 400, true, Kind::Cont, "per", "Material vocoder", "" },
    { "dropRate", "Droplet density", 1, 300, 60, true, Kind::Cont, "per", "Material vocoder", "" },
    { "dropPitch", "Droplet pitch", 0.3f, 3, 1, true, Kind::Cont, "x", "Material vocoder", "" },
    { "band0", "Band 1", 0, 1, 0.8f, false, Kind::Cont, "pct", "Bands", "" },
    { "band1", "Band 2", 0, 1, 0.8f, false, Kind::Cont, "pct", "Bands", "" },
    { "band2", "Band 3", 0, 1, 0.8f, false, Kind::Cont, "pct", "Bands", "" },
    { "band3", "Band 4", 0, 1, 0.8f, false, Kind::Cont, "pct", "Bands", "" },
    { "band4", "Band 5", 0, 1, 0.8f, false, Kind::Cont, "pct", "Bands", "" },
    { "band5", "Band 6", 0, 1, 0.8f, false, Kind::Cont, "pct", "Bands", "" },
    { "band6", "Band 7", 0, 1, 0.8f, false, Kind::Cont, "pct", "Bands", "" },
    { "band7", "Band 8", 0, 1, 0.8f, false, Kind::Cont, "pct", "Bands", "" },
    // Space
    { "spSize", "Size", 0.35f, 2, 1, false, Kind::Cont, "x", "Space", "" },
    { "spDecay", "Decay", 0.2f, 20, 2.5f, true, Kind::Cont, "time", "Space", "" },
    { "spTone", "Tone", 0, 1, 0.5f, false, Kind::Cont, "pct", "Space", "" },
    { "spMix", "Mix", 0, 1, 0.15f, false, Kind::Cont, "pct", "Space", "" },

        };
        auto add = [&](const std::string& id, const std::string& label, float mn, float mx, float def, bool lg, Kind k, const char* fmt, const std::string& group, const std::string& choices = "") {
            v.push_back({ keep(id), keep(label), mn, mx, def, lg, k, fmt, keep(group), keep(choices)}); };
        // ---- feedback network ----
        const char* taps = "Voices (feedback send)|Raw layers|Pre-filter|Voices dry|Vocoder wet|Space wet|Bus 1|Bus 2|Bus 3|Output";
        const char* ins = "None|Comb|Smear (allpass)|Frequency shift|Fold|Crush";
        for (int b = 0; b < NBUS; ++b) {
            const std::string p = b == 0 ? "fb" : "f" + std::to_string(b + 1), g = "Bus " + std::to_string(b + 1);
            add(p + "On", "Bus on", 0, 1, b == 0 ? 1.f : 0.f, false, Kind::Toggle, "", g);
            add(p + "Edge", "Edge guard", 0, 1, 0, false, Kind::Toggle, "", g);
            add(p + "Tap", "Tap from", 0, 9, b == 2 ? 6.f : (b == 1 ? 4.f : 0.f), false, Kind::Choice, "", g, taps);
            add(p + "Send", "Send", 0, 2, 1, false, Kind::Cont, "pct", g);
            add(p + "Time", "Delay time", 1, 1500, b == 0 ? 240.f : (b == 1 ? 37.f : 610.f), true, Kind::Cont, "ms", g);
            add(p + "Type", "Filter", 0, 3, 1, false, Kind::Choice, "", g, "Low pass|Band pass|High pass|Off");
            add(p + "Cut", "Cutoff", 40, 12000, b == 1 ? 3200.f : 1500.f, true, Kind::Cont, "hz", g);
            add(p + "Q", "Resonance", 0.5f, 20, 2, true, Kind::Cont, "num", g);
            add(p + "Ins", "Insert", 0, 5, 0, false, Kind::Choice, "", g, ins);
            add(p + "InsAmt", "Insert amount", 0, 1, 0.5f, false, Kind::Cont, "pct", g);
            add(p + "InsRate", "Insert tune", 0, 1, 0.5f, false, Kind::Cont, "pct", g);
            add(p + "Drive", "Drive", 0.3f, 8, 1.2f, true, Kind::Cont, "num", g);
            add(p + "Regen", "Regeneration", 0, 1.1f, 0.3f, false, Kind::Cont, "pct", g);
            add(p + "Mix", "Return level", 0, 1, b == 0 ? 0.f : 0.5f, false, Kind::Cont, "pct", g);
            add(p + "Dest", "Return to", 0, 2, 0, false, Kind::Choice, "", g, "Main|Into voices|Main and voices");
            add(p + "Width", "Width", 0, 1, 0.4f, false, Kind::Cont, "pct", g);
        }
        // ---- movement ----
        add("clkRun", "Clock running", 0, 1, 1, false, Kind::Toggle, "", "Clock");
        add("clkBpm", "Tempo", 20, 300, 110, false, Kind::Cont, "bpm", "Clock");
        add("clkDiv", "Pulse", 0, 3, 2, false, Kind::Choice, "", "Clock", "1/4|1/8|1/16|1/32");
        add("clkShape", "A shape", 0, 1, 0, false, Kind::Cont, "clk", "Clock");
        add("clkThr", "A threshold", -0.95f, 0.95f, 0, false, Kind::Cont, "bi", "Clock");
        add("clkBRate", "B rate", 0.01f, 20, 0.25f, true, Kind::Cont, "hz", "Clock");
        add("clkBShape", "B shape", 0, 1, 1, false, Kind::Cont, "clk", "Clock");
        add("clkMode", "B influences A by", 0, 6, 0, false, Kind::Choice, "", "Clock", "Rate|Shape|Phase|Sum|AND|OR|XOR");
        add("clkDepth", "B depth", 0, 1, 0, false, Kind::Cont, "pct", "Clock");
        add("clkAnchor", "Anchor", 0, 1, 0, false, Kind::Choice, "", "Clock", "Locked|Free");
        const char* lfoShapes = "Sine|Triangle|Saw up|Saw down|Square|Sample and hold|Smooth random";
        add("lfo1Rate", "LFO 1 rate", 0.01f, 40, 0.5f, true, Kind::Cont, "hz", "LFOs and dust");
        add("lfo1Shape", "LFO 1 shape", 0, 6, 0, false, Kind::Choice, "", "LFOs and dust", lfoShapes);
        add("lfo2Rate", "LFO 2 rate", 0.01f, 40, 0.13f, true, Kind::Cont, "hz", "LFOs and dust");
        add("lfo2Shape", "LFO 2 shape", 0, 6, 1, false, Kind::Choice, "", "LFOs and dust", lfoShapes);
        add("dustModRate", "Dust density", 0.1f, 60, 4, true, Kind::Cont, "per", "LFOs and dust");
        add("dustModDecay", "Dust decay", 0.005f, 2, 0.15f, true, Kind::Cont, "time", "LFOs and dust");
        for (int i = 0; i < NSLOT; ++i) {
            const std::string p = "m" + std::to_string(i), n = std::to_string(i + 1);
            add(p + "Src", "Slot " + n + " source", 0, float(MS_COUNT - 1), 0, false, Kind::Choice, "", "Mod matrix", "@src");
            add(p + "Dst", "Slot " + n + " destination", 0, 4096, 0, false, Kind::Choice, "", "Mod matrix", "@dst");
            add(p + "Amt", "Slot " + n + " amount", -1, 1, 0, false, Kind::Cont, "bi", "Mod matrix");
        }
        add("seqOn", "Run", 0, 1, 0, false, Kind::Toggle, "", "Sequencer");
        add("seqKeys", "Keys", 0, 1, 0, false, Kind::Choice, "", "Sequencer", "Transpose|Play over");
        add("seqLen", "Length", 1, 16, 16, false, Kind::Int, "n", "Sequencer");
        add("seqHeads", "Playheads", 1, 2, 1, false, Kind::Int, "n", "Sequencer");
        for (int h = 0; h < NHEAD; ++h) {
            const std::string p = "h" + std::to_string(h + 1), g = "Playhead " + std::to_string(h + 1);
            add(p + "Shape", "Traversal shape", 0, 5, 0, false, Kind::Choice, "", g, "Off|Rise|Fall|Arch|Spike|Wave");
            add(p + "Depth", "Shape depth", -2, 2, 1, false, Kind::Cont, "oct", g);
            add(p + "Cycle", "Shape length", 2, 64, 16, false, Kind::Int, "steps", g);
            add(p + "Branch", "Branches by", 0, 1, 0, false, Kind::Choice, "", g, "Probability|Shape");
            add(p + "Rate", "Rate", 0.25f, 4, 1, true, Kind::Cont, "x", g);
            add(p + "Start", "Start step", 1, 16, h == 0 ? 1.f : 9.f, false, Kind::Int, "n", g);
            add(p + "Trans", "Transpose", -24, 24, h == 0 ? 0.f : 12.f, false, Kind::Int, "semi", g);
        }
        for (int i = 0; i < NSTEP; ++i) {
            const std::string p = "st" + std::to_string(i), n = "Step " + std::to_string(i + 1) + " ";
            add(p + "On", n + "on", 0, 1, 1, false, Kind::Toggle, "", "Steps");
            add(p + "Note", n + "note", -24, 24, 0, false, Kind::Int, "semi", "Steps");
            add(p + "Vel", n + "velocity", 0, 1, 0.8f, false, Kind::Cont, "pct", "Steps");
            add(p + "Prob", n + "probability", 0, 1, 1, false, Kind::Cont, "pct", "Steps");
            add(p + "Len", n + "length", 0.05f, 4, 0.5f, true, Kind::Cont, "steps", "Steps");
            add(p + "Val", n + "value", -1, 1, 0, false, Kind::Cont, "bi", "Steps");
            add(p + "Cut", n + "microcut", 0, 1, 0, false, Kind::Toggle, "", "Steps");
            add(p + "To1", n + "cord 1", 0, 16, 0, false, Kind::Choice, "", "Steps", "@to1");
            add(p + "To2", n + "cord 2", 0, 16, 0, false, Kind::Choice, "", "Steps", "@to2");
            add(p + "Br", n + "branch", 0, 1, 0.5f, false, Kind::Cont, "pct", "Steps");
        }
        // ---- memory ----
        add("sedSrc", "Deposits from", 0, 3, 0, false, Kind::Choice, "", "Sediment", "Output|Notes|Voice level|Feedback");
        add("sedAmt", "Deposit", 0, 1, 0.5f, false, Kind::Cont, "pct", "Sediment");
        add("sedDecay", "Settles over", 0.5f, 300, 10, true, Kind::Cont, "time", "Sediment");
        add("sedFreeze", "Freeze", 0, 1, 0, false, Kind::Toggle, "", "Sediment");
        add("eroRate", "Wear rate", 0, 1, 0, false, Kind::Cont, "pct", "Erosion");
        add("eroTarget", "Wears", 0, 2, 0, false, Kind::Choice, "", "Erosion", "High partials|Density|Body");
        add("eroDepth", "Depth", 0, 1, 0.5f, false, Kind::Cont, "pct", "Erosion");
        add("eroRecover", "Recovers over", 1, 300, 30, true, Kind::Cont, "time", "Erosion");
        add("rgOn", "Regeneration on", 0, 1, 0, false, Kind::Toggle, "", "Regeneration");
        add("rgLen", "Memory length", 0.25f, 16, 4, true, Kind::Cont, "time", "Regeneration");
        add("rgIn", "Deposit", 0, 1, 0.5f, false, Kind::Cont, "pct", "Regeneration");
        add("rgFb", "Regeneration", 0, 1.05f, 0.7f, false, Kind::Cont, "pct", "Regeneration");
        add("rgTone", "Tone", 200, 12000, 3000, true, Kind::Cont, "hz", "Regeneration");
        add("rgDegrade", "Degrade", 0, 1, 0.3f, false, Kind::Cont, "pct", "Regeneration");
        add("rgDrift", "Drift", 0, 1, 0.2f, false, Kind::Cont, "pct", "Regeneration");
        add("rgStir", "Stir", 0, 1, 0.5f, false, Kind::Cont, "pct", "Regeneration");
        add("rgLevel", "Level", 0, 1, 0.5f, false, Kind::Cont, "pct", "Regeneration");
        add("mcOn", "Microcuts on", 0, 1, 0, false, Kind::Toggle, "", "Microcuts");
        add("mcTrig", "Triggered by", 0, 3, 0, false, Kind::Choice, "", "Microcuts", "Clock|Sequencer steps|Clock and steps|Free");
        add("mcProb", "Chance", 0, 1, 0.25f, false, Kind::Cont, "pct", "Microcuts");
        add("mcSize", "Sliver size", 5, 400, 60, true, Kind::Cont, "ms", "Microcuts");
        add("mcRepeats", "Repeats", 1, 16, 4, false, Kind::Int, "n", "Microcuts");
        add("mcRev", "Reverse chance", 0, 1, 0.25f, false, Kind::Cont, "pct", "Microcuts");
        add("mcSpan", "Reach back", 0.02f, 2, 0.5f, true, Kind::Cont, "time", "Microcuts");
        add("mcEdge", "Edge", 0, 1, 0.3f, false, Kind::Cont, "pct", "Microcuts");
        add("mcMode", "Mode", 0, 1, 0, false, Kind::Choice, "", "Microcuts", "Blend|Cut");
        add("mcLevel", "Level", 0, 1, 0.8f, false, Kind::Cont, "pct", "Microcuts");
        // ---- catch: the Nebulae engine (v2.0 build 2) ----
        add("ctMode", "Routing", 0, 3, 0, false, Kind::Choice, "", "Catch", "Bypass|Insert|Send|Catch");
        add("ctSend", "Send", 0, 1, 0.5f, false, Kind::Cont, "pct", "Catch");
        add("ctReturn", "Return", 0, 1.5f, 1, false, Kind::Cont, "pct", "Catch");
        add("ctSrc", "Records", 0, 1, 1, false, Kind::Choice, "", "Catch", "In|Mix (sound on sound)");
        add("ctCatchLen", "Catch length", 0.5f, 30, 8, true, Kind::Cont, "time", "Catch");
        add("ctFreeze", "Freeze", 0, 1, 0, false, Kind::Toggle, "", "Catch");
        add("ctExpo", "Expo window", 0, 1, 1, false, Kind::Toggle, "", "Catch");
        add("ctStart", "Start", 0, 1, 0, false, Kind::Cont, "pct", "Catch");
        add("ctSpeed", "Speed", 0, 1, 0.5f, false, Kind::Cont, "pct", "Catch");
        add("ctSize", "Size", 0, 1, 1, false, Kind::Cont, "pct", "Catch");
        add("ctDens", "Density", 0, 1, 0.55f, false, Kind::Cont, "pct", "Catch");
        add("ctPitch", "Pitch", 0, 1, 0.5f, false, Kind::Cont, "pct", "Catch");
        add("ctBlend", "Blend", 0, 1, 1, false, Kind::Cont, "pct", "Catch");
        add("ctDub", "Overdub level", 0, 1, 0.5f, false, Kind::Cont, "pct", "Catch: second page");
        add("ctSpray", "Spray", 0, 1, 0, false, Kind::Cont, "pct", "Catch: second page");
        add("ctOvl", "Overlap", 0, 1, 0.5f, false, Kind::Cont, "pct", "Catch: second page");
        add("ctOut", "Output level", 0, 1, 0, false, Kind::Cont, "pct", "Catch: second page");
        add("ctWinDec", "Window decay", 0, 1, 0, false, Kind::Cont, "pct", "Catch: second page");
        add("ctBpf", "Bandpass", 0, 1, 0.5f, false, Kind::Cont, "pct", "Catch: third page");
        add("ctBpq", "Bandpass Q", 0, 1, 0, false, Kind::Cont, "pct", "Catch: third page");
        add("ctWin", "Window", 0, 1, 0, false, Kind::Cont, "pct", "Catch: third page");
        add("ctFsMix", "Shift mix", 0, 1, 1, false, Kind::Cont, "pct", "Catch: third page");
        add("ctFshift", "Freq shift", 0, 1, 0.5f, false, Kind::Cont, "pct", "Catch: third page");
        add("ctPrand", "Pitch random", 0, 1, 0, false, Kind::Cont, "pct", "Catch: third page");
        add("ctL1Rate", "Rate", 0, 1, 0, false, Kind::Cont, "pct", "Catch: LFO 1");
        add("ctL1Skew", "Skew", 0, 1, 0.65f, false, Kind::Cont, "pct", "Catch: LFO 1");
        add("ctL1Read", "To read position", -1, 1, 0, false, Kind::Cont, "bi", "Catch: LFO 1");
        add("ctL1Size", "To loop length", -1, 1, 0, false, Kind::Cont, "bi", "Catch: LFO 1");
        add("ctL1Bpf", "To bandpass", -1, 1, 0, false, Kind::Cont, "bi", "Catch: LFO 1");
        add("ctL1Blend", "To blend", -1, 1, 0, false, Kind::Cont, "bi", "Catch: LFO 1");
        add("ctL2Rate", "Rate", 0, 1, 0, false, Kind::Cont, "pct", "Catch: LFO 2");
        add("ctL2Skew", "Skew", 0, 1, 0.5f, false, Kind::Cont, "pct", "Catch: LFO 2");
        add("ctL2Size", "To loop length", -1, 1, 0, false, Kind::Cont, "bi", "Catch: LFO 2");
        add("ctL2Bpf", "To bandpass", -1, 1, 0, false, Kind::Cont, "bi", "Catch: LFO 2");
        add("ctL2Blend", "To blend", -1, 1, 0, false, Kind::Cont, "bi", "Catch: LFO 2");
        add("ctL2Div", "Loop divisions", 0, 1, 0, false, Kind::Toggle, "", "Catch: LFO 2");
        // ---- bench (not part of a patch's sound) ----
        add("listen", "Listen", 0, 8, 0, false, Kind::Choice, "", "Bench", "Full output|Raw layers|Pre-filter|Voices|Vocoder wet|Feedback return|Space wet|Memory wet|Catch wet");
        add("tap", "Scope shows", 0, 2, 1, false, Kind::Choice, "", "Bench", "Output|Selected voice|Feedback loop");
        // the destination list is every articulation field plus every continuous parameter outside these groups
        int nd = 1 + NK;
        for (auto& p : v) { const std::string g = p.group; if (p.kind == Kind::Cont && g != "Bench" && g != "Mod matrix" && g != "Steps") ++nd; }
        for (auto& p : v) if (std::string(p.choices) == "@dst") p.max = float(nd - 1);
        return v;
    }();
    return table;
}
inline int numParams() { return (int) params().size(); }
inline const PDef* findParam(const std::string& id) {
    static const std::unordered_map<std::string, int> idx = [] { std::unordered_map<std::string, int> m; for (int i = 0; i < numParams(); ++i) m[params()[(size_t) i].id] = i; return m; }();
    auto it = idx.find(id); return it == idx.end() ? nullptr : &params()[(size_t) it->second];
}

struct SDef { const char* key; const char* label; float min, max; bool log; const char* fmt; };
inline const SDef kStateDefs[NK] = {
    { "level", "Level", 0, 1.3f, false, "pct" }, { "pitch", "Emission", -7, 2, false, "oct" }, { "formant", "Formant", -3, 3, false, "oct" },
    { "position", "Position", -1, 1, false, "bi" }, { "coupling", "Coupling", 0, 1, false, "pct" }, { "mask", "Masking", 0, 1, false, "pct" },
    { "scatter", "Scatter", 0, 1, false, "pct" }, { "fold", "Fold", 0, 1, false, "pct" }, { "cutoff", "Cutoff", 20, 20000, true, "hz" },
    { "resonator", "Resonator", 0, 1, false, "pct" }, { "ring", "Ring (AxB)", 0, 1, false, "pct" }, { "feedback", "Feedback send", 0, 1, false, "pct" },
};
inline constexpr const char* kStageNames[3] = { "Onset", "Continuant", "Termination" };
inline std::string stateId(int stage, int k) { return "s" + std::to_string(stage) + "_" + kStateKeys[k]; }

// Byte offset of every global parameter inside Params.
inline const std::unordered_map<std::string, size_t>& paramOffsets() {
    static const std::unordered_map<std::string, size_t> m = [] {
        std::unordered_map<std::string, size_t> o; Params d; const char* base = reinterpret_cast<const char*>(&d);
        auto reg = [&](const std::string& id, float* f) { o[id] = size_t(reinterpret_cast<const char*>(f) - base); };
        auto layer = [&](const std::string& p, LayerParams& L) {
            reg(p + "On", &L.on); reg(p + "Mat", &L.mat); reg(p + "Bank", &L.bank); reg(p + "Pos", &L.pos); reg(p + "Form", &L.form); reg(p + "Cyc", &L.cyc);
            reg(p + "Win", &L.win); reg(p + "Phase", &L.phase); reg(p + "N", &L.n); reg(p + "Det", &L.det); reg(p + "Wander", &L.wander); reg(p + "Lvl", &L.lvl); reg(p + "Pan", &L.pan); };
        layer("a", d.a); layer("b", d.b);
        for (int i = 0; i < 8; ++i) reg("band" + std::to_string(i), &d.bands[(size_t) i]);
        for (int b = 0; b < NBUS; ++b) { const std::string p = b == 0 ? "fb" : "f" + std::to_string(b + 1); BusParams& B = d.bus[b];
            reg(p + "On", &B.on); reg(p + "Tap", &B.tap); reg(p + "Send", &B.send); reg(p + "Time", &B.time); reg(p + "Type", &B.type); reg(p + "Cut", &B.cut);
            reg(p + "Q", &B.q); reg(p + "Ins", &B.ins); reg(p + "InsAmt", &B.insAmt); reg(p + "InsRate", &B.insRate); reg(p + "Drive", &B.drive);
            reg(p + "Regen", &B.regen); reg(p + "Edge", &B.edge); reg(p + "Mix", &B.mix); reg(p + "Dest", &B.dest); reg(p + "Width", &B.width); }
        for (int i = 0; i < NSLOT; ++i) { const std::string p = "m" + std::to_string(i); reg(p + "Src", &d.mod[i].src); reg(p + "Dst", &d.mod[i].dst); reg(p + "Amt", &d.mod[i].amt); }
        for (int h = 0; h < NHEAD; ++h) { const std::string p = "h" + std::to_string(h + 1); HeadParams& H = d.head[h];
            reg(p + "Shape", &H.shape); reg(p + "Depth", &H.depth); reg(p + "Cycle", &H.cycle); reg(p + "Branch", &H.branch); reg(p + "Rate", &H.rate); reg(p + "Start", &H.start); reg(p + "Trans", &H.trans); }
        for (int i = 0; i < NSTEP; ++i) { const std::string p = "st" + std::to_string(i); StepParams& S = d.step[i];
            reg(p + "On", &S.on); reg(p + "Note", &S.note); reg(p + "Vel", &S.vel); reg(p + "Prob", &S.prob); reg(p + "Len", &S.len); reg(p + "Val", &S.val);
            reg(p + "Cut", &S.cut); reg(p + "To1", &S.to1); reg(p + "To2", &S.to2); reg(p + "Br", &S.br); }
#define F(n) reg(#n, &d.n);
        F(poly) F(bSemi) F(bFine) F(bSync) F(pmAmt) F(formTrack) F(wanderRate) F(derez) F(drift) F(resTune) F(resDecay) F(strike)
        F(fMode) F(reso) F(drive) F(heat) F(keytrack) F(velCut) F(velAmp)
        F(atk) F(dec) F(rel) F(tail) F(fCut) F(curve) F(loop) F(loopTime) F(auth) F(stiff) F(visc) F(temp) F(mass) F(grainM) F(allure)
        F(vocOn) F(vocBody) F(vocSkin) F(vocShift) F(vocSpread) F(vocQ) F(vocResp) F(scrapeRate) F(scrapeDepth) F(vocMix) F(vocAuto)
        F(dustRate) F(dropRate) F(dropPitch) F(spSize) F(spDecay) F(spTone) F(spMix) F(vol) F(listen) F(tap) F(abTrim)
        F(clkRun) F(clkBpm) F(clkDiv) F(clkShape) F(clkThr) F(clkBRate) F(clkBShape) F(clkMode) F(clkDepth) F(clkAnchor)
        F(lfo1Rate) F(lfo1Shape) F(lfo2Rate) F(lfo2Shape) F(dustModRate) F(dustModDecay) F(seqOn) F(seqKeys) F(seqLen) F(seqHeads)
        F(sedSrc) F(sedAmt) F(sedDecay) F(sedFreeze) F(eroRate) F(eroTarget) F(eroDepth) F(eroRecover)
        F(rgOn) F(rgLen) F(rgIn) F(rgFb) F(rgTone) F(rgDegrade) F(rgStir) F(rgLevel) F(rgDrift)
        F(mcOn) F(mcTrig) F(mcProb) F(mcSize) F(mcRepeats) F(mcRev) F(mcSpan) F(mcEdge) F(mcMode) F(mcLevel)
        F(ctMode) F(ctSend) F(ctSrc) F(ctFreeze) F(ctExpo) F(ctL2Div) F(ctCatchLen) F(ctReturn)
        F(ctStart) F(ctSpeed) F(ctSize) F(ctDens) F(ctPitch) F(ctBlend) F(ctDub) F(ctSpray) F(ctOvl) F(ctOut) F(ctWinDec)
        F(ctBpf) F(ctBpq) F(ctWin) F(ctFsMix) F(ctFshift) F(ctPrand)
        F(ctL1Rate) F(ctL1Skew) F(ctL1Read) F(ctL1Size) F(ctL1Bpf) F(ctL1Blend) F(ctL2Rate) F(ctL2Skew) F(ctL2Size) F(ctL2Bpf) F(ctL2Blend)
#undef F
        return o;
    }();
    return m;
}
inline float* paramField(Params& P, const std::string& id) {
    auto& o = paramOffsets(); auto it = o.find(id);
    return it == o.end() ? nullptr : reinterpret_cast<float*>(reinterpret_cast<char*>(&P) + it->second);
}
inline Params defaultParams() {
    Params P;
    for (auto& d : params()) if (float* f = paramField(P, d.id)) *f = d.def;
    return P;
}

// Modulation destinations: every articulation field (added to the voice's state), then every
// continuous parameter except the bench, the matrix itself and the step data.
struct ModDest { std::string id, label; int stateKey = -1; size_t offset = 0; float min = 0, max = 1; bool log = false; };
inline const std::vector<ModDest>& modDests() {
    static const std::vector<ModDest> v = [] {
        std::vector<ModDest> d; d.push_back({ "", "None" });
        for (int k = 0; k < NK; ++k) { ModDest m; m.id = std::string("state:") + kStateKeys[k]; m.label = std::string("Articulation: ") + kStateDefs[k].label;
            m.stateKey = k; m.min = kStateDefs[k].min; m.max = kStateDefs[k].max; m.log = kStateDefs[k].log; d.push_back(m); }
        for (auto& p : params()) {
            const std::string id = p.id, g = p.group;
            if (p.kind != Kind::Cont || g == "Bench" || g == "Mod matrix" || g == "Steps") continue;
            ModDest m; m.id = id; m.label = g + ": " + p.label; m.offset = paramOffsets().at(id); m.min = p.min; m.max = p.max; m.log = p.log; d.push_back(m);
        }
        return d;
    }();
    return v;
}
// The items of a choice parameter, with the generated lists resolved.
inline std::vector<std::string> choiceItems(const PDef& d) {
    std::vector<std::string> out; const std::string c = d.choices;
    if (c == "@src") return modSourceNames();
    if (c == "@dst") { for (auto& m : modDests()) out.push_back(m.label); return out; }
    if (c == "@to1" || c == "@to2") { out.push_back(c == "@to1" ? "Next step" : "None"); for (int i = 1; i <= NSTEP; ++i) out.push_back("Step " + std::to_string(i)); return out; }
    size_t a = 0; while (true) { size_t b = c.find('|', a); out.push_back(c.substr(a, b == std::string::npos ? std::string::npos : b - a)); if (b == std::string::npos) break; a = b + 1; }
    return out;
}

inline int modDestIndex(const std::string& id) { auto& d = modDests(); for (size_t i = 0; i < d.size(); ++i) if (d[i].id == id) return (int) i; return 0; }

} // namespace silt
