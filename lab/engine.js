// ===================== SILT ENGINE v2 (AudioWorklet) =====================
const SR = sampleRate, TAU = Math.PI * 2, CTRL = 16, MAXV = 6, MAXE = 6, TL = 2048, NMIP = 11, NFR = 8, NBANK = 6;
const FS = TL + 1, MIPSZ = FS, FRSZ = FS * NMIP, BANKSZ = FRSZ * NFR;
const clamp = (x, a, b) => x < a ? a : x > b ? b : x;
function ft(x) { if (x > 3) return 1; if (x < -3) return -1; const x2 = x * x; return x * (27 + x2) / (27 + 9 * x2); }
const gauss = () => (Math.random() + Math.random() + Math.random() - 1.5) * 1.4142;

// ---------- FFT + band-limited wavetable banks ----------
function fft(re, im, inv) {
  const n = re.length;
  for (let i = 1, j = 0; i < n; i++) { let bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit;
    if (i < j) { let t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; } }
  for (let len = 2; len <= n; len <<= 1) { const ang = (inv ? 2 : -2) * Math.PI / len, wr = Math.cos(ang), wi = Math.sin(ang), h = len >> 1;
    for (let i = 0; i < n; i += len) { let cr = 1, ci = 0;
      for (let j = 0; j < h; j++) { const a = i + j, b = a + h, br = re[b] * cr - im[b] * ci, bi = re[b] * ci + im[b] * cr;
        re[b] = re[a] - br; im[b] = im[a] - bi; re[a] += br; im[a] += bi; const t = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = t; } } }
}
// spectrum helpers: return {A:Float64Array(1024), P:Float64Array(1024)} (index = harmonic number)
function spec(fn) { const A = new Float64Array(1024), P = new Float64Array(1024); for (let k = 1; k < 1024; k++) { const r = fn(k); if (r) { A[k] = r[0]; P[k] = r[1] || 0; } } return { A, P }; }
function timeSpec(fn) { const re = new Float64Array(TL), im = new Float64Array(TL); for (let i = 0; i < TL; i++) re[i] = fn(i / TL);
  fft(re, im, false); const A = new Float64Array(1024), P = new Float64Array(1024);
  for (let k = 1; k < 1024; k++) { A[k] = Math.hypot(re[k], im[k]) / TL * 2; P[k] = Math.atan2(im[k], re[k]); } return { A, P }; }
const chirp = k => 0.18 * k * k; // phase rotation keeps a defined transient but tames crest factor
function formants(f0, F, tilt, opt) { opt = opt || {};
  return spec(k => { const f = k * f0; if (f > 16000) return null; let e = 0;
    for (const [fc, a, bw] of F) e += a / (1 + Math.pow((f - fc) / bw, 2));
    let amp = (e + (opt.floor || 0.02)) * Math.pow(k, -tilt);
    if (opt.oddBelow && f < opt.oddBelow && k % 2 === 0) amp *= 0.08;
    if (opt.weakFund && k === 1) amp *= opt.weakFund;
    return [amp, chirp(k)]; }); }
