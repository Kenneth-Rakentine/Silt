// Offline harness: runs the worklet engine in Node, renders patches, measures levels, CPU and aliasing.
const fs = require('fs');
globalThis.sampleRate = 48000; let Proc; globalThis.registerProcessor = (n, c) => Proc = c;
globalThis.AudioWorkletProcessor = class { constructor() { this.port = { postMessage() {}, onmessage: null }; } };
const t0 = Date.now(); require('./engine.js'); const { PATCHES, makePatch } = require('./patches.js');
function newEngine() { const t = Date.now(); const p = new Proc(); return { p, ms: Date.now() - t }; }
function render(p, secs, events) { // events: [{t, type, n, v}]
  const N = Math.ceil(secs * 48000 / 128), L = new Float32Array(N * 128), R = new Float32Array(N * 128); let ev = 0;
  const t = Date.now();
  for (let b = 0; b < N; b++) { const now = b * 128 / 48000; while (ev < events.length && events[ev].t <= now) { p.msg(events[ev]); ev++; }
    const o = [[new Float32Array(128), new Float32Array(128)], [new Float32Array(128)]]; p.process([], o); L.set(o[0][0], b * 128); R.set(o[0][1], b * 128); }
  return { L, R, cpu: (Date.now() - t) / 1000 / secs };
}
function stats(L, R) { let pk = 0, ss = 0, nan = 0; for (let i = 0; i < L.length; i++) { const a = L[i], b = R[i]; if (a !== a || b !== b) nan++; pk = Math.max(pk, Math.abs(a), Math.abs(b)); ss += (a * a + b * b) / 2; }
  return { peak: +pk.toFixed(3), rmsDb: +(10 * Math.log10(ss / L.length + 1e-12)).toFixed(1), nan }; }
function wav(path, L, R) { const n = L.length, b = Buffer.alloc(44 + n * 4); b.write('RIFF', 0); b.writeUInt32LE(36 + n * 4, 4); b.write('WAVEfmt ', 8); b.writeUInt32LE(16, 16); b.writeUInt16LE(1, 20); b.writeUInt16LE(2, 22); b.writeUInt32LE(48000, 24); b.writeUInt32LE(192000, 28); b.writeUInt16LE(4, 32); b.writeUInt16LE(16, 34); b.write('data', 36); b.writeUInt32LE(n * 4, 40);
  for (let i = 0; i < n; i++) { b.writeInt16LE(Math.round(Math.max(-1, Math.min(1, L[i])) * 32767), 44 + i * 4); b.writeInt16LE(Math.round(Math.max(-1, Math.min(1, R[i])) * 32767), 46 + i * 4); } fs.writeFileSync(path, b); }
module.exports = { newEngine, render, stats, wav, PATCHES, makePatch };
if (require.main === module) {
  const { p, ms } = newEngine(); console.log('table build ms', ms);
  fs.mkdirSync('out', { recursive: true });
  for (const name of Object.keys(PATCHES)) {
    const e = newEngine().p; const pt = makePatch(name); e.msg({ type: 'globals', P: pt.P }); e.msg({ type: 'states', S: pt.S });
    const chord = name.includes('Transient') ? [36, 36.0001] : [48, 55, 60, 64];
    const evs = []; chord.forEach((n, i) => evs.push({ t: 0.05 + i * 0.01, type: 'on', n: Math.round(n), v: 0.9 }));
    if (name.includes('Transient')) { [0.05, 0.45, 0.85, 1.25].forEach((t, i) => { evs.push({ t, type: 'on', n: [36, 36, 43, 31][i], v: 0.95 }); evs.push({ t: t + 0.2, type: 'off', n: [36, 36, 43, 31][i] }); }); }
    else chord.forEach(n => evs.push({ t: 3, type: 'off', n }));
    evs.sort((a, b) => a.t - b.t);
    const r = render(e, 5, evs); const s = stats(r.L, r.R);
    console.log(name.padEnd(26), JSON.stringify(s), 'cpu', (r.cpu * 100).toFixed(1) + '%');
    wav('out/' + name.replace(/[^a-z0-9]+/gi, '_') + '.wav', r.L, r.R);
  }
}
