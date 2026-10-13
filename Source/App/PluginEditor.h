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
    Panel(const juce::String& t, juce::Colour c = col::accent) : title(t), tint(c) {}
    void paint(juce::Graphics&) override;
    void resized() override;
    int preferredHeight() const;
    Row& add(juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label);
    void addExtra(juce::Component* c, int height) { extras.push_back({ c, height }); addAndMakeVisible(c); }
    void clearRows() { rows.clear(); }
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

// The articulation drawn as a path through its states, with the playing voice moving along it.
class ArticulationView : public juce::Component, public juce::SettableTooltipClient {
public:
    explicit ArticulationView(SiltProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
    void update(const silt::VizSnapshot& v);
private:
    float raw(const juce::String& id) const { return proc.apvts.getRawParameterValue(id)->load(); }
    SiltProcessor& proc;
    juce::ComboBox field;
    struct Pt { float x, lv, cut, fd; };
    std::vector<Pt> trail; int lastNote = -1, lastStage = -1; float lastX = 0;
    silt::VizSnapshot viz;
};

// Small live meter: a label and a bar.
class Meter : public juce::Component {
public:
    Meter(const juce::String& name, std::function<float()> value, juce::Colour c = col::trace) : name(name), get(std::move(value)), colour(c) {}
    void paint(juce::Graphics&) override;
    juce::String name; std::function<float()> get; juce::Colour colour; std::function<juce::String()> text;
};

// A row of action buttons.
class ButtonBar : public juce::Component {
public:
    void addButton(const juce::String& text, std::function<void()> fn);
    void resized() override;
    juce::OwnedArray<juce::TextButton> buttons;
};

// Mod matrix slot: source, destination, amount, with the source's live value.
class ModRow : public juce::Component {
public:
    ModRow(SiltProcessor& p, int slot);
    void resized() override;
    void paint(juce::Graphics&) override;
    SiltProcessor& proc; int slot; float live = 0;
    juce::ComboBox src, dst; juce::Slider amt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sAtt, dAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aAtt;
};

// The graph sequencer: steps on a ring, cords between them, playheads moving through.
class SeqGraph : public juce::Component {
public:
    explicit SeqGraph(SiltProcessor& p) : proc(p) {}
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    std::function<void(int)> onSelect;
    int selected = 0, heads[2] { -1, -1 };
private:
    juce::Point<float> node(int i) const;
    int hit(juce::Point<float> p) const;
    float raw(const juce::String& id) const { return proc.apvts.getRawParameterValue(id)->load(); }
    void set(const juce::String& id, float v);
    SiltProcessor& proc; int dragFrom = -1; juce::Point<float> dragPos; bool dragging = false;
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
    void showTab(int t);
    void selectStep(int i);
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

    // panels, one page per tab
    static constexpr int kTabs = 5;
    juce::TextButton tabBtn[kTabs];
    juce::Viewport viewport;
    juce::Component content[kTabs];
    std::vector<std::unique_ptr<ui::Panel>> panels[kTabs];
    int tab = 0;
    std::unique_ptr<ui::Bands> bands;
    std::unique_ptr<ui::ArticulationView> artView;
    std::unique_ptr<ui::SeqGraph> graph;
    ui::Panel* stepPanel = nullptr;
    std::vector<std::unique_ptr<juce::Component>> extras;
    std::vector<ui::ModRow*> modRows;
    ui::Row* posRows[2] { nullptr, nullptr };
    juce::TooltipWindow tooltips { this, 600 };

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