const qz = (v, lv) => Math.round(v * lv) / lv;
function bankDefs() {
  const B = [];
  B.push([ // 0 Classic
    spec(k => k === 1 ? [1, 0] : null),
    spec(k => k % 2 ? [1 / (k * k), (k % 4 === 3) ? Math.PI : 0] : null),
    spec(k => [Math.pow(k, -1.5), 0]),
    spec(k => [1 / k, 0]),
    spec(k => k % 2 ? [1 / k, 0] : null),
    timeSpec(t => t < 0.3 ? 1 : -1),
    timeSpec(t => t < 0.12 ? 1 : -1),
    spec(k => [(1 / k) * (1 + 3 * Math.exp(-Math.pow((k - 8) / 2.2, 2))), 0]),
  ]);
  B.push([ // 1 Hollow 90s
    spec(k => k === 1 ? [1, 0] : k === 3 ? [0.35, 0] : k === 5 ? [0.12, 0] : null),
    spec(k => k % 2 ? [1 / (k * k), 0] : null),
    spec(k => ({ 1: 1, 2: 0.6, 4: 0.3, 8: 0.15, 16: 0.06 })[k] ? [({ 1: 1, 2: 0.6, 4: 0.3, 8: 0.15, 16: 0.06 })[k], 0] : null),
    spec(k => k === 1 ? [1, 0] : (k >= 6 && k <= 11) ? [0.32 * Math.exp(-(k - 6) * 0.25), chirp(k)] : null),
    spec(k => k === 1 ? [1, 0] : k === 2 ? [0.25, 0] : k === 3 ? [0.1, 0] : k < 48 ? [0.02 / Math.sqrt(k) * (0.5 + ((k * 7919) % 13) / 13), (k * 2.3) % TAU] : null),
    timeSpec(t => qz(Math.tanh(3 * Math.sin(TAU * t)) * 0.9, 7)),
    spec(k => [Math.exp(-k / 4) * (k % 2 ? 1.6 : 0.6), 0]),
    spec(k => k % 2 ? [Math.exp(-Math.pow((k - 5) / 3, 2)) + 0.4 / k, chirp(k)] : null),
  ]);
  B.push([ // 2 Reeds & brass (sampled-style single cycles, phase gives a transient)
    formants(110, [[500, 0.9, 200], [1200, 0.6, 400], [2600, 0.25, 600]], 0.3),
    formants(175, [[400, 1, 150], [900, 0.4, 300]], 0.8),
    formants(233, [[500, 1, 150], [1500, 0.7, 250], [2700, 0.4, 400], [4500, 0.2, 700]], 0.25),
    formants(147, [[420, 1, 140], [1300, 0.6, 250], [2600, 0.35, 400], [5500, 0.08, 1500]], 0.2),
    formants(196, [[1500, 0.6, 500], [3000, 0.2, 800]], 0.5, { oddBelow: 1500, floor: 0.3 }),
    formants(330, [[1100, 1, 250], [2800, 0.5, 500]], 0.2, { weakFund: 0.35 }),
    formants(98, [[450, 1, 120], [1150, 0.5, 250], [2500, 0.15, 400]], 0.4),
    formants(233, [[1800, 1, 300], [4000, 0.6, 600]], 0.15, { weakFund: 0.2 }),
  ]);
  const vow = (f0, a, b, c, d) => formants(f0, [[a, 1, 70], [b, 0.5, 90], [c, 0.25, 120], [d || 3500, 0.1, 200]], 1, { floor: 0.005 });
  B.push([ // 3 Vocal
    vow(130, 730, 1090, 2440), vow(130, 530, 1840, 2480), vow(130, 270, 2290, 3010), vow(130, 570, 840, 2410),
    vow(130, 300, 870, 2240), vow(220, 850, 1220, 2810), vow(220, 610, 2330, 2990), vow(130, 250, 2500, 3300, 3800),
  ]);
  const rnd = s => { let x = Math.sin(s * 12.9898) * 43758.5453; return x - Math.floor(x); };
  B.push([ // 4 Glass
    spec(k => [1, 7, 13, 19, 25, 31].indexOf(k) >= 0 ? [1 / Math.sqrt([1, 7, 13, 19, 25, 31].indexOf(k) + 1), chirp(k)] : null),
    spec(k => k === 1 ? [1, 0] : [0.6 * Math.exp(-Math.pow((k - 20) / 1.5, 2)), chirp(k)]),
    spec(k => k === 1 ? [1, 0] : k === 14 ? [0.5, 0] : k === 28 ? [0.15, 0] : null),
    spec(k => { const r = Math.sqrt(k); return r === Math.floor(r) ? [1 / r, chirp(k)] : null; }),
    spec(k => k === 1 ? [1, 0] : (k >= 8 && k <= 40) ? [0.3 * rnd(k) * rnd(k + 9), rnd(k + 3) * TAU] : null),
    spec(k => (k % 3 && k < 64) ? [1 / Math.sqrt(k), chirp(k)] : null),
    spec(k => k === 1 ? [1, 0] : k === 2 ? [0.3, 0] : (k >= 30 && k <= 40) ? [0.2, rnd(k) * TAU] : null),
    spec(k => [1, 3, 8, 21, 55, 144].indexOf(k) >= 0 ? [1 / ([1, 3, 8, 21, 55, 144].indexOf(k) + 1), chirp(k)] : null),
  ]);
  const step = f => t => f(Math.floor(t * 256) & 255);
  let lfsr = 0xACE1; const nseq = []; for (let i = 0; i < 256; i++) { const b = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1; lfsr = (lfsr >> 1) | (b << 15); nseq.push(lfsr & 1); }
  B.push([ // 5 Bitmask (integer-pattern cycles, band-limited)
    timeSpec(t => qz(Math.sin(TAU * Math.floor(t * 32) / 32), 7)),
    timeSpec(step(i => ((i & (i >> 3)) & 31) / 15.5 - 1)),
    timeSpec(step(i => ((i ^ (i >> 4)) & 15) / 7.5 - 1)),
    timeSpec(step(i => (((i * 5) & (i >> 2)) & 63) / 31.5 - 1)),
    timeSpec(step(i => ((i | (i >> 5)) & 31) / 15.5 - 1)),
    timeSpec(step(i => { let c = 0, v = i; while (v) { c += v & 1; v >>= 1; } return c / 4 - 1; })),
    timeSpec(step(i => (((i * 3) ^ (i >> 1)) & 63) / 31.5 - 1)),
    timeSpec(step(i => (i < 128 ? 1 : -1) * (nseq[i] ? 1 : 0.4))),
  ]);
  return B;
}
function buildTables() {
  const defs = bankDefs(), out = [];
  const re = new Float64Array(TL), im = new Float64Array(TL);
  for (let b = 0; b < NBANK; b++) {
    const T = new Float32Array(BANKSZ);
    for (let f = 0; f < NFR; f++) {
      const { A, P } = defs[b][f]; let scale = 0;
      for (let m = 0; m < NMIP; m++) {
        const maxH = Math.min(1023, 1024 >> m);
        re.fill(0); im.fill(0);
        for (let k = 1; k <= maxH; k++) { if (!A[k]) continue; const c = A[k] * Math.cos(P[k]), s = A[k] * Math.sin(P[k]); re[k] = c; im[k] = s; re[TL - k] = c; im[TL - k] = -s; }
        fft(re, im, true);
        if (m === 0) { let pk = 0, rms = 0; for (let i = 0; i < TL; i++) { pk = Math.max(pk, Math.abs(re[i])); rms += re[i] * re[i]; }
          rms = Math.sqrt(rms / TL); scale = Math.min(0.42 / Math.max(rms, 1e-9), 1 / Math.max(pk, 1e-9)); }
        const o = f * FRSZ + m * MIPSZ;
        for (let i = 0; i < TL; i++) T[o + i] = re[i] * scale; T[o + TL] = T[o];
      }
    }
    out.push(T);
  }
  return out;
}
// window table: 33 shapes from rect(0) -> hann(16) -> expodec(32)
const NWS = 33, WL = 512, WIN = new Float32Array(NWS * (WL + 1));
for (let s = 0; s < NWS; s++) { const sh = s / (NWS - 1);
  for (let i = 0; i <= WL; i++) { const u = i / WL, hann = 0.5 - 0.5 * Math.cos(TAU * u), exd = Math.exp(-6.5 * u) * Math.min(1, u / 0.015);
    WIN[s * (WL + 1) + i] = sh < 0.5 ? 1 + (hann - 1) * (sh * 2) : hann + (exd - hann) * ((sh - 0.5) * 2); } }

// halfband FIR (31 taps) for 2x oversampling
const HB = new Float64Array(31);
(function () { const I0 = x => { let s = 1, t = 1; for (let k = 1; k < 30; k++) { t *= (x / 2) / k; s += t * t; } return s; }, beta = 7.5;
  for (let n = 0; n < 31; n++) { const d = n - 15, w = I0(beta * Math.sqrt(1 - Math.pow(d / 15, 2))) / I0(beta);
    HB[n] = d === 0 ? 0.5 : (d % 2 === 0 ? 0 : Math.sin(Math.PI * d / 2) / (Math.PI * d)) * w; } })();
const HBE = []; for (let k = 0; k <= 30; k += 2) HBE.push(HB[k]); // 16 even taps
class Up { constructor() { this.x = new Float64Array(16); this.i = 0; }
  push(v) { this.i = (this.i + 15) & 15; this.x[this.i] = v; }
  even() { let s = 0; for (let j = 0; j < 16; j++) s += HBE[j] * this.x[(this.i + j) & 15]; return 2 * s; }
  odd() { return this.x[(this.i + 7) & 15]; } }
class Down { constructor() { this.e = new Float64Array(16); this.o = new Float64Array(16); this.i = 0; }
  push(ev, od) { this.i = (this.i + 15) & 15; this.e[this.i] = ev; this.o[this.i] = od; }
  out() { let s = 0; for (let j = 0; j < 16; j++) s += HBE[j] * this.e[(this.i + j) & 15]; return s + 0.5 * this.o[(this.i + 8) & 15]; } }

class SVF { constructor() { this.a = 0; this.b = 0; this.k = 1; this.a1 = 1; this.a2 = 0; this.a3 = 0; this.lp = 0; this.bp = 0; this.hp = 0; }
  set(f, q, sr) { f = clamp(f, 10, sr * 0.45); const g = Math.tan(Math.PI * f / sr); this.k = 1 / q; this.a1 = 1 / (1 + g * (g + this.k)); this.a2 = g * this.a1; this.a3 = g * this.a2; }
  tick(x) { const v3 = x - this.b, v1 = this.a1 * this.a + this.a2 * v3, v2 = this.b + this.a2 * this.a + this.a3 * v3;
    this.a = 2 * v1 - this.a; this.b = 2 * v2 - this.b; this.lp = v2; this.bp = v1; this.hp = x - this.k * v1 - v2; return v2; }
  reset() { this.a = 0; this.b = 0; } }

