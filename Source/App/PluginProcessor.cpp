#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Patches.inc"
#include "Format.h"

using namespace silt;

static constexpr int kNumFactory = int(sizeof(kFactoryPatches) / sizeof(kFactoryPatches[0]));

SiltProcessor::SiltProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SILT", createLayout()) {
    bankRaw[0] = apvts.getRawParameterValue("aBank");
    bankRaw[1] = apvts.getRawParameterValue("bBank");
    for (int i = 0; i < kNumGlobal; ++i)
        if (float* f = paramField(P, kParams[i].id)) bindings.push_back({ apvts.getRawParameterValue(kParams[i].id), f });
    for (int s = 0; s < 3; ++s) for (int k = 0; k < NK; ++k) stateRaw[s][k] = apvts.getRawParameterValue(stateId(s, k));
    allureParam = apvts.getParameter("allure");
    writerThread.startThread();
    loadFactory(1);
}

SiltProcessor::~SiltProcessor() { stopRecording(); writerThread.stopThread(2000); }

juce::AudioProcessorValueTreeState::ParameterLayout SiltProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto makeRange = [](float mn, float mx, bool lg, bool stepped) {
        if (stepped) return juce::NormalisableRange<float>(mn, mx, 1.f);
        if (lg) return juce::NormalisableRange<float>(mn, mx,
            [](float a, float b, float x) { return a * std::pow(b / a, x); },
            [](float a, float b, float v) { return std::log(v / a) / std::log(b / a); }, nullptr);
        return juce::NormalisableRange<float>(mn, mx);
    };
    for (int i = 0; i < kNumGlobal; ++i) {
        const PDef& d = kParams[i];
        const bool stepped = d.kind != Kind::Cont;
        auto attrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [this, d](float v, int) -> juce::String {
                if (d.kind == Kind::Toggle) return v >= 0.5f ? "On" : "Off";
                if (d.kind == Kind::Choice) { auto c = juce::StringArray::fromTokens(d.choices, "|", ""); return c[juce::jlimit(0, c.size() - 1, (int) std::lround(v))]; }
                juce::String f = d.fmt;
                if (f == "posA" || f == "posB") { auto* b = bankRaw[f == "posA" ? 0 : 1]; return fmtPos(b ? (int) b->load() : 0, v); }
                return fmtValue(f, v); })
            .withValueFromStringFunction([d](const juce::String& t) -> float {
                if (d.kind == Kind::Choice) { auto c = juce::StringArray::fromTokens(d.choices, "|", ""); const int i = c.indexOf(t.trim(), true); return i >= 0 ? (float) i : t.getFloatValue(); }
                if (d.kind == Kind::Toggle) return t.trim().equalsIgnoreCase("on") || t.getFloatValue() >= 0.5f ? 1.f : 0.f;
                return parseValue(d.fmt, t); });
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { d.id, 1 }, juce::String(d.group) + ": " + d.label,
                                                               makeRange(d.min, d.max, d.log, stepped), d.def, attrs));
    }
    // Default states: the Init patch.
    const float initS[3][NK] = { { 1, 0, 0, 0, 0, 0, 0, 0, 6000, 0, 0, 0 }, { 0.8f, 0, 0, 0, 0, 0, 0, 0, 2500, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 1200, 0, 0, 0 } };
    for (int s = 0; s < 3; ++s) for (int k = 0; k < NK; ++k) {
        const SDef& d = kStateDefs[k];
        auto attrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction([d](float v, int) { return fmtValue(d.fmt, v); })
                                                   .withValueFromStringFunction([d](const juce::String& t) { return parseValue(d.fmt, t); });
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { stateId(s, k), 1 }, juce::String(kStageNames[s]) + ": " + d.label,
                                                               makeRange(d.min, d.max, d.log, false), initS[s][k], attrs));
    }
    return layout;
}

bool SiltProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SiltProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sampleRateHz = sampleRate;
    engine.prepare(sampleRate);
    tapBuf.assign((size_t) juce::jmax(samplesPerBlock, 4096), 0.f);
    keyboardState.reset();
}

