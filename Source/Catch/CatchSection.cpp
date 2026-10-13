#include "CatchSection.h"
#include <cmath>
#include <algorithm>

namespace silt {

void CatchSection::prepare(double s) {
    sr = s;
    const int cap = int(120 * sr);
    if (!prepared || (int) A.size() != cap) { A.assign((size_t) cap, 0.f); B.assign((size_t) cap, 0.f); }
    roll.assign((size_t) (31 * sr), 0.f); rollW = 0; rollFilled = 0;
    scratchIn.assign(512, 0.f); scratchOut.assign(512, 0.f); catchTmp.assign(roll.size(), 0.f);
    eng.Init(float(sr), A.data(), B.data(), cap);
    ctl = neb::Controls {};
    ctl.src_mix = true; ctl.expo = true; ctl.bypass = false;
    recLatch = false; prepared = true;
}

void CatchSection::clear() { eng.ClearBuffer(); recLatch = false; }

void CatchSection::catchNow(const Params& P) {
    int len = std::min(rollFilled, int(P.ctCatchLen * sr));
    if (len < 256) return;
    // unwrap the last len samples of the rolling buffer
    std::vector<float>& tmp = catchTmp;
    const int cap = (int) roll.size();
    int start = rollW - len; while (start < 0) start += cap;
    for (int i = 0; i < len; ++i) tmp[(size_t) i] = roll[(size_t) ((start + i) % cap)];
    eng.rec().Load(tmp.data(), len);
    recLatch = false;
}

void CatchSection::process(const Params& P, const float* inL, const float* inR, float* outL, float* outR, int n, bool wetOnly) {
    // direct parameters, lightly smoothed (the pedal's pots had a 20 Hz smoother)
    d.start = P.ctStart; d.speed = P.ctSpeed; d.size = P.ctSize; d.dens = P.ctDens; d.pitch = P.ctPitch; d.blend = P.ctBlend;
    d.inlvl = P.ctDub; d.spray = P.ctSpray; d.ovl = P.ctOvl; d.outlvl = P.ctOut; d.windec = P.ctWinDec;
    d.bpf = P.ctBpf; d.bpq = P.ctBpq; d.win = P.ctWin; d.fsmix = P.ctFsMix; d.fshift = P.ctFshift; d.prand = P.ctPrand;
    d.rate = P.ctL1Rate; d.skew = P.ctL1Skew; d.rd1 = P.ctL1Read; d.sz1 = P.ctL1Size; d.bpf1 = P.ctL1Bpf; d.bl1 = P.ctL1Blend;
    d.rate2 = P.ctL2Rate; d.skew2 = P.ctL2Skew; d.sz2 = P.ctL2Size; d.bpf2 = P.ctL2Bpf; d.bl2 = P.ctL2Blend;
    const float a = 1 - std::exp(-float(n) * 2 * 3.14159265f * 20.f / float(sr));
    float* tf = &d.start; float* sf = &ds.start;
    const int nf = int(sizeof(neb::Direct) / sizeof(float));
    for (int i = 0; i < nf; ++i) sf[i] += (tf[i] - sf[i]) * a;
    // size and window decay rebuild tables or jump loops: take those without smoothing
    ds.windec = d.windec;
    eng.SetDirect(ds);
    ctl.src_mix = P.ctSrc > 0.5f; ctl.expo = P.ctExpo > 0.5f; ctl.freeze = P.ctFreeze > 0.5f; ctl.lfo2_div = P.ctL2Div > 0.5f;
    ctl.bypass = false; ctl.record = recLatch;

    float* mono = scratchIn.data(); float* out = scratchOut.data();
    const int cap = (int) roll.size();
    double ss = 0;
    for (int i = 0; i < n; ++i) {
        const float m = 0.5f * (inL[i] + inR[i]);
        mono[i] = m;
        roll[(size_t) rollW] = m; rollW = (rollW + 1) % cap; if (rollFilled < cap) ++rollFilled;
    }
    eng.Process(mono, out, n, ctl);
    // the engine auto-stops an overdub after one pass (or a fresh take at the buffer's end): follow it, as the pedal does
    if (recLatch && !eng.rec().recording()) recLatch = false;
    // keep Silt's stereo: subtract the dry part the engine mixed in, and add the stereo dry back
    float voc_g, dry_g, grain_g;
    neb::LawBlendLive(ds.blend, &voc_g, &dry_g, &grain_g);
    dryG = dry_g;
    for (int i = 0; i < n; ++i) {
        const float wet = out[i] - mono[i] * dry_g;
        outL[i] = wetOnly ? wet : inL[i] * dry_g + wet;
        outR[i] = wetOnly ? wet : inR[i] * dry_g + wet;
        ss += double(wet) * wet;
    }
    level = float(std::sqrt(ss / std::max(1, n)));
}

} // namespace silt
