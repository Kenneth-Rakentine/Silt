// Catch: the Nebulae v2.0 build 2 engine as a section of Silt. Routing is Cocoquantus-style:
// bypass, insert at the end of the chain, send/return, or catch (always listening; one button
// grabs the last few seconds and loops them).
#pragma once
#include "../Engine/SiltParams.h"
#include "nebulae_engine.h"
#include <vector>

namespace silt {

class CatchSection {
public:
    void prepare(double sr);
    // Processes a block in place on L/R (insert) or returns the wet signal in wetL/wetR (send, catch).
    // in: the signal the section listens to. Max block 512.
    void process(const Params& P, const float* inL, const float* inR, float* outL, float* outR, int n, bool wetOnly);
    // actions (call from the audio thread)
    void catchNow(const Params& P);
    void toggleRecord() { recLatch = !recLatch; }
    void clear();
    // state for the editor
    bool recording() { return eng.rec().recording(); }
    float dryGain() const { return dryG; }
    bool hasLoop() { return eng.rec().length() > 0; }
    float loopSeconds() { return eng.rec().length() / float(sr); }
    float level = 0, dryG = 1;
private:
    double sr = 48000;
    neb::Engine eng;
    neb::Controls ctl {};
    neb::Direct d, ds;      // target and smoothed
    std::vector<float> A, B, roll, scratchIn, scratchOut, catchTmp;
    int rollW = 0, rollFilled = 0;
    bool recLatch = false, prepared = false;
};

} // namespace silt