// ---------- state parameters ----------
const KEYS = ['level', 'pitch', 'formant', 'position', 'coupling', 'mask', 'scatter', 'fold', 'cutoff', 'resonator', 'ring', 'feedback'];
const NK = KEYS.length;
const LO = [0, -7, -3, -1, 0, 0, 0, 0, Math.log2(20), 0, 0, 0], HI = [1.3, 2, 3, 1, 1, 1, 1, 1, Math.log2(20000), 1, 1, 1];
const SPAN = [1, 3, 2, 1, 1, 1, 1, 1, 4, 1, 1, 1];
const K_LEVEL = 0, K_PITCH = 1, K_FORM = 2, K_POS = 3, K_COUP = 4, K_MASK = 5, K_SCAT = 6, K_FOLD = 7, K_CUT = 8, K_RES = 9, K_RING = 10, K_FB = 11;

class Layer {
  constructor() {
    this.th = new Float64Array(MAXE); this.ci = new Float64Array(MAXE); this.wd = new Float64Array(MAXE); this.inc = new Float64Array(MAXE);
    this.pl = new Float64Array(MAXE); this.pr = new Float64Array(MAXE); this.cur = new Uint8Array(MAXE);
    const S = MAXE * 2; this.on = new Uint8Array(S); this.rd = new Float64Array(S); this.el = new Float64Array(S); this.amp = new Float64Array(S);
    this.fi = new Float64Array(S); this.rg = new Float64Array(S); this.rl = new Uint8Array(S); this.fm = new Float64Array(S);
    this.n0 = new Float64Array(S); this.n1 = new Float64Array(S); this.ni = new Float64Array(S);
    this.N = 1; this.ff = 0.01; this.C = 1; this.ws = 0; this.fade = 0.05; this.tb = null; this.o00 = 0; this.o10 = 0; this.o01 = 0; this.o11 = 0;
    this.w00 = 1; this.w10 = 0; this.w01 = 0; this.w11 = 0; this.R = 0; this.rc = 0; this.psi = 0; this.K = 0; this.mat = 0; this.ph0 = 0; this.out = 0;
  }
  reset() { this.on.fill(0); for (let i = 0; i < MAXE; i++) { this.th[i] = Math.random(); this.wd[i] = 0; this.cur[i] = 0; } }
}

class Voice {
  constructor() {
    this.active = false; this.gate = false; this.serial = 0; this.note = 60; this.vel = 1;
    this.A = new Layer(); this.B = new Layer();
    this.p = new Float64Array(NK); this.s = new Float64Array(NK); this.v = new Float64Array(NK); this.n = new Float64Array(NK); this.cur = new Float64Array(NK); this.from = new Float64Array(NK);
    this.toIdx = 0; this.T = 1; this.t = 0; this.stage = 0; this.tail = 1;
    this.ry1 = [0, 0, 0]; this.ry2 = [0, 0, 0]; this.rb1 = [0, 0, 0]; this.rb2 = [0, 0, 0]; this.rg = [0, 0, 0]; this.strike = 0;
    this.upL = new Up(); this.upR = new Up(); this.dnL = new Down(); this.dnR = new Down();
    this.lad = [new Float64Array(4), new Float64Array(4)]; this.svf = [new SVF(), new SVF()];
    this.g0 = 0.1; this.g1 = 0.1; this.lv0 = 0; this.lv1 = 0; this.hc = 0; this.hL = 0; this.hR = 0; this.bPrev = 0;
    this.dP = 0; this.dC = 0; this.dPv = 0; this.dCv = 0; this.fold = 0; this.res = 0; this.ring = 0; this.fbs = 0;
  }
}

class Silt extends AudioWorkletProcessor {
  constructor() {
    super(); this.T = buildTables();
    this.voices = []; for (let i = 0; i < MAXV; i++) this.voices.push(new Voice());
    this.P = null; this.S = null; this.serial = 0; this.lastV = null;
    // global buffers
    this.bL = new Float32Array(128); this.bR = new Float32Array(128); this.fbB = new Float32Array(128);
    this.t1L = new Float32Array(128); this.t1R = new Float32Array(128); this.t2L = new Float32Array(128); this.t2R = new Float32Array(128); this.vt = new Float32Array(128);
    // feedback
    this.dlen = Math.ceil(SR * 1.6); this.dbuf = new Float32Array(this.dlen); this.dw = 0; this.dtS = 0.2 * SR; this.fbF = new SVF(); this.dcx = 0; this.dcy = 0; this.fbRetL = 0; this.fbRetR = 0;
    // vocoder
    this.anS = []; this.syL = []; this.syR = []; this.env = new Float64Array(8); for (let b = 0; b < 8; b++) { this.anS.push(new SVF()); this.syL.push(new SVF()); this.syR.push(new SVF()); }
    this.scr = 0; this.vDry = 1e-4; this.vWet = 1e-4; this.vGain = 1;
    this.dustL = [new SVF(), new SVF()]; this.dustR = [new SVF(), new SVF()]; this.dustL[0].set(3000, 3, SR); this.dustL[1].set(6500, 2, SR); this.dustR[0].set(3300, 3, SR); this.dustR[1].set(7100, 2, SR);
    this.popL = new SVF(); this.popR = new SVF(); this.popL.set(420, 2, SR); this.popR.set(380, 2, SR);
    this.drops = []; for (let i = 0; i < 14; i++) this.drops.push({ on: 0, ph: 0, f0: 0, ch: 0, t: 0, tc: 0, td: 0, a: 0, pl: 0, pr: 0 });
    // space (FDN)
    const base = [29.7, 37.1, 41.1, 43.7, 53.3, 59.9, 67.1, 73.9];
    this.fdn = base.map(ms => ({ buf: new Float32Array(Math.ceil(SR * ms / 1000 * 2.2) + 16), w: 0, len: 0, base: ms, lp: 0 }));
    this.apL = [142, 107, 379, 277].map(n => ({ b: new Float32Array(Math.ceil(n * SR / 48000)), i: 0 }));
    this.apR = [151, 113, 367, 293].map(n => ({ b: new Float32Array(Math.ceil(n * SR / 48000)), i: 0 }));
    this.modPh = 0;
    // output / tools
    this.agc = 1; this.agcR = 1e-4; this.outR = 0; this.mtrC = 0; this.vizC = 0; this.dcL = [0, 0]; this.dcR = [0, 0];
    this._ef = { e0: 0 }; this._efB = { e0: 0 }; this._ins = new Float64Array(4); this._outs = new Float64Array(4);
    this.rec = false; this.recBuf = new Float32Array(8192 * 2); this.recN = 0;
    this.port.onmessage = e => this.msg(e.data);
    this.port.postMessage({ type: 'ready' });
  }
  msg(d) {
    if (d.type === 'globals') this.P = d.P;
    else if (d.type === 'states') this.S = d.S.map(s => KEYS.map((k, i) => k === 'cutoff' ? Math.log2(clamp(s[k], 20, 20000)) : s[k]));
    else if (d.type === 'on') this.noteOn(d.n, d.v);
    else if (d.type === 'off') this.noteOff(d.n);
    else if (d.type === 'rec') { if (!d.on && this.rec) { if (this.recN > 0) this.port.postMessage({ type: 'recData', d: this.recBuf.slice(0, this.recN * 2) }); this.port.postMessage({ type: 'recEnd' }); }
      this.rec = d.on; this.recN = 0; }
    else if (d.type === 'panic') { for (const v of this.voices) v.active = false; this.dbuf.fill(0); this.fbF.reset(); for (const f of this.fdn) f.buf.fill(0); }
  }
  noteOn(n, vel) {
    if (!this.S || !this.P) return;
    const poly = clamp(this.P.poly | 0, 1, MAXV);
    let v = null; for (let i = 0; i < poly; i++) if (!this.voices[i].active) { v = this.voices[i]; break; }
    if (!v) { v = this.voices[0]; for (let i = 1; i < poly; i++) if (this.voices[i].serial < v.serial) v = this.voices[i]; }
    const wasActive = v.active;
    v.active = true; v.gate = true; v.note = n; v.freq = 440 * Math.pow(2, (n - 69) / 12); v.vel = vel; v.serial = ++this.serial;
    const S0 = this.S[0];
    for (let k = 0; k < NK; k++) { const st = wasActive ? v.cur[k] : S0[k]; v.p[k] = st; v.s[k] = st; v.v[k] = 0; v.n[k] = 0; v.cur[k] = st; v.from[k] = st; }
    if (!wasActive) { v.from[K_LEVEL] = 0; v.p[K_LEVEL] = 0; v.s[K_LEVEL] = 0; v.cur[K_LEVEL] = 0; v.lv0 = v.lv1 = 0;
      v.A.reset(); v.B.reset(); for (const l of v.lad) l.fill(0); v.svf[0].reset(); v.svf[1].reset(); v.ry1 = [0, 0, 0]; v.ry2 = [0, 0, 0]; }
    v.toIdx = 0; v.T = Math.max(0.0005, this.P.atk); v.t = 0; v.stage = 0; v.tail = 1; v.strike = 1;
    this.lastV = v;
  }
  noteOff(n) { for (const v of this.voices) if (v.active && v.gate && v.note === n) { v.gate = false; v.from.set(v.p); v.toIdx = 2; v.T = Math.max(0.0005, this.P.rel); v.t = 0; v.stage = 2; } }

