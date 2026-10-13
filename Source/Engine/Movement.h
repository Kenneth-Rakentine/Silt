// Movement: the two-oscillator master clock, modulation LFOs, dust, and the graph sequencer.
#pragma once
#include "SiltParams.h"
#include <cstdint>

namespace silt {

struct MRng { uint32_t s = 0x2545F491u;
    inline float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s >> 8) * (1.0f / 16777216.0f); }
    inline float bi() { return next() * 2 - 1; } };

// Clock: oscillator A makes the pulse where its wave crosses the threshold (rising through +thr,
// falling through -thr, so a symmetric wave at thr 0 gives straight time and two pulses per cycle).
// Oscillator B bends A by rate, shape, phase, sum or logic.
class Clock {
public:
    void reset() { phA = phB = 0; wPrev = -1; gatePrev = false; drift = 1; }
    // Advances n samples; returns the number of pulses fired.
    int tick(const Params& P, int n, double sr, MRng& rng);
    float valA = 0, valB = 0, env = 0, rnd = 0;   // mod sources
    double pulsePeriod = 0.136;                  // seconds between pulses at the current rate
    static float wave(double ph, double shape);
private:
    double phA = 0, phB = 0, wPrev = -1, drift = 1; bool gatePrev = false;
};

class ModLfo {
public:
    float tick(double rate, int shape, double dt, MRng& rng);
    float value = 0;
private:
    double ph = 0; float held = 0, from = 0, to = 0;
};

class DustMod {
public:
    float tick(double rate, double decay, double dt, MRng& rng);
    float value = 0;
};

// Graph sequencer. Steps are nodes; each step has up to two outgoing cords (unpatched = next step).
// Each playhead moves at the clock's pulse rate times its own rate and its traversal shape.
struct SeqEvent { bool on; int note; float vel; };
class Sequencer {
public:
    void reset();
    // pulses: clock pulses this tick; dt: tick length. Fills out events (max 16).
    void tick(const Params& P, int pulses, double pulsePeriod, double dt, int root, MRng& rng, SeqEvent* ev, int& nev);
    void allOff(SeqEvent* ev, int& nev);
    float value = 0, gate = 0;
    bool cutTrigger = false;          // a step with the microcut flag fired this tick
    int cur[NHEAD] { -1, -1 };        // for the editor
    bool running = false;
private:
    struct Head { int step = -1; double phase = 0, travelled = 0; bool fresh = true; };
    struct Held { int note = -1; double left = 0; bool head0 = false; };
    void advance(const Params& P, int h, double stepDur, int root, MRng& rng, SeqEvent* ev, int& nev);
    float shapeAt(const Params& P, int h) const;
    Head heads[NHEAD];
    Held held[32];
};

} // namespace silt
