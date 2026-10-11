// Writes test cases (flat params, states, note events) for tools/engine_check.cpp, and renders the same cases with the JS engine.
// Usage: node tools/dump_cases.js <outdir>
const fs = require('fs'), path = require('path');
const out = process.argv[2] || 'cmp'; fs.mkdirSync(out, { recursive: true });
const H = require(path.join(__dirname, '..', 'lab', 'harness.js'));
const KEYS = ['level', 'pitch', 'formant', 'position', 'coupling', 'mask', 'scatter', 'fold', 'cutoff', 'resonator', 'ring', 'feedback'];
let txt = '', i = 0;
for (const name of Object.keys(H.PATCHES)) {
  const pt = H.makePatch(name);
  let evs;
  if (name.includes('Transient')) evs = [0.05, 0.45, 0.85, 1.25].flatMap((t, k) => [{ t, type: 'on', n: [36, 36, 43, 31][k], v: 0.95 }, { t: t + 0.2, type: 'off', n: [36, 36, 43, 31][k] }]);
  else evs = [48, 55, 60, 64].map(n => ({ t: 0.05, type: 'on', n, v: 0.9 })).concat([48, 55, 60, 64].map(n => ({ t: 3, type: 'off', n })));
  evs.sort((a, b) => a.t - b.t);
  txt += 'case ' + i + '\n';
  for (const [k, v] of Object.entries(pt.P)) { if (k === 'bands') v.forEach((b, j) => txt += 'p band' + j + ' ' + b + '\n'); else txt += 'p ' + k + ' ' + v + '\n'; }
  pt.S.forEach((s, st) => KEYS.forEach((k, j) => txt += 's ' + st + ' ' + j + ' ' + s[k] + '\n'));
  evs.forEach(e => txt += 'e ' + e.t + ' ' + (e.type === 'on' ? 1 : 0) + ' ' + e.n + ' ' + (e.v || 0) + '\n');
  txt += 'end\n';
  const e = H.newEngine().p; e.msg({ type: 'globals', P: pt.P }); e.msg({ type: 'states', S: pt.S });
  const r = H.render(e, 5, evs);
  const buf = Buffer.alloc(r.L.length * 8); for (let j = 0; j < r.L.length; j++) { buf.writeFloatLE(r.L[j], j * 8); buf.writeFloatLE(r.R[j], j * 8 + 4); }
  fs.writeFileSync(path.join(out, 'js' + i + '.f32'), buf); i++;
}
fs.writeFileSync(path.join(out, 'cases.txt'), txt); console.log('cases', i);