  // ---------- per-voice control (every CTRL samples) ----------
  ctrlLayer(v, L, pre, f0, dt) {
    const P = this.P, cur = v.cur;
    L.on_ = P[pre + 'On']; if (!L.on_) return;
    const N = clamp(P[pre + 'N'] | 0, 1, MAXE); L.N = N; L.mat = P[pre + 'Mat'] | 0; L.tb = this.T[clamp(P[pre + 'Bank'] | 0, 0, NBANK - 1)];
    const det = P[pre + 'Det'], wan = P[pre + 'Wander'] + P.allure * 0.5, wr = P.wanderRate, spread = P[pre + 'Pan'];
    const step = wr * Math.sqrt(dt) * 40;
    for (let e = 0; e < N; e++) {
      const d = N > 1 ? (e / (N - 1)) * 2 - 1 : 0;
      if (wan > 0) L.wd[e] = clamp(L.wd[e] + (Math.random() - 0.5) * step, -wan * 60, wan * 60); else L.wd[e] *= 0.95;
      const cents = d * det + L.wd[e] + v.dPv;
      L.inc[e] = f0 * Math.pow(2, cents / 1200) / SR;
      const pan = 0.5 + d * spread * 0.5, a = pan * Math.PI / 2; L.pl[e] = Math.cos(a) * 1.414 / Math.sqrt(N); L.pr[e] = Math.sin(a) * 1.414 / Math.sqrt(N);
    }
    // formant (ratio x tracked pitch, or fixed around C4)
    const tr = P.formTrack, fb = Math.exp(Math.log(f0) * tr + Math.log(261.63 * Math.pow(2, cur[K_PITCH])) * (1 - tr));
    const ffHz = fb * P[pre + 'Form'] * Math.pow(2, cur[K_FORM] + P.mass * 1.2);
    L.ff = Math.min(ffHz, SR * 0.45) / SR;
    L.contin = (P[pre + 'Cyc'] * Math.pow(2, -P.grainM * 1.5)) * f0 / Math.max(1e-6, ffHz) >= 0.999;
    let C = P[pre + 'Cyc'] * Math.pow(2, -P.grainM * 1.5); L.C = Math.max(0.1, C);
    const ws = clamp(P[pre + 'Win'] + P.grainM * 0.45, 0, 1); L.ws = Math.round(ws * (NWS - 1)) * (WL + 1);
    const perSamp = f0 > 0 ? SR / f0 : 1000; L.fade = 1 / clamp(perSamp * 0.5, 4, 48);
    // mip + frame blend
    const lf = Math.log2(Math.max(1e-6, 1024 * ffHz / ((L.contin ? 0.25 : 0.17) * SR)));
    let m0 = 0, mf = 0; if (lf > 0) { m0 = Math.floor(lf); mf = lf - m0; } if (m0 >= NMIP - 1) { m0 = NMIP - 1; mf = 0; }
    const m1 = Math.min(NMIP - 1, m0 + 1);
    const pos = clamp(P[pre + 'Pos'] + cur[K_POS], 0, 1) * (NFR - 1); let f0i = Math.floor(pos), ffr = pos - f0i; if (f0i >= NFR - 1) { f0i = NFR - 1; ffr = 0; }
    const f1i = Math.min(NFR - 1, f0i + 1);
    L.o00 = f0i * FRSZ + m0 * MIPSZ; L.o10 = f1i * FRSZ + m0 * MIPSZ; L.o01 = f0i * FRSZ + m1 * MIPSZ; L.o11 = f1i * FRSZ + m1 * MIPSZ;
    L.w00 = (1 - ffr) * (1 - mf); L.w10 = ffr * (1 - mf); L.w01 = (1 - ffr) * mf; L.w11 = ffr * mf;
    L.ph0 = P[pre + 'Phase'];
    L.K = cur[K_COUP] * cur[K_COUP] * (f0 * 0.6 + 4);
    L.lvl = P[pre + 'Lvl'];
  }
  ctrlVoice(v, dt) {
    const P = this.P, S = this.S, to = S[v.toIdx];
    v.t += dt; let u = v.t / v.T; if (u > 1) u = 1;
    const c = P.curve * 9; const sh = Math.abs(c) < 0.01 ? u : c > 0 ? (1 - Math.exp(-c * u)) / (1 - Math.exp(-c)) : (Math.exp(-c * u) - 1) / (Math.exp(-c) - 1);
    for (let k = 0; k < NK; k++) v.p[k] = v.from[k] + (to[k] - v.from[k]) * sh;
    if (u >= 1) {
      if (v.stage === 0) { v.from.set(v.p); v.toIdx = 1; v.T = Math.max(0.001, P.dec); v.t = 0; v.stage = 1; }
      else if (v.stage === 1) { if (P.loop && v.gate) { v.from.set(v.p); v.toIdx = 0; v.T = Math.max(0.005, P.loopTime); v.t = 0; v.stage = 3; } }
      else if (v.stage === 3) { v.from.set(v.p); v.toIdx = 1; v.T = Math.max(0.005, P.loopTime); v.t = 0; v.stage = 1; }
      else if (v.stage === 2) v.stage = 4;
    }
    const kS = Math.pow(TAU * P.stiff, 2), cc = 2 * P.visc * Math.sqrt(kS), A = P.auth, temp = P.temp + P.allure * 0.3, dest = S[v.toIdx], nr = Math.min(1, dt * 6);
    for (let k = 0; k < NK; k++) {
      v.n[k] += (gauss() * temp * SPAN[k] * 0.3 - v.n[k]) * nr;
      const a = kS * (dest[k] + v.n[k] - v.s[k]) - cc * v.v[k]; v.v[k] += a * dt; v.s[k] += v.v[k] * dt;
      if (!(v.s[k] === v.s[k]) || Math.abs(v.s[k]) > 1e4) { v.s[k] = v.p[k]; v.v[k] = 0; }
      v.cur[k] = clamp(v.p[k] + A * (v.s[k] - v.p[k]), LO[k], HI[k]);
    }
    if (v.stage === 4) { v.tail *= Math.exp(-dt / Math.max(0.02, P.rel * 0.35)); if (v.tail < 1e-4 || v.cur[K_LEVEL] < 1e-5) { v.active = false; return; } }
    // analog drift
    const dr = P.drift, ds = Math.sqrt(dt) * 3;
    v.dP = clamp(v.dP + gauss() * ds, -1, 1); v.dC = clamp(v.dC + gauss() * ds, -1, 1);
    v.dPv += (v.dP * dr * 7 - v.dPv) * Math.min(1, dt * 3); v.dCv += (v.dC * dr * 0.25 - v.dCv) * Math.min(1, dt * 3);
    const cur = v.cur, f0 = v.freq * Math.pow(2, cur[K_PITCH]);
    this.ctrlLayer(v, v.A, 'a', f0, dt);
    this.ctrlLayer(v, v.B, 'b', f0 * Math.pow(2, (P.bSemi + P.bFine / 100) / 12), dt);
    // resonator
    const rf = v.freq * P.resTune, rat = [1, 2.756, 5.404], gn = [1, 0.5, 0.28];
    for (let i = 0; i < 3; i++) { const f = Math.min(rf * rat[i], SR * 0.45), w = TAU * f / SR, r = Math.exp(-1 / (Math.max(0.005, P.resDecay / (1 + i * 0.7)) * SR));
      v.rb1[i] = 2 * r * Math.cos(w); v.rb2[i] = -r * r; v.rg[i] = (1 - r) * 2.5 * gn[i]; }
    v.res = cur[K_RES]; v.ring = cur[K_RING]; v.fold = clamp(cur[K_FOLD], 0, 1); v.fbs = cur[K_FB];
    // filter coefficient (2x rate)
    const vc = (v.vel - 0.7) * P.velCut * 3;
    const cut = clamp(Math.pow(2, cur[K_CUT] + P.keytrack * (v.note - 60) / 12 + vc + v.dCv + P.mass * 0.8), 20, 21000);
    v.g0 = v.g1; v.g1 = Math.tan(Math.PI * Math.min(cut, SR * 0.9) / (2 * SR));
    v.lv0 = v.lv1; v.lv1 = cur[K_LEVEL] * (1 - P.velAmp + P.velAmp * v.vel) * v.tail;
  }

