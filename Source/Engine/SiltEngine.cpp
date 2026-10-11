// Silt sound engine: line-for-line port of the browser lab's AudioWorklet engine (v2).
#include "SiltEngine.h"
#include <algorithm>
#include <functional>
#include <cstring>

namespace silt {

static constexpr double PI = 3.14159265358979323846, TAU = PI * 2;
template <typename T> static inline T clampv(T x, T a, T b) { return x < a ? a : x > b ? b : x; }
static inline double ft(double x) { if (x > 3) return 1; if (x < -3) return -1; const double x2 = x * x; return x * (27 + x2) / (27 + 9 * x2); }
static inline double jsRound(double x) { return std::floor(x + 0.5); }

// ---------- static tables ----------
static float WIN[NWS * (WL + 1)];
static double HBE[16];
static const double LO[NK] = { 0, -7, -3, -1, 0, 0, 0, 0, 4.321928094887363, 0, 0, 0 };
static const double HI[NK] = { 1.3, 2, 3, 1, 1, 1, 1, 1, 14.287712379549609, 1, 1, 1 };
static const double SPAN[NK] = { 1, 3, 2, 1, 1, 1, 1, 1, 4, 1, 1, 1 };

static void initStatic() {
    static bool done = false; if (done) return; done = true;
    for (int s = 0; s < NWS; ++s) { const double sh = s / double(NWS - 1);
        for (int i = 0; i <= WL; ++i) { const double u = i / double(WL), hann = 0.5 - 0.5 * std::cos(TAU * u), exd = std::exp(-6.5 * u) * std::min(1.0, u / 0.015);
            WIN[s * (WL + 1) + i] = float(sh < 0.5 ? 1 + (hann - 1) * (sh * 2) : hann + (exd - hann) * ((sh - 0.5) * 2)); } }
    auto I0 = [](double x) { double s = 1, t = 1; for (int k = 1; k < 30; ++k) { t *= (x / 2) / k; s += t * t; } return s; };
    double HB[31]; const double beta = 7.5;
    for (int n = 0; n < 31; ++n) { const int d = n - 15; const double w = I0(beta * std::sqrt(1 - std::pow(d / 15.0, 2))) / I0(beta);
        HB[n] = d == 0 ? 0.5 : (d % 2 == 0 ? 0 : std::sin(PI * d / 2) / (PI * d)) * w; }
    for (int k = 0, j = 0; k <= 30; k += 2) HBE[j++] = HB[k];
}

double Up::even() const { double s = 0; for (int j = 0; j < 16; ++j) s += HBE[j] * x[(i + j) & 15]; return 2 * s; }
double Down::out() const { double s = 0; for (int j = 0; j < 16; ++j) s += HBE[j] * e[(i + j) & 15]; return s + 0.5 * o[(i + 8) & 15]; }

void SVF::set(double f, double q, double sr) { f = clampv(f, 10.0, sr * 0.45); const double g = std::tan(PI * f / sr); k = 1 / q; a1 = 1 / (1 + g * (g + k)); a2 = g * a1; a3 = g * a2; }

// ---------- FFT + band-limited wavetable banks ----------
static void fft(std::vector<double>& re, std::vector<double>& im, bool inv) {
    const int n = (int) re.size();
    for (int i = 1, j = 0; i < n; ++i) { int bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit;
        if (i < j) { std::swap(re[i], re[j]); std::swap(im[i], im[j]); } }
    for (int len = 2; len <= n; len <<= 1) { const double ang = (inv ? 2 : -2) * PI / len, wr = std::cos(ang), wi = std::sin(ang); const int h = len >> 1;
        for (int i = 0; i < n; i += len) { double cr = 1, ci = 0;
            for (int j = 0; j < h; ++j) { const int a = i + j, b = a + h; const double br = re[b] * cr - im[b] * ci, bi = re[b] * ci + im[b] * cr;
                re[b] = re[a] - br; im[b] = im[a] - bi; re[a] += br; im[a] += bi; const double t = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = t; } } }
}

struct Spec { std::vector<double> A = std::vector<double>(1024, 0.0), P = std::vector<double>(1024, 0.0); };
using SpecFn = std::function<bool(int, double&, double&)>; // returns false for "no partial"
static Spec spec(const SpecFn& fn) { Spec s; for (int k = 1; k < 1024; ++k) { double a = 0, p = 0; if (fn(k, a, p)) { s.A[k] = a; s.P[k] = p; } } return s; }
static Spec timeSpec(const std::function<double(double)>& fn) {
    std::vector<double> re(TL), im(TL, 0.0); for (int i = 0; i < TL; ++i) re[i] = fn(i / double(TL));
    fft(re, im, false); Spec s;
    for (int k = 1; k < 1024; ++k) { s.A[k] = std::hypot(re[k], im[k]) / TL * 2; s.P[k] = std::atan2(im[k], re[k]); } return s; }
static inline double chirp(int k) { return 0.18 * k * k; }
struct Fm { double fc, a, bw; };
struct FormOpt { double floor = 0.02, oddBelow = 0, weakFund = 0; };
static Spec formants(double f0, std::vector<Fm> F, double tilt, FormOpt opt = {}) {
    return spec([=](int k, double& A, double& P) { const double f = k * f0; if (f > 16000) return false; double e = 0;
        for (auto& m : F) e += m.a / (1 + std::pow((f - m.fc) / m.bw, 2));
        double amp = (e + opt.floor) * std::pow(double(k), -tilt);
        if (opt.oddBelow > 0 && f < opt.oddBelow && k % 2 == 0) amp *= 0.08;
        if (opt.weakFund > 0 && k == 1) amp *= opt.weakFund;
        A = amp; P = chirp(k); return true; });
}
static inline double qz(double v, double lv) { return jsRound(v * lv) / lv; }
static int idxOf(std::initializer_list<int> l, int k) { int i = 0; for (int x : l) { if (x == k) return i; ++i; } return -1; }

static std::vector<std::vector<Spec>> bankDefs() {
    std::vector<std::vector<Spec>> B;
    B.push_back({ // 0 Classic
        spec([](int k, double& A, double& P) { if (k != 1) return false; A = 1; P = 0; return true; }),
        spec([](int k, double& A, double& P) { if (!(k % 2)) return false; A = 1.0 / (k * k); P = (k % 4 == 3) ? PI : 0; return true; }),
        spec([](int k, double& A, double& P) { A = std::pow(double(k), -1.5); P = 0; return true; }),
        spec([](int k, double& A, double& P) { A = 1.0 / k; P = 0; return true; }),
        spec([](int k, double& A, double& P) { if (!(k % 2)) return false; A = 1.0 / k; P = 0; return true; }),
        timeSpec([](double t) { return t < 0.3 ? 1.0 : -1.0; }),
        timeSpec([](double t) { return t < 0.12 ? 1.0 : -1.0; }),
        spec([](int k, double& A, double& P) { A = (1.0 / k) * (1 + 3 * std::exp(-std::pow((k - 8) / 2.2, 2))); P = 0; return true; }),
    });
    B.push_back({ // 1 Hollow 90s
        spec([](int k, double& A, double& P) { P = 0; if (k == 1) A = 1; else if (k == 3) A = 0.35; else if (k == 5) A = 0.12; else return false; return true; }),
        spec([](int k, double& A, double& P) { if (!(k % 2)) return false; A = 1.0 / (k * k); P = 0; return true; }),
        spec([](int k, double& A, double& P) { P = 0; switch (k) { case 1: A = 1; break; case 2: A = 0.6; break; case 4: A = 0.3; break; case 8: A = 0.15; break; case 16: A = 0.06; break; default: return false; } return true; }),
        spec([](int k, double& A, double& P) { if (k == 1) { A = 1; P = 0; return true; } if (k >= 6 && k <= 11) { A = 0.32 * std::exp(-(k - 6) * 0.25); P = chirp(k); return true; } return false; }),
        spec([](int k, double& A, double& P) { if (k == 1) { A = 1; P = 0; return true; } if (k == 2) { A = 0.25; P = 0; return true; } if (k == 3) { A = 0.1; P = 0; return true; }
            if (k < 48) { A = 0.02 / std::sqrt(double(k)) * (0.5 + ((k * 7919) % 13) / 13.0); P = std::fmod(k * 2.3, TAU); return true; } return false; }),
        timeSpec([](double t) { return qz(std::tanh(3 * std::sin(TAU * t)) * 0.9, 7); }),
        spec([](int k, double& A, double& P) { A = std::exp(-k / 4.0) * (k % 2 ? 1.6 : 0.6); P = 0; return true; }),
        spec([](int k, double& A, double& P) { if (!(k % 2)) return false; A = std::exp(-std::pow((k - 5) / 3.0, 2)) + 0.4 / k; P = chirp(k); return true; }),
    });
    auto opt = [](double floor, double oddBelow, double weakFund) { FormOpt o; o.floor = floor; o.oddBelow = oddBelow; o.weakFund = weakFund; return o; };
    B.push_back({ // 2 Reeds & brass
        formants(110, { { 500, 0.9, 200 }, { 1200, 0.6, 400 }, { 2600, 0.25, 600 } }, 0.3),
        formants(175, { { 400, 1, 150 }, { 900, 0.4, 300 } }, 0.8),
        formants(233, { { 500, 1, 150 }, { 1500, 0.7, 250 }, { 2700, 0.4, 400 }, { 4500, 0.2, 700 } }, 0.25),
        formants(147, { { 420, 1, 140 }, { 1300, 0.6, 250 }, { 2600, 0.35, 400 }, { 5500, 0.08, 1500 } }, 0.2),
        formants(196, { { 1500, 0.6, 500 }, { 3000, 0.2, 800 } }, 0.5, opt(0.3, 1500, 0)),
        formants(330, { { 1100, 1, 250 }, { 2800, 0.5, 500 } }, 0.2, opt(0.02, 0, 0.35)),
        formants(98, { { 450, 1, 120 }, { 1150, 0.5, 250 }, { 2500, 0.15, 400 } }, 0.4),
        formants(233, { { 1800, 1, 300 }, { 4000, 0.6, 600 } }, 0.15, opt(0.02, 0, 0.2)),
    });
    auto vow = [&](double f0, double a, double b, double c, double d) { return formants(f0, { { a, 1, 70 }, { b, 0.5, 90 }, { c, 0.25, 120 }, { d, 0.1, 200 } }, 1, opt(0.005, 0, 0)); };
    B.push_back({ // 3 Vocal
        vow(130, 730, 1090, 2440, 3500), vow(130, 530, 1840, 2480, 3500), vow(130, 270, 2290, 3010, 3500), vow(130, 570, 840, 2410, 3500),
        vow(130, 300, 870, 2240, 3500), vow(220, 850, 1220, 2810, 3500), vow(220, 610, 2330, 2990, 3500), vow(130, 250, 2500, 3300, 3800),
    });
    auto rnd = [](double s) { double x = std::sin(s * 12.9898) * 43758.5453; return x - std::floor(x); };
    B.push_back({ // 4 Glass
        spec([](int k, double& A, double& P) { int i = idxOf({ 1, 7, 13, 19, 25, 31 }, k); if (i < 0) return false; A = 1 / std::sqrt(double(i + 1)); P = chirp(k); return true; }),
        spec([](int k, double& A, double& P) { if (k == 1) { A = 1; P = 0; } else { A = 0.6 * std::exp(-std::pow((k - 20) / 1.5, 2)); P = chirp(k); } return true; }),
        spec([](int k, double& A, double& P) { P = 0; if (k == 1) A = 1; else if (k == 14) A = 0.5; else if (k == 28) A = 0.15; else return false; return true; }),
        spec([](int k, double& A, double& P) { double r = std::sqrt(double(k)); if (r != std::floor(r)) return false; A = 1 / r; P = chirp(k); return true; }),
        spec([=](int k, double& A, double& P) { if (k == 1) { A = 1; P = 0; return true; } if (k >= 8 && k <= 40) { A = 0.3 * rnd(k) * rnd(k + 9); P = rnd(k + 3) * TAU; return true; } return false; }),
        spec([](int k, double& A, double& P) { if (!((k % 3) && k < 64)) return false; A = 1 / std::sqrt(double(k)); P = chirp(k); return true; }),
        spec([=](int k, double& A, double& P) { if (k == 1) { A = 1; P = 0; return true; } if (k == 2) { A = 0.3; P = 0; return true; } if (k >= 30 && k <= 40) { A = 0.2; P = rnd(k) * TAU; return true; } return false; }),
        spec([](int k, double& A, double& P) { int i = idxOf({ 1, 3, 8, 21, 55, 144 }, k); if (i < 0) return false; A = 1.0 / (i + 1); P = chirp(k); return true; }),
    });
    auto step = [](std::function<double(int)> f) { return [f](double t) { return f(int(std::floor(t * 256)) & 255); }; };
    std::vector<int> nseq; { unsigned lfsr = 0xACE1; for (int i = 0; i < 256; ++i) { unsigned b = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1u; lfsr = (lfsr >> 1) | (b << 15); nseq.push_back(int(lfsr & 1u)); } }
    B.push_back({ // 5 Bitmask
        timeSpec([](double t) { return qz(std::sin(TAU * std::floor(t * 32) / 32), 7); }),
        timeSpec(step([](int i) { return ((i & (i >> 3)) & 31) / 15.5 - 1; })),
        timeSpec(step([](int i) { return ((i ^ (i >> 4)) & 15) / 7.5 - 1; })),
        timeSpec(step([](int i) { return (((i * 5) & (i >> 2)) & 63) / 31.5 - 1; })),
        timeSpec(step([](int i) { return ((i | (i >> 5)) & 31) / 15.5 - 1; })),
        timeSpec(step([](int i) { int c = 0, v = i; while (v) { c += v & 1; v >>= 1; } return c / 4.0 - 1; })),
        timeSpec(step([](int i) { return (((i * 3) ^ (i >> 1)) & 63) / 31.5 - 1; })),
        timeSpec(step([nseq](int i) { return (i < 128 ? 1.0 : -1.0) * (nseq[i] ? 1 : 0.4); })),
    });
    return B;
}

void Engine::buildTables() {
    auto defs = bankDefs();
    std::vector<double> re(TL), im(TL);
    T.assign(NBANK, std::vector<float>(BANKSZ, 0.f));
    for (int b = 0; b < NBANK; ++b) {
        auto& Tb = T[b];
        for (int f = 0; f < NFR; ++f) {
            const Spec& s = defs[b][f]; double scale = 0;
            for (int m = 0; m < NMIP; ++m) {
                const int maxH = std::min(1023, 1024 >> m);
                std::fill(re.begin(), re.end(), 0.0); std::fill(im.begin(), im.end(), 0.0);
                for (int k = 1; k <= maxH; ++k) { if (s.A[k] == 0) continue; const double c = s.A[k] * std::cos(s.P[k]), sn = s.A[k] * std::sin(s.P[k]);
                    re[k] = c; im[k] = sn; re[TL - k] = c; im[TL - k] = -sn; }
                fft(re, im, true);
                if (m == 0) { double pk = 0, rms = 0; for (int i = 0; i < TL; ++i) { pk = std::max(pk, std::abs(re[i])); rms += re[i] * re[i]; }
                    rms = std::sqrt(rms / TL); scale = std::min(0.42 / std::max(rms, 1e-9), 1 / std::max(pk, 1e-9)); }
                const int o = f * FRSZ + m * MIPSZ;
                for (int i = 0; i < TL; ++i) Tb[o + i] = float(re[i] * scale);
                Tb[o + TL] = Tb[o];
            }
        }
    }
}

// ---------- engine ----------
Engine::Engine() { initStatic(); buildTables(); prepare(48000); }

void Engine::prepare(double sampleRate) {
    sr = sampleRate;
    dlen = int(std::ceil(sr * 1.6)); dbuf.assign(dlen, 0.f); dw = 0; dtS = 0.2 * sr; fbF = SVF(); dcx = dcy = 0;
    for (int b = 0; b < 8; ++b) { anS[b] = SVF(); syL[b] = SVF(); syR[b] = SVF(); env[b] = 0; }
    scr = 0; vDry = vWet = 1e-4; vGain = 1;
    dustL[0].set(3000, 3, sr); dustL[1].set(6500, 2, sr); dustR[0].set(3300, 3, sr); dustR[1].set(7100, 2, sr);
    popL.set(420, 2, sr); popR.set(380, 2, sr);
    for (auto& d : drops) d = Drop();
    const double base[8] = { 29.7, 37.1, 41.1, 43.7, 53.3, 59.9, 67.1, 73.9 };
    for (int q = 0; q < 8; ++q) { fdn[q].buf.assign(size_t(std::ceil(sr * base[q] / 1000 * 2.2) + 16), 0.f); fdn[q].w = 0; fdn[q].len = 0; fdn[q].base = base[q]; fdn[q].lp = 0; fdn[q].g = 0; }
    const int nl[4] = { 142, 107, 379, 277 }, nr[4] = { 151, 113, 367, 293 };
    for (int i = 0; i < 4; ++i) { apL[i].b.assign(size_t(std::ceil(nl[i] * sr / 48000)), 0.f); apL[i].i = 0; apR[i].b.assign(size_t(std::ceil(nr[i] * sr / 48000)), 0.f); apR[i].i = 0; }
    modPh = 0; agc = 1; agcR = 1e-4; outR = 0; vizC = 0; dcL0 = dcL1 = dcR0 = dcR1 = 0;
    for (auto& v : voices) v.active = false;
    lastV = nullptr;
}

void Engine::noteOn(int note, float vel) {
    if (!P) return;
    const int poly = clampv(int(P->poly), 1, MAXV);
    Voice* v = nullptr; for (int i = 0; i < poly; ++i) if (!voices[i].active) { v = &voices[i]; break; }
    if (!v) { v = &voices[0]; for (int i = 1; i < poly; ++i) if (voices[i].serial < v->serial) v = &voices[i]; }
    const bool wasActive = v->active;
    v->active = true; v->gate = true; v->note = note; v->freq = 440 * std::pow(2.0, (note - 69) / 12.0); v->vel = vel; v->serial = ++serial;
    const double* S0 = Sc[0];
    for (int k = 0; k < NK; ++k) { const double st = wasActive ? v->cur[k] : S0[k]; v->p[k] = st; v->s[k] = st; v->v[k] = 0; v->n[k] = 0; v->cur[k] = st; v->from[k] = st; }
    if (!wasActive) { v->from[K_LEVEL] = 0; v->p[K_LEVEL] = 0; v->s[K_LEVEL] = 0; v->cur[K_LEVEL] = 0; v->lv0 = v->lv1 = 0;
        v->A.reset(rng); v->B.reset(rng); std::memset(v->lad, 0, sizeof(v->lad)); v->svf[0].reset(); v->svf[1].reset();
        for (int i = 0; i < 3; ++i) v->ry1[i] = v->ry2[i] = 0; }
    v->toIdx = 0; v->T = std::max(0.0005, double(P->atk)); v->t = 0; v->stage = 0; v->tail = 1; v->strike = 1;
    lastV = v;
}

void Engine::noteOff(int note) {
    for (auto& v : voices) if (v.active && v.gate && v.note == note) {
        v.gate = false; std::memcpy(v.from, v.p, sizeof(v.p)); v.toIdx = 2; v.T = std::max(0.0005, P ? double(P->rel) : 0.5); v.t = 0; v.stage = 2; }
}

void Engine::panic() { for (auto& v : voices) v.active = false; std::fill(dbuf.begin(), dbuf.end(), 0.f); fbF.reset(); for (auto& f : fdn) std::fill(f.buf.begin(), f.buf.end(), 0.f); }

void Engine::panicDSP() {
    std::fill(dbuf.begin(), dbuf.end(), 0.f); fbF.reset();
    for (auto& f : fdn) { std::fill(f.buf.begin(), f.buf.end(), 0.f); f.lp = 0; }
    for (int b = 0; b < 8; ++b) { syL[b].reset(); syR[b].reset(); anS[b].reset(); }
    for (auto& v : voices) { std::memset(v.lad, 0, sizeof(v.lad)); v.svf[0].reset(); v.svf[1].reset(); for (int i = 0; i < 3; ++i) v.ry1[i] = v.ry2[i] = 0; }
}

void Engine::ctrlLayer(Voice& v, Layer& L, const LayerParams& lp, double f0, double dt) {
    const Params& Pp = *P; const double* cur = v.cur;
    L.enabled = lp.on > 0.5f; if (!L.enabled) return;
    const int N = clampv(int(lp.n), 1, MAXE); L.N = N; L.mat = int(lp.mat); L.tb = T[clampv(int(lp.bank), 0, NBANK - 1)].data();
    const double det = lp.det, wan = lp.wander + Pp.allure * 0.5, wr = Pp.wanderRate, spread = lp.pan;
    const double stp = wr * std::sqrt(dt) * 40;
    for (int e = 0; e < N; ++e) {
        const double d = N > 1 ? (e / double(N - 1)) * 2 - 1 : 0;
        if (wan > 0) L.wd[e] = clampv(L.wd[e] + (rng.next() - 0.5) * stp, -wan * 60, wan * 60); else L.wd[e] *= 0.95;
        const double cents = d * det + L.wd[e] + v.dPv;
        L.inc[e] = f0 * std::pow(2.0, cents / 1200) / sr;
        const double pan = 0.5 + d * spread * 0.5, a = pan * PI / 2; L.pl[e] = std::cos(a) * 1.414 / std::sqrt(double(N)); L.pr[e] = std::sin(a) * 1.414 / std::sqrt(double(N));
    }
    const double tr = Pp.formTrack, fb = std::exp(std::log(f0) * tr + std::log(261.63 * std::pow(2.0, cur[K_PITCH])) * (1 - tr));
    const double ffHz = fb * lp.form * std::pow(2.0, cur[K_FORM] + Pp.mass * 1.2);
    L.ff = std::min(ffHz, sr * 0.45) / sr;
    const double C = lp.cyc * std::pow(2.0, -Pp.grainM * 1.5);
    L.contin = C * f0 / std::max(1e-6, ffHz) >= 0.999;
    L.C = std::max(0.1, C);
    const double ws = clampv(lp.win + Pp.grainM * 0.45, 0.0, 1.0); L.ws = int(jsRound(ws * (NWS - 1))) * (WL + 1);
    const double perSamp = f0 > 0 ? sr / f0 : 1000; L.fade = 1 / clampv(perSamp * 0.5, 4.0, 48.0);
    const double lf = std::log2(std::max(1e-6, 1024 * ffHz / ((L.contin ? 0.25 : 0.17) * sr)));
    int m0 = 0; double mf = 0; if (lf > 0) { m0 = int(std::floor(lf)); mf = lf - m0; } if (m0 >= NMIP - 1) { m0 = NMIP - 1; mf = 0; }
    const int m1 = std::min(NMIP - 1, m0 + 1);
    const double pos = clampv(lp.pos + cur[K_POS], 0.0, 1.0) * (NFR - 1); int f0i = int(std::floor(pos)); double ffr = pos - f0i; if (f0i >= NFR - 1) { f0i = NFR - 1; ffr = 0; }
    const int f1i = std::min(NFR - 1, f0i + 1);
    L.o00 = f0i * FRSZ + m0 * MIPSZ; L.o10 = f1i * FRSZ + m0 * MIPSZ; L.o01 = f0i * FRSZ + m1 * MIPSZ; L.o11 = f1i * FRSZ + m1 * MIPSZ;
    L.w00 = (1 - ffr) * (1 - mf); L.w10 = ffr * (1 - mf); L.w01 = (1 - ffr) * mf; L.w11 = ffr * mf;
    L.ph0 = lp.phase;
    L.K = cur[K_COUP] * cur[K_COUP] * (f0 * 0.6 + 4);
    L.lvl = lp.lvl;
}

void Engine::ctrlVoice(Voice& v, double dt) {
    const Params& Pp = *P; const double* to = Sc[v.toIdx];
    v.t += dt; double u = v.t / v.T; if (u > 1) u = 1;
    const double c = Pp.curve * 9; const double sh = std::abs(c) < 0.01 ? u : c > 0 ? (1 - std::exp(-c * u)) / (1 - std::exp(-c)) : (std::exp(-c * u) - 1) / (std::exp(-c) - 1);
    for (int k = 0; k < NK; ++k) v.p[k] = v.from[k] + (to[k] - v.from[k]) * sh;
    if (u >= 1) {
        if (v.stage == 0) { std::memcpy(v.from, v.p, sizeof(v.p)); v.toIdx = 1; v.T = std::max(0.001, double(Pp.dec)); v.t = 0; v.stage = 1; }
        else if (v.stage == 1) { if (Pp.loop > 0.5f && v.gate) { std::memcpy(v.from, v.p, sizeof(v.p)); v.toIdx = 0; v.T = std::max(0.005, double(Pp.loopTime)); v.t = 0; v.stage = 3; } }
        else if (v.stage == 3) { std::memcpy(v.from, v.p, sizeof(v.p)); v.toIdx = 1; v.T = std::max(0.005, double(Pp.loopTime)); v.t = 0; v.stage = 1; }
        else if (v.stage == 2) v.stage = 4;
    }
    const double kS = std::pow(TAU * Pp.stiff, 2), cc = 2 * Pp.visc * std::sqrt(kS), A = Pp.auth, temp = Pp.temp + Pp.allure * 0.3, nr = std::min(1.0, dt * 6);
    const double* dest = Sc[v.toIdx];
    for (int k = 0; k < NK; ++k) {
        v.n[k] += (rng.gauss() * temp * SPAN[k] * 0.3 - v.n[k]) * nr;
        const double a = kS * (dest[k] + v.n[k] - v.s[k]) - cc * v.v[k]; v.v[k] += a * dt; v.s[k] += v.v[k] * dt;
        if (!(v.s[k] == v.s[k]) || std::abs(v.s[k]) > 1e4) { v.s[k] = v.p[k]; v.v[k] = 0; }
        v.cur[k] = clampv(v.p[k] + A * (v.s[k] - v.p[k]), LO[k], HI[k]);
    }
    if (v.stage == 4) { v.tail *= std::exp(-dt / std::max(0.02, Pp.rel * 0.35)); if (v.tail < 1e-4 || v.cur[K_LEVEL] < 1e-5) { v.active = false; return; } }
    const double dr = Pp.drift, ds = std::sqrt(dt) * 3;
    v.dP = clampv(v.dP + rng.gauss() * ds, -1.0, 1.0); v.dC = clampv(v.dC + rng.gauss() * ds, -1.0, 1.0);
    v.dPv += (v.dP * dr * 7 - v.dPv) * std::min(1.0, dt * 3); v.dCv += (v.dC * dr * 0.25 - v.dCv) * std::min(1.0, dt * 3);
    const double* cur = v.cur; const double f0 = v.freq * std::pow(2.0, cur[K_PITCH]);
    ctrlLayer(v, v.A, Pp.a, f0, dt);
    ctrlLayer(v, v.B, Pp.b, f0 * std::pow(2.0, (Pp.bSemi + Pp.bFine / 100.0) / 12), dt);
    const double rf = v.freq * Pp.resTune, rat[3] = { 1, 2.756, 5.404 }, gn[3] = { 1, 0.5, 0.28 };
    for (int i = 0; i < 3; ++i) { const double f = std::min(rf * rat[i], sr * 0.45), w = TAU * f / sr, r = std::exp(-1 / (std::max(0.005, Pp.resDecay / (1 + i * 0.7)) * sr));
        v.rb1[i] = 2 * r * std::cos(w); v.rb2[i] = -r * r; v.rg[i] = (1 - r) * 2.5 * gn[i]; }
    v.res = cur[K_RES]; v.ring = cur[K_RING]; v.fold = clampv(cur[K_FOLD], 0.0, 1.0); v.fbs = cur[K_FB];
    const double vc = (v.vel - 0.7) * Pp.velCut * 3;
    const double cut = clampv(std::pow(2.0, cur[K_CUT] + Pp.keytrack * (v.note - 60) / 12.0 + vc + v.dCv + Pp.mass * 0.8), 20.0, 21000.0);
    v.g0 = v.g1; v.g1 = std::tan(PI * std::min(cut, sr * 0.9) / (2 * sr));
    v.lv0 = v.lv1; v.lv1 = cur[K_LEVEL] * (1 - Pp.velAmp + Pp.velAmp * v.vel) * v.tail;
}

void Engine::layerSample(Layer& L, double pm, bool syncIn, bool& emitted0Out) {
    if (!L.enabled) { L.outL = 0; L.outR = 0; emitted0Out = false; return; }
    const int N = L.N; const float* tb = L.tb; const double mask = mask_, scat = scat_, fade = L.fade, C = L.C, ff = L.ff; const int ws = L.ws;
    double sl = 0, srr = 0; bool emitted0 = false;
    if (N > 1 && (L.K > 0.01 || (++L.rc & 63) == 0)) { double cx = 0, sy = 0; for (int e = 0; e < N; ++e) { cx += std::cos(TAU * L.th[e]); sy += std::sin(TAU * L.th[e]); }
        const double R = std::sqrt(cx * cx + sy * sy) / N, psi = std::atan2(sy, cx) / TAU, kk = L.K > 0.01 ? L.K * R / sr : 0; L.R = R; L.psi = psi;
        for (int e = 0; e < N; ++e) L.ci[e] = kk * std::sin(TAU * (psi - L.th[e])); }
    else if (N == 1) { L.ci[0] = 0; L.R = 1; }
    for (int e = 0; e < N; ++e) {
        const double ie = L.inc[e] + L.ci[e];
        double th = L.th[e] + ie;
        if (syncIn) th = 1;
        if (th >= 1) { th -= std::floor(th);
            const double since = syncIn || ie <= 0 ? 0 : std::min(1.0, th / ie);
            if (e == 0) emitted0 = true;
            if (rng.next() >= mask) {
                const int old = e * 2 + L.cur[e];
                if (L.on[old] && !L.rl[old]) { L.rl[old] = 1; L.rg[old] = 1; }
                const int ns = e * 2 + (L.cur[e] ^ 1); L.cur[e] ^= 1;
                L.amp[ns] = 1 - scat * rng.next() * 0.6; L.fm[ns] = scat > 0 ? std::pow(2.0, scat * rng.gauss() * 0.12) : 1;
                const double st0 = (since - 1) * ff * L.fm[ns];
                L.on[ns] = 1; L.rd[ns] = st0; L.el[ns] = st0; L.fi[ns] = 0; L.rl[ns] = 0; L.rg[ns] = 1;
                L.n0[ns] = rng.next() * 2 - 1; L.n1[ns] = rng.next() * 2 - 1; L.ni[ns] = 0;
                if (scat > 0) th -= scat * rng.next() * 0.45;
            }
        }
        L.th[e] = th;
        for (int q = 0; q < 2; ++q) {
            const int s = e * 2 + q; if (!L.on[s]) continue;
            const double inc = ff * L.fm[s]; L.rd[s] += inc; const double el = (L.el[s] += inc);
            const double u = el / C;
            if (u >= 1 && !L.rl[s] && !L.contin) { L.rl[s] = 1; L.rg[s] = 1; }
            double g; if (L.fi[s] < 1) g = (L.fi[s] += fade); else g = 1; if (g > 1) g = 1;
            if (L.rl[s]) { L.rg[s] -= fade; if (L.rg[s] <= 0) { L.on[s] = 0; continue; } g *= L.rg[s]; }
            const double wi = u >= 1 ? WL : u > 0 ? u * WL : 0; const int wI = int(wi); const double wF = wi - wI; const int wb = ws + wI;
            const double w = wI >= WL ? WIN[ws + WL] : WIN[wb] + wF * (WIN[wb + 1] - WIN[wb]);
            double x;
            if (L.mat == 1) { const double r = L.rd[s], nn = std::floor(r); if (nn != L.ni[s]) { L.n0[s] = L.n1[s]; L.n1[s] = rng.next() * 2 - 1; L.ni[s] = nn; } x = (L.n0[s] + (r - nn) * (L.n1[s] - L.n0[s])) * 0.75; }
            else { double ph = L.rd[s] + L.ph0 + pm; ph -= std::floor(ph); const double fi = ph * TL; int i0 = int(fi); if (i0 >= TL) i0 = TL - 1; const double fr = fi - i0;
                int a = L.o00 + i0; x = L.w00 * (tb[a] + fr * (tb[a + 1] - tb[a]));
                if (L.w10 > 0) { a = L.o10 + i0; x += L.w10 * (tb[a] + fr * (tb[a + 1] - tb[a])); }
                if (L.w01 > 0) { a = L.o01 + i0; x += L.w01 * (tb[a] + fr * (tb[a + 1] - tb[a])); }
                if (L.w11 > 0) { a = L.o11 + i0; x += L.w11 * (tb[a] + fr * (tb[a + 1] - tb[a])); } }
            x *= w * g * L.amp[s];
            sl += x * L.pl[e]; srr += x * L.pr[e];
        }
    }
    L.outL = sl; L.outR = srr; emitted0Out = emitted0;
}

void Engine::renderVoice(Voice& v, int off, int m) {
    const Params& Pp = *P; Layer& A = v.A; Layer& B = v.B; const bool isLast = &v == lastV;
    const double dz = Pp.derez; const int holdN = 1 + int(std::floor(dz * dz * 24)); const double q = dz > 0.01 ? std::pow(2.0, 15 - dz * 12) : 0;
    const double ring = v.ring, res = v.res, fold = v.fold, fgain = (1 + fold * 5) * PI / 2, fbs = v.fbs, pmA = Pp.pmAmt * 0.6;
    const int mode = int(Pp.fMode); const double k = Pp.reso * 4.1, drv = 1 + Pp.drive * 5, dcomp = 1 / (1 + Pp.drive * 1.2), heat = Pp.heat, hd = 1 + heat * 4, hb = 0.18 * heat, thb = ft(hb), hn = 1 / (hd * (1 - thb * thb) + 1e-9);
    const double q2 = 0.5 + Pp.reso * 40; SVF& sv0 = v.svf[0]; SVF& sv1 = v.svf[1];
    if (mode > 0) { sv0.k = 1 / q2; sv1.k = 1 / q2; }
    const double la = A.enabled ? A.lvl : 0, lb = B.enabled ? B.lvl : 0;
    double ins[4], outs[4];
    for (int j = 0; j < m; ++j) {
        const double fr = (j + 0.5) / m, lv = v.lv0 + (v.lv1 - v.lv0) * fr;
        bool eA = false, eB = false;
        layerSample(A, pmA * v.bPrev, false, eA);
        layerSample(B, 0, Pp.bSync > 0.5f && eA, eB);
        const double aL = A.outL, aR = A.outR, bLs = B.outL, bRs = B.outR;
        v.bPrev = (bLs + bRs) * 0.5;
        double xL = la * aL + lb * bLs + ring * aL * bLs * 3.2, xR = la * aR + lb * bRs + ring * aR * bRs * 3.2;
        t1L[off + j] += float(xL * lv); t1R[off + j] += float(xR * lv);
        if (res > 0.001) { double ex = (xL + xR) * 0.5; if (v.strike > 0) { ex += (rng.next() * 2 - 1) * v.strike * Pp.strike * 2; v.strike *= 0.92; if (v.strike < 1e-3) v.strike = 0; }
            double rs = 0; for (int i = 0; i < 3; ++i) { const double y = v.rb1[i] * v.ry1[i] + v.rb2[i] * v.ry2[i] + v.rg[i] * ex; v.ry2[i] = v.ry1[i]; v.ry1[i] = y; rs += y; }
            rs = ft(rs * 0.8) * 1.25; xL = xL * (1 - res) + rs * res; xR = xR * (1 - res) + rs * res; }
        if (dz > 0.01) { if (++v.hc >= holdN) { v.hc = 0; v.hL = xL; v.hR = xR; } xL = jsRound(v.hL * q) / q; xR = jsRound(v.hR * q) / q; }
        t2L[off + j] += float(xL * lv); t2R[off + j] += float(xR * lv);
        v.upL.push(xL); v.upR.push(xR);
        ins[0] = v.upL.even(); ins[1] = v.upR.even(); ins[2] = v.upL.odd(); ins[3] = v.upR.odd();
        for (int h = 0; h < 2; ++h) {
            const double g = v.g0 + (v.g1 - v.g0) * ((j * 2 + h + 1) / double(m * 2));
            for (int ch = 0; ch < 2; ++ch) {
                double x = ins[h * 2 + ch];
                if (fold > 0.001) x = x * (1 - fold) + std::sin(x * fgain) * fold * 0.8;
                double y;
                if (mode == 0) { double* st = v.lad[ch]; const double G = g / (1 + g), om = 1 - G, G2 = G * G, G4 = G2 * G2;
                    const double S = om * (G * G2 * st[0] + G2 * st[1] + G * st[2] + st[3]), xin = x * drv;
                    const double y4 = (G4 * xin + S) / (1 + k * G4); double uu = ft(xin - k * y4);
                    for (int i = 0; i < 4; ++i) { const double vv = (uu - st[i]) * G, yy = vv + st[i]; st[i] = yy + vv; uu = yy; }
                    y = uu * (1 + k * 0.45) * dcomp; }
                else { SVF& sv = ch ? sv1 : sv0; const double a1 = 1 / (1 + g * (g + sv.k)), a2 = g * a1, a3 = g * a2;
                    const double xin = x * drv * 0.5, v3 = xin - sv.b, v1 = a1 * sv.a + a2 * v3, v2 = sv.b + a2 * sv.a + a3 * v3;
                    sv.a = ft((2 * v1 - sv.a) * 0.35) / 0.35; sv.b = 2 * v2 - sv.b;
                    y = (mode == 1 ? v2 : mode == 2 ? v1 * sv.k * 2.2 : xin - sv.k * v1 - v2) * 2 * dcomp; }
                y *= lv;
                if (heat > 0.001) y = (ft(hd * y + hb) - thb) * hn;
                outs[h * 2 + ch] = y;
            }
        }
        v.dnL.push(outs[0], outs[2]); v.dnR.push(outs[1], outs[3]);
        const double oL = v.dnL.out() * 0.5, oR = v.dnR.out() * 0.5;
        bL[off + j] += float(oL); bR[off + j] += float(oR); fbB[off + j] += float((oL + oR) * 0.5 * fbs);
        if (isLast) vt[off + j] = float((oL + oR) * 0.5);
    }
}

void Engine::setControls(const Params& Pin, const StateSet& S) {
    P = &Pin;
    for (int i = 0; i < 3; ++i) for (int k = 0; k < NK; ++k) Sc[i][k] = k == K_CUT ? std::log2(clampv(double(S[i][k]), 20.0, 20000.0)) : double(S[i][k]);
}

void Engine::process(const Params& Pin, const StateSet& S, float* L, float* R, float* tap, int n) {
    setControls(Pin, S);
    for (int off = 0; off < n; off += CHUNK) {
        const int m = std::min(CHUNK, n - off);
        renderChunk(m, L + off, R + off, tap ? tap + off : nullptr);
    }
}

void Engine::renderChunk(int n, float* L, float* R, float* tap) {
    const Params& Pp = *P;
    std::fill(bL, bL + CHUNK, 0.f); std::fill(bR, bR + CHUNK, 0.f); std::fill(fbB, fbB + CHUNK, 0.f);
    std::fill(t1L, t1L + CHUNK, 0.f); std::fill(t1R, t1R + CHUNK, 0.f); std::fill(t2L, t2L + CHUNK, 0.f); std::fill(t2R, t2R + CHUNK, 0.f); std::fill(vt, vt + CHUNK, 0.f);
    mask_ = 0; scat_ = 0;
    for (int off = 0; off < n; off += CTRL) {
        const int m = std::min(CTRL, n - off); const double dt = m / sr;
        for (auto& v : voices) { if (!v.active) continue; ctrlVoice(v, dt); if (!v.active) continue;
            mask_ = clampv(v.cur[K_MASK] + Pp.grainM * 0.3, 0.0, 0.98); scat_ = clampv(v.cur[K_SCAT] + Pp.allure * 0.4, 0.0, 1.0);
            renderVoice(v, off, m); }
    }
    // vocoder setup
    const bool vocOn = Pp.vocOn > 0.5f; const int body = int(Pp.vocBody), skin = int(Pp.vocSkin); const double spread = Pp.vocSpread, sh = std::pow(2.0, Pp.vocShift + Pp.mass * 0.6), vq = Pp.vocQ;
    if (vocOn) for (int b = 0; b < 8; ++b) { const double fc = 1000 * std::pow(2.0, (b - 3.5) * 0.85 * spread); anS[b].set(fc, vq, sr); syL[b].set(fc * sh, vq, sr); syR[b].set(fc * sh, vq, sr); }
    const double resp = Pp.vocResp, atk = 1 - std::exp(-1 / ((0.001 + resp * 0.04) * sr)), relc = 1 - std::exp(-1 / ((0.02 + resp * 0.7) * sr));
    const double dep = std::min(1.0, double(Pp.scrapeDepth) + Pp.allure * 0.4), dustP = Pp.dustRate / sr, dropP = Pp.dropRate / sr;
    double bandNorm[8]; for (int b = 0; b < 8; ++b) bandNorm[b] = body == 1 ? std::sqrt(1000 / (1000 * std::pow(2.0, (b - 3.5) * 0.85 * spread))) * 0.5 : 1;
    // feedback setup
    fbF.set(Pp.fbCut, Pp.fbQ, sr);
    const int len = dlen; const double tgt = clampv(Pp.fbTime / 1000.0 * sr, 2.0, len - 4.0), t3 = ft(std::max(0.3, double(Pp.fbDrive)));
    const int fbType = int(Pp.fbType);
    // space setup
    const double size = Pp.spSize, rt = std::max(0.1, double(Pp.spDecay)), damp = 1 - std::exp(-TAU * (800 + Pp.spTone * 14000) / sr);
    for (auto& f : fdn) { f.len = std::min(int(f.buf.size()) - 12, int(std::floor(f.base * size * sr / 1000))); f.g = std::pow(10.0, -3.0 * f.len / (rt * sr)); }
    const double spMix = Pp.spMix; const int listen = int(Pp.listen);
    double sumSq = 0; int act = 0; for (auto& v : voices) if (v.active) ++act;
    double fo[8];
    for (int i = 0; i < n; ++i) {
        double l = bL[i], r = bR[i];
        // feedback loop
        dtS += (tgt - dtS) * 0.0008; double rp = dw - dtS; while (rp < 0) rp += len;
        const int i0 = int(rp); const double frc = rp - i0, dly = dbuf[i0] * (1 - frc) + dbuf[(i0 + 1) % len] * frc;
        double rp2 = dw - dtS * 1.07; while (rp2 < 0) rp2 += len; const int j0 = int(rp2); const double dly2 = dbuf[j0] + (rp2 - j0) * (dbuf[(j0 + 1) % len] - dbuf[j0]);
        fbF.tick(dly); const double ffv = fbType == 0 ? fbF.lp : fbType == 1 ? fbF.bp * fbF.k : fbF.hp;
        const double y = ft(ffv * Pp.fbDrive) / t3; const double dc = y - dcx + 0.996 * dcy; dcx = y; dcy = dc;
        dbuf[dw] = float(ft(fbB[i] + dc * Pp.fbRegen)); dw = (dw + 1) % len;
        const double fL = dc * Pp.fbMix, fR = (0.6 * dc + 0.4 * dly2) * Pp.fbMix; l += fL; r += fR;
        // material vocoder
        double wL = 0, wR = 0;
        if (vocOn) {
            double bdL = 0, bdR = 0;
            if (body == 0) { bdL = l; bdR = r; }
            else if (body == 1) { bdL = rng.next() * 2 - 1; bdR = rng.next() * 2 - 1; }
            else if (body == 2) { double im = 0, pp = 0; if (rng.next() < dustP) { const double a = std::pow(double(rng.next()), 3) * (rng.next() < 0.5 ? -1 : 1); if (rng.next() < 0.08) pp = a * 3; else im = a; }
                const double pan = rng.next(), iL = im * (1 - pan) * 6, iR = im * pan * 6;
                dustL[0].tick(iL); dustL[1].tick(iL); dustR[0].tick(iR); dustR[1].tick(iR); popL.tick(pp * (1 - pan)); popR.tick(pp * pan);
                bdL = dustL[0].bp + dustL[1].bp + iL * 0.3 + popL.bp; bdR = dustR[0].bp + dustR[1].bp + iR * 0.3 + popR.bp; }
            else if (body == 3) {
                if (rng.next() < dropP) for (auto& d : drops) if (!d.on) { d.on = true; d.t = 0; d.ph = 0; d.f0 = (300 + rng.next() * 2200) * Pp.dropPitch; d.ch = 0.6 + rng.next() * 2; d.tc = 0.004 + rng.next() * 0.01; d.td = 0.006 + rng.next() * 0.02; d.a = 0.4 + rng.next() * 0.6; const double pn = rng.next(); d.pl = std::cos(pn * 1.5708); d.pr = std::sin(pn * 1.5708); break; }
                for (auto& d : drops) { if (!d.on) continue; d.t += 1 / sr; const double f = d.f0 * (1 + d.ch * (1 - std::exp(-d.t / d.tc))); d.ph += f / sr; const double en = d.a * std::exp(-d.t / d.td) * std::min(1.0, d.t * 4000);
                    const double s = std::sin(TAU * d.ph) * en; bdL += s * d.pl; bdR += s * d.pr; if (d.t > d.td * 7) d.on = false; } }
            else { bdL = fL * 2; bdR = fR * 2; }
            const double sk = skin == 0 ? (l + r) * 0.5 : skin == 1 ? dc : 0;
            scr += Pp.scrapeRate / sr; if (scr > 1) scr -= 1;
            for (int b = 0; b < 8; ++b) {
                double e = 1; if (skin != 2) { SVF& an = anS[b]; an.tick(sk); const double a = std::abs(an.bp) * 4; const double ev = env[b]; env[b] = ev + (a - ev) * (a > ev ? atk : relc); e = env[b]; }
                const double lvb = Pp.bands[b] * (dep > 0 ? 1 - dep * 0.5 * (1 + std::sin(TAU * (scr + b / 8.0))) : 1);
                const double g = e * lvb * bandNorm[b];
                syL[b].tick(bdL); syR[b].tick(bdR); wL += syL[b].bp * syL[b].k * g; wR += syR[b].bp * syR[b].k * g; }
            const double dry = (l * l + r * r) * 0.5, wet = (wL * wL + wR * wR) * 0.5;
            vDry += (dry - vDry) * 0.00007; vWet += (wet - vWet) * 0.00007;
            const bool autoL = Pp.vocAuto > 0.5f;
            if (autoL && vDry > 1e-8) vGain += (clampv(0.75 * std::sqrt(vDry / std::max(vWet, 1e-12)), 0.05, 40.0) - vGain) * 0.0005;
            const double vg = autoL ? vGain : 1; wL *= vg; wR *= vg;
            l = l * (1 - Pp.vocMix) + wL * Pp.vocMix; r = r * (1 - Pp.vocMix) + wR * Pp.vocMix;
        }
        // space
        double sL = 0, sR = 0;
        if (spMix > 0.001) {
            double xl = l * 0.5, xr = r * 0.5;
            for (auto& a : apL) { const double z = a.b[a.i], w = xl + z * 0.62; a.b[a.i] = float(w); xl = z - w * 0.62; a.i = (a.i + 1) % int(a.b.size()); }
            for (auto& a : apR) { const double z = a.b[a.i], w = xr + z * 0.62; a.b[a.i] = float(w); xr = z - w * 0.62; a.i = (a.i + 1) % int(a.b.size()); }
            modPh += 0.37 / sr; if (modPh > 1) modPh -= 1;
            double sum = 0;
            for (int qq = 0; qq < 8; ++qq) { Line& f = fdn[qq]; const int bl = int(f.buf.size()); double d = f.len; if (qq == 1) d += std::sin(TAU * modPh) * 6; if (qq == 6) d += std::cos(TAU * modPh * 1.3) * 7;
                double rpq = f.w - d; while (rpq < 0) rpq += bl; const int a0 = int(rpq); const double af = rpq - a0; const int b1 = (a0 + 1) % bl;
                double s = f.buf[a0] + af * (f.buf[b1] - f.buf[a0]); f.lp += (s - f.lp) * damp; s = f.lp * f.g; fo[qq] = s; sum += s; }
            sum *= 0.25;
            for (int qq = 0; qq < 8; ++qq) { Line& f = fdn[qq]; f.buf[f.w] = float(fo[qq] - sum + ((qq & 1) ? xr : xl)); f.w = (f.w + 1) % int(f.buf.size()); }
            sL = (fo[0] - fo[2] + fo[4] - fo[6]) * 0.5; sR = (fo[1] - fo[3] + fo[5] - fo[7]) * 0.5;
            l += sL * spMix; r += sR * spMix;
        }
        // listen point
        double oL = l, oR = r;
        switch (listen) { case 1: oL = t1L[i]; oR = t1R[i]; break; case 2: oL = t2L[i]; oR = t2R[i]; break; case 3: oL = bL[i]; oR = bR[i]; break;
            case 4: oL = wL; oR = wR; break; case 5: oL = fL; oR = fR; break; case 6: oL = sL; oR = sR; break; default: break; }
        if (listen) { const double ms = (oL * oL + oR * oR) * 0.5; agcR += (ms - agcR) * 0.00005;
            if (agcR > 1e-9) agc += (clampv(0.12 / std::sqrt(agcR), 0.25, 16.0) - agc) * 0.0002; oL *= agc; oR *= agc; }
        sumSq += (oL * oL + oR * oR) * 0.5;
        const double g = Pp.vol * Pp.abTrim; oL *= g; oR *= g;
        const double dl = oL - dcL0 + 0.9995 * dcL1; dcL0 = oL; dcL1 = dl;
        const double drr = oR - dcR0 + 0.9995 * dcR1; dcR0 = oR; dcR1 = drr;
        oL = ft(dl); oR = ft(drr);
        if (!(oL == oL) || !(oR == oR)) { oL = 0; oR = 0; dcL0 = dcL1 = dcR0 = dcR1 = 0; panicDSP(); }
        L[i] = float(oL); R[i] = float(oR);
        if (tap) { const int tp = int(Pp.tap); tap[i] = float(tp == 0 ? (oL + oR) * 0.5 : tp == 1 ? vt[i] : dc); }
    }
    if (act) outR = outR * 0.9 + 0.1 * (sumSq / n);
    meterRms = float(std::sqrt(outR)); activeVoices = act;
    if ((vizC += n) > sr / 30) { vizC = 0; Voice* v = lastV; VizSnapshot s; s.voices = act;
        if (v && v->active) { s.on = true; s.N = v->A.N;
            for (int i = 0; i < s.N; ++i) { s.th[i] = float(v->A.th[i]); double a = 0; for (int qq = 0; qq < 2; ++qq) if (v->A.on[i * 2 + qq]) a = std::max(a, v->A.rl[i * 2 + qq] ? v->A.rg[i * 2 + qq] : 1.0); s.e[i] = float(a); }
            s.R = float(v->A.R); s.stage = v->stage; s.f0 = float(v->A.inc[0] * sr); s.ff = float(v->A.ff * sr); for (int k = 0; k < NK; ++k) s.cur[k] = float(v->cur[k]); }
        viz = s; }
}

} // namespace silt
