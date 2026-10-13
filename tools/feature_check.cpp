// Exercises the newer sections (feedback network, memory, movement, mod matrix, catch) and fuzzes
// every parameter, checking for silence where there should be sound, runaway levels and NaN.
#include "../Source/Engine/SiltEngine.h"
#include <cstdio>
#include <chrono>
#include <random>
#include <functional>
using namespace silt;

struct Res { double rms, peak; bool nan; double cpu; };
static Res run(Engine& e, Params& P, StateSet& S, double secs, std::function<void(Engine&, double)> events) {
    const int N = int(secs * 48000 / 128); float L[128], R[128]; double ss = 0, pk = 0; bool nan = false;
    auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < N; ++b) {
        e.setControls(P, S); events(e, b * 128 / 48000.0);
        e.process(P, S, L, R, nullptr, 128);
        for (int i = 0; i < 128; ++i) { if (!(L[i] == L[i]) || !(R[i] == R[i])) nan = true; ss += L[i] * L[i]; pk = std::max(pk, double(std::abs(L[i]))); }
    }
    const double cpu = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / secs;
    return { 10 * std::log10(ss / (N * 128) + 1e-20), pk, nan, cpu * 100 };
}
static StateSet initS() { StateSet S; S[0] = { 1, 0, 0, 0, 0, 0, 0, 0, 6000, 0, 0, 0.3f }; S[1] = { 0.8f, 0, 0, 0, 0, 0, 0, 0, 2500, 0, 0, 0.3f }; S[2] = { 0, 0, 0, 0, 0, 0, 0, 0, 1200, 0, 0, 0 }; return S; }
static void report(const char* name, Res r) { std::printf("%-44s rms %6.1f dB  peak %.2f  nan %d  cpu %.1f%%\n", name, r.rms, r.peak, r.nan, r.cpu); }
static auto chord = [](Engine& e, double t) { static double last = -1; if (t < last) last = -1; if (last < 0.05 && t >= 0.05) for (int n : { 48, 55, 60 }) e.keyOn(n, 0.9f); if (last < 2.5 && t >= 2.5) for (int n : { 48, 55, 60 }) e.keyOff(n); last = t; };

