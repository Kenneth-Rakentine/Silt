// ===================== SILT patches (shared by lab UI and test harness) =====================
const BANDS = { Flat: [.8,.8,.8,.8,.8,.8,.8,.8], Water: [.12,.08,.05,.1,.25,.85,.35,1], Fire: [.75,.35,.2,.45,.55,.8,.6,.9], Glass: [0,.02,.1,.25,.1,.7,1,.85], Gravel: [.9,.85,.65,.5,.3,.18,.1,.05] };
const BASE = {
  poly: 6,
  aOn: 1, aMat: 0, aBank: 0, aPos: 3 / 7, aForm: 1, aCyc: 1, aWin: 0, aPhase: 0, aN: 1, aDet: 6, aWander: 0, aLvl: 0.8, aPan: 0.5,
  bOn: 0, bMat: 0, bBank: 0, bPos: 3 / 7, bForm: 1, bCyc: 1, bWin: 0, bPhase: 0, bN: 1, bDet: 6, bWander: 0, bLvl: 0.6, bPan: 0.5, bSemi: 0, bFine: 0, bSync: 0,
  pmAmt: 0, formTrack: 1, wanderRate: 0.3,
  derez: 0, drift: 0.25, resTune: 1, resDecay: 0.5, strike: 0.3,
  fMode: 0, reso: 0.2, drive: 0.2, heat: 0.25, keytrack: 0.5, velCut: 0.3, velAmp: 0.5,
  atk: 0.005, dec: 0.6, rel: 0.5, curve: 0.5, loop: 0, loopTime: 1.5, auth: 0, stiff: 6, visc: 0.8, temp: 0,
  mass: 0, grainM: 0, allure: 0,
  vocOn: 0, vocBody: 1, vocSkin: 0, bands: BANDS.Flat.slice(), vocShift: 0, vocSpread: 1, vocQ: 5, vocResp: 0.2, scrapeRate: 1, scrapeDepth: 0, vocMix: 0.5, vocAuto: 1,
  dustRate: 400, dropRate: 60, dropPitch: 1,
  fbTime: 240, fbType: 1, fbCut: 1500, fbQ: 2, fbDrive: 1.2, fbRegen: 0.3, fbMix: 0,
  spSize: 1, spDecay: 2.5, spTone: 0.5, spMix: 0.15,
  vol: 0.9, listen: 0, tap: 1, abTrim: 1
};
const ST = (level, pitch, formant, position, coupling, mask, scatter, fold, cutoff, resonator, ring, feedback) =>
  ({ level, pitch, formant, position, coupling, mask, scatter, fold, cutoff, resonator, ring, feedback });
