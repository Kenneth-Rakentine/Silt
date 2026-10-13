// Hover help for the controls. Plain descriptions of what each one does.
#pragma once
#include <juce_core/juce_core.h>
#include <cctype>
#include <map>
#include <string>

inline const char* helpFor(const std::string& id) {
    static const std::map<std::string, const char*> h = {
        // layers
        { "On", "Turns this layer of particle emitters on or off." },
        { "Mat", "What each particle is made of: a slice of wavetable, or noise." },
        { "Bank", "Which family of wavetables the particles read from." },
        { "Pos", "Position in the bank: sweeps between the eight waves." },
        { "Form", "Formant: how fast the wave plays inside each particle, as a ratio of the note. Higher sounds brighter and more vocal, like a resonance that moves with the note." },
        { "Cyc", "Cycles of the wave per particle. Long particles join into a steady tone; short ones become pulses and grains." },
        { "Win", "The particle's envelope: square (Rect), smooth (Hann) or a sharp click with a tail (Expodec)." },
        { "Phase", "Where in the wave each particle starts. Changes the attack of the transient." },
        { "N", "How many emitters in the cluster. More emitters thicken and detune the sound." },
        { "Det", "Spreads the emitters' pitches apart." },
        { "Wander", "Lets each emitter's pitch drift on its own." },
        { "Pan", "Spreads the emitters across the stereo field." },
        { "Lvl", "Level of this layer in the mix." },
        { "bSemi", "Layer B's pitch in semitones relative to A." },
        { "bFine", "Fine tuning of layer B, in cents. A few cents makes slow beating against A." },
        { "bSync", "Restarts layer B every time layer A emits: hard sync." },
        // voice
        { "pmAmt", "Layer B modulates the phase of layer A (FM). Adds metallic and bell-like overtones." },
        { "formTrack", "How much the formant follows the note. At 0 the formant stays fixed (a vowel); at 100% it moves with pitch." },
        { "wanderRate", "How fast the emitters wander." },
        { "derez", "Lowers the digital resolution: fewer bits and a lower sample rate. Crunch and glassy aliasing." },
        { "drift", "Slow analog-style drift of pitch and cutoff." },
        { "resTune", "Tuning of the modal resonator, relative to the note." },
        { "resDecay", "How long the resonator rings." },
        { "strike", "A noise hit into the resonator at the start of each note." },
        // filter
        { "fCut", "Filter cutoff. Moves the whole filter up or down; the articulation states shape the movement around it." },
        { "fMode", "Filter type: a 24 dB ladder, or a 12 dB state-variable low pass, band pass or high pass." },
        { "reso", "Filter resonance." },
        { "drive", "Drives the signal into the filter for saturation." },
        { "heat", "Saturation at the amplifier, after the filter. Thickens and rounds." },
        { "keytrack", "How much the cutoff follows the keyboard." },
        { "velCut", "How much harder playing opens the filter." },
        { "velAmp", "How much velocity changes the level." },
        // macros
        { "mass", "Mass: shifts the formant, vocoder and filter together. Heavier or lighter material." },
        { "grainM", "Grain: shorter, sharper particles one way; longer, smoother ones the other." },
        { "allure", "Flux: adds movement everywhere at once: wander, scatter, scrape and restlessness. The mod wheel plays it." },
        { "vol", "Output volume." },
        // articulation
        { "atk", "Attack: how long the sound takes to reach the Onset state after a key goes down." },
        { "dec", "Decay: how long it takes to move from Onset to the Continuant state, where it rests while the key is held." },
        { "rel", "Release: how long it takes to reach the Termination state after the key comes up." },
        { "tail", "Tail: how long the sound rings at the Termination state before fading. Auto ties it to the Release." },
        { "curve", "Shape of every move between states: negative eases in slowly, positive jumps then settles." },
        { "loop", "While the key is held, cycle between Onset and Continuant instead of resting." },
        { "loopTime", "Time for each half of the loop." },
        { "auth", "Authority: 0 follows the path exactly, like an envelope. Higher lets the springs take over, so the sound overshoots and wobbles on its way." },
        { "stiff", "Spring stiffness: how quickly the physics pulls toward the target." },
        { "visc", "Viscosity: damping. Low wobbles and rings; high moves like syrup." },
        { "temp", "Temperature: random jitter in every field." },
        { "poly", "How many notes can play at once." },
        // vocoder
        { "vocOn", "Turns the material vocoder on." },
        { "vocBody", "Body: the material the sound is made of. The vocoder plays your voice's spectrum with this source." },
        { "vocSkin", "Skin: whose spectrum is imposed on the body. Fixed uses only the band levels below." },
        { "vocMix", "Blend between the dry voice and the vocoder." },
        { "vocShift", "Shifts the output bands up or down, in octaves." },
        { "vocSpread", "Spacing between the bands." },
        { "vocQ", "Band width. High values ring like drips." },
        { "vocResp", "Response: fast tracks every detail; slow smears into liquid." },
        { "scrapeRate", "Speed of the scrape: a sweep that rotates through the bands." },
        { "scrapeDepth", "Depth of the scrape. Ripples at slow rates, buzz at fast ones." },
        { "dustRate", "Density of the dust crackle (Body: Dust)." },
        { "dropRate", "Density of the droplets (Body: Droplets)." },
        { "dropPitch", "Pitch of the droplets." },
        { "vocAuto", "Keeps the vocoder's output level matched to the dry voice." },
        // space
        { "spSize", "Size of the space." }, { "spDecay", "Decay time of the space." }, { "spTone", "Brightness of the space." }, { "spMix", "How much space." },
        // feedback
        { "BusOn", "Turns this feedback bus on." },
        { "BusEdge", "Edge guard: holds the loop just below runaway, so high regeneration stays playable." },
        { "BusTap", "Where the bus listens: the voices' feedback send (set per state), a point in the voice, the vocoder, the space, another bus, or the output." },
        { "BusSend", "How much of the tap enters the loop." },
        { "BusTime", "Delay time inside the loop. Short times make pitch and body; long times make echoes." },
        { "BusType", "Filter inside the loop." },
        { "BusIns", "A processor inside the loop: comb (tuned pinging), smear (diffusion), frequency shift (barber-pole spirals), fold, or crush." },
        { "BusInsAmt", "Amount of the insert." },
        { "BusInsRate", "Tuning of the insert: comb pitch, smear length, shift amount (centre is none), fold character, crush rate." },
        { "BusDrive", "Saturation inside the loop." },
        { "BusRegen", "Regeneration: how much of the loop feeds back. Above 100% it self-oscillates." },
        { "BusMix", "How much of the loop you hear." },
        { "BusDest", "Where the loop returns: the main mix, back into the voices (as an exciter before the filter), or both." },
        { "BusWidth", "Stereo width of the return." },
        // memory
        { "sedSrc", "What leaves sediment: the output level, note-ons, voice level, or feedback." },
        { "sedAmt", "How fast sediment builds up." },
        { "sedDecay", "How long sediment takes to settle away." },
        { "sedFreeze", "Holds the sediment level where it is." },
        { "eroRate", "How fast playing wears the material down." },
        { "eroTarget", "What wears: the high partials (darker), density (more gaps), or body (thinner)." },
        { "eroDepth", "How far erosion can go." },
        { "eroRecover", "How long the material takes to grow back." },
        { "rgOn", "A long audio memory of the voice that degrades on every pass." },
        { "rgLen", "Length of the memory loop." }, { "rgIn", "How much new sound goes in." }, { "rgFb", "How much survives each pass." },
        { "rgTone", "Each pass loses the highs above this." }, { "rgDegrade", "Resolution loss and noise on each pass." },
        { "rgDrift", "Wow: tape-like pitch drift." }, { "rgStir", "Playing stirs the memory back up: the louder you play, the more you hear it." }, { "rgLevel", "Memory level." },
        { "mcOn", "Microcuts: slivers of the output, looped and stuttered." },
        { "mcTrig", "What triggers a cut: clock pulses, sequencer steps marked for cuts, both, or random." },
        { "mcProb", "Chance of a cut on each trigger." }, { "mcSize", "Length of each sliver." }, { "mcRepeats", "How many times a sliver repeats." },
        { "mcRev", "Chance a sliver plays backwards." }, { "mcSpan", "How far back in time a sliver can come from." },
        { "mcEdge", "Edge: soft fades or hard clicky cuts." }, { "mcMode", "Blend adds the cut; Cut replaces the sound while it plays." }, { "mcLevel", "Level of the cuts." },
        // movement
        { "clkRun", "Starts and stops the master clock." }, { "clkBpm", "Tempo." }, { "clkDiv", "Pulse length: how many pulses per beat." },
        { "clkShape", "Oscillator A's wave: square is straight time; skewed shapes swing; sine with a threshold stretches the gaps." },
        { "clkThr", "Where on A's wave a pulse fires. Away from the centre the gaps become uneven." },
        { "clkBRate", "Speed of oscillator B." }, { "clkBShape", "Oscillator B's wave." },
        { "clkMode", "How B bends A: rate (tempo waves), shape (moving swing), phase (push and pull), sum (composite rhythm) or logic (AND, OR, XOR of two pulse trains)." },
        { "clkDepth", "How much B bends A." }, { "clkAnchor", "Locked keeps the average tempo; Free lets it drift." },
        { "lfo1Rate", "LFO 1 speed." }, { "lfo2Rate", "LFO 2 speed." }, { "lfo1Shape", "LFO 1 wave." }, { "lfo2Shape", "LFO 2 wave." },
        { "dustModRate", "Dust: random impulses as a modulation source. How many per second." }, { "dustModDecay", "How long each dust impulse lasts." },
        { "seqOn", "Runs the graph sequencer on the master clock." },
        { "seqKeys", "Transpose: keys move the sequence. Play over: keys play normally on top." },
        { "seqLen", "Steps used when a step has no cord patched." }, { "seqHeads", "One or two playheads moving through the same graph." },
        { "HeadShape", "Traversal shape: changes the playhead's speed over its cycle. Rising accelerates into a roll; a spike is a ratchet burst." },
        { "HeadDepth", "How far the shape changes the speed, in octaves." }, { "HeadCycle", "How many steps one pass of the shape lasts." },
        { "HeadBranch", "At a step with two cords: choose by chance, or by the shape's level." },
        { "HeadRate", "Speed relative to the clock." }, { "HeadStart", "Step where this playhead starts." }, { "HeadTrans", "Transposes this playhead's notes." },
        // catch
        { "ctMode", "Bypass; Insert at the end of the chain; Send a share of the mix and return it; or Catch: always listening, so you can grab the last few seconds." },
        { "ctSend", "How much of Silt goes into the section (Send mode)." }, { "ctReturn", "How much of the section comes back." },
        { "ctSrc", "Record the input only, or input plus playback (sound on sound)." }, { "ctCatchLen", "How many seconds the Catch button grabs." },
        { "ctFreeze", "Holds the playhead: freeze." }, { "ctExpo", "Uses the expodec grain window instead of the ramp." },
        { "ctStart", "Start of the loop in the buffer." }, { "ctSpeed", "Playback speed. Centre is normal; it goes backward below." },
        { "ctSize", "Loop size." }, { "ctDens", "Grain density." }, { "ctPitch", "Pitch. Centre is unchanged." },
        { "ctBlend", "Blend between the phase vocoder, dry and the granular engine." },
    };
    auto it = h.find(id); if (it != h.end()) return it->second;
    // layer and bus parameters share descriptions
    auto tryKey = [&](const std::string& k) -> const char* { auto j = h.find(k); return j == h.end() ? nullptr : j->second; };
    if (id.size() > 1 && (id[0] == 'a' || id[0] == 'b') && std::isupper((unsigned char) id[1])) if (auto* t = tryKey(id.substr(1))) return t;
    for (const char* pfx : { "fb", "f2", "f3" }) if (id.rfind(pfx, 0) == 0) if (auto* t = tryKey("Bus" + id.substr(2))) return t;
    if (id.size() > 2 && id[0] == 'h' && std::isdigit((unsigned char) id[1])) if (auto* t = tryKey("Head" + id.substr(2))) return t;
    return "";
}
