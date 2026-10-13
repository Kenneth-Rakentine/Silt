#include "Memory.h"
#include <cmath>
#include <algorithm>

namespace silt {

static inline double ftm(double x) { if (x > 3) return 1; if (x < -3) return -1; const double x2 = x * x; return x * (27 + x2) / (27 + 9 * x2); }

void Sediment::tick(const Params& P, double activity, int notes, double outEnergy, double dt) {
    if (P.sedFreeze < 0.5f) {
        const double gain = P.sedAmt * 2.0;
        level += float((gain * activity * (1 - level)) * dt - level * dt / std::max(0.5, double(P.sedDecay)));
        level = std::clamp(level, 0.f, 1.f);
    }
    // erosion grows with playing (note-ons and output energy) and slowly recovers
    const double wear = P.eroRate * (notes * 0.08 + std::min(1.0, outEnergy * 6) * dt * 0.25);
    erosion += float(wear * (1 - erosion) - erosion * dt / std::max(1.0, double(P.eroRecover)));
    erosion = std::clamp(erosion, 0.f, 1.f);
}

// ---------------- regeneration ----------------
void RegenBuffer::prepare(double s) { sr = s; cap = int(sr * 16.5); L.assign((size_t) cap, 0.f); R.assign((size_t) cap, 0.f); reset(); }
void RegenBuffer::reset() { std::fill(L.begin(), L.end(), 0.f); std::fill(R.begin(), R.end(), 0.f); w = 0; lpL = lpR = inEnv = stir = 0; lenS = 0; }
void RegenBuffer::setup(const Params& P) {
    lpA = 1 - std::exp(-2 * 3.14159265358979 * P.rgTone / sr);
    const double target = std::clamp(P.rgLen * sr, 64.0, cap - 1024.0);
    if (lenS <= 0) lenS = target;
    lenS += (target - lenS) * 0.02; // glide when the length moves, like tape
}
void RegenBuffer::tick(double inL, double inR, const Params& P, double& outL, double& outR) {
    wowPh += (0.5 + P.rgDrift * 2.5) / sr; if (wowPh >= 1) wowPh -= 1;
    const double wow = std::sin(6.283185307 * wowPh) * P.rgDrift * 0.004 * sr;
    double rp = w - lenS - wow; while (rp < 0) rp += cap;
    const int i0 = int(rp); const double fr = rp - i0; const int i1 = (i0 + 1) % cap;
    double yL = L[(size_t) i0] + fr * (L[(size_t) i1] - L[(size_t) i0]);
    double yR = R[(size_t) i0] + fr * (R[(size_t) i1] - R[(size_t) i0]);
    // degrade on every pass: tone, resolution, a little noise and saturation
    lpL += (yL - lpL) * lpA; lpR += (yR - lpR) * lpA; yL = lpL; yR = lpR;
    const double dg = P.rgDegrade;
    if (dg > 0.01) {
        const int hold = 1 + int(dg * dg * 6);
        if (++holdC >= hold) { holdC = 0; hL = yL; hR = yR; }
        const double q = std::pow(2.0, 14 - dg * 9);
        yL = std::floor(hL * q + 0.5) / q; yR = std::floor(hR * q + 0.5) / q;
        ns ^= ns << 13; ns ^= ns >> 17; ns ^= ns << 5;
        const double n = ((ns >> 8) * (1.0 / 16777216.0) - 0.5) * dg * 0.004;
        yL += n; yR += n;
    }
    const double in = std::abs(inL) + std::abs(inR);
    inEnv += (in - inEnv) * (in > inEnv ? 0.01 : 0.00005);
    L[(size_t) w] = float(ftm(inL * P.rgIn + yL * P.rgFb));
    R[(size_t) w] = float(ftm(inR * P.rgIn + yR * P.rgFb));
    w = (w + 1) % cap;
    // stirring: new playing lifts the memory back up
    const double target = (1 - P.rgStir) + P.rgStir * std::min(1.0, inEnv * 4);
    stir += (target - stir) * 0.0005;
    outL = yL * P.rgLevel * stir; outR = yR * P.rgLevel * stir;
}

// ---------------- microcuts ----------------
void Microcuts::prepare(double s) { sr = s; cap = int(sr * 2.6); L.assign((size_t) cap, 0.f); R.assign((size_t) cap, 0.f); reset(); }
void Microcuts::reset() { std::fill(L.begin(), L.end(), 0.f); std::fill(R.begin(), R.end(), 0.f); w = 0; left = 0; }
void Microcuts::trigger(const Params& P, float r1, float r2) {
    fragLen = std::max(32, int(P.mcSize / 1000.0 * sr));
    const int reach = std::min(cap - fragLen - 8, int(P.mcSpan * sr));
    const int back = fragLen + int(r1 * std::max(0, reach - fragLen));
    start = w - back; while (start < 0) start += cap;
    rev = r2 < P.mcRev;
    pos = 0; left = fragLen * std::clamp(int(P.mcRepeats), 1, 16);
    fade = std::max(16, int((1 - P.mcEdge) * 0.5 * fragLen + P.mcEdge * 24));
    fade = std::min(fade, fragLen / 2);
}
void Microcuts::tick(const Params& P, double& oL, double& oR, double& duck) {
    oL = oR = 0; duck = 0;
    if (left <= 0) return;
    const int p = rev ? fragLen - 1 - pos : pos;
    const int idx = (int(start) + p) % cap;
    const double e = std::min({ 1.0, (pos + 1) / double(fade), (fragLen - pos) / double(fade) });
    oL = L[(size_t) idx] * e * P.mcLevel; oR = R[(size_t) idx] * e * P.mcLevel;
    duck = P.mcMode > 0.5f ? P.mcLevel : 0;
    if (++pos >= fragLen) pos = 0;
    --left;
}

} // namespace silt
