// Silt engine parameters. Plain C++ so the same engine builds for the app, a plugin, and the hardware.
#pragma once
#include <array>

namespace silt {

// Order and meaning match the browser lab (patches.js). Cutoff is stored in Hz.
enum StateKey { K_LEVEL, K_PITCH, K_FORM, K_POS, K_COUP, K_MASK, K_SCAT, K_FOLD, K_CUT, K_RES, K_RING, K_FB, NK };
inline constexpr const char* kStateKeys[NK] = { "level", "pitch", "formant", "position", "coupling", "mask", "scatter", "fold", "cutoff", "resonator", "ring", "feedback" };
using State = std::array<float, NK>;
using StateSet = std::array<State, 3>; // onset, continuant, termination

struct LayerParams {
    float on = 1, mat = 0, bank = 0, pos = 3.f / 7.f, form = 1, cyc = 1, win = 0, phase = 0, n = 1, det = 6, wander = 0, lvl = 0.8f, pan = 0.5f;
};

struct Params {
    float poly = 6;
    LayerParams a, b;
    float bSemi = 0, bFine = 0, bSync = 0;
    float pmAmt = 0, formTrack = 1, wanderRate = 0.3f;
    float derez = 0, drift = 0.25f, resTune = 1, resDecay = 0.5f, strike = 0.3f;
    float fMode = 0, reso = 0.2f, drive = 0.2f, heat = 0.25f, keytrack = 0.5f, velCut = 0.3f, velAmp = 0.5f;
    float atk = 0.005f, dec = 0.6f, rel = 0.5f, curve = 0.5f, loop = 0, loopTime = 1.5f, auth = 0, stiff = 6, visc = 0.8f, temp = 0;
    float mass = 0, grainM = 0, allure = 0;
    float vocOn = 0, vocBody = 1, vocSkin = 0;
    std::array<float, 8> bands { .8f, .8f, .8f, .8f, .8f, .8f, .8f, .8f };
    float vocShift = 0, vocSpread = 1, vocQ = 5, vocResp = 0.2f, scrapeRate = 1, scrapeDepth = 0, vocMix = 0.5f, vocAuto = 1;
    float dustRate = 400, dropRate = 60, dropPitch = 1;
    float fbTime = 240, fbType = 1, fbCut = 1500, fbQ = 2, fbDrive = 1.2f, fbRegen = 0.3f, fbMix = 0;
    float spSize = 1, spDecay = 2.5f, spTone = 0.5f, spMix = 0.15f;
    float vol = 0.9f, listen = 0, tap = 1, abTrim = 1;
};

} // namespace silt
