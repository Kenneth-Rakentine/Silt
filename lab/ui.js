(function () {
"use strict";
const $ = id => document.getElementById(id);
const el = (tag, cls, txt) => { const e = document.createElement(tag); if (cls) e.className = cls; if (txt != null) e.textContent = txt; return e; };
const clone = o => JSON.parse(JSON.stringify(o));
let P = clone(BASE), S = [];
let patchDefault = null, currentName = '';
let ctx = null, node = null, analyser = null, running = false, viz = null;
const refreshers = []; let uid = 0;
const refreshAll = () => refreshers.forEach(f => f());
let pushQ = false;
const pushP = () => { if (!node || pushQ) return; pushQ = true; requestAnimationFrame(() => { pushQ = false; node.port.postMessage({ type: 'globals', P }); }); };
const pushS = () => { if (node) node.port.postMessage({ type: 'states', S }); };
const pushAll = () => { if (node) { node.port.postMessage({ type: 'globals', P }); node.port.postMessage({ type: 'states', S }); } };

const BANKS = ['Classic', 'Hollow 90s', 'Reeds and brass', 'Vocal', 'Glass', 'Bitmask'];
const FRAMES = [
  ['Sine', 'Triangle', 'Soft saw', 'Saw', 'Square', 'Pulse 30', 'Pulse 12', 'Resonant saw'],
  ['Hollow', 'Soft square', 'Organ', 'PPG cluster', 'Digital flute', '3-bit tanh', 'Wood', 'Hollow reed'],
  ['Trombone', 'Horn', 'Alto sax', 'Tenor sax', 'Clarinet', 'Oboe', 'Bassoon', 'Muted trumpet'],
  ['A', 'E', 'I', 'O', 'U', 'A (high)', 'E (high)', 'Ng'],
  ['Odd spikes', 'Peak 20', 'Tine', 'Squares', 'Shimmer', 'Crystal', 'High cluster', 'Fibonacci'],
  ['4-bit sine', 'i & i>>3', 'i ^ i>>4', '5i & i>>2', 'i | i>>5', 'Popcount', '3i ^ i>>1', 'Pulse + LFSR'],
];
const F = {
  pct: x => Math.round(x * 100) + '%', hz: x => x >= 1000 ? (x / 1000).toFixed(2) + ' kHz' : x.toFixed(x < 10 ? 2 : 0) + ' Hz',
  time: x => x < 1 ? (x * 1000).toFixed(x < 0.01 ? 1 : 0) + ' ms' : x.toFixed(2) + ' s', ms: x => x.toFixed(x < 10 ? 1 : 0) + ' ms',
  oct: x => (x >= 0 ? '+' : '') + x.toFixed(2) + ' oct', x: x => '×' + x.toFixed(2), n: x => String(Math.round(x)), num: x => x.toFixed(2),
  bi: x => (x >= 0 ? '+' : '') + x.toFixed(2), cents: x => (x >= 0 ? '+' : '') + x.toFixed(1) + ' ct', semi: x => (x >= 0 ? '+' : '') + Math.round(x) + ' st',
  per: x => x.toFixed(x < 10 ? 1 : 0) + ' /s', ratio: x => '×' + (x < 10 ? x.toFixed(2) : x.toFixed(1)),
  win: x => x < 0.02 ? 'Rect' : x < 0.48 ? 'Rect→Hann' : x < 0.52 ? 'Hann' : x < 0.98 ? 'Hann→Exp' : 'Expodec',
};
const posFmt = bankKey => x => { const b = FRAMES[P[bankKey] | 0], p = x * 7, i = Math.min(6, Math.floor(p)), f = p - i;
  return f < 0.05 ? b[i] : f > 0.95 ? b[i + 1] : b[i] + '→' + b[i + 1]; };

function slider(parent, o) {
  const row = el('div', 'row'), id = 'c' + (++uid), lab = el('label', 'lab', o.label); lab.htmlFor = id;
  const inp = document.createElement('input'); inp.type = 'range'; inp.id = id; inp.min = '0'; inp.max = '1000'; inp.step = '1';
  const out = el('output', 'val');
  const toPos = x => o.log ? Math.log(x / o.min) / Math.log(o.max / o.min) * 1000 : (x - o.min) / (o.max - o.min) * 1000;
  const fromPos = p => { let x = o.log ? o.min * Math.pow(o.max / o.min, p / 1000) : o.min + (o.max - o.min) * p / 1000; if (o.int) x = Math.round(x); return x; };
  const fmt = typeof o.fmt === 'function' ? o.fmt : (F[o.fmt] || F.num);
  const refresh = () => { const x = o.get(); inp.value = String(Math.max(0, Math.min(1000, toPos(x)))); out.textContent = fmt(x); };
  inp.addEventListener('input', () => { const x = fromPos(+inp.value); o.set(x); out.textContent = fmt(x); });
  inp.addEventListener('dblclick', () => { const d = o.def ? o.def() : undefined; if (d !== undefined) { o.set(d); refresh(); } });
  row.append(lab, inp, out); parent.append(row); refreshers.push(refresh); refresh(); return { refresh };
}
function gs(parent, key, label, min, max, fmt, extra) {
  return slider(parent, Object.assign({ label, min, max, fmt, def: () => patchDefault ? patchDefault.P[key] : BASE[key], get: () => P[key], set: x => { P[key] = x; pushP(); } }, extra || {}));
}
function select(parent, key, label, opts, onChange) {
  const w = el('div', 'sel'), id = 'c' + (++uid), lab = el('label', null, label); lab.htmlFor = id;
  const s = document.createElement('select'); s.id = id; opts.forEach((t, i) => { const o = el('option', null, t); o.value = String(i); s.append(o); });
  s.addEventListener('change', () => { P[key] = +s.value; pushP(); if (onChange) onChange(); });
  refreshers.push(() => { s.value = String(P[key] | 0); }); w.append(lab, s); parent.append(w); s.value = String(P[key] | 0);
}
function toggle(parent, key, onTxt, offTxt) {
  const b = el('button'); b.id = 'c' + (++uid); const r = () => { b.setAttribute('aria-pressed', P[key] ? 'true' : 'false'); b.textContent = P[key] ? onTxt : offTxt; };
  b.addEventListener('click', () => { P[key] = P[key] ? 0 : 1; pushP(); r(); }); refreshers.push(r); r(); parent.append(b); return b;
}

/* ---------- panels ---------- */
function layerPanel(root, pre) {
  const tr = el('div', 'btnrow'); root.append(tr); toggle(tr, pre + 'On', 'Layer on', 'Layer off');
  if (pre === 'b') toggle(tr, 'bSync', 'Sync to A on', 'Sync to A off');
  select(root, pre + 'Mat', 'Material', ['Wavetable', 'Noise'], refreshAll);
  select(root, pre + 'Bank', 'Bank', BANKS, refreshAll);
  gs(root, pre + 'Pos', 'Position', 0, 1, posFmt(pre + 'Bank'));
  gs(root, pre + 'Form', 'Formant', 0.25, 32, 'ratio', { log: true });
  gs(root, pre + 'Cyc', 'Cycles', 0.25, 8, 'num', { log: true });
  gs(root, pre + 'Win', 'Window', 0, 1, 'win');
  gs(root, pre + 'Phase', 'Phase', 0, 1, 'pct');
  gs(root, pre + 'N', 'Emitters', 1, 6, 'n', { int: true });
  gs(root, pre + 'Det', 'Detune spread', 0, 100, 'cents');
  gs(root, pre + 'Wander', 'Wander', 0, 1, 'pct');
  gs(root, pre + 'Pan', 'Stereo spread', 0, 1, 'pct');
  gs(root, pre + 'Lvl', 'Level', 0, 1, 'pct');
  if (pre === 'b') { gs(root, 'bSemi', 'Pitch', -24, 24, 'semi', { int: true }); gs(root, 'bFine', 'Fine', -50, 50, 'cents'); }
}
layerPanel($('layerA'), 'a'); layerPanel($('layerB'), 'b');
{ const r = $('voiceC');
  gs(r, 'pmAmt', 'FM (B into A)', 0, 1, 'pct'); gs(r, 'formTrack', 'Formant track', 0, 1, 'pct'); gs(r, 'wanderRate', 'Wander rate', 0, 1, 'pct');
  gs(r, 'derez', 'Derez', 0, 1, 'pct'); gs(r, 'drift', 'Analog drift', 0, 1, 'pct');
  gs(r, 'resTune', 'Resonator tune', 0.25, 8, 'x', { log: true }); gs(r, 'resDecay', 'Resonator decay', 0.01, 4, 'time', { log: true }); gs(r, 'strike', 'Strike', 0, 1, 'pct'); }
{ const r = $('filterC');
  select(r, 'fMode', 'Mode', ['Ladder LP 24', 'State variable LP 12', 'State variable BP', 'State variable HP']);
  gs(r, 'reso', 'Resonance', 0, 1, 'pct'); gs(r, 'drive', 'Drive', 0, 1, 'pct'); gs(r, 'heat', 'VCA heat', 0, 1, 'pct');
  gs(r, 'keytrack', 'Key tracking', 0, 1, 'pct'); gs(r, 'velCut', 'Velocity → cutoff', 0, 1, 'pct'); }
{ const r = $('macros'); gs(r, 'mass', 'Mass', -1, 1, 'bi'); gs(r, 'grainM', 'Grain', -1, 1, 'bi'); gs(r, 'allure', 'Allure', 0, 1, 'pct'); gs(r, 'vol', 'Volume', 0, 2.5, 'pct'); }
{ const r = $('vocTop'); const tr = el('div', 'btnrow'); r.append(tr); toggle(tr, 'vocOn', 'Vocoder on', 'Vocoder off'); toggle(tr, 'vocAuto', 'Auto-level on', 'Auto-level off');
  select(r, 'vocBody', 'Body', ['Voices', 'Noise', 'Dust (crackle)', 'Droplets (water)', 'Feedback']);
  select(r, 'vocSkin', 'Skin', ['Voices', 'Feedback', 'Fixed (bands only)']); }
{ const r = $('vocoder');
  gs(r, 'vocMix', 'Mix', 0, 1, 'pct'); gs(r, 'vocShift', 'Shift', -1.5, 1.5, 'oct'); gs(r, 'vocSpread', 'Band spacing', 0.5, 1.6, 'x');
  gs(r, 'vocQ', 'Band width (Q)', 0.7, 20, 'num', { log: true }); gs(r, 'vocResp', 'Response', 0, 1, 'pct');
  gs(r, 'scrapeRate', 'Scrape rate', 0.05, 400, 'hz', { log: true }); gs(r, 'scrapeDepth', 'Scrape depth', 0, 1, 'pct');
  gs(r, 'dustRate', 'Dust density', 5, 3000, 'per', { log: true }); gs(r, 'dropRate', 'Droplet density', 1, 300, 'per', { log: true }); gs(r, 'dropPitch', 'Droplet pitch', 0.3, 3, 'x', { log: true }); }
{ const r = $('feedback'); select(r, 'fbType', 'Filter', ['Low pass', 'Band pass', 'High pass']);
  gs(r, 'fbTime', 'Delay time', 1, 1500, 'ms', { log: true }); gs(r, 'fbCut', 'Cutoff', 40, 12000, 'hz', { log: true }); gs(r, 'fbQ', 'Resonance', 0.5, 20, 'num', { log: true });
  gs(r, 'fbDrive', 'Drive', 0.3, 8, 'num', { log: true }); gs(r, 'fbRegen', 'Regeneration', 0, 1.1, 'pct'); gs(r, 'fbMix', 'Return level', 0, 1, 'pct'); }
{ const r = $('space'); gs(r, 'spSize', 'Size', 0.35, 2, 'x'); gs(r, 'spDecay', 'Decay', 0.2, 20, 'time', { log: true }); gs(r, 'spTone', 'Tone', 0, 1, 'pct'); gs(r, 'spMix', 'Mix', 0, 1, 'pct'); }
{ const a = $('motionA'); gs(a, 'atk', 'Onset time', 0.001, 8, 'time', { log: true }); gs(a, 'dec', 'To continuant', 0.001, 8, 'time', { log: true });
  gs(a, 'rel', 'Release', 0.001, 8, 'time', { log: true }); gs(a, 'curve', 'Curve', -1, 1, 'bi'); gs(a, 'loopTime', 'Loop time', 0.02, 8, 'time', { log: true });
  const tr = el('div', 'btnrow'); tr.style.marginTop = '6px'; a.append(tr); toggle(tr, 'loop', 'Loop continuant on', 'Loop continuant off');
  const b = $('motionB'); gs(b, 'auth', 'Authority', 0, 1, 'pct'); gs(b, 'stiff', 'Stiffness', 0.2, 30, 'hz', { log: true }); gs(b, 'visc', 'Viscosity', 0.05, 3, 'num', { log: true }); gs(b, 'temp', 'Temperature', 0, 1, 'pct');
  const c = $('playing'); gs(c, 'poly', 'Voices', 1, 6, 'n', { int: true }); gs(c, 'velAmp', 'Velocity → level', 0, 1, 'pct'); }
const SSPEC = [['level', 'Level', 0, 1.3, 'pct'], ['pitch', 'Emission', -7, 2, 'oct'], ['formant', 'Formant', -3, 3, 'oct'], ['position', 'Position', -1, 1, 'bi'],
  ['coupling', 'Coupling', 0, 1, 'pct'], ['mask', 'Masking', 0, 1, 'pct'], ['scatter', 'Scatter', 0, 1, 'pct'], ['fold', 'Fold', 0, 1, 'pct'],
  ['cutoff', 'Cutoff', 20, 20000, 'hz', true], ['resonator', 'Resonator', 0, 1, 'pct'], ['ring', 'Ring (A×B)', 0, 1, 'pct'], ['feedback', 'Feedback send', 0, 1, 'pct']];
['Onset', 'Continuant', 'Termination'].forEach((nm, i) => { const box = el('div', 'state'); box.append(el('h3', null, nm));
  SSPEC.forEach(([k, l, mn, mx, f, lg]) => slider(box, { label: l, min: mn, max: mx, fmt: f, log: !!lg, def: () => patchDefault ? patchDefault.S[i][k] : undefined, get: () => S[i] ? S[i][k] : 0, set: x => { S[i][k] = x; pushS(); } }));
  $('states').append(box); });
const centers = [0, 1, 2, 3, 4, 5, 6, 7].map(b => 1000 * Math.pow(2, (b - 3.5) * 0.85));
centers.forEach((c, b) => { const w = el('div'); w.style.display = 'grid'; w.style.justifyItems = 'center'; w.style.gap = '4px';
  const inp = document.createElement('input'); inp.type = 'range'; inp.min = '0'; inp.max = '1'; inp.step = '0.01'; inp.id = 'band' + b; inp.setAttribute('aria-label', 'Band ' + (b + 1) + ' level');
  inp.addEventListener('input', () => { P.bands[b] = +inp.value; pushP(); }); refreshers.push(() => { inp.value = String(P.bands[b]); });
  w.append(inp, el('small', null, c >= 1000 ? (c / 1000).toFixed(1) + 'k' : String(Math.round(c)))); $('bands').append(w); });
Object.keys(BANDS).forEach(k => { const b = el('button', null, k); b.addEventListener('click', () => { P.bands = BANDS[k].slice(); pushP(); refreshAll(); }); $('bandPresets').append(b); });
$('tap').addEventListener('change', e => { P.tap = +e.target.value; pushP(); });

/* ---------- patches ---------- */
const NOTE_LABELS = [['target', 'Target'], ['listen', 'Listen for'], ['decides', 'It decides'], ['play', 'Try']];
function showNotes(notes) { const dl = $('notes'); dl.textContent = '';
  if (!notes) { const d = el('div'); d.append(el('dt', null, 'Patch'), el('dd', null, 'Your saved patch. The notes belong to the numbered test patches.')); dl.append(d); return; }
  NOTE_LABELS.forEach(([k, t]) => { const d = el('div'); d.append(el('dt', null, t), el('dd', null, notes[k])); dl.append(d); }); }
function applyPatch(pt, notes) { const keep = { listen: P.listen, tap: P.tap }; P = Object.assign(clone(BASE), clone(pt.P), keep); S = clone(pt.S); patchDefault = clone({ P, S }); showNotes(notes); refreshAll(); pushAll(); }
function loadPreset(name) { currentName = name; if (PATCHES[name]) applyPatch(makePatch(name), PATCHES[name].notes); else { const u = userPatches()[name.slice(5)]; if (u) applyPatch(u, null); } }
function userPatches() { try { return JSON.parse(localStorage.getItem('silt-patches') || '{}'); } catch (e) { return {}; } }
function fillPresetMenu() { const s = $('preset'); s.textContent = ''; const g1 = el('optgroup'); g1.label = 'Tests and references';
  Object.keys(PATCHES).forEach(n => { const o = el('option', null, n); o.value = n; g1.append(o); }); s.append(g1);
  const up = userPatches(), names = Object.keys(up); if (names.length) { const g2 = el('optgroup'); g2.label = 'Your patches'; names.forEach(n => { const o = el('option', null, n); o.value = 'user:' + n; g2.append(o); }); s.append(g2); }
  s.value = currentName; }
$('preset').addEventListener('change', e => loadPreset(e.target.value));
$('savePatch').addEventListener('click', () => { const n = $('saveName').value.trim(); if (!n) { $('takeNote').textContent = 'Type a name first.'; return; }
  try { const up = userPatches(); up[n] = { P: clone(P), S: clone(S) }; localStorage.setItem('silt-patches', JSON.stringify(up)); currentName = 'user:' + n; fillPresetMenu(); $('takeNote').textContent = 'Saved "' + n + '" in this browser.'; }
  catch (e) { $('takeNote').textContent = 'This browser is not letting the page save. Use Copy patch instead.'; } });
const patchText = () => JSON.stringify({ silt: 2, name: currentName, P, S });
$('copyPatch').addEventListener('click', () => { const t = patchText();
  const fallback = () => { $('loadBox').hidden = false; $('loadToggle').setAttribute('aria-expanded', 'true'); $('loadText').value = t; $('loadText').select(); $('takeNote').textContent = 'Copy the selected text, then paste it to me in chat.'; };
  try { navigator.clipboard.writeText(t).then(() => { $('takeNote').textContent = 'Patch copied. Paste it to me in chat and I will see every setting.'; }, fallback); } catch (e) { fallback(); } });
$('loadToggle').addEventListener('click', () => { const b = $('loadBox'); b.hidden = !b.hidden; $('loadToggle').setAttribute('aria-expanded', String(!b.hidden)); });
$('loadGo').addEventListener('click', () => { try { const o = JSON.parse($('loadText').value); if (!o.P || !o.S) throw 0; applyPatch({ P: o.P, S: o.S }, null); currentName = ''; $('takeNote').textContent = 'Patch loaded.'; }
  catch (e) { $('takeNote').textContent = 'That text is not a Silt patch. Copy it again from the lab.'; } });

/* ---------- listen points ---------- */
const LISTEN_NOTES = ['Hearing the full output.', 'Raw layers: the emitters and mixer only, before resonator, derez, filter and VCA. Auto-levelled.',
  'Pre-filter: after the resonator and derez, before the filter. Auto-levelled.', 'Voices: after filter, VCA and heat, before feedback, vocoder and space. Auto-levelled.',
  'Vocoder wet only: what the material vocoder adds. Auto-levelled.', 'Feedback return only. Auto-levelled.', 'Space wet only. Auto-levelled.'];
$('listen').addEventListener('change', e => { P.listen = +e.target.value; pushP(); $('listenNote').textContent = LISTEN_NOTES[P.listen]; });

/* ---------- A/B ---------- */
const ab = { A: null, B: null, cur: null, loud: { A: 0, B: 0 } };
function abUi() { $('hearA').disabled = !ab.A; $('hearB').disabled = !ab.B; $('hearA').setAttribute('aria-pressed', ab.cur === 'A' ? 'true' : 'false'); $('hearB').setAttribute('aria-pressed', ab.cur === 'B' ? 'true' : 'false'); }
function store(k) { ab[k] = { P: clone(P), S: clone(S) }; ab.loud[k] = 0; ab.cur = k; abUi(); $('abNote').textContent = 'Stored ' + k + '. ' + (ab.A && ab.B ? 'Flip between Hear A and Hear B while playing.' : 'Change something, then store the other slot.'); }
function hear(k) { if (!ab[k]) return; ab.cur = k; const keep = { listen: P.listen, tap: P.tap }; P = Object.assign(clone(ab[k].P), keep); S = clone(ab[k].S); P.abTrim = 1; refreshAll(); pushAll(); abUi(); }
$('storeA').addEventListener('click', () => store('A')); $('storeB').addEventListener('click', () => store('B'));
$('hearA').addEventListener('click', () => hear('A')); $('hearB').addEventListener('click', () => hear('B'));
let matchOn = true; $('match').addEventListener('click', () => { matchOn = !matchOn; $('match').setAttribute('aria-pressed', String(matchOn)); if (!matchOn) { P.abTrim = 1; pushP(); } });
function onMeter(d) { if (!ab.cur || !ab.A || !ab.B || !d.active) return;
  const L = d.rms * P.vol; ab.loud[ab.cur] = ab.loud[ab.cur] ? ab.loud[ab.cur] * 0.85 + L * 0.15 : L;
  if (matchOn && ab.loud.A > 0 && ab.loud.B > 0) { const t = Math.min(1, Math.min(ab.loud.A, ab.loud.B) / ab.loud[ab.cur]); if (Math.abs(t - P.abTrim) > 0.02) { P.abTrim = t; pushP(); }
    $('abNote').textContent = 'Hearing ' + ab.cur + '. Level match trims it by ' + (20 * Math.log10(t)).toFixed(1) + ' dB.'; } }

/* ---------- recording (zip with WAV + patch) ---------- */
let recChunks = [], recOn = false, recStart = 0, recTimer = 0, downloads = null;
const noTakes = () => { $('rec').disabled = true; $('takeNote').textContent = 'Saving takes is not available in this view. Copy patch still works.'; };
$('rec').disabled = true;
(function findDownloads(tries) {
  if (window.claude && window.claude.use) { window.claude.use('downloads').then(d => { downloads = d; if (d) $('rec').disabled = false; else noTakes(); }).catch(noTakes); return; }
  if (tries > 0) setTimeout(() => findDownloads(tries - 1), 300); else noTakes(); })(10);
const CRC = (() => { const t = new Uint32Array(256); for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; } return t; })();
const crc32 = u8 => { let c = 0xFFFFFFFF; for (let i = 0; i < u8.length; i++) c = CRC[(c ^ u8[i]) & 255] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; };
function zip(files) { const parts = [], cen = []; let off = 0; const enc = new TextEncoder();
  for (const f of files) { const name = enc.encode(f.name), d = f.data, crc = crc32(d), h = new DataView(new ArrayBuffer(30));
    h.setUint32(0, 0x04034b50, true); h.setUint16(4, 20, true); h.setUint32(14, crc, true); h.setUint32(18, d.length, true); h.setUint32(22, d.length, true); h.setUint16(26, name.length, true);
    parts.push(new Uint8Array(h.buffer), name, d);
    const c = new DataView(new ArrayBuffer(46)); c.setUint32(0, 0x02014b50, true); c.setUint16(4, 20, true); c.setUint16(6, 20, true); c.setUint32(16, crc, true); c.setUint32(20, d.length, true); c.setUint32(24, d.length, true); c.setUint16(28, name.length, true); c.setUint32(42, off, true);
    cen.push(new Uint8Array(c.buffer), name); off += 30 + name.length + d.length; }
  const csz = cen.reduce((a, b) => a + b.length, 0), e = new DataView(new ArrayBuffer(22)); e.setUint32(0, 0x06054b50, true); e.setUint16(8, files.length, true); e.setUint16(10, files.length, true); e.setUint32(12, csz, true); e.setUint32(16, off, true);
  return new Blob([...parts, ...cen, new Uint8Array(e.buffer)], { type: 'application/zip' }); }
function wav(chunks, sr) { let n = 0; chunks.forEach(c => n += c.length / 2); const b = new ArrayBuffer(44 + n * 6), v = new DataView(b), w = (o, s) => { for (let i = 0; i < s.length; i++) v.setUint8(o + i, s.charCodeAt(i)); };
  w(0, 'RIFF'); v.setUint32(4, 36 + n * 6, true); w(8, 'WAVEfmt '); v.setUint32(16, 16, true); v.setUint16(20, 1, true); v.setUint16(22, 2, true); v.setUint32(24, sr, true); v.setUint32(28, sr * 6, true); v.setUint16(32, 6, true); v.setUint16(34, 24, true); w(36, 'data'); v.setUint32(40, n * 6, true);
  let o = 44; chunks.forEach(c => { for (let i = 0; i < c.length; i++) { const s = Math.max(-1, Math.min(1, c[i])) * 8388607 | 0; v.setUint8(o, s & 255); v.setUint8(o + 1, (s >> 8) & 255); v.setUint8(o + 2, (s >> 16) & 255); o += 3; } });
  return new Uint8Array(b); }
$('rec').addEventListener('click', async () => {
  if (!running) { $('takeNote').textContent = 'Start audio first.'; return; }
  if (!recOn) { recChunks = []; recOn = true; recStart = performance.now(); node.port.postMessage({ type: 'rec', on: true }); $('rec').setAttribute('aria-pressed', 'true');
    recTimer = setInterval(() => { const s = (performance.now() - recStart) / 1000; $('rec').textContent = 'Stop · ' + s.toFixed(0) + ' s'; if (s > 300) $('rec').click(); }, 250); return; }
  recOn = false; clearInterval(recTimer); $('rec').textContent = 'Record take'; $('rec').setAttribute('aria-pressed', 'false'); node.port.postMessage({ type: 'rec', on: false });
});
async function finishTake() {
  if (!recChunks.length) return; const stamp = new Date().toISOString().slice(0, 16).replace(/[-:T]/g, '');
  const nm = (currentName || 'patch').replace(/^user:/, '').replace(/[^a-z0-9]+/gi, '-').replace(/^-|-$/g, '').toLowerCase();
  const blob = zip([{ name: 'take.wav', data: wav(recChunks, ctx.sampleRate) }, { name: 'patch.json', data: new TextEncoder().encode(patchText()) }]);
  try { await downloads.save({ filename: 'silt-take-' + nm + '-' + stamp + '.zip', data: blob }); $('takeNote').textContent = 'Take saved. Attach the .zip in chat and tell me what you hear.'; }
  catch (e) { $('takeNote').textContent = e && e.code === 'declined' ? 'Take not saved.' : 'Could not save the take here.'; }
  recChunks = []; }

/* ---------- audio ---------- */
async function start() {
  if (running) { if (ctx.state === 'running') { await ctx.suspend(); $('start').textContent = 'Resume audio'; } else { await ctx.resume(); $('start').textContent = 'Pause audio'; } return; }
  try { ctx = new (window.AudioContext || window.webkitAudioContext)({ latencyHint: 'interactive' });
    const src = $('dsp').textContent; let ok = false;
    try { const url = URL.createObjectURL(new Blob([src], { type: 'application/javascript' })); await ctx.audioWorklet.addModule(url); ok = true; } catch (e) {}
    if (!ok) await ctx.audioWorklet.addModule('data:application/javascript;charset=utf-8,' + encodeURIComponent(src));
    node = new AudioWorkletNode(ctx, 'silt', { numberOfInputs: 0, numberOfOutputs: 2, outputChannelCount: [2, 1] });
    node.connect(ctx.destination, 0);
    analyser = ctx.createAnalyser(); analyser.fftSize = 4096; analyser.smoothingTimeConstant = 0.6;
    const sink = ctx.createGain(); sink.gain.value = 0; node.connect(analyser, 1); analyser.connect(sink); sink.connect(ctx.destination);
    node.port.onmessage = e => { const d = e.data; if (d.type === 'viz') viz = d; else if (d.type === 'meter') onMeter(d); else if (d.type === 'recData') recChunks.push(d.d); else if (d.type === 'recEnd') finishTake(); };
    pushAll(); await ctx.resume(); running = true; $('start').textContent = 'Pause audio'; $('start').classList.remove('primary'); setupMIDI();
  } catch (err) { $('readout').textContent = 'Audio could not start: ' + (err && err.message ? err.message : err) + '. Try Chrome or Edge.'; } }
$('start').addEventListener('click', start);
$('panic').addEventListener('click', () => { if (node) node.port.postMessage({ type: 'panic' }); held.clear(); latched.clear(); paintKeys(); });

/* ---------- notes and keyboard ---------- */
const held = new Set(), latched = new Set(); let octave = 3, latchOn = false;
function on(n, v) { if (!node) return; node.port.postMessage({ type: 'on', n, v }); held.add(n); paintKeys(); }
function off(n) { if (node) node.port.postMessage({ type: 'off', n }); held.delete(n); paintKeys(); }
function press(n, v) { if (!running) return; if (latchOn) { if (latched.has(n)) { latched.delete(n); off(n); } else { latched.add(n); on(n, v); } } else on(n, v); }
function release(n) { if (!latchOn) off(n); }
$('latch').addEventListener('click', () => { latchOn = !latchOn; $('latch').setAttribute('aria-pressed', String(latchOn)); if (!latchOn) { latched.forEach(n => off(n)); latched.clear(); } });
const KEYMAP = { a: 0, w: 1, s: 2, e: 3, d: 4, f: 5, t: 6, g: 7, y: 8, h: 9, u: 10, j: 11, k: 12, o: 13, l: 14, p: 15, ';': 16 };
const keyDown = new Map();
window.addEventListener('keydown', e => { if (e.repeat || e.ctrlKey || e.metaKey || e.altKey) return; const t = e.target;
  if (t && (t.tagName === 'SELECT' || t.tagName === 'TEXTAREA' || (t.tagName === 'INPUT' && t.type !== 'range'))) return;
  const k = e.key.toLowerCase(); if (k === 'z') { setOct(octave - 1); return; } if (k === 'x') { setOct(octave + 1); return; }
  if (k in KEYMAP) { const n = 12 * (octave + 1) + KEYMAP[k]; keyDown.set(k, n); press(n, 0.85); e.preventDefault(); } });
window.addEventListener('keyup', e => { const k = e.key.toLowerCase(); if (keyDown.has(k)) { release(keyDown.get(k)); keyDown.delete(k); } });
function setOct(o) { octave = Math.max(0, Math.min(7, o)); $('octLabel').textContent = 'C' + octave; buildKeys(); }
$('octDown').addEventListener('click', () => setOct(octave - 1)); $('octUp').addEventListener('click', () => setOct(octave + 1));
const kb = $('kb'), keyEls = new Map();
function buildKeys() { kb.textContent = ''; keyEls.clear(); const base = 12 * (octave + 1), whites = [0, 2, 4, 5, 7, 9, 11], W = 100 / 15; let wi = 0;
  for (let s = 0; s <= 24; s++) { const n = base + s, pc = s % 12, isW = whites.includes(pc), k = el('div', isW ? 'w' : 'b'); k.dataset.n = String(n);
    if (isW) { k.style.left = (wi * W) + '%'; k.style.width = W + '%'; if (pc === 0) k.append(el('span', null, 'C' + (octave + Math.floor(s / 12)))); wi++; }
    else { k.style.left = (wi * W - W * 0.32) + '%'; k.style.width = (W * 0.64) + '%'; }
    kb.append(k); keyEls.set(n, k); } paintKeys(); }
function paintKeys() { keyEls.forEach((k, n) => k.classList.toggle('on', held.has(n))); }
const ptrNote = new Map();
kb.addEventListener('pointerdown', e => { const t = e.target.closest('[data-n]'); if (!t) return; kb.setPointerCapture(e.pointerId); const n = +t.dataset.n; ptrNote.set(e.pointerId, n);
  const r = t.getBoundingClientRect(); press(n, Math.max(0.3, Math.min(1, (e.clientY - r.top) / r.height * 1.1))); });
const endPtr = e => { if (ptrNote.has(e.pointerId)) { release(ptrNote.get(e.pointerId)); ptrNote.delete(e.pointerId); } };
kb.addEventListener('pointerup', endPtr); kb.addEventListener('pointercancel', endPtr);
setOct(3);
function setupMIDI() { if (!navigator.requestMIDIAccess) { $('midi').textContent = 'MIDI: not available in this browser'; return; }
  navigator.requestMIDIAccess().then(acc => { const bind = () => { let c = 0; acc.inputs.forEach(inp => { c++; inp.onmidimessage = m => { const [s, d1, d2] = m.data, t = s & 0xf0;
      if (t === 0x90 && d2 > 0) press(d1, d2 / 127); else if (t === 0x80 || (t === 0x90 && d2 === 0)) release(d1);
      else if (t === 0xB0 && d1 === 1) { P.allure = d2 / 127; pushP(); refreshAll(); } }; });
    $('midi').textContent = c ? 'MIDI: ' + c + ' input' + (c > 1 ? 's' : '') : 'MIDI: no devices'; }; bind(); acc.onstatechange = bind; })
  .catch(() => { $('midi').textContent = 'MIDI: permission blocked'; }); }

/* ---------- drawing ---------- */
const trace = [], STAGES = ['Onset', 'Continuant', 'Termination', 'Looping', 'Tail']; let col = {}, colAge = 99;
function readCols() { const cs = getComputedStyle(document.documentElement); ['--screen', '--screen-ink', '--grid', '--trace', '--trace2', '--sed'].forEach(k => col[k] = cs.getPropertyValue(k).trim()); }
function fit(c) { const d = window.devicePixelRatio || 1, w = c.clientWidth, h = +c.getAttribute('height');
  if (c.width !== Math.round(w * d)) { c.width = Math.round(w * d); c.style.height = h + 'px'; } c.height = Math.round(h * d); const g = c.getContext('2d'); g.setTransform(d, 0, 0, d, 0, 0); return [g, w, h]; }
const tbuf = new Float32Array(4096), fbuf = new Float32Array(2048);
function grid(g, w, h) { g.strokeStyle = col['--grid']; g.lineWidth = 1; g.beginPath(); for (let i = 1; i < 8; i++) { const x = Math.round(w * i / 8) + .5; g.moveTo(x, 0); g.lineTo(x, h); }
  for (let i = 1; i < 4; i++) { const y = Math.round(h * i / 4) + .5; g.moveTo(0, y); g.lineTo(w, y); } g.stroke(); }
function idle(g, w, h, txt) { g.fillStyle = col['--screen-ink']; g.font = '12px "JetBrains Mono", monospace'; g.textAlign = 'center'; g.fillText(txt, w / 2, h / 2 + 4); }
function draw() { if (++colAge > 30) { readCols(); colAge = 0; }
  { const [g, w, h] = fit($('scope')); g.fillStyle = col['--screen']; g.fillRect(0, 0, w, h); grid(g, w, h);
    if (analyser) { analyser.getFloatTimeDomainData(tbuf); let s = 1; for (; s < 2048; s++) if (tbuf[s - 1] < 0 && tbuf[s] >= 0) break; if (s >= 2048) s = 0;
      let pk = 0; for (let i = 0; i < 2048; i++) pk = Math.max(pk, Math.abs(tbuf[s + i])); const sc = 0.85 / Math.max(0.01, pk);
      g.strokeStyle = col['--trace']; g.lineWidth = 1.6; g.beginPath(); for (let i = 0; i < 2048; i++) { const x = i / 2047 * w, y = h / 2 - tbuf[s + i] * sc * h / 2; i ? g.lineTo(x, y) : g.moveTo(x, y); } g.stroke();
    } else idle(g, w, h, 'Start audio to see the waveform'); }
  { const [g, w, h] = fit($('spec')); g.fillStyle = col['--screen']; g.fillRect(0, 0, w, h); grid(g, w, h);
    if (analyser) { analyser.getFloatFrequencyData(fbuf); const nyq = ctx.sampleRate / 2, lo = Math.log(30), hi = Math.log(16000);
      g.beginPath(); for (let x = 0; x <= w; x += 2) { const f = Math.exp(lo + (hi - lo) * x / w), bin = Math.min(fbuf.length - 1, Math.round(f / nyq * fbuf.length)), db = Math.max(-110, Math.min(-10, fbuf[bin])), y = h - (db + 110) / 100 * h; x ? g.lineTo(x, y) : g.moveTo(x, y); }
      g.lineTo(w, h); g.lineTo(0, h); g.closePath(); g.fillStyle = col['--trace']; g.globalAlpha = 0.18; g.fill(); g.globalAlpha = 1; g.strokeStyle = col['--trace']; g.lineWidth = 1.2; g.stroke(); }
    else idle(g, w, h, '—'); }
  { const [g, w, h] = fit($('cloud')); g.fillStyle = col['--screen']; g.fillRect(0, 0, w, h); const cx = w / 2, cy = h / 2 + 6, r = Math.min(w, h) * 0.36;
    g.strokeStyle = col['--grid']; g.lineWidth = 1; g.beginPath(); g.arc(cx, cy, r, 0, Math.PI * 2); g.stroke();
    if (viz && viz.on) { let sx = 0, sy = 0;
      for (let i = 0; i < viz.N; i++) { const a = viz.th[i] * Math.PI * 2 - Math.PI / 2, e = Math.min(1, viz.e[i]); sx += Math.cos(a); sy += Math.sin(a);
        g.fillStyle = e > 0.02 ? col['--trace'] : col['--screen-ink']; g.globalAlpha = 0.35 + 0.65 * e; g.beginPath(); g.arc(cx + Math.cos(a) * r, cy + Math.sin(a) * r, 3 + e * 5, 0, Math.PI * 2); g.fill(); }
      g.globalAlpha = 1; sx /= viz.N; sy /= viz.N; g.strokeStyle = col['--trace2']; g.lineWidth = 2; g.beginPath(); g.moveTo(cx, cy); g.lineTo(cx + sx * r, cy + sy * r); g.stroke();
      g.fillStyle = col['--screen-ink']; g.font = '11px "JetBrains Mono", monospace'; g.textAlign = 'right'; g.fillText('coherence ' + Math.hypot(sx, sy).toFixed(2), w - 8, 16); }
    else idle(g, w, h, 'Play a note'); }
  { const [g, w, h] = fit($('trace')); g.fillStyle = col['--screen']; g.fillRect(0, 0, w, h); grid(g, w, h);
    const lines = [[0, col['--trace'], v => v / 1.3], [4, col['--trace2'], v => v], [8, col['--screen-ink'], v => (v - Math.log2(20)) / (Math.log2(20000) - Math.log2(20))]];
    lines.forEach(([k, c, nrm]) => { g.strokeStyle = c; g.lineWidth = 1.4; g.beginPath(); let st = false;
      trace.forEach((p, i) => { if (!p) { st = false; return; } const x = w - (trace.length - 1 - i) * 2, y = h - 6 - Math.max(0, Math.min(1, nrm(p[k]))) * (h - 12); if (!st) { g.moveTo(x, y); st = true; } else g.lineTo(x, y); }); g.stroke(); });
    g.font = '10.5px "JetBrains Mono", monospace'; g.textAlign = 'right'; g.fillStyle = col['--trace']; g.fillText('level', w - 8, 14); g.fillStyle = col['--trace2']; g.fillText('coupling', w - 52, 14); g.fillStyle = col['--screen-ink']; g.fillText('cutoff', w - 122, 14); }
  requestAnimationFrame(draw); }
let lastRead = '';
setInterval(() => { if (!running) return; const w = $('trace').clientWidth; trace.push(viz && viz.on ? viz.cur : null); while (trace.length > w / 2) trace.shift();
  let txt; const lp = ['full output', 'raw layers', 'pre-filter', 'voices', 'vocoder wet', 'feedback', 'space wet'][P.listen];
  if (viz && viz.on) { const f = viz.f0; txt = `<span>Stage <b>${STAGES[viz.stage]}</b></span><span>Emission <b>${f < 10 ? f.toFixed(2) : f.toFixed(1)} Hz</b> · ${f < 20 ? 'rhythm' : 'pitch'}</span><span>Formant <b>${Math.round(viz.ff)} Hz</b></span><span>Coherence <b>${(viz.R || 0).toFixed(2)}</b></span><span>Voices <b>${viz.voices}</b></span><span>Listening <b>${lp}</b></span>`; }
  else txt = `<span>Ready. Play the keys or a MIDI controller.</span><span>Listening <b>${lp}</b></span>`;
  if (txt !== lastRead) { $('readout').innerHTML = txt; lastRead = txt; } }, 50);

currentName = Object.keys(PATCHES)[1]; fillPresetMenu(); loadPreset(currentName); $('preset').value = currentName;
readCols(); requestAnimationFrame(draw);
})();
