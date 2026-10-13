#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Engine/SiltEngine.h"
#include "../Engine/ParamTable.h"

class SiltProcessor : public juce::AudioProcessor {
public:
    SiltProcessor();
    ~SiltProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Silt"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ---- patches (JSON format shared with the browser lab) ----
    juce::String patchToJson() const;
    bool loadPatchJson(const juce::String& json, const juce::String& name, const juce::var& notes); // keeps Listen and Scope
    void loadFactory(int index);
    juce::var patchVar() const;
    bool loadPatchVar(const juce::var& v);
    static juce::File userPatchDir();
    static juce::File takesDir();
    static juce::var bandPresets(); // {"Flat":[...], "Water":[...], ...}

    // ---- recording ----
    bool startRecording();
    juce::File stopRecording(); // returns the WAV file
    bool isRecording() const { return recording.load(); }
    double recordSeconds() const { return recSamples.load() / juce::jmax(1.0, sampleRateHz); }

    // ---- shared with the editor ----
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    std::atomic<float> abTrim { 1.f };
    std::atomic<bool> panicRequest { false };
    enum Action { A_CATCH = 1, A_REC = 2, A_CLEAR = 4, A_SEDCLEAR = 8, A_ERORESTORE = 16 };
    std::atomic<int> actions { 0 };
    void act(Action a) { actions.fetch_or(a); }
    std::atomic<float> meterRms { 0.f };
    std::atomic<int> activeVoices { 0 };
    static constexpr int kRing = 16384;
    float scopeRing[kRing] {};
    std::atomic<int> scopeWrite { 0 };
    silt::VizSnapshot getViz() { const juce::SpinLock::ScopedLockType l(vizLock); return vizCopy; }
    juce::String patchName;
    juce::var patchNotes;
    std::atomic<int> patchSerial { 0 }; // bumps whenever a patch is loaded so the editor can refresh
    double sampleRateHz = 48000;
    std::atomic<float>* bankRaw[2] { nullptr, nullptr };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void renderSegment(float* L, float* R, int n);

    silt::Engine engine;
    silt::Params P;
    silt::StateSet S {};
    std::vector<std::pair<std::atomic<float>*, float*>> bindings;
    std::atomic<float>* stateRaw[3][silt::NK] {};
    juce::RangedAudioParameter* allureParam = nullptr;
    std::vector<float> tapBuf;
    juce::SpinLock vizLock; silt::VizSnapshot vizCopy;
    int currentProgram = 1;

    juce::TimeSliceThread writerThread { "Silt take writer" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threaded;
    juce::CriticalSection writerLock;
    juce::AudioFormatWriter::ThreadedWriter* activeWriter = nullptr;
    std::atomic<bool> recording { false };
    std::atomic<juce::int64> recSamples { 0 };
    juce::File recFile;
    juce::String recPatchJson;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SiltProcessor)
};
