// Feedback network: buses with a tap, a delay, a filter, an insert slot, drive and a return.
#pragma once
#include "SiltParams.h"
#include <vector>

namespace silt {

struct BusSVF {
    double a = 0, b = 0, k = 1, a1 = 1, a2 = 0, a3 = 0, lp = 0, bp = 0, hp = 0;
    void set(double f, double q, double sr);
    inline void tick(double x) { const double v3 = x - b, v1 = a1 * a + a2 * v3, v2 = b + a2 * a + a3 * v3;
        a = 2 * v1 - a; b = 2 * v2 - b; lp = v2; bp = v1; hp = x - k * v1 - v2; }
    void reset() { a = b = 0; }
};

class FbBus {
public:
    void prepare(double sr);
    void reset();
    // Per block: coefficients. Per sample: process one input, returns the loop signal.
    void setup(const BusParams& B, double sr);
    double tick(double in, const BusParams& B);
    double dc = 0, dly2 = 0;          // loop signal and the second tap (for width)
    double env = 0;                   // level, for the mod matrix and the edge guard
private:
    double insert(double x, const BusParams& B);
    double sr = 48000;
    std::vector<float> buf; int len = 0, w = 0; double dtS = 0, tgt = 0, t3 = 1;
    BusSVF f; double dcx = 0, dcy = 0;
    // inserts
    std::vector<float> comb; int cw = 0; double combLp = 0;
    struct AP { std::vector<float> b; int i = 0; } smear[4];
    struct AP2 { double c = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0; inline double P(double x) { const double y = c * x - x2 + c * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; } } hi[4], hq[4];
    double hz1 = 0, shPh = 0;
    double crushHold = 0; int crushC = 0;
    double edgeGain = 1;
};

} // namespace silt