void SiltProcessor::renderSegment(float* L, float* R, int n) {
    while (n > 0) {
        const int m = juce::jmin(n, (int) tapBuf.size());
        engine.process(P, S, L, R, tapBuf.data(), m);
        int w = scopeWrite.load(std::memory_order_relaxed);
        for (int i = 0; i < m; ++i) { scopeRing[w] = tapBuf[(size_t) i]; w = (w + 1) & (kRing - 1); }
        scopeWrite.store(w, std::memory_order_release);
        L += m; R += m; n -= m;
    }
}

void SiltProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboardState.processNextMidiBuffer(midi, 0, n, true);
    for (auto& b : bindings) *b.second = b.first->load(std::memory_order_relaxed);
    P.abTrim = abTrim.load();
    for (int s = 0; s < 3; ++s) for (int k = 0; k < NK; ++k) S[(size_t) s][(size_t) k] = stateRaw[s][k]->load(std::memory_order_relaxed);
    engine.setControls(P, S);
    if (panicRequest.exchange(false)) engine.panic();

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;
    int pos = 0;
    for (const auto meta : midi) {
        const auto msg = meta.getMessage();
        const int sp = juce::jlimit(0, n, meta.samplePosition);
        if (sp > pos) { renderSegment(L + pos, R + pos, sp - pos); pos = sp; }
        if (msg.isNoteOn()) engine.noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff()) engine.noteOff(msg.getNoteNumber());
        else if (msg.isController() && msg.getControllerNumber() == 1 && allureParam) allureParam->setValueNotifyingHost(msg.getControllerValue() / 127.f);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.panic();
    }
    if (pos < n) renderSegment(L + pos, R + pos, n - pos);

    meterRms.store(engine.meterRms); activeVoices.store(engine.activeVoices);
    { const juce::SpinLock::ScopedTryLockType l(vizLock); if (l.isLocked()) vizCopy = engine.viz; }
    if (recording.load()) {
        const juce::ScopedTryLock sl(writerLock);
        if (sl.isLocked() && activeWriter) { activeWriter->write(buffer.getArrayOfReadPointers(), n); recSamples += n; }
    }
}