  // ---------- per-voice audio ----------
  layerSample(L, pm, emitFlags, syncIn) {
    if (!L.on_) { L.outL = 0; L.outR = 0; emitFlags.e0 = 0; return 0; }
    const N = L.N, tb = L.tb, P = this.P, mask = this._mask, scat = this._scat, fade = L.fade, C = L.C, ws = L.ws, ff = L.ff;
    let sl = 0, sr = 0, emitted0 = 0;
    // coupling (mean field), every sample is fine at N<=6
    if (N > 1 && (L.K > 0.01 || (++L.rc & 63) === 0)) { let cx = 0, sy = 0; for (let e = 0; e < N; e++) { cx += Math.cos(TAU * L.th[e]); sy += Math.sin(TAU * L.th[e]); }
      const R = Math.sqrt(cx * cx + sy * sy) / N, psi = Math.atan2(sy, cx) / TAU, kk = L.K > 0.01 ? L.K * R / SR : 0; L.R = R; L.psi = psi;
      for (let e = 0; e < N; e++) L.ci[e] = kk * Math.sin(TAU * (psi - L.th[e])); }
    else if (N === 1) { L.ci[0] = 0; L.R = 1; }
    for (let e = 0; e < N; e++) {
      const ie = L.inc[e] + L.ci[e];
      let th = L.th[e] + ie;
      if (syncIn) th = 1;
      if (th >= 1) { th -= Math.floor(th);
        const since = syncIn || ie <= 0 ? 0 : Math.min(1, th / ie); // sub-sample start time
        if (e === 0) emitted0 = 1;
        if (Math.random() >= mask) {
          const old = e * 2 + L.cur[e];
          if (L.on[old] && !L.rl[old]) { L.rl[old] = 1; L.rg[old] = 1; }
          const ns = e * 2 + (L.cur[e] ^ 1); L.cur[e] ^= 1;
          L.amp[ns] = 1 - scat * Math.random() * 0.6; L.fm[ns] = scat > 0 ? Math.pow(2, scat * gauss() * 0.12) : 1;
          const st0 = (since - 1) * ff * L.fm[ns];
          L.on[ns] = 1; L.rd[ns] = st0; L.el[ns] = st0; L.fi[ns] = 0; L.rl[ns] = 0; L.rg[ns] = 1;
          L.n0[ns] = Math.random() * 2 - 1; L.n1[ns] = Math.random() * 2 - 1; L.ni[ns] = 0;
          if (scat > 0) th -= scat * Math.random() * 0.45;
        }
      }
      L.th[e] = th;
      for (let q = 0; q < 2; q++) {
        const s = e * 2 + q; if (!L.on[s]) continue;
        const inc = ff * L.fm[s]; L.rd[s] += inc; const el = (L.el[s] += inc);
        const u = el / C;
        if (u >= 1 && !L.rl[s] && !L.contin) { L.rl[s] = 1; L.rg[s] = 1; }
        let g = L.fi[s] < 1 ? (L.fi[s] += fade) : 1; if (g > 1) g = 1;
        if (L.rl[s]) { L.rg[s] -= fade; if (L.rg[s] <= 0) { L.on[s] = 0; continue; } g *= L.rg[s]; }
        const wi = u >= 1 ? WL : u > 0 ? u * WL : 0, wI = wi | 0, wF = wi - wI, wb = ws + wI;
        const w = wI >= WL ? WIN[ws + WL] : WIN[wb] + wF * (WIN[wb + 1] - WIN[wb]);
        let x;
        if (L.mat === 1) { const r = L.rd[s], n = Math.floor(r); if (n !== L.ni[s]) { L.n0[s] = L.n1[s]; L.n1[s] = Math.random() * 2 - 1; L.ni[s] = n; } x = (L.n0[s] + (r - n) * (L.n1[s] - L.n0[s])) * 0.75; }
        else { let ph = L.rd[s] + L.ph0 + pm; ph -= Math.floor(ph); const fi = ph * TL, i0 = fi | 0, fr = fi - i0;
          let a = L.o00 + i0; x = L.w00 * (tb[a] + fr * (tb[a + 1] - tb[a]));
          if (L.w10 > 0) { a = L.o10 + i0; x += L.w10 * (tb[a] + fr * (tb[a + 1] - tb[a])); }
          if (L.w01 > 0) { a = L.o01 + i0; x += L.w01 * (tb[a] + fr * (tb[a + 1] - tb[a])); }
          if (L.w11 > 0) { a = L.o11 + i0; x += L.w11 * (tb[a] + fr * (tb[a + 1] - tb[a])); } }
        x *= w * g * L.amp[s];
        sl += x * L.pl[e]; sr += x * L.pr[e];
      }
    }
    L.outL = sl; L.outR = sr; emitFlags.e0 = emitted0;
    return 1;
  }
  renderVoice(v, off, m) {
    const P = this.P, A = v.A, B = v.B, isLast = v === this.lastV;
    const dz = P.derez, holdN = 1 + Math.floor(dz * dz * 24), q = dz > 0.01 ? Math.pow(2, 15 - dz * 12) : 0;
    const ring = v.ring, res = v.res, fold = v.fold, fgain = (1 + fold * 5) * Math.PI / 2, fbs = v.fbs, pmA = P.pmAmt * 0.6;
    const mode = P.fMode | 0, k = P.reso * 4.1, drv = 1 + P.drive * 5, dcomp = 1 / (1 + P.drive * 1.2), heat = P.heat, hd = 1 + heat * 4, hb = 0.18 * heat, thb = ft(hb), hn = 1 / (hd * (1 - thb * thb) + 1e-9);
    const q2 = 0.5 + P.reso * 40, sv0 = v.svf[0], sv1 = v.svf[1];
    if (mode > 0) { sv0.k = 1 / q2; sv1.k = 1 / q2; }
    const bL = this.bL, bR = this.bR, fbB = this.fbB, t1L = this.t1L, t1R = this.t1R, t2L = this.t2L, t2R = this.t2R, vt = this.vt;
    const ef = this._ef, efB = this._efB, ins = this._ins, outs = this._outs, la = A.on_ ? A.lvl : 0, lb = B.on_ ? B.lvl : 0;
    for (let j = 0; j < m; j++) {
      const fr = (j + 0.5) / m, lv = v.lv0 + (v.lv1 - v.lv0) * fr;
      // layers
      this.layerSample(A, pmA * v.bPrev, ef, false);
      this.layerSample(B, 0, efB, P.bSync && ef.e0);
      const aL = A.outL, aR = A.outR, bLs = B.outL, bRs = B.outR;
      v.bPrev = (bLs + bRs) * 0.5;
      // mixer: osc A level, osc B level, ring (A x B) as its own channel, like the M
      let xL = la * aL + lb * bLs + ring * aL * bLs * 3.2, xR = la * aR + lb * bRs + ring * aR * bRs * 3.2;
      t1L[off + j] += xL * lv; t1R[off + j] += xR * lv;
      // resonator (modal, excited by the voice plus a strike on note-on)
      if (res > 0.001) { let ex = (xL + xR) * 0.5; if (v.strike > 0) { ex += (Math.random() * 2 - 1) * v.strike * P.strike * 2; v.strike *= 0.92; if (v.strike < 1e-3) v.strike = 0; }
        let rs = 0; for (let i = 0; i < 3; i++) { const y = v.rb1[i] * v.ry1[i] + v.rb2[i] * v.ry2[i] + v.rg[i] * ex; v.ry2[i] = v.ry1[i]; v.ry1[i] = y; rs += y; }
        rs = ft(rs * 0.8) * 1.25; xL = xL * (1 - res) + rs * res; xR = xR * (1 - res) + rs * res; }
      // derez (digital resolution)
      if (dz > 0.01) { if (++v.hc >= holdN) { v.hc = 0; v.hL = xL; v.hR = xR; } xL = Math.round(v.hL * q) / q; xR = Math.round(v.hR * q) / q; }
      t2L[off + j] += xL * lv; t2R[off + j] += xR * lv;
      // 2x oversampled: fold -> filter -> VCA saturation
      v.upL.push(xL); v.upR.push(xR);
      ins[0] = v.upL.even(); ins[1] = v.upR.even(); ins[2] = v.upL.odd(); ins[3] = v.upR.odd();
      for (let h = 0; h < 2; h++) {
        const g = v.g0 + (v.g1 - v.g0) * ((j * 2 + h + 1) / (m * 2));
        for (let ch = 0; ch < 2; ch++) {
          let x = ins[h * 2 + ch];
          if (fold > 0.001) x = x * (1 - fold) + Math.sin(x * fgain) * fold * 0.8;
          let y;
          if (mode === 0) { const st = v.lad[ch], G = g / (1 + g), om = 1 - G, G2 = G * G, G4 = G2 * G2;
            const S = om * (G * G2 * st[0] + G2 * st[1] + G * st[2] + st[3]), xin = x * drv;
            const y4 = (G4 * xin + S) / (1 + k * G4); let uu = ft(xin - k * y4);
            for (let i = 0; i < 4; i++) { const vv = (uu - st[i]) * G, yy = vv + st[i]; st[i] = yy + vv; uu = yy; }
            y = uu * (1 + k * 0.45) * dcomp; }
          else { const sv = ch ? sv1 : sv0, a1 = 1 / (1 + g * (g + sv.k)), a2 = g * a1, a3 = g * a2;
            const xin = x * drv * 0.5, v3 = xin - sv.b, v1 = a1 * sv.a + a2 * v3, v2 = sv.b + a2 * sv.a + a3 * v3;
            sv.a = ft((2 * v1 - sv.a) * 0.35) / 0.35; sv.b = 2 * v2 - sv.b;
            y = (mode === 1 ? v2 : mode === 2 ? v1 * sv.k * 2.2 : xin - sv.k * v1 - v2) * 2 * dcomp; }
          y *= lv;
          if (heat > 0.001) y = (ft(hd * y + hb) - thb) * hn;
          outs[h * 2 + ch] = y;
        }
      }
      v.dnL.push(outs[0], outs[2]); v.dnR.push(outs[1], outs[3]);
      const oL = v.dnL.out() * 0.5, oR = v.dnR.out() * 0.5;
      bL[off + j] += oL; bR[off + j] += oR; fbB[off + j] += (oL + oR) * 0.5 * fbs;
      if (isLast) vt[off + j] = (oL + oR) * 0.5;
    }
  }