int main() {
    Engine e; e.prepare(48000);
    StateSet S = initS();
    { Params P = defaultParams(); report("baseline chord", run(e, P, S, 3, chord)); }
    { Engine e2; e2.prepare(48000); Params P = defaultParams(); P.seqOn = 1; P.clkBpm = 120;
      for (int i = 0; i < 16; ++i) { P.step[i].note = float((i * 5) % 12); P.step[i].len = 0.8f; }
      P.step[3].to2 = 9; P.step[3].br = 0.5f; P.head[0].shape = 3; P.head[0].depth = 1.5f;
      report("sequencer alone (no keys), arch traversal", run(e2, P, S, 4, [](Engine&, double) {})); }
    { Engine e2; e2.prepare(48000); Params P = defaultParams(); P.seqOn = 1; P.seqHeads = 2; P.clkMode = 4; P.clkDepth = 1; P.clkBRate = 3;
      report("two playheads, clock AND logic", run(e2, P, S, 4, [](Engine&, double) {})); }
    for (int ins = 0; ins <= 5; ++ins) {
        Engine e2; e2.prepare(48000); Params P = defaultParams();
        P.bus[0].mix = 0.5f; P.bus[0].regen = 1.1f; P.bus[0].ins = float(ins); P.bus[0].insAmt = 0.8f; P.bus[0].edge = 1;
        P.bus[1].on = 1; P.bus[1].tap = 6; P.bus[1].ins = float((ins + 2) % 6); P.bus[1].regen = 1.0f; P.bus[1].dest = 2;
        P.bus[2].on = 1; P.bus[2].tap = 9; P.bus[2].regen = 0.9f; P.bus[2].dest = 1; P.bus[2].mix = 0.7f;
        char nm[64]; std::snprintf(nm, 64, "feedback network, insert %d, regen 1.1 + edge", ins);
        report(nm, run(e2, P, S, 4, chord));
    }
    { Engine e2; e2.prepare(48000); Params P = defaultParams(); P.rgOn = 1; P.rgFb = 1.0f; P.rgLen = 1; P.mcOn = 1; P.mcTrig = 3; P.mcProb = 1; P.mcMode = 1;
      P.sedSrc = 1; P.sedAmt = 1; P.eroRate = 1; P.eroTarget = 0;
      Res r = run(e2, P, S, 6, chord); report("regeneration + microcuts + sediment/erosion", r);
      std::printf("   sediment %.2f erosion %.2f\n", e2.viz.sediment, e2.viz.erosion); }
    { Engine e2; e2.prepare(48000); Params P = defaultParams(); P.mod[0] = { float(MS_LFO1), float(modDestIndex("fCut")), 0.5f };
      P.mod[1] = { float(MS_DUST), float(modDestIndex("state:cutoff")), 0.3f }; P.mod[2] = { float(MS_VRAND), float(modDestIndex("aPos")), 1 };
      P.mod[3] = { float(MS_CLKA), float(modDestIndex("vocShift")), 0.5f }; P.lfo1Rate = 3;
      report("mod matrix: LFO>cutoff, dust>state cutoff, ...", run(e2, P, S, 3, chord)); }
    { Engine e2; e2.prepare(48000); Params P = defaultParams(); P.ctMode = 1; P.ctBlend = 0.8f;
      report("catch insert, record 1.0-2.5 s then loop", run(e2, P, S, 6, [&](Engine& en, double t) { chord(en, t);
          static int st = 0; if (t < 0.01) st = 0; if (st == 0 && t >= 1.0) { en.catchRecord(); st = 1; } if (st == 1 && t >= 2.5) { en.catchRecord(); st = 2; } }));
      std::printf("   loop %.2f s\n", e2.viz.catchLoop); }
    { Engine e2; e2.prepare(48000); Params P = defaultParams(); P.ctMode = 3; P.ctCatchLen = 2; P.ctPitch = 0.7f; P.ctBlend = 0.2f;
      Res r = run(e2, P, S, 6, [&](Engine& en, double t) { chord(en, t); static int st = 0; if (t < 0.01) st = 0; if (st == 0 && t >= 2.0) { en.catchNow(); st = 1; } });
      report("catch mode, grab at 2 s, plays after chord ends", r); std::printf("   loop %.2f s\n", e2.viz.catchLoop); }
    // fuzz
    std::mt19937 rng(7); int bad = 0; double worst = 0;
    for (int it = 0; it < 40; ++it) {
        Engine e2; e2.prepare(48000); Params P = defaultParams(); StateSet S2;
        for (auto& d : params()) { if (std::string(d.group) == "Bench") continue; float* f = paramField(P, d.id);
            std::uniform_real_distribution<float> u(0, 1); float x = u(rng);
            *f = d.log ? d.min * std::pow(d.max / d.min, x) : d.min + (d.max - d.min) * x; if (d.kind != Kind::Cont) *f = std::round(*f);
            if (std::string(d.choices) == "@dst") *f = float(int(x * (modDests().size() - 1))); }
        P.vol = 0.9f; P.poly = 6;
        for (int s = 0; s < 3; ++s) for (int k = 0; k < NK; ++k) { std::uniform_real_distribution<float> u(kStateDefs[k].min, kStateDefs[k].max); S2[s][k] = u(rng); }
        S2[2][K_CUT] = 500;
        Res r = run(e2, P, S2, 2.5, [](Engine& en, double t) { static double last = -1; if (t < last) last = -1;
            if (last < 0.02 && t >= 0.02) for (int n : { 36, 60, 67, 84 }) en.keyOn(n, 1); if (last < 1.5 && t >= 1.5) for (int n : { 36, 60, 67, 84 }) en.keyOff(n); last = t; });
        worst = std::max(worst, r.cpu);
        if (r.nan || r.peak > 1.0) { ++bad; std::printf("fuzz %d: nan %d peak %.2f\n", it, r.nan, r.peak); }
    }
    std::printf("fuzz: 40 random patches, %d bad, worst cpu %.1f%%\n", bad, worst);
}
