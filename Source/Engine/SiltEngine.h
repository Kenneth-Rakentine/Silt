// Silt sound engine (C++ port of the browser lab engine). No framework dependencies.
#pragma once
#include "SiltParams.h"
#include "ParamTable.h"
#include "Movement.h"
#include "Feedback.h"
#include "Memory.h"
#include "../Catch/CatchSection.h"
#include <vector>
#include <cstdint>
#include <cmath>

namespace silt {

constexpr int MAXV = 6, MAXE = 6, TL = 2048, NMIP = 11, NFR = 8, NBANK = 6;
constexpr int FS = TL + 1, MIPSZ = FS, FRSZ = FS * NMIP, BANKSZ = FRSZ * NFR;
constexpr int NWS = 33, WL = 512, CTRL = 16, CHUNK = 128;

struct Rng { uint32_t s = 0x9E3779B9u;
    inline float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s >> 8) * (1.0f / 16777216.0f); }
    inline float gauss() { return (next() + next() + next() - 1.5f) * 1.4142f; } };

struct SVF {
    double a = 0, b = 0, k = 1, a1 = 1, a2 = 0, a3 = 0, lp = 0, bp = 0, hp = 0;
    void set(double f, double q, double sr);
    inline double tick(double x) { double v3 = x - b, v1 = a1 * a + a2 * v3, v2 = b + a2 * a + a3 * v3;
        a = 2 * v1 - a; b = 2 * v2 - b; lp = v2; bp = v1; hp = x - k * v1 - v2; return v2; }
    void reset() { a = b = 0; }
};

struct Up { double x[16] {}; int i = 0;
    void push(double v) { i = (i + 15) & 15; x[i] = v; }
    double even() const; double odd() const { return x[(i + 7) & 15]; } };
struct Down { double e[16] {}, o[16] {}; int i = 0;
    void push(double ev, double od) { i = (i + 15) & 15; e[i] = ev; o[i] = od; }
    double out() const; };

struct Layer {
    double th[MAXE] {}, ci[MAXE] {}, wd[MAXE] {}, inc[MAXE] {}, pl[MAXE] {}, pr[MAXE] {};
    uint8_t cur[MAXE] {};
    static constexpr int S = MAXE * 2;
    uint8_t on[S] {}, rl[S] {};
    double rd[S] {}, el[S] {}, amp[S] {}, fi[S] {}, rg[S] {}, fm[S] {}, n0[S] {}, n1[S] {}, ni[S] {};
    int N = 1, mat = 0, rc = 0; bool enabled = false, contin = true;
    double ff = 0.01, C = 1, fade = 0.05, ph0 = 0, K = 0, R = 0, psi = 0, lvl = 0, outL = 0, outR = 0;
    int ws = 0, o00 = 0, o10 = 0, o01 = 0, o11 = 0; double w00 = 1, w10 = 0, w01 = 0, w11 = 0;
    const float* tb = nullptr;
    void reset(Rng& r) { for (auto& x : on) x = 0; for (int i = 0; i < MAXE; ++i) { th[i] = r.next(); wd[i] = 0; cur[i] = 0; } }
};

struct Voice {
    bool active = false, gate = false; uint32_t serial = 0; int note = 60; double vel = 1, freq = 261.6;
    Layer A, B;
    double p[NK] {}, s[NK] {}, v[NK] {}, n[NK] {}, cur[NK] {}, from[NK] {};
    int toIdx = 0, stage = 0; double T = 1, t = 0, tail = 1;
    double ry1[3] {}, ry2[3] {}, rb1[3] {}, rb2[3] {}, rg[3] {}, strike = 0;
    Up upL, upR; Down dnL, dnR; double lad[2][4] {}; SVF svf[2];
    double g0 = 0.1, g1 = 0.1, lv0 = 0, lv1 = 0, hL = 0, hR = 0, bPrev = 0; int hc = 0;
    double dP = 0, dC = 0, dPv = 0, dCv = 0, fold = 0, res = 0, ring = 0, fbs = 0;
    float vrand = 0;                 // per-note random, a mod source
    Params pm;                       // this voice's parameters after modulation
    double stOff[NK] {};             // modulation added to the articulation fields
};

struct Drop { bool on = false; double ph = 0, f0 = 0, ch = 0, t = 0, tc = 0, td = 0, a = 0, pl = 0, pr = 0; };
struct Line { std::vector<float> buf; int w = 0, len = 0; double base = 0, lp = 0, g = 0; };
struct AP { std::vector<float> b; int i = 0; };

