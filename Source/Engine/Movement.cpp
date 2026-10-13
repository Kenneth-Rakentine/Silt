#include "Movement.h"
#include <cmath>
#include <algorithm>

namespace silt {

static constexpr double PI = 3.14159265358979323846, TAU = 2 * PI;

// Shape morph: 0 square, 0.25 skewed pulse, 0.5 saw, 0.75 triangle, 1 sine. All bipolar.
float Clock::wave(double ph, double s) {
    ph -= std::floor(ph);
    auto sq = [&](double duty) { return ph < duty ? 1.0 : -1.0; };
    const double saw = 2 * ph - 1, tri = ph < 0.5 ? 4 * ph - 1 : 3 - 4 * ph, sine = -std::cos(TAU * ph);
    s = std::clamp(s, 0.0, 1.0);
    double w;
    if (s < 0.25) w = sq(0.5 + 0.2 * (s / 0.25));
    else if (s < 0.5) { const double t = (s - 0.25) / 0.25; w = sq(0.7) * (1 - t) + saw * t; }
    else if (s < 0.75) { const double t = (s - 0.5) / 0.25; w = saw * (1 - t) + tri * t; }
    else { const double t = (s - 0.75) / 0.25; w = tri * (1 - t) + sine * t; }
    return float(w);
}

int Clock::tick(const Params& P, int n, double sr, MRng& rng) {
    static const double ppb[4] = { 1, 2, 4, 8 };
    const double fA0 = P.clkBpm / 60.0 * ppb[std::clamp(int(P.clkDiv), 0, 3)] / 2.0;
    const int mode = int(P.clkMode); const double depth = P.clkDepth, thr = P.clkThr;
    const bool locked = P.clkAnchor < 0.5f;
    if (!locked) { drift *= std::exp(rng.bi() * std::sqrt(n / sr) * 0.6 * depth); drift = std::clamp(drift, 0.7, 1.4); }
    else drift = 1;
    int fired = 0;
    double fEff = fA0;
    for (int i = 0; i < n; ++i) {
        phB += P.clkBRate / sr; if (phB >= 1) phB -= 1;
        const double wB = wave(phB, P.clkBShape);
        double fA = fA0, shape = P.clkShape, phOff = 0, w;
        if (mode == 0) fA = locked ? fA0 * (1 + 0.9 * depth * wB) : fA0 * std::pow(2.0, depth * 1.5 * wB) * drift;
        else if (mode == 1) shape = shape + depth * 0.5 * wB;
        else if (mode == 2) phOff = depth * 0.25 * wB;
        fEff = fA;
        phA += fA / sr; if (phA >= 1) phA -= 1;
        const double wA = wave(phA + phOff, shape);
        w = mode == 3 ? wA + depth * wB : wA;
        bool fire = false;
        if (mode >= 4 && depth > 0.01) {
            const bool gA = wA >= thr, gB = wB >= 0;
            const bool g = mode == 4 ? (gA && gB) : mode == 5 ? (gA || gB) : (gA != gB);
            fire = g != gatePrev; gatePrev = g;
        } else {
            fire = (wPrev < thr && w >= thr) || (wPrev > -thr && w <= -thr);
        }
        wPrev = w;
        if (fire && P.clkRun > 0.5f) { ++fired; env = 1; rnd = rng.bi(); }
        valA = float(wA); valB = float(wB);
    }
    pulsePeriod = 1.0 / std::max(1e-3, 2 * fEff);
    env *= float(std::exp(-(n / sr) / std::max(0.01, pulsePeriod * 0.5)));
    return fired;
}

float ModLfo::tick(double rate, int shape, double dt, MRng& rng) {
    ph += rate * dt;
    bool wrapped = false; while (ph >= 1) { ph -= 1; wrapped = true; }
    switch (shape) {
        case 0: value = float(std::sin(TAU * ph)); break;
        case 1: value = float(ph < 0.5 ? 4 * ph - 1 : 3 - 4 * ph); break;
        case 2: value = float(2 * ph - 1); break;
        case 3: value = float(1 - 2 * ph); break;
        case 4: value = ph < 0.5 ? 1.f : -1.f; break;
        case 5: if (wrapped) held = rng.bi(); value = held; break;
        default: if (wrapped) { from = to; to = rng.bi(); } { const double t = 0.5 - 0.5 * std::cos(PI * ph); value = float(from + (to - from) * t); } break;
    }
    return value;
}

float DustMod::tick(double rate, double decay, double dt, MRng& rng) {
    value *= float(std::exp(-dt / std::max(0.001, decay)));
    if (rng.next() < rate * dt) value = std::max(value, 0.3f + 0.7f * rng.next());
    return value;
}

// ---------------- sequencer ----------------
void Sequencer::reset() {
    for (int h = 0; h < NHEAD; ++h) { heads[h] = Head(); cur[h] = -1; }
    value = 0; gate = 0;
}

void Sequencer::allOff(SeqEvent* ev, int& nev) {
    for (auto& hd : held) if (hd.note >= 0) { if (nev < 16) ev[nev++] = { false, hd.note, 0 }; hd.note = -1; }
    gate = 0;
}

float Sequencer::shapeAt(const Params& P, int h) const {
    const HeadParams& H = P.head[h];
    const double cyc = std::max(2.0, double(H.cycle));
    const double t = std::fmod(heads[h].travelled, cyc) / cyc;
    switch (int(H.shape)) {
        case 1: return float(t);
        case 2: return float(1 - t);
        case 3: return float(std::sin(PI * t));
        case 4: return float(std::exp(-8 * t));
        case 5: return float(0.5 - 0.5 * std::cos(TAU * t));
        default: return 0.f;
    }
}

void Sequencer::advance(const Params& P, int h, double stepDur, int root, MRng& rng, SeqEvent* ev, int& nev) {
    Head& hd = heads[h];
    const int len = std::clamp(int(P.seqLen), 1, NSTEP);
    int next;
    if (hd.fresh || hd.step < 0) { next = std::clamp(int(P.head[h].start) - 1, 0, NSTEP - 1); hd.fresh = false; }
    else {
        const StepParams& S = P.step[hd.step];
        const int to1 = int(S.to1), to2 = int(S.to2);
        int a = to1 > 0 ? to1 - 1 : (hd.step + 1) % len;
        if (to1 == 0 && hd.step >= len) a = 0;
        next = a;
        if (to2 > 0) {
            const bool take2 = P.head[h].branch < 0.5f ? rng.next() < S.br : shapeAt(P, h) > S.br;
            if (take2) next = to2 - 1;
        }
    }
    hd.step = next; cur[h] = next; hd.travelled += 1;
    const StepParams& S = P.step[next];
    if (h == 0) value = S.val;
    if (S.on > 0.5f && rng.next() < S.prob) {
        const int note = std::clamp(root + int(S.note) + int(P.head[h].trans), 0, 127);
        for (auto& x : held) if (x.note == note) { if (nev < 16) ev[nev++] = { false, note, 0 }; x.note = -1; }
        if (nev < 16) ev[nev++] = { true, note, S.vel };
        for (auto& x : held) if (x.note < 0) { x.note = note; x.left = std::max(0.005, S.len * stepDur); x.head0 = h == 0; break; }
        if (h == 0) gate = 1;
        if (S.cut > 0.5f) cutTrigger = true;
    }
}

void Sequencer::tick(const Params& P, int pulses, double pulsePeriod, double dt, int root, MRng& rng, SeqEvent* ev, int& nev) {
    nev = 0; cutTrigger = false;
    const bool on = P.seqOn > 0.5f;
    if (!on) { if (running) { allOff(ev, nev); reset(); } running = false; return; }
    if (!running) { reset(); running = true; }
    // gates ending
    bool g0 = false;
    for (auto& x : held) if (x.note >= 0) { x.left -= dt; if (x.left <= 0) { if (nev < 16) ev[nev++] = { false, x.note, 0 }; x.note = -1; } else if (x.head0) g0 = true; }
    gate = g0 ? 1.f : 0.f;
    const int nh = std::clamp(int(P.seqHeads), 1, NHEAD);
    for (int h = 0; h < nh; ++h) {
        const HeadParams& H = P.head[h];
        const bool shaped = int(H.shape) != 0 && std::abs(H.depth) > 1e-3f;
        const double m = H.rate * (shaped ? std::pow(2.0, H.depth * shapeAt(P, h)) : 1.0);
        const double stepDur = pulsePeriod / std::max(0.01, m);
        if (!shaped && std::abs(H.rate - 1) < 1e-3f) {
            // locked to the clock's pulses, so the clock's feel carries straight through
            for (int p = 0; p < pulses; ++p) advance(P, h, stepDur, root, rng, ev, nev);
            heads[h].phase = 0;
        } else {
            heads[h].phase += dt / std::max(1e-4, pulsePeriod) * m;
            int guard = 0;
            while (heads[h].phase >= 1 && guard++ < 8) { heads[h].phase -= 1; advance(P, h, stepDur, root, rng, ev, nev); }
            if (heads[h].phase > 1) heads[h].phase = 0;
        }
    }
    for (int h = nh; h < NHEAD; ++h) cur[h] = -1;
}

} // namespace silt
