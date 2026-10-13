// Value formatting shared by parameters and the editor (mirrors the browser lab's formatters).
#pragma once
#include <juce_core/juce_core.h>

inline const char* const kFrameNames[6][8] = {
    { "Sine", "Triangle", "Soft saw", "Saw", "Square", "Pulse 30", "Pulse 12", "Resonant saw" },
    { "Hollow", "Soft square", "Organ", "PPG cluster", "Digital flute", "3-bit tanh", "Wood", "Hollow reed" },
    { "Trombone", "Horn", "Alto sax", "Tenor sax", "Clarinet", "Oboe", "Bassoon", "Muted trumpet" },
    { "A", "E", "I", "O", "U", "A (high)", "E (high)", "Ng" },
    { "Odd spikes", "Peak 20", "Tine", "Squares", "Shimmer", "Crystal", "High cluster", "Fibonacci" },
    { "4-bit sine", "i & i>>3", "i ^ i>>4", "5i & i>>2", "i | i>>5", "Popcount", "3i ^ i>>1", "Pulse + LFSR" },
};

// juce::String(x, 0) prints full precision, so round explicitly when no decimals are wanted.
inline juce::String fx(double x, int d) { return d == 0 ? juce::String((juce::int64) std::llround(x)) : juce::String(x, d); }

inline juce::String fmtValue(const juce::String& f, float xf) {
    const double x = xf;
    auto sgn = [](double v) { return v >= 0 ? juce::String("+") : juce::String(); };
    if (f == "pct") return juce::String((int) std::lround(x * 100)) + "%";
    if (f == "hz") return x >= 1000 ? fx(x / 1000, 2) + " kHz" : fx(x, x < 10 ? 2 : 0) + " Hz";
    if (f == "time") return x < 1 ? fx(x * 1000, x < 0.01 ? 1 : 0) + " ms" : fx(x, 2) + " s";
    if (f == "ms") return fx(x, x < 10 ? 1 : 0) + " ms";
    if (f == "oct") return sgn(x) + fx(x, 2) + " oct";
    if (f == "x") return juce::String::fromUTF8("\xc3\x97") + fx(x, 2);
    if (f == "n") return juce::String((int) std::lround(x));
    if (f == "bi") return sgn(x) + fx(x, 2);
    if (f == "cents") return sgn(x) + fx(x, 1) + " ct";
    if (f == "semi") return sgn(x) + juce::String((int) std::lround(x)) + " st";
    if (f == "per") return fx(x, x < 10 ? 1 : 0) + " /s";
    if (f == "ratio") return juce::String::fromUTF8("\xc3\x97") + fx(x, x < 10 ? 2 : 1);
    if (f == "tail") return x < 0.001 ? juce::String("Auto") : (x < 1 ? fx(x * 1000, 0) + " ms" : fx(x, 2) + " s");
    if (f == "steps") return fx(x, x < 10 && std::abs(x - std::round(x)) > 1e-3 ? 2 : 0) + (std::abs(x - 1) < 1e-3 ? " step" : " steps");
    if (f == "bpm") return fx(x, 1) + " bpm";
    if (f == "clk") return x < 0.08 ? "Square" : x < 0.2 ? "Square > pulse" : x < 0.3 ? "Skewed pulse" : x < 0.45 ? "Pulse > saw" : x < 0.55 ? "Saw" : x < 0.7 ? "Saw > triangle" : x < 0.8 ? "Triangle" : x < 0.95 ? "Triangle > sine" : "Sine";
    if (f == "win") return x < 0.02 ? "Rect" : x < 0.48 ? "Rect > Hann" : x < 0.52 ? "Hann" : x < 0.98 ? "Hann > Exp" : "Expodec";
    return fx(x, 2);
}

inline juce::String fmtPos(int bank, float x) {
    bank = juce::jlimit(0, 5, bank);
    const double p = x * 7; const int i = juce::jmin(6, (int) std::floor(p)); const double fr = p - i;
    if (fr < 0.05) return juce::String::fromUTF8(kFrameNames[bank][i]);
    if (fr > 0.95) return juce::String::fromUTF8(kFrameNames[bank][i + 1]);
    return juce::String::fromUTF8(kFrameNames[bank][i]) + " > " + juce::String::fromUTF8(kFrameNames[bank][i + 1]);
}

// Reads a typed value back ("3.2 kHz", "120 ms", "45%", "x1.5").
inline float parseValue(const juce::String& f, const juce::String& text) {
    const auto t = text.trim().toLowerCase();
    const double v = t.retainCharacters("0123456789.-+eE").getDoubleValue();
    if (f == "pct") return (float) (v / 100);
    if (f == "hz") return (float) (t.contains("k") ? v * 1000 : v);
    if (f == "tail" && t.startsWithIgnoreCase("auto")) return 0.f;
    if (f == "time" || f == "tail") return (float) (t.endsWith("ms") ? v / 1000 : (t.endsWith("s") ? v : (v > 10 ? v / 1000 : v)));
    return (float) v;
}
