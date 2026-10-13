#include "Feedback.h"
#include <cmath>
#include <algorithm>

namespace silt {

static constexpr double PI = 3.14159265358979323846;
static inline double ftb(double x) { if (x > 3) return 1; if (x < -3) return -1; const double x2 = x * x; return x * (27 + x2) / (27 + 9 * x2); }

void BusSVF::set(double fr, double q, double sr) {
    fr = std::clamp(fr, 10.0, sr * 0.45); const double g = std::tan(PI * fr / sr); k = 1 / q; a1 = 1 / (1 + g * (g + k)); a2 = g * a1; a3 = g * a2;
}

void FbBus::prepare(double s) {
    sr = s;
    len = int(std::ceil(sr * 1.6)); buf.assign((size_t) len, 0.f); w = 0; dtS = 0.2 * sr;
    comb.assign((size_t) (sr / 30) + 4, 0.f); cw = 0;
    const int sl[4] = { 113, 337, 557, 797 };
    for (int i = 0; i < 4; ++i) { smear[i].b.assign((size_t) std::ceil(sl[i] * 2 * sr / 48000.0) + 2, 0.f); smear[i].i = 0; }
    const double ci[4] = { 0.47940086558884, 0.87621849353931, 0.97659758950819, 0.99749925593555 };
    const double cq[4] = { 0.16175849836770, 0.73302893234149, 0.94534970032911, 0.99059915668453 };
    for (int k = 0; k < 4; ++k) { hi[k] = AP2(); hq[k] = AP2(); hi[k].c = ci[k]; hq[k].c = cq[k]; }
    reset();
}

void FbBus::reset() {
    std::fill(buf.begin(), buf.end(), 0.f); std::fill(comb.begin(), comb.end(), 0.f);
    for (auto& a : smear) std::fill(a.b.begin(), a.b.end(), 0.f);
    f.reset(); dcx = dcy = dc = dly2 = env = 0; combLp = 0; hz1 = 0; edgeGain = 1;
}

void FbBus::setup(const BusParams& B, double s) {
    sr = s;
    f.set(B.cut, B.q, sr);
    tgt = std::clamp(B.time / 1000.0 * sr, 2.0, len - 4.0);
    t3 = ftb(std::max(0.3, double(B.drive)));
}

double FbBus::insert(double x, const BusParams& B) {
    const int ins = int(B.ins); const double amt = B.insAmt, r = B.insRate;
    switch (ins) {
        case 1: { // comb: tuned 40 Hz .. 5 kHz, feedback from the amount
            const double fr = 40 * std::pow(2.0, r * 7);
            const int clen = (int) comb.size();
            double d = std::clamp(sr / fr, 2.0, clen - 2.0);
            double rp = cw - d; while (rp < 0) rp += clen;
            const int i0 = int(rp); const double fr0 = rp - i0;
            const double z = comb[(size_t) i0] + fr0 * (comb[(size_t) ((i0 + 1) % clen)] - comb[(size_t) i0]);
            combLp += (z - combLp) * 0.6;
            const double y = x + combLp * amt * 0.95;
            comb[(size_t) cw] = float(ftb(y)); cw = (cw + 1) % clen;
            return y * (1 - amt * 0.5);
        }
        case 2: { // smear: allpass chain, length from tune, diffusion from amount
            const double g = amt * 0.75, scale = 0.2 + r * 1.8;
            double y = x;
            for (auto& a : smear) {
                const int L = std::max(2, std::min((int) a.b.size() - 1, int((a.b.size() - 2) / 2 * scale)));
                const int ri = (a.i - L + (int) a.b.size()) % (int) a.b.size();
                const double z = a.b[(size_t) ri], v = y + z * g;
                a.b[(size_t) a.i] = float(v); y = z - v * g; a.i = (a.i + 1) % (int) a.b.size();
            }
            return y;
        }
        case 3: { // frequency shift, -200..+200 Hz, centre dead; amount is the wet mix
            const double c = (r - 0.5) * 2, ac = std::abs(c);
            double hz = 0; if (ac > 0.03) { hz = 0.5 * std::pow(400.0, (ac - 0.03) / 0.97); if (c < 0) hz = -hz; }
            double ai = x, aq = x;
            for (auto& a : hi) ai = a.P(ai);
            for (auto& a : hq) aq = a.P(aq);
            const double aI = hz1; hz1 = ai;
            shPh += hz / sr; shPh -= std::floor(shPh);
            const double s = aI * std::cos(2 * PI * shPh) + aq * std::sin(2 * PI * shPh);
            return x * (1 - amt) + s * amt;
        }
        case 4: { // fold
            const double g = 1 + amt * 8 * (0.5 + r);
            return x * (1 - amt) + std::sin(x * g) * amt;
        }
        case 5: { // crush: sample hold from tune, bits from amount
            const int hold = 1 + int(r * r * 40);
            if (++crushC >= hold) { crushC = 0; crushHold = x; }
            const double q = std::pow(2.0, 15 - amt * 13);
            return std::floor(crushHold * q + 0.5) / q;
        }
        default: return x;
    }
}

double FbBus::tick(double in, const BusParams& B) {
    dtS += (tgt - dtS) * 0.0008;
    double rp = w - dtS; while (rp < 0) rp += len;
    const int i0 = int(rp); const double fr = rp - i0;
    const double dly = buf[(size_t) i0] * (1 - fr) + buf[(size_t) ((i0 + 1) % len)] * fr;
    double rp2 = w - dtS * 1.07; while (rp2 < 0) rp2 += len;
    const int j0 = int(rp2);
    dly2 = buf[(size_t) j0] + (rp2 - j0) * (buf[(size_t) ((j0 + 1) % len)] - buf[(size_t) j0]);
    const int type = int(B.type);
    double ff = dly;
    if (type <= 2) { f.tick(dly); ff = type == 0 ? f.lp : type == 1 ? f.bp * f.k : f.hp; }
    ff = insert(ff, B);
    const double y = ftb(ff * B.drive) / t3;
    const double d = y - dcx + 0.996 * dcy; dcx = y; dcy = d; dc = d;
    const double a = std::abs(d);
    env += (a - env) * (a > env ? 0.01 : 0.0005);
    double regen = B.regen;
    if (B.edge > 0.5f) { // hold the loop just under runaway: pull regeneration down as the level climbs
        const double target = env > 0.35 ? 0.35 / env : 1.0;
        edgeGain += (target - edgeGain) * (target < edgeGain ? 0.01 : 0.0002);
        regen *= edgeGain;
    } else edgeGain = 1;
    buf[(size_t) w] = float(ftb(in + d * regen)); w = (w + 1) % len;
    return d;
}

} // namespace silt