// ---------- patches ----------
juce::File SiltProcessor::userPatchDir() { return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Silt").getChildFile("Patches"); }
juce::File SiltProcessor::takesDir() { return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Silt").getChildFile("Takes"); }

juce::var SiltProcessor::patchVar() const {
    auto* Pobj = new juce::DynamicObject();
    juce::Array<juce::var> bands;
    for (int i = 0; i < kNumGlobal; ++i) {
        const juce::String id = kParams[i].id;
        const float v = apvts.getRawParameterValue(id)->load();
        if (id.startsWith("band")) { bands.add(v); continue; }
        Pobj->setProperty(id, kParams[i].kind == Kind::Cont ? juce::var((double) v) : juce::var((int) std::lround(v)));
    }
    Pobj->setProperty("bands", bands);
    Pobj->setProperty("abTrim", 1);
    juce::Array<juce::var> states;
    for (int s = 0; s < 3; ++s) { auto* o = new juce::DynamicObject();
        for (int k = 0; k < NK; ++k) o->setProperty(kStateKeys[k], (double) apvts.getRawParameterValue(stateId(s, k))->load());
        states.add(juce::var(o)); }
    auto* root = new juce::DynamicObject();
    root->setProperty("silt", 2); root->setProperty("name", patchName); root->setProperty("P", juce::var(Pobj)); root->setProperty("S", states);
    return juce::var(root);
}

juce::String SiltProcessor::patchToJson() const { return juce::JSON::toString(patchVar(), true); }

bool SiltProcessor::loadPatchVar(const juce::var& v) {
    const juce::var Pv = v["P"], Sv = v["S"];
    if (!Pv.isObject() || !Sv.isArray() || Sv.size() < 3) return false;
    const juce::var base = juce::JSON::parse(juce::String::fromUTF8(kBaseJson));
    auto setParam = [this](const juce::String& id, float value) {
        if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(p->getNormalisableRange().snapToLegalValue(value))); };
    for (int i = 0; i < kNumGlobal; ++i) {
        const juce::String id = kParams[i].id;
        if (id == "listen" || id == "tap") continue; // bench settings stay where they are
        if (id.startsWith("band")) { const int b = id.getTrailingIntValue(); juce::var arr = Pv["bands"]; if (!arr.isArray()) arr = base["bands"];
            setParam(id, arr.isArray() && b < arr.size() ? (float) (double) arr[b] : 0.8f); continue; }
        juce::var x = Pv[juce::Identifier(id)]; if (x.isVoid()) x = base[juce::Identifier(id)];
        if (x.isBool()) x = (bool) x ? 1 : 0;
        if (!x.isVoid()) setParam(id, (float) (double) x);
    }
    for (int s = 0; s < 3; ++s) for (int k = 0; k < NK; ++k) {
        juce::var x = Sv[s][juce::Identifier(kStateKeys[k])];
        setParam(stateId(s, k), x.isVoid() ? 0.f : (float) (double) x);
    }
    abTrim = 1.f;
    return true;
}

bool SiltProcessor::loadPatchJson(const juce::String& json, const juce::String& name, const juce::var& notes) {
    const juce::var v = juce::JSON::parse(json);
    if (!loadPatchVar(v)) return false;
    patchName = name.isNotEmpty() ? name : v["name"].toString();
    patchNotes = notes;
    ++patchSerial;
    return true;
}

void SiltProcessor::loadFactory(int index) {
    if (index < 0 || index >= kNumFactory) return;
    currentProgram = index;
    loadPatchJson(juce::String::fromUTF8(kFactoryPatches[index].json), juce::String::fromUTF8(kFactoryPatches[index].name),
                  juce::JSON::parse(juce::String::fromUTF8(kFactoryPatches[index].notes)));
}

int SiltProcessor::getNumPrograms() { return kNumFactory; }
void SiltProcessor::setCurrentProgram(int index) { loadFactory(index); }
const juce::String SiltProcessor::getProgramName(int index) { return index >= 0 && index < kNumFactory ? juce::String::fromUTF8(kFactoryPatches[index].name) : juce::String(); }

void SiltProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty("patchName", patchName, nullptr);
    state.setProperty("patchNotes", juce::JSON::toString(patchNotes, true), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void SiltProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(apvts.state.getType())) {
            auto tree = juce::ValueTree::fromXml(*xml);
            patchName = tree.getProperty("patchName").toString();
            patchNotes = juce::JSON::parse(tree.getProperty("patchNotes").toString());
            apvts.replaceState(tree);
            ++patchSerial;
        }
    }
}

// ---------- recording ----------
bool SiltProcessor::startRecording() {
    if (recording.load()) return true;
    auto dir = takesDir(); dir.createDirectory();
    auto nm = patchName.isNotEmpty() ? patchName : juce::String("patch");
    nm = nm.retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -").trim().replaceCharacter(' ', '-').toLowerCase();
    while (nm.contains("--")) nm = nm.replace("--", "-");
    nm = nm.trimCharactersAtStart("-").trimCharactersAtEnd("-");
    recFile = dir.getNonexistentChildFile("silt-take-" + nm + "-" + juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S"), ".wav", false);
    std::unique_ptr<juce::OutputStream> os(recFile.createOutputStream());
    if (!os) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(os.get(), sampleRateHz, 2, 24, {}, 0));
    if (!writer) return false;
    os.release();
    recPatchJson = patchToJson();
    threaded = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer.release(), writerThread, 65536);
    { const juce::ScopedLock sl(writerLock); activeWriter = threaded.get(); }
    recSamples = 0; recording = true;
    return true;
}

juce::File SiltProcessor::stopRecording() {
    if (!recording.load()) return {};
    recording = false;
    { const juce::ScopedLock sl(writerLock); activeWriter = nullptr; }
    threaded.reset();
    recFile.withFileExtension(".json").replaceWithText(recPatchJson);
    return recFile;
}

juce::var SiltProcessor::bandPresets() { return juce::JSON::parse(juce::String::fromUTF8(kBandPresetsJson)); }

juce::AudioProcessorEditor* SiltProcessor::createEditor() { return new SiltEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SiltProcessor(); }
