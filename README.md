# Silt

A material and particle synthesizer: clusters of pulsar emitters that couple, scatter and lock into voices, shaped by articulation states, then pushed through a derez stage, an analog-style filter, a material vocoder, a feedback loop and a space.

This repository holds the **Windows emulator** (standalone app and VST3) and the **browser lab** it was ported from. Both run the same engine, so patches move between them unchanged. The hardware will be built from this engine once the sound is right.

## Getting the app

1. Open the **Releases** page of this repository and download `Silt-windows.zip` from **Silt (latest build)**. (Every push to `main` rebuilds it; the Actions tab also keeps each build for 14 days.)
2. Unzip it anywhere and run `Silt.exe`.
3. Windows SmartScreen may say the app is unrecognised, because it is not code-signed. Choose **More info**, then **Run anyway**.
4. Click **Options > Audio/MIDI Settings** (top left) once to pick your output device. "Windows Audio (Low Latency Mode)" with a 128 or 256 sample buffer works well. Any MIDI controllers you enable there play Silt too.

To use it in a DAW instead, copy `Silt.vst3` into `C:\Program Files\Common Files\VST3`.

## Playing

| Keys | Notes |
| --- | --- |
| `A W S E D F T G Y H U J K O L P ;` | Two octaves of notes, laid out like a piano (white keys on the middle row, sharps above) |
| `Z` / `X` | Octave down / up |
| Mouse | Click the on-screen keys |
| MIDI | Notes and velocity; the mod wheel (CC1) drives Allure |

The computer keys work wherever you last clicked, except while you are typing a value or a patch name.

## The listening bench

- **Patches**: the menu holds the numbered test patches from the lab, each with notes on what it targets, what to listen for and what to try. Your own patches live in `Documents\Silt\Patches`.
- **Save as / Copy patch / Paste patch**: patches are small JSON files. Copy one and paste it to Claude in chat, or paste one from the browser lab.
- **Record take**: writes a 24-bit WAV and the exact patch as JSON to `Documents\Silt\Takes`. Attach both in chat to talk about a sound.
- **Listen**: solo a point in the signal path (raw layers, pre-filter, voices, vocoder wet, feedback, space). Everything except the full output is auto-levelled.
- **A/B with level match**: store two versions and flip between them; the louder one is trimmed so you judge the sound, not the volume.
- **Screens**: scope (output, selected voice or the feedback loop), spectrum, the emitter cloud of the last voice with its coherence, and a motion trace of level, coupling and cutoff.

## Repository layout

| Path | What it is |
| --- | --- |
| `Source/Engine` | The sound engine in plain C++ with no framework, so it can move to the hardware unchanged |
| `Source/App` | The JUCE app: parameters, patches, recording, editor |
| `lab/` | The browser lab (`engine.js` is the reference the C++ engine was ported from). `python3 build.py` in that folder writes `silt-lab.html` |
| `tools/` | `gen_patches.js` regenerates the app's patch list from `lab/patches.js`; `dump_cases.js`, `engine_check.cpp` and `app_check.cpp` render the same notes through both engines for comparison |
| `.github/workflows` | The Windows build |

## Building it yourself

Needs CMake 3.22+ and Visual Studio 2022 or newer (or GCC/Clang on Linux). JUCE 8.0.4 is downloaded automatically.

```
cmake -S . -B build
cmake --build build --config Release --target Silt_Standalone Silt_VST3
```

## Checking the port against the lab

```
node tools/dump_cases.js cmp
cmake --build build --target silt_engine_check silt_app_check
build/silt_engine_check cmp      # writes cmp/cpp*.f32
build/silt_app_check_artefacts/Release/silt_app_check cmp   # writes cmp/app*.f32
```

The cases are the nine lab patches played as a held chord (or the transient pattern). The RNG differs between engines, so compare levels and spectra rather than samples. At the time of the port every patch matched the lab within 0.3 dB RMS, and octave-band spectra matched within about 1 dB except where the patch is random by design (droplets, strike noise, drift beating).
