// Renders every factory patch through the full app processor (parameters, patch loading, MIDI path)
// for comparison with the browser lab. Usage: silt_app_check <outdir>
#include "../Source/App/PluginProcessor.h"
#include "../Source/App/PluginEditor.h"
int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String dir = argc > 1 ? argv[1] : "cmp";
    for (int pi = 0; pi < SiltProcessor().getNumPrograms(); ++pi) {
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
    // render the articulation view and the sequencer graph from a real run, to check the live drawing
    {
        SiltProcessor p; p.setCurrentProgram(10); p.prepareToPlay(48000, 128);
        if (auto* q = p.apvts.getParameter("rel")) q->setValueNotifyingHost(q->convertTo0to1(1.5f));
        if (auto* q = p.apvts.getParameter("st2To1")) q->setValueNotifyingHost(q->convertTo0to1(9));
        if (auto* q = p.apvts.getParameter("st5To2")) q->setValueNotifyingHost(q->convertTo0to1(1));
        ui::Look look; juce::LookAndFeel::setDefaultLookAndFeel(&look);
        ui::ArticulationView av(p); av.setSize(420, 190);
        ui::SeqGraph sg(p); sg.setSize(420, 330);
        juce::AudioBuffer<float> buf(2, 128);
        const double shots[3] = { 0.6, 1.4, 2.6 }; int shot = 0;
        for (int b = 0; b < int(3.0 * 48000 / 128); ++b) {
            juce::MidiBuffer mb; const double now = b * 128 / 48000.0;
            if (b == 2) mb.addEvent(juce::MidiMessage::noteOn(1, 48, 0.9f), 0);
            if (now >= 1.0 && now < 1.0 + 128 / 48000.0) mb.addEvent(juce::MidiMessage::noteOff(1, 48), 0);
            p.processBlock(buf, mb);
            if (b % 8 == 0) av.update(p.getViz());   // the editor's timer does this at 20 Hz
            if (shot < 3 && now >= shots[shot]) {
                const auto v = p.getViz(); sg.heads[0] = 4 + shot; sg.heads[1] = 11;
                juce::Image img(juce::Image::ARGB, 420, 530, true); juce::Graphics g(img);
                av.paintEntireComponent(g, false); g.setOrigin(0, 200); sg.paintEntireComponent(g, false);
                juce::File f = juce::File(dir).getChildFile("view" + juce::String(shot) + ".png"); f.deleteFile();
                juce::FileOutputStream os(f); juce::PNGImageFormat().writeImageToStream(img, os);
                std::printf("view %d: stage %d prog %.2f level %.2f step %d\n", shot, v.stage, v.prog, v.cur[0], v.seqStep[0]);
                ++shot;
            }
        }
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }
    // patch JSON round trip
    SiltProcessor p; p.setCurrentProgram(7); const auto js = p.patchToJson(); SiltProcessor q; q.loadPatchJson(js, {}, {});
    const auto a = p.patchVar(), b = q.patchVar(); int diffs = 0;
    if (auto* o = a["P"].getDynamicObject()) for (auto& prop : o->getProperties()) { const auto x = prop.value, y = b["P"][prop.name];
        if (x.toString() != y.toString() && !(x.isDouble() && y.isDouble() && std::abs((double) x - (double) y) <= 1e-5 * (1 + std::abs((double) x)))) { if (++diffs < 8) std::printf("  differs %s: %s vs %s\n", prop.name.toString().toRawUTF8(), x.toString().toRawUTF8(), y.toString().toRawUTF8()); } }
    std::printf("roundtrip %s (%d keys differ)\n", diffs == 0 ? "identical" : "DIFFERENT", diffs);
    return 0;
}
