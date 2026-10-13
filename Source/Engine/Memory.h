// Memory: sediment and erosion (slow levels that remember playing), the regeneration buffer
// (a long, degrading audio memory) and microcuts (slivers of the output looped and stuttered).
#pragma once
#include "SiltParams.h"
#include <vector>
#include <cstdint>

namespace silt {

class Sediment {
public:
    // input: 0..1-ish activity this tick, notes: note-ons this tick
    void tick(const Params& P, double activity, int notes, double outEnergy, double dt);
    void clear() { level = 0; }
    void restore() { erosion = 0; }
    float level = 0, erosion = 0;
};

class RegenBuffer {
public:
    void prepare(double sr);
    void reset();
    void tick(double inL, double inR, const Params& P, double& outL, double& outR);
    void setup(const Params& P);
private:
    double sr = 48000; std::vector<float> L, R; int w = 0, cap = 0;
    double lpL = 0, lpR = 0, lpA = 0.5, inEnv = 0, stir = 0, wowPh = 0, lenS = 0;
    int holdC = 0; double hL = 0, hR = 0; uint32_t ns = 0x1234567u;
};

class Microcuts {
public:
    void prepare(double sr);
    void reset();
    void write(double l, double r) { L[(size_t) w] = float(l); R[(size_t) w] = float(r); w = (w + 1) % cap; }
    void trigger(const Params& P, float r1, float r2);
    // returns fragment sample; duck is how much the dry should be ducked (Cut mode)
    void tick(const Params& P, double& outL, double& outR, double& duck);
    bool active() const { return left > 0; }
private:
    double sr = 48000; std::vector<float> L, R; int w = 0, cap = 0;
    double start = 0; int fragLen = 0, pos = 0, left = 0, fade = 32; bool rev = false;
};

} // namespace silt
