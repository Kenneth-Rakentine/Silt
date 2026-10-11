// The full parameter list, shared by the app and tests. IDs match the browser lab's patch keys,
// so patch JSON moves between the lab and the app unchanged.
#pragma once
#include "SiltParams.h"
#include <cstring>
#include <string>

namespace silt {

enum class Kind { Cont, Int, Toggle, Choice };

struct PDef {
    const char* id; const char* label; float min, max, def; bool log; Kind kind; const char* fmt; const char* group; const char* choices;
};

// fmt keys: pct hz time ms oct x n num bi cents semi per ratio win pos
inline const PDef kParams[] = {
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
    { "reso", "Resonance", 0, 1, 0.2f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    { "drive", "Drive", 0, 1, 0.2f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    { "heat", "VCA heat", 0, 1, 0.25f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    { "keytrack", "Key tracking", 0, 1, 0.5f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    { "velCut", "Velocity > cutoff", 0, 1, 0.3f, false, Kind::Cont, "pct", "Filter and VCA", "" },
    // Macros
    { "mass", "Mass", -1, 1, 0, false, Kind::Cont, "bi", "Macros", "" },
    { "grainM", "Grain", -1, 1, 0, false, Kind::Cont, "bi", "Macros", "" },
    { "allure", "Allure", 0, 1, 0, false, Kind::Cont, "pct", "Macros", "" },
    { "vol", "Volume", 0, 2.5f, 0.9f, false, Kind::Cont, "pct", "Macros", "" },
    // Articulation
    { "atk", "Onset time", 0.001f, 8, 0.005f, true, Kind::Cont, "time", "Articulation", "" },
    { "dec", "To continuant", 0.001f, 8, 0.6f, true, Kind::Cont, "time", "Articulation", "" },
    { "rel", "Release", 0.001f, 8, 0.5f, true, Kind::Cont, "time", "Articulation", "" },
    { "curve", "Curve", -1, 1, 0.5f, false, Kind::Cont, "bi", "Articulation", "" },
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
    // Feedback
    { "fbType", "Filter", 0, 2, 1, false, Kind::Choice, "", "Feedback", "Low pass|Band pass|High pass" },
    { "fbTime", "Delay time", 1, 1500, 240, true, Kind::Cont, "ms", "Feedback", "" },
    { "fbCut", "Cutoff", 40, 12000, 1500, true, Kind::Cont, "hz", "Feedback", "" },
    { "fbQ", "Resonance", 0.5f, 20, 2, true, Kind::Cont, "num", "Feedback", "" },
    { "fbDrive", "Drive", 0.3f, 8, 1.2f, true, Kind::Cont, "num", "Feedback", "" },
    { "fbRegen", "Regeneration", 0, 1.1f, 0.3f, false, Kind::Cont, "pct", "Feedback", "" },
    { "fbMix", "Return level", 0, 1, 0, false, Kind::Cont, "pct", "Feedback", "" },
    // Space
    { "spSize", "Size", 0.35f, 2, 1, false, Kind::Cont, "x", "Space", "" },
    { "spDecay", "Decay", 0.2f, 20, 2.5f, true, Kind::Cont, "time", "Space", "" },
    { "spTone", "Tone", 0, 1, 0.5f, false, Kind::Cont, "pct", "Space", "" },
    { "spMix", "Mix", 0, 1, 0.15f, false, Kind::Cont, "pct", "Space", "" },
    // Bench (not part of a patch's sound)
    { "listen", "Listen", 0, 6, 0, false, Kind::Choice, "", "Bench", "Full output|Raw layers|Pre-filter|Voices|Vocoder wet|Feedback return|Space wet" },
    { "tap", "Scope shows", 0, 2, 1, false, Kind::Choice, "", "Bench", "Output|Selected voice|Feedback loop" },
};
static constexpr int kNumGlobal = int(sizeof(kParams) / sizeof(kParams[0]));

struct SDef { const char* key; const char* label; float min, max; bool log; const char* fmt; };
inline const SDef kStateDefs[NK] = {
    { "level", "Level", 0, 1.3f, false, "pct" }, { "pitch", "Emission", -7, 2, false, "oct" }, { "formant", "Formant", -3, 3, false, "oct" },
    { "position", "Position", -1, 1, false, "bi" }, { "coupling", "Coupling", 0, 1, false, "pct" }, { "mask", "Masking", 0, 1, false, "pct" },
    { "scatter", "Scatter", 0, 1, false, "pct" }, { "fold", "Fold", 0, 1, false, "pct" }, { "cutoff", "Cutoff", 20, 20000, true, "hz" },
    { "resonator", "Resonator", 0, 1, false, "pct" }, { "ring", "Ring (AxB)", 0, 1, false, "pct" }, { "feedback", "Feedback send", 0, 1, false, "pct" },
};
inline constexpr const char* kStageNames[3] = { "Onset", "Continuant", "Termination" };
inline std::string stateId(int stage, int k) { return "s" + std::to_string(stage) + "_" + kStateKeys[k]; }

// Where each global parameter lives in the engine's Params struct.
inline float* paramField(Params& P, const char* id) {
    auto layer = [](LayerParams& L, const char* f) -> float* {
        struct M { const char* n; float LayerParams::*m; };
        static const M map[] = { { "On", &LayerParams::on }, { "Mat", &LayerParams::mat }, { "Bank", &LayerParams::bank }, { "Pos", &LayerParams::pos },
            { "Form", &LayerParams::form }, { "Cyc", &LayerParams::cyc }, { "Win", &LayerParams::win }, { "Phase", &LayerParams::phase }, { "N", &LayerParams::n },
            { "Det", &LayerParams::det }, { "Wander", &LayerParams::wander }, { "Lvl", &LayerParams::lvl }, { "Pan", &LayerParams::pan } };
        for (auto& x : map) if (!std::strcmp(f, x.n)) return &(L.*(x.m));
        return nullptr; };
    if (id[0] == 'a' && id[1] >= 'A' && id[1] <= 'Z') return layer(P.a, id + 1);
    if (id[0] == 'b' && id[1] >= 'A' && id[1] <= 'Z') { if (!std::strcmp(id, "bSemi")) return &P.bSemi; if (!std::strcmp(id, "bFine")) return &P.bFine; if (!std::strcmp(id, "bSync")) return &P.bSync; return layer(P.b, id + 1); }
    if (!std::strncmp(id, "band", 4)) return &P.bands[size_t(id[4] - '0')];
#define F(n) if (!std::strcmp(id, #n)) return &P.n;
    F(poly) F(pmAmt) F(formTrack) F(wanderRate) F(derez) F(drift) F(resTune) F(resDecay) F(strike)
    F(fMode) F(reso) F(drive) F(heat) F(keytrack) F(velCut) F(velAmp)
    F(atk) F(dec) F(rel) F(curve) F(loop) F(loopTime) F(auth) F(stiff) F(visc) F(temp) F(mass) F(grainM) F(allure)
    F(vocOn) F(vocBody) F(vocSkin) F(vocShift) F(vocSpread) F(vocQ) F(vocResp) F(scrapeRate) F(scrapeDepth) F(vocMix) F(vocAuto)
    F(dustRate) F(dropRate) F(dropPitch) F(fbTime) F(fbType) F(fbCut) F(fbQ) F(fbDrive) F(fbRegen) F(fbMix)
    F(spSize) F(spDecay) F(spTone) F(spMix) F(vol) F(listen) F(tap) F(abTrim)
#undef F
    return nullptr;
}

} // namespace silt