const fr = i => i / 7; // wavetable frame -> position
const PATCHES = {
  'Init: saw': { P: { vol: 1.5 }, S: [ST(1, 0, 0, 0, 0, 0, 0, 0, 6000, 0, 0, 0), ST(0.8, 0, 0, 0, 0, 0, 0, 0, 2500, 0, 0, 0), ST(0, 0, 0, 0, 0, 0, 0, 0, 1200, 0, 0, 0)],
    notes: { target: 'A plain saw through the ladder filter. A clean reference for comparing everything else against.', listen: 'Nothing special. Use it to check your levels and the keyboard.', decides: 'Nothing; it is the control sample.', play: 'Any notes.' } },

  '1 · Weight (VAST fuzz)': {
    P: { aBank: 1, aPos: fr(2), aN: 2, aDet: 7, vol: 0.5, aLvl: 0.75, bOn: 1, bBank: 1, bPos: fr(5), bSemi: -12, bLvl: 0.45, pmAmt: 0.08,
      derez: 0.25, drift: 0.3, reso: 0.15, drive: 0.3, heat: 0.4, atk: 0.12, dec: 1.2, rel: 1.2, curve: 0.3, spMix: 0.2, spSize: 1.2 },
    S: [ST(1, 0, 0, 0, 0, 0, 0, 0, 3200, 0, 0, 0), ST(0.85, 0, 0, 0, 0, 0, 0, 0, 1800, 0, 0, 0), ST(0, 0, 0, 0, 0, 0, 0, 0, 900, 0, 0, 0)],
    notes: { target: '"Soft fuzzy hollow digital 90s waveforms that have weight".',
      listen: 'Weight in the low mids under a fizzy top that never turns thin or brittle.',
      decides: 'Whether band-limited hollow tables, light derez and a saturating analog-style filter give that weight, or the oscillator core needs to change.',
      play: 'Slow chords around C2 to C4. Sweep Derez from 0.15 to 0.45, then compare Listen at "Pre-filter" with "Voices".' } },

  '2 · Glass shards (M)': {
    P: { vol: 0.6, aBank: 4, aPos: fr(4), aN: 3, aDet: 10, aPan: 0.8, derez: 0.55, reso: 0.55, drive: 0.25, heat: 0.3, loop: 1, loopTime: 2.4,
      atk: 0.02, dec: 1.4, rel: 2, curve: 0.2, spMix: 0.3, spDecay: 3.5, drift: 0.3 },
    S: [ST(0.9, 0, 0, 0.15, 0, 0, 0.12, 0, 3500, 0, 0, 0), ST(0.8, 0, 0, 0, 0, 0, 0.06, 0, 1100, 0, 0, 0), ST(0, 0, 0, 0.1, 0, 0, 0.2, 0, 700, 0, 0, 0)],
    notes: { target: '"Glistening glass shards of particulate when low resolution bits are crushed and then filtered through the underwater prism of that analog filter".',
      listen: 'Bright crushed sparkle that is rounded off by a resonant low-pass, with the loop slowly moving the filter like a wave envelope.',
      decides: 'Whether derez before a resonant analog-style filter is the right order, and how much resonance the "prism" needs.',
      play: 'Hold one chord. Sweep Derez and Resonance. Switch Listen to "Pre-filter" to hear the raw shards.' } },

  '3 · M ring recipe': {
    P: { vol: 0.63, aBank: 2, aPos: fr(3), aLvl: 0.12, aPhase: 0, bOn: 1, bBank: 2, bPos: fr(3), bLvl: 0.12, bFine: 3, derez: 0.3, reso: 0.25, drive: 0.25, heat: 0.3,
      loop: 1, loopTime: 0.9, atk: 0.003, dec: 0.35, rel: 0.8, curve: 0.6, spMix: 0.15 },
    S: [ST(1, 0, 0, 0, 0, 0, 0, 0, 4200, 0, 0.85, 0), ST(0.8, 0, 0, 0, 0, 0, 0, 0, 1500, 0, 0.85, 0), ST(0, 0, 0, 0, 0, 0, 0, 0, 800, 0, 0.85, 0)],
    notes: { target: 'A Waldorf M recipe: both oscillator levels down, ring mod up, a sax-like table, phase set where the transient smacks hardest, oscillator 2 detuned until it just beats, derez until it comes alive, and a looping envelope moving the filter.',
      listen: 'The slow beating from oscillator B, the bite of the transient, and the moment derez makes it come alive.',
      decides: 'Whether the reed tables and ring mixer can stand in for the M recipe, and whether the phase control gives the transient you expect.',
      play: 'Sweep Layer A Phase to find the hardest transient. Set B Fine between 1 and 6 cents. Raise Derez slowly. Try FM (B to A) instead of ring.' } },

  '4 · Transient with meat': {
    P: { vol: 2.2, aBank: 0, aPos: fr(3), bOn: 1, bBank: 0, bPos: fr(4), bSemi: -12, bLvl: 0.35, reso: 0.3, drive: 0.5, heat: 0.45, resTune: 1, resDecay: 0.25, strike: 0.5,
      atk: 0.001, dec: 0.18, rel: 0.25, curve: 0.9, keytrack: 0.4, velCut: 0.5, spMix: 0.08, poly: 4 },
    S: [ST(1.2, 0, 0, 0, 0, 0, 0, 0.25, 9000, 0.25, 0, 0), ST(0.35, 0, 0, 0, 0, 0, 0, 0, 380, 0.15, 0, 0), ST(0, 0, 0, 0, 0, 0, 0, 0, 300, 0.1, 0, 0)],
    notes: { target: '"A strong transient at the beginning of the sharp attack with enough meat in the body to carry such a quick signal" (an envelope even the M struggles with).',
      listen: 'A hard click and snap, a fast exponential drop, then a low, saturated body that still has weight.',
      decides: 'Whether the articulation curve plus filter drive and VCA heat give analog-mono punch.',
      play: 'Short notes, C1 to C3. Move Curve between 0.5 and 1. Raise Heat and Drive. Turn the resonator mix in the states up or down.' } },

  '5 · Cloud to tone': {
    P: { vol: 0.6, aBank: 3, aPos: 0, aN: 6, aDet: 35, aPan: 1, aWin: 0.5, aWander: 0.4, fMode: 1, reso: 0.05, drive: 0.15, heat: 0.2, atk: 0.6, dec: 2.5, rel: 2.5, curve: 0,
      auth: 0.4, stiff: 2, visc: 0.6, temp: 0.2, spMix: 0.35, spDecay: 4 },
    S: [ST(0.8, 0, 0, 0, 0, 0.3, 0.6, 0, 6000, 0, 0, 0), ST(0.9, 0, 0, 0, 1, 0, 0, 0, 6000, 0, 0, 0), ST(0, 0, 0, 0, 0, 0.5, 0.7, 0, 5000, 0, 0, 0)],
    notes: { target: 'The signature idea: a cluster of particle emitters that starts as dust, locks into one voice as coupling rises, then falls apart on release.',
      listen: 'The moment the emitters pull into step. Watch the particle view: the needle grows as they cohere.',
      decides: 'Whether coupling is a musical way to move between texture and tone, and whether it should be a main panel control.',
      play: 'Hold single notes and short chords around C3. Then move Coupling in the Continuant state between 0.4 and 1.' } },

  '6 · Pulsar vowels': {
    P: { vol: 1.1, aBank: 3, aPos: fr(2), aN: 2, aDet: 4, aForm: 4, aCyc: 2, aWin: 0.6, formTrack: 0, fMode: 1, reso: 0.05, heat: 0.15, atk: 0.01, dec: 3, rel: 1.5, curve: -0.3, spMix: 0.25 },
    S: [ST(0.9, -5, 0, 0, 0.5, 0.2, 0.1, 0, 8000, 0, 0, 0), ST(0.9, 0, 0, 0.4, 0.5, 0, 0, 0, 8000, 0, 0, 0), ST(0, -4, 0, 0.2, 0.5, 0.4, 0.2, 0, 8000, 0, 0, 0)],
    notes: { target: 'Pulsar synthesis after Curtis Roads: the emission rate rises from rhythm into pitch while the formant stays fixed, so the pulses become a vowel.',
      listen: 'The crossing near 20 Hz where clicks become a pitch, while the vowel colour stays put.',
      decides: 'Whether the rhythm-to-pitch crossing is musical inside one voice (macro-scale time zoom).',
      play: 'Hold a note and let the 3-second rise play out. Change Formant ratio and Cycles on Layer A. Try Formant track at 1.' } },

  '7 · Water skin': {
    P: { vol: 0.45, aBank: 3, aPos: fr(5), aN: 3, aDet: 12, aPan: 0.8, aWin: 0, fMode: 1, reso: 0.05, atk: 0.4, dec: 1.5, rel: 2, curve: 0,
      vocOn: 1, vocBody: 3, vocSkin: 0, bands: [.5,.4,.35,.45,.6,.9,.7,.8], vocResp: 0.15, vocMix: 0.85, dropRate: 120, spMix: 0.35, spDecay: 3 },
    S: [ST(0.9, 0, 0, 0, 0, 0, 0, 0, 7000, 0, 0, 0), ST(0.9, 0, 0, 0, 0, 0, 0, 0, 7000, 0, 0, 0), ST(0, 0, 0, 0, 0, 0, 0, 0, 6000, 0, 0, 0)],
    notes: { target: 'Changing the material: the chord\'s spectrum is played by water droplets instead of oscillators. The material vocoder with Body set to Droplets and Skin set to Voices.',
      listen: 'Whether the chord is still recognisable when it is made of water.',
      decides: 'Whether the material vocoder transforms material the way the GRAINS vocoder did, and what Body sources it needs.',
      play: 'Hold chords. Sweep Droplet density and Response. Then set Body to Voices, Skin to Fixed and press the Water bands (the GRAINS move).' } },

  '8 · Fire skin': {
    P: { vol: 0.6, aBank: 2, aPos: fr(0), aN: 2, aDet: 9, bOn: 1, bBank: 2, bPos: fr(6), bSemi: 12, bLvl: 0.4, reso: 0.1, atk: 0.3, dec: 1.2, rel: 1.5, curve: 0,
      vocOn: 1, vocBody: 2, vocSkin: 0, bands: BANDS.Flat.slice(), vocResp: 0.1, vocMix: 0.8, dustRate: 1500, spMix: 0.2 },
    S: [ST(0.9, 0, 0, 0, 0, 0, 0, 0, 5000, 0, 0, 0), ST(0.9, 0, 0, 0, 0, 0, 0, 0, 3500, 0, 0, 0), ST(0, 0, 0, 0, 0, 0, 0, 0, 2500, 0, 0, 0)],
    notes: { target: 'The same chord turned into crackle: Body set to Dust, Skin set to Voices.',
      listen: 'Crackle that carries the chord\'s movement and brightness rather than just sitting on top of it.',
      decides: 'How dense the dust has to be before it carries a pitch, and whether that is musical or just noise.',
      play: 'Hold a chord. Sweep Dust density from 100 to 3000. Lower Mix to blend the dry chord back in.' } },

  '9 · Dusty watery vocoder': {
    P: {"aOn": 1, "aMat": 0, "aBank": 3, "aPos": 0.8487, "aForm": 2.0, "aCyc": 3.3392, "aWin": 0.3277, "aPhase": 0.2521, "aN": 4, "aDet": 5.8824, "aWander": 0.2773, "aPan": 0.6975, "aLvl": 0.75, "bOn": 1, "bSync": 0, "bMat": 0, "bBank": 5, "bPos": 0.8151, "bForm": 1.0, "bCyc": 2.972, "bWin": 0.1345, "bPhase": 0.3529, "bN": 5, "bDet": 8.4034, "bWander": 0.7731, "bPan": 0.6891, "bLvl": 0.5882, "bSemi": -24, "bFine": -2.1008, "pmAmt": 0.6471, "formTrack": 0.2353, "wanderRate": 0.1597, "derez": 0.6807, "drift": 0.395, "resTune": 0.5178, "resDecay": 0.5, "strike": 0.0, "fMode": 1, "reso": 0.3529, "drive": 0.5126, "heat": 0.5798, "keytrack": 0.2605, "velCut": 0.4454, "mass": 0.0, "grainM": 0.0, "allure": 0.0, "vol": 0.5, "atk": 0.12, "dec": 1.2, "rel": 1.2, "curve": 0.3, "loopTime": 1.5, "loop": 0, "auth": 0.0352, "stiff": 8.5341, "visc": 0.8, "temp": 0.0, "poly": 6, "velAmp": 0.5, "vocOn": 1, "vocAuto": 1, "vocBody": 2, "vocSkin": 0, "vocMix": 1, "vocShift": -0.7689, "vocSpread": 0.8235, "vocQ": 12.0449, "vocResp": 0.8487, "scrapeRate": 5.0086, "scrapeDepth": 0.8319, "dustRate": 119.2265, "dropRate": 60.0, "dropPitch": 1.0, "fbType": 1, "fbTime": 240.0, "fbCut": 1500.0002, "fbQ": 2.0, "fbDrive": 1.2, "fbRegen": 0.3, "fbMix": 0.0, "spSize": 1.2, "spDecay": 2.5, "spTone": 0.5, "spMix": 0.2},
    S: [{"level": 1.0, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.0, "scatter": 0.0, "fold": 0.0, "cutoff": 3199.9998, "resonator": 0.0, "ring": 0.9244, "feedback": 0.3346}, {"level": 0.7129, "pitch": 0.0, "formant": -0.0756, "position": -1.0, "coupling": 0.7059, "mask": 0.4538, "scatter": 0.5798, "fold": 0.5546, "cutoff": 343.8145, "resonator": 0.2521, "ring": 1.0, "feedback": 0.8235}, {"level": 0.6227, "pitch": -3.9408, "formant": -0.6328, "position": 0.0, "coupling": 0.0, "mask": 0.0, "scatter": 0.0, "fold": 0.0, "cutoff": 900.0002, "resonator": 0.0, "ring": 0.479, "feedback": 0.9328}],
    notes: {"target": "The chord's spectrum played by dust instead of oscillators, through narrow, slow, sweeping bands. Saved from the Windows app, where it was heard on the Vocoder wet listen point; Mix is set to 100% here so the full output matches what you heard.", "listen": "Dust crackle ringing through narrow bands (high Q), smeared by the slow Response, while Scrape sweeps the bands around at about 5 Hz. That sweep is the ripple.", "decides": "Whether dust should also be a modulation source for the rest of the voice, and how much of the dry chord belongs under the water.", "play": "Hold chords. Lower Mix to bring the dry voice back in. Sweep Scrape rate and Response, and Dust density from 30 to 400."} },

  '10 · Gravel': {
    P: {"aOn": 1, "aMat": 0, "aBank": 4, "aPos": 0.5714, "aForm": 1.0, "aCyc": 1.0, "aWin": 0.0, "aPhase": 0.0, "aN": 3, "aDet": 10.0, "aWander": 0.0, "aPan": 0.8, "aLvl": 0.8, "bOn": 0, "bSync": 0, "bMat": 0, "bBank": 0, "bPos": 0.4286, "bForm": 1.0, "bCyc": 1.0, "bWin": 0.0, "bPhase": 0.0, "bN": 1, "bDet": 6.0, "bWander": 0.0, "bPan": 0.5, "bLvl": 0.6, "bSemi": 0, "bFine": 0.0, "pmAmt": 0.958, "formTrack": 0.1933, "wanderRate": 0.1451, "derez": 0.8235, "drift": 0.5882, "resTune": 0.3065, "resDecay": 0.0135, "strike": 0.7731, "fMode": 3, "reso": 0.4118, "drive": 0.3782, "heat": 0.3025, "keytrack": 0.7158, "velCut": 0.2422, "mass": 0.6303, "grainM": -0.1765, "allure": 0.5966, "vol": 0.6933, "atk": 0.001, "dec": 1.4, "rel": 0.1002, "curve": 0.7815, "loopTime": 2.4, "loop": 0, "auth": 0.2605, "stiff": 1.1724, "visc": 0.3434, "temp": 0.0, "poly": 6, "velAmp": 0.5, "vocOn": 1, "vocAuto": 1, "vocBody": 2, "vocSkin": 2, "vocMix": 0.9832, "vocShift": -0.2395, "vocSpread": 0.8513, "vocQ": 4.3687, "vocResp": 0.916, "scrapeRate": 1.8764, "scrapeDepth": 1.0, "dustRate": 1752.5208, "dropRate": 27.3095, "dropPitch": 0.3858, "fbType": 1, "fbTime": 240.0, "fbCut": 1500.0002, "fbQ": 2.0, "fbDrive": 1.2, "fbRegen": 0.3, "fbMix": 0.0, "spSize": 1.0, "spDecay": 3.5, "spTone": 0.5, "spMix": 0.3, "bands": [0.795, 0.484, 0.221, 0.943, 0.533, 0.508, 1.0, 0.5]},
    S: [{"level": 0.9, "pitch": 0.0, "formant": 0.0, "position": 0.15, "coupling": 0.0, "mask": 0.0, "scatter": 0.12, "fold": 0.0, "cutoff": 3500.0, "resonator": 0.0, "ring": 0.0, "feedback": 0.0}, {"level": 0.8, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.0, "scatter": 0.06, "fold": 0.0, "cutoff": 1100.0001, "resonator": 0.0, "ring": 0.0, "feedback": 0.0}, {"level": 1.1361, "pitch": -3.8992, "formant": -1.084, "position": 0.3277, "coupling": 0.5462, "mask": 0.6639, "scatter": 0.605, "fold": 0.8235, "cutoff": 143.9371, "resonator": 0.7899, "ring": 0.6639, "feedback": 0.0}],
    notes: {"target": "Built from Glass shards: dust played through fixed vocoder bands (Skin: Fixed), with heavy FM, derez and a high-pass filter, and a Termination state that drops four octaves while folding, masking and ringing the resonator.", "listen": "The gravel texture while held, and the mechanical crunch when a key comes up: Release is 0.1 s, so the whole Termination state is squeezed into a tenth of a second.", "decides": "How much of the character comes from the release shape versus the vocoder bands.", "play": "Short and long notes. Lengthen Release (or set Tail) to hear Termination stretched out. Redraw the eight bands."} },

  '11 · Sandblaster': {
    P: {"aOn": 1, "aMat": 0, "aBank": 4, "aPos": 0.2101, "aForm": 4.3399, "aCyc": 0.25, "aWin": 0.563, "aPhase": 0.5126, "aN": 4, "aDet": 57.9832, "aWander": 0.6639, "aPan": 0.6555, "aLvl": 0.2857, "bOn": 1, "bSync": 0, "bMat": 0, "bBank": 0, "bPos": 0.5042, "bForm": 0.4424, "bCyc": 0.8793, "bWin": 0.605, "bPhase": 0.4637, "bN": 3, "bDet": 45.3782, "bWander": 0.4538, "bPan": 0.521, "bLvl": 0.563, "bSemi": -10, "bFine": -10.5042, "pmAmt": 0.0, "formTrack": 1.0, "wanderRate": 0.3, "derez": 0.3, "drift": 0.25, "resTune": 1.0, "resDecay": 0.5, "strike": 0.3, "fMode": 1, "reso": 0.5126, "drive": 0.7899, "heat": 0.9137, "keytrack": 0.7127, "velCut": 0.8036, "mass": 0.2605, "grainM": 0.4286, "allure": 0.0703, "vol": 1.0791, "atk": 0.003, "dec": 0.35, "rel": 0.8, "curve": 0.6, "loopTime": 0.9, "loop": 1, "auth": 0.0, "stiff": 6.0, "visc": 0.8, "temp": 0.0, "poly": 6, "velAmp": 0.5, "vocOn": 1, "vocAuto": 1, "vocBody": 2, "vocSkin": 0, "vocMix": 0.6134, "vocShift": -0.2899, "vocSpread": 1.1655, "vocQ": 9.0878, "vocResp": 0.7563, "scrapeRate": 0.9509, "scrapeDepth": 0.5714, "dustRate": 399.9999, "dropRate": 90.5144, "dropPitch": 1.96, "fbType": 1, "fbTime": 285.4076, "fbCut": 185.4286, "fbQ": 2.214, "fbDrive": 3.2185, "fbRegen": 0.9891, "fbMix": 0.8235, "spSize": 2.0, "spDecay": 7.6008, "spTone": 0.8739, "spMix": 1.0, "bands": [0.0, 0.451, 0.656, 0.754, 0.1, 0.7, 1.0, 0.85]},
    S: [{"level": 1.0, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.0, "scatter": 0.0, "fold": 0.0, "cutoff": 4199.999, "resonator": 0.0, "ring": 0.85, "feedback": 0.0}, {"level": 0.8, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.0, "scatter": 0.0, "fold": 0.0, "cutoff": 1500.0002, "resonator": 0.0, "ring": 0.85, "feedback": 0.0}, {"level": 0.0, "pitch": 0.0, "formant": -1.9412, "position": 0.4118, "coupling": 0.479, "mask": 0.615, "scatter": 0.2101, "fold": 0.6387, "cutoff": 800.0001, "resonator": 0.4469, "ring": 0.85, "feedback": 0.4286}],
    notes: {"target": "Grit from two detuned glass and classic clusters, short particles (Cycles 0.25), a hot filter and VCA, dust through the vocoder, and a feedback loop at the edge.", "listen": "The grit in the raw particles, and the looping articulation (Loop on, 0.9 s) pumping the ring modulation and cutoff.", "decides": "Where the grit lives: the raw emitters, the filter heat, or the vocoder and feedback around them.", "play": "Saved with Listen on Raw layers. Compare Raw layers with Full output: on full output the feedback (regeneration 99%) and a 100% space take over."} },

  '12 · Transient with body': {
    P: {"aOn": 1, "aMat": 0, "aBank": 0, "aPos": 0.4286, "aForm": 1.0, "aCyc": 1.0, "aWin": 0.0, "aPhase": 0.0, "aN": 1, "aDet": 6.0, "aWander": 0.0, "aPan": 0.5, "aLvl": 0.8, "bOn": 1, "bSync": 0, "bMat": 0, "bBank": 0, "bPos": 0.5714, "bForm": 1.0, "bCyc": 1.0, "bWin": 0.0, "bPhase": 0.0, "bN": 1, "bDet": 6.0, "bWander": 0.0, "bPan": 0.5, "bLvl": 0.35, "bSemi": -12, "bFine": 0.0, "pmAmt": 0.6555, "formTrack": 0.8151, "wanderRate": 0.6387, "derez": 0.563, "drift": 0.5882, "resTune": 1.521, "resDecay": 0.0318, "strike": 0.479, "fMode": 0, "reso": 0.5966, "drive": 0.6134, "heat": 0.7479, "keytrack": 0.2185, "velCut": 0.2605, "mass": 0.6975, "grainM": 0.2941, "allure": 0.5882, "vol": 2.2879, "atk": 0.001, "dec": 0.18, "rel": 0.25, "curve": 0.9, "loopTime": 1.5, "loop": 0, "auth": 0.479, "stiff": 1.1724, "visc": 0.6378, "temp": 0.5042, "poly": 3, "velAmp": 0.4118, "vocOn": 1, "vocAuto": 1, "vocBody": 0, "vocSkin": 0, "vocMix": 0.605, "vocShift": -1.5, "vocSpread": 1.0, "vocQ": 5.0, "vocResp": 0.6807, "scrapeRate": 24.4621, "scrapeDepth": 0.6891, "dustRate": 399.9998, "dropRate": 9.5139, "dropPitch": 1.5842, "fbType": 0, "fbTime": 3.2145, "fbCut": 3620.5759, "fbQ": 7.1905, "fbDrive": 2.581, "fbRegen": 0.0, "fbMix": 0.0, "spSize": 1.5702, "spDecay": 2.5, "spTone": 0.7227, "spMix": 0.5546, "bands": [0.8, 0.508, 0.156, 0.8, 0.352, 0.226, 0.287, 0.8]},
    S: [{"level": 1.2, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.0, "scatter": 0.0, "fold": 0.25, "cutoff": 9000.001, "resonator": 0.25, "ring": 0.0, "feedback": 0.0}, {"level": 0.35, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.0, "scatter": 0.0, "fold": 0.0, "cutoff": 379.9998, "resonator": 0.15, "ring": 0.0, "feedback": 0.0}, {"level": 0.0, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.0, "scatter": 0.0, "fold": 0.0, "cutoff": 300.0, "resonator": 0.1, "ring": 0.0, "feedback": 0.0}],
    notes: {"target": "A take on Transient with meat: heavy FM, derez, VCA heat and Mass, with the vocoder shifting the voice down an octave and a half under a short, physical articulation.", "listen": "The snap at the front, then how the springs (Authority 48%, Temperature 50%) make each hit land a little differently.", "decides": "How much of the punch comes from the physics versus the filter and heat.", "play": "Short notes, C1 to C3, with varied velocity. Sweep Temperature and Authority."} },

  "13 \u00b7 Moan cloud": {
    P: {"aOn": 1, "aMat": 0, "aBank": 3, "aPos": 0.0, "aForm": 1.0, "aCyc": 1.0, "aWin": 0.5, "aPhase": 0.0, "aN": 6, "aDet": 35.0, "aWander": 0.4, "aPan": 1.0, "aLvl": 0.8, "bOn": 0, "bSync": 0, "bMat": 0, "bBank": 0, "bPos": 0.4286, "bForm": 1.0, "bCyc": 1.0, "bWin": 0.0, "bPhase": 0.0, "bN": 1, "bDet": 6.0, "bWander": 0.0, "bPan": 0.5, "bLvl": 0.6, "bSemi": 0, "bFine": 0.0, "pmAmt": 0.6891, "formTrack": 1.0, "wanderRate": 0.3697, "derez": 0.1849, "drift": 0.6387, "resTune": 0.8495, "resDecay": 0.1442, "strike": 0.7479, "fMode": 1, "reso": 0.3445, "drive": 0.2773, "heat": 0.6134, "keytrack": 0.4622, "velCut": 0.5462, "mass": -0.4454, "grainM": -0.4454, "allure": 0.4286, "vol": 1.3655, "atk": 0.4537, "dec": 0.0049, "rel": 0.0018, "curve": -0.7647, "loopTime": 1.5, "loop": 1, "auth": 0.2101, "stiff": 2.0, "visc": 0.6, "temp": 0.6555, "poly": 5, "velAmp": 0.5, "vocOn": 1, "vocAuto": 1, "vocBody": 0, "vocSkin": 0, "vocMix": 0.9748, "vocShift": 0.7689, "vocSpread": 0.916, "vocQ": 3.9032, "vocResp": 0.395, "scrapeRate": 0.9509, "scrapeDepth": 0.7059, "dustRate": 253.0545, "dropRate": 104.5119, "dropPitch": 0.4866, "fbType": 1, "fbTime": 4.1102, "fbCut": 272.0858, "fbQ": 3.6357, "fbDrive": 1.6147, "fbRegen": 0.6378, "fbMix": 0.0, "spSize": 1.0433, "spDecay": 1.0561, "spTone": 0.3193, "spMix": 0.4622, "bands": [0.811, 0.533, 0.385, 0.508, 0.25, 0.85, 0.35, 1.0]},
    S: [{"level": 0.8, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.3, "scatter": 0.6, "fold": 0.0, "cutoff": 6000.0, "resonator": 0.0, "ring": 0.0, "feedback": 0.0}, {"level": 0.9, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 1.0, "mask": 0.0, "scatter": 0.0, "fold": 0.0, "cutoff": 6000.0, "resonator": 0.0, "ring": 0.0, "feedback": 0.0}, {"level": 0.0, "pitch": 0.0, "formant": 0.0, "position": 0.0, "coupling": 0.0, "mask": 0.5, "scatter": 0.7, "fold": 0.0, "cutoff": 5000.001, "resonator": 0.0, "ring": 0.0, "feedback": 0.0}],
    notes: {"target": "Built from Cloud to tone: six vocal emitters with FM, wander and drift, pumped by a looping articulation (attack 0.45 s, near-instant decay) and run almost fully through the vocoder, shifted up.", "listen": "The moan as the loop swings between a scattered, masked onset and a coupled continuant, with Temperature keeping each pass different.", "decides": "Whether the looping articulation plus coupling is a playable way to make long, breathing textures.", "play": "Hold chords. Change Loop time. Lower Vocoder mix to hear the dry cloud under it."} },
};
function makePatch(name) { const pr = PATCHES[name]; const P = JSON.parse(JSON.stringify(BASE)); Object.assign(P, JSON.parse(JSON.stringify(pr.P))); return { P, S: JSON.parse(JSON.stringify(pr.S)) }; }
if (typeof module !== 'undefined') module.exports = { BASE, PATCHES, BANDS, makePatch };
