// Renders every factory patch through the full app processor (parameters, patch loading, MIDI path)
// for comparison with the browser lab. Usage: silt_app_check <outdir>
#include "../Source/App/PluginProcessor.h"
int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String dir = argc > 1 ? argv[1] : "cmp";
    for (int pi = 0; pi < 10; ++pi) {
        SiltProcessor p;
        p.setCurrentProgram(pi);
        p.prepareToPlay(48000, 128);
        const bool tr = p.getProgramName(pi).contains("Transient");
        struct Ev { double t; bool on; int n; float v; };
        std::vector<Ev> evs;
        if (tr) { const double ts[4] = { 0.05, 0.45, 0.85, 1.25 }; const int ns[4] = { 36, 36, 43, 31 };
            for (int k = 0; k < 4; ++k) { evs.push_back({ ts[k], true, ns[k], 0.95f }); evs.push_back({ ts[k] + 0.2, false, ns[k], 0 }); } }
        else { for (int n : { 48, 55, 60, 64 }) evs.push_back({ 0.05, true, n, 0.9f }); for (int n : { 48, 55, 60, 64 }) evs.push_back({ 3.0, false, n, 0 }); }
        std::stable_sort(evs.begin(), evs.end(), [](auto& a, auto& b) { return a.t < b.t; });
        const int N = (int) std::ceil(5 * 48000 / 128.0);
        juce::AudioBuffer<float> buf(2, 128); std::vector<float> out((size_t) N * 256);
        size_t ev = 0;
        for (int b = 0; b < N; ++b) {
            juce::MidiBuffer mb; const double now = b * 128 / 48000.0;
            while (ev < evs.size() && evs[ev].t <= now) { auto& e = evs[ev];
                mb.addEvent(e.on ? juce::MidiMessage::noteOn(1, e.n, e.v) : juce::MidiMessage::noteOff(1, e.n), 0); ++ev; }
            p.processBlock(buf, mb);
            for (int i = 0; i < 128; ++i) { out[(size_t) (b * 128 + i) * 2] = buf.getSample(0, i); out[(size_t) (b * 128 + i) * 2 + 1] = buf.getSample(1, i); }
        }
        juce::File(dir).getChildFile("app" + juce::String(pi) + ".f32").replaceWithData(out.data(), out.size() * 4);
        std::printf("%d %s\n", pi, p.getProgramName(pi).toRawUTF8());
    }
    // patch JSON round trip
    SiltProcessor p; p.setCurrentProgram(7); const auto js = p.patchToJson(); SiltProcessor q; q.loadPatchJson(js, {}, {});
    std::printf("roundtrip %s\n", q.patchToJson() == js ? "identical" : "DIFFERENT");
    return 0;
}