  // ---------- global ----------
  process(ins, outs) {
    const o = outs[0], L = o[0], R = o[1] || o[0], tap = outs[1] && outs[1][0], n = L.length;
    if (!this.P || !this.S) { L.fill(0); if (R !== L) R.fill(0); if (tap) tap.fill(0); return true; }
    const P = this.P;
    const bL = this.bL, bR = this.bR, fbB = this.fbB; bL.fill(0); bR.fill(0); fbB.fill(0); this.t1L.fill(0); this.t1R.fill(0); this.t2L.fill(0); this.t2R.fill(0); this.vt.fill(0);
    this._mask = 0; this._scat = 0;
    for (let off = 0; off < n; off += CTRL) {
      const m = Math.min(CTRL, n - off), dt = m / SR;
      for (const v of this.voices) { if (!v.active) continue; this.ctrlVoice(v, dt); if (!v.active) continue;
        this._mask = clamp(v.cur[K_MASK] + P.grainM * 0.3, 0, 0.98); this._scat = clamp(v.cur[K_SCAT] + P.allure * 0.4, 0, 1);
        this.renderVoice(v, off, m); }
    }
    // vocoder setup
    const vocOn = P.vocOn, body = P.vocBody | 0, skin = P.vocSkin | 0, spread = P.vocSpread, sh = Math.pow(2, P.vocShift + P.mass * 0.6), vq = P.vocQ;
    if (vocOn) for (let b = 0; b < 8; b++) { const fc = 1000 * Math.pow(2, (b - 3.5) * 0.85 * spread); this.anS[b].set(fc, vq, SR); this.syL[b].set(fc * sh, vq, SR); this.syR[b].set(fc * sh, vq, SR); }
    const resp = P.vocResp, atk = 1 - Math.exp(-1 / ((0.001 + resp * 0.04) * SR)), relc = 1 - Math.exp(-1 / ((0.02 + resp * 0.7) * SR));
    const dep = Math.min(1, P.scrapeDepth + P.allure * 0.4), dustP = P.dustRate / SR, dropP = P.dropRate / SR;
    // feedback setup
    this.fbF.set(P.fbCut, P.fbQ, SR);
    const len = this.dlen, tgt = clamp(P.fbTime / 1000 * SR, 2, len - 4), t3 = ft(Math.max(0.3, P.fbDrive));
    // space setup
    const size = P.spSize, rt = Math.max(0.1, P.spDecay), damp = 1 - Math.exp(-TAU * (800 + P.spTone * 14000) / SR);
    for (const f of this.fdn) { f.len = Math.min(f.buf.length - 12, Math.floor(f.base * size * SR / 1000)); f.g = Math.pow(10, -3 * f.len / (rt * SR)); }
    const spMix = P.spMix, listen = P.listen | 0;
    let sumSq = 0, act = 0; for (const v of this.voices) if (v.active) act++;
    for (let i = 0; i < n; i++) {
      let l = bL[i], r = bR[i];
      // feedback loop
      this.dtS += (tgt - this.dtS) * 0.0008; let rp = this.dw - this.dtS; while (rp < 0) rp += len;
      const i0 = rp | 0, fr = rp - i0, dly = this.dbuf[i0] * (1 - fr) + this.dbuf[(i0 + 1) % len] * fr;
      let rp2 = this.dw - this.dtS * 1.07; while (rp2 < 0) rp2 += len; const j0 = rp2 | 0, dly2 = this.dbuf[j0] + (rp2 - j0) * (this.dbuf[(j0 + 1) % len] - this.dbuf[j0]);
      this.fbF.tick(dly); const ff = P.fbType === 0 ? this.fbF.lp : P.fbType === 1 ? this.fbF.bp * this.fbF.k : this.fbF.hp;
      const y = ft(ff * P.fbDrive) / t3; const dc = y - this.dcx + 0.996 * this.dcy; this.dcx = y; this.dcy = dc;
      this.dbuf[this.dw] = ft(fbB[i] + dc * P.fbRegen); this.dw = (this.dw + 1) % len;
      const fL = dc * P.fbMix, fR = (0.6 * dc + 0.4 * dly2) * P.fbMix; l += fL; r += fR;
      // material vocoder
      let wL = 0, wR = 0;
      if (vocOn) {
        let bdL = 0, bdR = 0;
        if (body === 0) { bdL = l; bdR = r; }
        else if (body === 1) { bdL = Math.random() * 2 - 1; bdR = Math.random() * 2 - 1; }
        else if (body === 2) { let im = 0, pp = 0; if (Math.random() < dustP) { const a = Math.pow(Math.random(), 3) * (Math.random() < 0.5 ? -1 : 1); if (Math.random() < 0.08) pp = a * 3; else im = a; }
          const pan = Math.random(), iL = im * (1 - pan) * 6, iR = im * pan * 6;
          this.dustL[0].tick(iL); this.dustL[1].tick(iL); this.dustR[0].tick(iR); this.dustR[1].tick(iR); this.popL.tick(pp * (1 - pan)); this.popR.tick(pp * pan);
          bdL = this.dustL[0].bp + this.dustL[1].bp + iL * 0.3 + this.popL.bp; bdR = this.dustR[0].bp + this.dustR[1].bp + iR * 0.3 + this.popR.bp; }
        else if (body === 3) { if (Math.random() < dropP) for (const d of this.drops) if (!d.on) { d.on = 1; d.t = 0; d.ph = 0; d.f0 = (300 + Math.random() * 2200) * P.dropPitch; d.ch = 0.6 + Math.random() * 2; d.tc = 0.004 + Math.random() * 0.01; d.td = 0.006 + Math.random() * 0.02; d.a = 0.4 + Math.random() * 0.6; const pn = Math.random(); d.pl = Math.cos(pn * 1.5708); d.pr = Math.sin(pn * 1.5708); break; }
          for (const d of this.drops) { if (!d.on) continue; d.t += 1 / SR; const f = d.f0 * (1 + d.ch * (1 - Math.exp(-d.t / d.tc))); d.ph += f / SR; const env = d.a * Math.exp(-d.t / d.td) * Math.min(1, d.t * 4000);
            const s = Math.sin(TAU * d.ph) * env; bdL += s * d.pl; bdR += s * d.pr; if (d.t > d.td * 7) d.on = 0; } }
        else { bdL = fL * 2; bdR = fR * 2; }
        const sk = skin === 0 ? (l + r) * 0.5 : skin === 1 ? dc : 0;
        this.scr += P.scrapeRate / SR; if (this.scr > 1) this.scr -= 1;
        for (let b = 0; b < 8; b++) {
          let e = 1; if (skin !== 2) { const an = this.anS[b]; an.tick(sk); const a = Math.abs(an.bp) * 4; const ev = this.env[b]; this.env[b] = ev + (a - ev) * (a > ev ? atk : relc); e = this.env[b]; }
          const lvb = P.bands[b] * (dep > 0 ? 1 - dep * 0.5 * (1 + Math.sin(TAU * (this.scr + b / 8))) : 1);
          const g = e * lvb * (body === 1 ? Math.sqrt(1000 / (1000 * Math.pow(2, (b - 3.5) * 0.85 * spread))) * 0.5 : 1);
          this.syL[b].tick(bdL); this.syR[b].tick(bdR); wL += this.syL[b].bp * this.syL[b].k * g; wR += this.syR[b].bp * this.syR[b].k * g; }
        // auto-level the wet signal to the dry signal so Mix compares like with like
        const dry = (l * l + r * r) * 0.5, wet = (wL * wL + wR * wR) * 0.5;
        this.vDry += (dry - this.vDry) * 0.00007; this.vWet += (wet - this.vWet) * 0.00007;
        if (P.vocAuto && this.vDry > 1e-8) this.vGain += (clamp(0.75 * Math.sqrt(this.vDry / Math.max(this.vWet, 1e-12)), 0.05, 40) - this.vGain) * 0.0005;
        const vg = P.vocAuto ? this.vGain : 1; wL *= vg; wR *= vg;
        l = l * (1 - P.vocMix) + wL * P.vocMix; r = r * (1 - P.vocMix) + wR * P.vocMix;
      }
      // space (8-line FDN with pre-diffusion)
      let sL = 0, sR = 0;
      if (spMix > 0.001) {
        let xl = l * 0.5, xr = r * 0.5;
        for (const a of this.apL) { const z = a.b[a.i], w = xl + z * 0.62; a.b[a.i] = w; xl = z - w * 0.62; a.i = (a.i + 1) % a.b.length; }
        for (const a of this.apR) { const z = a.b[a.i], w = xr + z * 0.62; a.b[a.i] = w; xr = z - w * 0.62; a.i = (a.i + 1) % a.b.length; }
        this.modPh += 0.37 / SR; if (this.modPh > 1) this.modPh -= 1;
        const fd = this.fdn, outv = this._fo || (this._fo = new Float64Array(8)); let sum = 0;
        for (let q = 0; q < 8; q++) { const f = fd[q]; let d = f.len; if (q === 1) d += Math.sin(TAU * this.modPh) * 6; if (q === 6) d += Math.cos(TAU * this.modPh * 1.3) * 7;
          let rpq = f.w - d; while (rpq < 0) rpq += f.buf.length; const a0 = rpq | 0, af = rpq - a0, b1 = (a0 + 1) % f.buf.length;
          let s = f.buf[a0] + af * (f.buf[b1] - f.buf[a0]); f.lp += (s - f.lp) * damp; s = f.lp * f.g; outv[q] = s; sum += s; }
        sum *= 0.25;
        for (let q = 0; q < 8; q++) { const f = fd[q]; f.buf[f.w] = outv[q] - sum + (q & 1 ? xr : xl); f.w = (f.w + 1) % f.buf.length; }
        sL = (outv[0] - outv[2] + outv[4] - outv[6]) * 0.5; sR = (outv[1] - outv[3] + outv[5] - outv[7]) * 0.5;
        l += sL * spMix; r += sR * spMix;
      }
      // listen point
      let oL = l, oR = r;
      if (listen === 1) { oL = this.t1L[i]; oR = this.t1R[i]; } else if (listen === 2) { oL = this.t2L[i]; oR = this.t2R[i]; } else if (listen === 3) { oL = bL[i]; oR = bR[i]; }
      else if (listen === 4) { oL = wL; oR = wR; } else if (listen === 5) { oL = fL; oR = fR; } else if (listen === 6) { oL = sL; oR = sR; }
      if (listen) { const ms = (oL * oL + oR * oR) * 0.5; this.agcR += (ms - this.agcR) * 0.00005;
        if (this.agcR > 1e-9) this.agc += (clamp(0.12 / Math.sqrt(this.agcR), 0.25, 16) - this.agc) * 0.0002; oL *= this.agc; oR *= this.agc; }
      sumSq += (oL * oL + oR * oR) * 0.5;
      const g = P.vol * P.abTrim;
      oL *= g; oR *= g;
      // DC block + soft clip
      const dl = oL - this.dcL[0] + 0.9995 * this.dcL[1]; this.dcL[0] = oL; this.dcL[1] = dl;
      const drr = oR - this.dcR[0] + 0.9995 * this.dcR[1]; this.dcR[0] = oR; this.dcR[1] = drr;
      oL = ft(dl); oR = ft(drr);
      if (!(oL === oL) || !(oR === oR)) { oL = 0; oR = 0; this.panicDSP(); }
      L[i] = oL; R[i] = oR;
      if (tap) tap[i] = P.tap === 0 ? (oL + oR) * 0.5 : P.tap === 1 ? this.vt[i] : dc;
      if (this.rec) { this.recBuf[this.recN * 2] = oL; this.recBuf[this.recN * 2 + 1] = oR; if (++this.recN >= 8192) { this.port.postMessage({ type: 'recData', d: this.recBuf.slice(0) }); this.recN = 0; } }
    }
    if (act) this.outR = this.outR * 0.9 + 0.1 * (sumSq / n);
    if ((this.mtrC += n) > SR / 10) { this.mtrC = 0; this.port.postMessage({ type: 'meter', rms: Math.sqrt(this.outR), active: act }); }
    if ((this.vizC += n) > SR / 30) { this.vizC = 0; const v = this.lastV;
      if (v && v.active) { const N = v.A.N, th = [], e = [];
        for (let i = 0; i < N; i++) { th.push(v.A.th[i]); let a = 0; for (let q = 0; q < 2; q++) if (v.A.on[i * 2 + q]) a = Math.max(a, v.A.rl[i * 2 + q] ? v.A.rg[i * 2 + q] : 1); e.push(a); }
        this.port.postMessage({ type: 'viz', on: true, N, th, e, R: v.A.R, stage: v.stage, f0: v.A.inc[0] * SR, ff: v.A.ff * SR, cur: Array.from(v.cur), voices: act }); }
      else this.port.postMessage({ type: 'viz', on: false, voices: act }); }
    return true;
  }
  panicDSP() { this.dbuf.fill(0); this.fbF.reset(); for (const f of this.fdn) { f.buf.fill(0); f.lp = 0; } for (const s of this.syL.concat(this.syR, this.anS)) s.reset();
    for (const v of this.voices) { for (const l of v.lad) l.fill(0); v.svf[0].reset(); v.svf[1].reset(); v.ry1 = [0, 0, 0]; v.ry2 = [0, 0, 0]; } }
}
registerProcessor('silt', Silt);