// Snapshot for displays (read by the UI thread; tearing is harmless).
struct VizSnapshot { int seqStep[NHEAD] { -1, -1 }; float clockEnv = 0, sediment = 0, erosion = 0, catchLevel = 0, catchLoop = 0; bool catchRec = false;
    float src[MS_COUNT] {}; float prog = 0; int note = -1; bool on = false; int N = 1; float th[MAXE] {}; float e[MAXE] {}; float R = 0; int stage = 0; float f0 = 0, ff = 0; float cur[NK] {}; int voices = 0; };

class Engine {
public:
    Engine();
    void prepare(double sampleRate);
    void noteOn(int note, float vel);
    void noteOff(int note);
    // Keys from the player (MIDI or computer keys): the sequencer may take them as transposition.
    void keyOn(int note, float vel);
    void keyOff(int note);
    void setModWheel(float v) { wheel = v; }
    void panic();
    // actions from the panel
    void catchNow() { if (P) catchSec.catchNow(*P); }
    void catchRecord() { catchSec.toggleRecord(); }
    void catchClear() { catchSec.clear(); }
    void sedimentClear() { sed.clear(); }
    void erosionRestore() { sed.restore(); }
    // Sets parameters and states without rendering (call before note events in a block).
    void setControls(const Params& P, const StateSet& S);
    // Renders n samples. tap (optional) receives the scope signal.
    void process(const Params& P, const StateSet& S, float* L, float* R, float* tap, int n);
    VizSnapshot viz;
    float meterRms = 0; int activeVoices = 0;
private:
    void renderChunk(int n, float* L, float* R, float* tap);
    void applyMods(Params& out, const float* src, double* stateOff) const;
    bool anyMods() const;
    void ctrlLayer(Voice& v, Layer& L, const LayerParams& lp, double f0, double dt);
    void ctrlVoice(Voice& v, double dt);
    void layerSample(Layer& L, double pm, bool syncIn, bool& emitted0);
    void renderVoice(Voice& v, int off, int m);
    void panicDSP();
    void buildTables();

    double sr = 48000;
    Rng rng;
    std::vector<std::vector<float>> T; // banks
    Voice voices[MAXV]; Voice* lastV = nullptr; uint32_t serial = 0;
    const Params* P = nullptr; double Sc[3][NK] {}; // states in engine units (cutoff as log2 Hz)
    float bL[CHUNK] {}, bR[CHUNK] {}, fbB[CHUNK] {}, t1L[CHUNK] {}, t1R[CHUNK] {}, t2L[CHUNK] {}, t2R[CHUNK] {}, vt[CHUNK] {};
    double mask_ = 0, scat_ = 0;
    const Params* base = nullptr;   // unmodulated parameters for this block
    Params Pg;                      // globally modulated parameters (feedback, memory, vocoder, space, catch)
    float src[MS_COUNT] {};         // global modulation source values
    // movement
    Clock clock; ModLfo lfo1, lfo2; DustMod dmod; Sequencer seq; MRng mrng;
    int root = 48, notesThisChunk = 0; float wheel = 0;
    // feedback network
    FbBus bus[NBUS]; double busPrev[NBUS] {}; float excCur[CHUNK] {}, excNext[CHUNK] {};
    double prevWet = 0, prevSpace = 0, prevOut = 0;
    // memory
    Sediment sed; RegenBuffer regen; Microcuts cuts; double outEnergy = 0;
    // catch
    CatchSection catchSec;
    // per-chunk listen buffers
    float cL[CHUNK] {}, cR[CHUNK] {}, lsWL[CHUNK] {}, lsWR[CHUNK] {}, lsFL[CHUNK] {}, lsFR[CHUNK] {}, lsSL[CHUNK] {}, lsSR[CHUNK] {},
          lsML[CHUNK] {}, lsMR[CHUNK] {}, lsCL[CHUNK] {}, lsCR[CHUNK] {}, lsDC[CHUNK] {}, ctL[CHUNK] {}, ctR[CHUNK] {};
    // vocoder
    SVF anS[8], syL[8], syR[8]; double env[8] {}; double scr = 0, vDry = 1e-4, vWet = 1e-4, vGain = 1;
    SVF dustL[2], dustR[2], popL, popR; Drop drops[14];
    // space
    Line fdn[8]; AP apL[4], apR[4]; double modPh = 0;
    // output
    double agc = 1, agcR = 1e-4, outR = 0, dcL0 = 0, dcL1 = 0, dcR0 = 0, dcR1 = 0; int vizC = 0;
};

} // namespace silt
