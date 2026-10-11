#pragma once
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <array>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

namespace ui {
namespace col {
    const juce::Colour bg { 0xff0f1311 }, panel { 0xff171d1a }, ink { 0xffdde5de }, muted { 0xff8c998f }, line { 0xff2a332e },
        accent { 0xff62b8cb }, accentInk { 0xff0b171a }, sed { 0xffd69a58 }, warn { 0xffe08a6a }, screen { 0xff090d0b },
        screenInk { 0xff8ea096 }, grid { 0xff1d2723 }, trace { 0xff7fd6c6 }, trace2 { 0xffe0a25e }, key { 0xffcfd6cf }, keyDark { 0xff0d1210 };
}

class Look : public juce::LookAndFeel_V4 {
public:
    Look();
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&, int h) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getLabelFont(juce::Label&) override;
};

// One labelled control bound to a parameter.
class Row : public juce::Component {
public:
    Row(juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label);
    void resized() override;
    juce::String id;
    juce::Label label;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<juce::TextButton> toggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
};

class Panel : public juce::Component {
public:
    Panel(const juce::String& title, juce::Colour tint = col::accent) : title(title), tint(tint) {}
    void paint(juce::Graphics&) override;
    void resized() override;
    int preferredHeight() const;
    Row& add(juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label);
    void addExtra(juce::Component* c, int height) { extras.push_back({ c, height }); addAndMakeVisible(c); }
    juce::String title; juce::Colour tint;
    std::vector<std::unique_ptr<Row>> rows;
    std::vector<std::pair<juce::Component*, int>> extras;
};

class Bands : public juce::Component {
public:
    explicit Bands(SiltProcessor& p);
    void resized() override;
    void paint(juce::Graphics&) override;
    SiltProcessor& proc;
    juce::Slider sl[8];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att[8];
    juce::OwnedArray<juce::TextButton> presets;
};

class Screens : public juce::Component, private juce::Timer {
public:
    explicit Screens(SiltProcessor& p);
    void paint(juce::Graphics&) override;
    juce::String readout;
private:
    void timerCallback() override;
    void drawScope(juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum(juce::Graphics&, juce::Rectangle<float>);
    void drawCloud(juce::Graphics&, juce::Rectangle<float>);
    void drawTrace(juce::Graphics&, juce::Rectangle<float>);
    void screen(juce::Graphics&, juce::Rectangle<float>, const juce::String& caption);
    SiltProcessor& proc;
    float tbuf[4096] {};
    juce::dsp::FFT fft { 12 };
    std::vector<float> fftData = std::vector<float>(8192, 0.f), smooth = std::vector<float>(2048, 0.f);
    juce::dsp::WindowingFunction<float> window { 4096, juce::dsp::WindowingFunction<float>::blackman };
    silt::VizSnapshot viz;
    std::deque<std::optional<std::array<float, silt::NK>>> trace;
    int tick = 0;
};

class Notes : public juce::Component {
public:
    void set(const juce::var& notes);
    void paint(juce::Graphics&) override;
    int heightFor(int w) const;
private:
    juce::AttributedString text(int w) const;
    juce::var notes;
};
} // namespace ui

class SiltEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit SiltEditor(SiltProcessor&);
    ~SiltEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    bool keyStateChanged(bool isKeyDown) override;

private:
    void timerCallback() override;
    void buildPanels();
    void layoutPanels();
    void refreshPatchMenu();
    void onPatchChosen();
    void afterPatchLoad();
    void saveAs();
    void copyPatch();
    void pastePatch();
    void toggleRecord();
    void store(int slot);
    void hear(int slot);
    void abUi();
    void setOctave(int o);
    void press(int note);
    void release(int note);
    void panic();
    void releaseFocusTheft(juce::Component& c);

    SiltProcessor& proc;
    ui::Look look;

    // header
    juce::Label title;
    juce::ComboBox patchBox;
    juce::TextButton saveBtn { "Save as..." }, copyBtn { "Copy patch" }, pasteBtn { "Paste patch" }, recBtn { "Record take" }, takesBtn { "Takes folder" };
    juce::Label status;
    juce::StringArray userPatchNames;

    // left column
    ui::Screens screens;
    juce::Label readout;
    juce::ComboBox listenBox, tapBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> listenAtt, tapAtt;
    juce::Label listenLbl, tapLbl, listenNote;
    juce::TextButton storeA { "Store A" }, storeB { "Store B" }, hearA { "Hear A" }, hearB { "Hear B" }, matchBtn { "Level match" };
    juce::Label abNote;
    ui::Notes notes;

    // panels
    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<ui::Panel>> panels;
    std::unique_ptr<ui::Bands> bands;
    ui::Row* posRows[2] { nullptr, nullptr };

    // keyboard
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton octDown { "Z  oct -" }, octUp { "oct +  X" }, latchBtn { "Latch" }, panicBtn { "Panic" };
    juce::Label octLabel, keysHint;

    int octave = 3; bool latchOn = false;
    std::set<int> latched;
    std::map<int, int> keyHeld; // key code -> note
    int lastSerial = -1;
    juce::var abSlot[2]; int abCur = -1; float abLoud[2] { 0, 0 }; bool matchOn = true;
    int meterTick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SiltEditor)
};
