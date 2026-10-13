// Silt engine parameters. Plain C++ so the same engine builds for the app, a plugin, and the hardware.
// Defaults live in ParamTable.h (defaultParams()); the values here only matter for fields the table
// does not list.
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

constexpr int NBUS = 3, NSTEP = 16, NHEAD = 2, NSLOT = 12;

// One feedback bus: delay -> filter -> insert -> drive, with a tap at the input and a return at the output.
struct BusParams {
    float on = 0, tap = 0, send = 1, time = 240, type = 1, cut = 1500, q = 2, ins = 0, insAmt = 0.5f, insRate = 0.5f,
          drive = 1.2f, regen = 0.3f, edge = 0, mix = 0, dest = 0, width = 0.5f;
};
struct StepParams { float on = 1, note = 0, vel = 0.8f, prob = 1, len = 0.5f, val = 0, cut = 0, to1 = 0, to2 = 0, br = 0.5f; };
struct HeadParams { float shape = 0, depth = 0, cycle = 16, branch = 0, rate = 1, start = 1, trans = 0; };
struct ModSlot { float src = 0, dst = 0, amt = 0; };

struct Params {
    float poly = 6;
    LayerParams a, b;
    float bSemi = 0, bFine = 0, bSync = 0;
    float pmAmt = 0, formTrack = 1, wanderRate = 0.3f;
    float derez = 0, drift = 0.25f, resTune = 1, resDecay = 0.5f, strike = 0.3f;
    float fMode = 0, reso = 0.2f, drive = 0.2f, heat = 0.25f, keytrack = 0.5f, velCut = 0.3f, velAmp = 0.5f;
    float atk = 0.005f, dec = 0.6f, rel = 0.5f, curve = 0.5f, loop = 0, loopTime = 1.5f, auth = 0, stiff = 6, visc = 0.8f, temp = 0;
    float mass = 0, grainM = 0, allure = 0;   // allure is labelled Flux
    float fCut = 0, tail = 0;                // cutoff offset (octaves) and tail time (0 = automatic)
    float vocOn = 0, vocBody = 1, vocSkin = 0;
    std::array<float, 8> bands { .8f, .8f, .8f, .8f, .8f, .8f, .8f, .8f };
    float vocShift = 0, vocSpread = 1, vocQ = 5, vocResp = 0.2f, scrapeRate = 1, scrapeDepth = 0, vocMix = 0.5f, vocAuto = 1;
    float dustRate = 400, dropRate = 60, dropPitch = 1;
    BusParams bus[NBUS];
    float spSize = 1, spDecay = 2.5f, spTone = 0.5f, spMix = 0.15f;
    float vol = 0.9f, listen = 0, tap = 1, abTrim = 1;

    // ---- movement ----
    float clkRun = 1, clkBpm = 110, clkDiv = 2, clkShape = 0, clkThr = 0, clkBRate = 0.25f, clkBShape = 1, clkMode = 0, clkDepth = 0, clkAnchor = 0;
    float lfo1Rate = 0.5f, lfo1Shape = 0, lfo2Rate = 0.13f, lfo2Shape = 1, dustModRate = 4, dustModDecay = 0.15f;
    ModSlot mod[NSLOT];
    float seqOn = 0, seqKeys = 0, seqLen = 16, seqHeads = 1;
    HeadParams head[NHEAD];
    StepParams step[NSTEP];

    // ---- memory ----
    float sedSrc = 0, sedAmt = 0.5f, sedDecay = 10, sedFreeze = 0;
    float eroRate = 0, eroTarget = 0, eroDepth = 0.5f, eroRecover = 30;
    float rgOn = 0, rgLen = 4, rgIn = 0.5f, rgFb = 0.7f, rgTone = 3000, rgDegrade = 0.3f, rgStir = 0.5f, rgLevel = 0.5f, rgDrift = 0.2f;
    float mcOn = 0, mcTrig = 0, mcProb = 0.25f, mcSize = 60, mcRepeats = 4, mcRev = 0.25f, mcSpan = 0.5f, mcEdge = 0.3f, mcMode = 0, mcLevel = 0.8f;

    // ---- catch (Nebulae) ----
    float ctMode = 0, ctSend = 0.5f, ctSrc = 1, ctFreeze = 0, ctExpo = 1, ctL2Div = 0, ctCatchLen = 8, ctReturn = 1;
    float ctStart = 0, ctSpeed = 0.5f, ctSize = 1, ctDens = 0.55f, ctPitch = 0.5f, ctBlend = 1;
    float ctDub = 0.5f, ctSpray = 0, ctOvl = 0.5f, ctOut = 0, ctWinDec = 0;
    float ctBpf = 0.5f, ctBpq = 0, ctWin = 0, ctFsMix = 1, ctFshift = 0.5f, ctPrand = 0;
    float ctL1Rate = 0, ctL1Skew = 0.65f, ctL1Read = 0, ctL1Size = 0, ctL1Bpf = 0, ctL1Blend = 0;
    float ctL2Rate = 0, ctL2Skew = 0.5f, ctL2Size = 0, ctL2Bpf = 0, ctL2Blend = 0;
};

} // namespace silt
