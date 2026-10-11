#include "PluginEditor.h"
#include "Format.h"

using namespace ui;

static juce::Font mono(float h) { return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain)); }
static juce::Font sans(float h, bool bold = false) { return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain)); }

// ============================== look ==============================
Look::Look() {
    setColour(juce::ResizableWindow::backgroundColourId, col::bg);
    setColour(juce::Label::textColourId, col::ink);
    setColour(juce::Slider::textBoxTextColourId, col::ink);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, col::accent.withAlpha(0.4f));
    setColour(juce::ComboBox::backgroundColourId, col::bg);
    setColour(juce::ComboBox::outlineColourId, col::line);
    setColour(juce::ComboBox::textColourId, col::ink);
    setColour(juce::ComboBox::arrowColourId, col::muted);
    setColour(juce::PopupMenu::backgroundColourId, col::panel);
    setColour(juce::PopupMenu::textColourId, col::ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, col::accent);
    setColour(juce::PopupMenu::highlightedTextColourId, col::accentInk);
    setColour(juce::PopupMenu::headerTextColourId, col::muted);
    setColour(juce::TextButton::buttonColourId, col::bg);
    setColour(juce::TextButton::buttonOnColourId, col::accent);
    setColour(juce::TextButton::textColourOffId, col::ink);
    setColour(juce::TextButton::textColourOnId, col::accentInk);
    setColour(juce::TextEditor::backgroundColourId, col::bg);
    setColour(juce::TextEditor::textColourId, col::ink);
    setColour(juce::TextEditor::outlineColourId, col::line);
    setColour(juce::TextEditor::focusedOutlineColourId, col::accent);
    setColour(juce::AlertWindow::backgroundColourId, col::panel);
    setColour(juce::AlertWindow::textColourId, col::ink);
    setColour(juce::AlertWindow::outlineColourId, col::line);
    setColour(juce::ScrollBar::thumbColourId, col::line.brighter(0.3f));
    setColour(juce::MidiKeyboardComponent::whiteNoteColourId, col::key);
    setColour(juce::MidiKeyboardComponent::blackNoteColourId, col::keyDark);
    setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, col::bg);
    setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, col::accent);
    setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, col::accent.withAlpha(0.25f));
    setColour(juce::MidiKeyboardComponent::textLabelColourId, col::keyDark);
    setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
}

void Look::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s) {
    if (style == juce::Slider::LinearVertical) {
        const float cx = x + w * 0.5f;
        g.setColour(col::line); g.fillRoundedRectangle(cx - 2.f, (float) y, 4.f, (float) h, 2.f);
        g.setColour(col::sed); g.fillRoundedRectangle(cx - 2.f, pos, 4.f, (float) (y + h) - pos, 2.f);
        g.setColour(col::ink); g.fillEllipse(cx - 6.f, pos - 6.f, 12.f, 12.f);
        return;
    }
    const float cy = y + h * 0.5f;
    g.setColour(col::line); g.fillRoundedRectangle((float) x, cy - 2.f, (float) w, 4.f, 2.f);
    // bipolar ranges fill from zero
    const auto& r = s.getNormalisableRange();
    float from = (float) x;
    if (r.start < 0 && r.end > 0) from = (float) x + (float) r.convertTo0to1(0.0) * (float) w;
    g.setColour(s.isMouseOverOrDragging() ? col::accent.brighter(0.2f) : col::accent);
    g.fillRoundedRectangle(juce::jmin(from, pos), cy - 2.f, std::abs(pos - from), 4.f, 2.f);
    g.setColour(col::ink); g.fillEllipse(pos - 6.f, cy - 6.f, 12.f, 12.f);
}

void Look::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const bool on = b.getToggleState();
    g.setColour(on ? col::accent : (down ? col::line : (over ? col::panel.brighter(0.08f) : col::bg)));
    g.fillRoundedRectangle(r, 5.f);
    g.setColour(on ? col::accent : col::line); g.drawRoundedRectangle(r, 5.f, 1.f);
}
juce::Font Look::getTextButtonFont(juce::TextButton&, int) { return sans(13.f); }
juce::Font Look::getComboBoxFont(juce::ComboBox&) { return sans(13.f); }
juce::Font Look::getPopupMenuFont() { return sans(14.f); }
juce::Font Look::getLabelFont(juce::Label& l) { return l.getFont(); }

// ============================== rows and panels ==============================
Row::Row(juce::AudioProcessorValueTreeState& s, const juce::String& pid, const juce::String& text) : id(pid) {
    label.setText(text, juce::dontSendNotification);
    label.setFont(sans(13.f)); label.setColour(juce::Label::textColourId, col::muted);
    addAndMakeVisible(label);
    const silt::PDef* def = nullptr;
    for (auto& d : silt::kParams) if (pid == d.id) def = &d;
    const auto kind = def ? def->kind : silt::Kind::Cont;
    if (kind == silt::Kind::Choice) {
        combo = std::make_unique<juce::ComboBox>();
        auto items = juce::StringArray::fromTokens(def->choices, "|", "");
        for (int i = 0; i < items.size(); ++i) combo->addItem(items[i], i + 1);
        addAndMakeVisible(*combo);
        cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, pid, *combo);
    } else if (kind == silt::Kind::Toggle) {
        toggle = std::make_unique<juce::TextButton>(text);
        toggle->setClickingTogglesState(true);
        addAndMakeVisible(*toggle);
        bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, pid, *toggle);
        label.setVisible(false);
    } else {
        slider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 92, 20);
        slider->setColour(juce::Slider::textBoxTextColourId, col::ink);
        addAndMakeVisible(*slider);
        sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, pid, *slider);
        if (auto* p = s.getParameter(pid)) slider->setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
    }
}

void Row::resized() {
    auto r = getLocalBounds();
    if (toggle) { toggle->setBounds(r.reduced(0, 1)); return; }
    label.setBounds(r.removeFromLeft(118));
    if (slider) slider->setBounds(r);
    if (combo) combo->setBounds(r.reduced(0, 1));
}

static constexpr int kRowH = 26, kTitleH = 30, kPad = 10;

Row& Panel::add(juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& label) {
    rows.push_back(std::make_unique<Row>(s, id, label));
    addAndMakeVisible(*rows.back());
    return *rows.back();
}
int Panel::preferredHeight() const {
    int h = kTitleH + kPad;
    int toggles = 0;
    for (auto& r : rows) { if (r->toggle) ++toggles; else h += kRowH; }
    h += (toggles + 1) / 2 * kRowH;
    for (auto& e : extras) h += e.second;
    return h;
}
void Panel::resized() {
    auto r = getLocalBounds().reduced(kPad, 0); r.removeFromTop(kTitleH); r.removeFromBottom(kPad);
    // toggles share a line, two per row
    std::vector<Row*> tg;
    for (auto& row : rows) if (row->toggle) tg.push_back(row.get());
    for (size_t i = 0; i < tg.size(); i += 2) {
        auto line = r.removeFromTop(kRowH);
        const int w = (line.getWidth() - 6) / 2;
        tg[i]->setBounds(line.removeFromLeft(w).reduced(0, 1));
        if (i + 1 < tg.size()) { line.removeFromLeft(6); tg[i + 1]->setBounds(line.reduced(0, 1)); }
    }
    for (auto& row : rows) if (!row->toggle) row->setBounds(r.removeFromTop(kRowH));
    for (auto& e : extras) e.first->setBounds(r.removeFromTop(e.second));
}
void Panel::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(col::panel); g.fillRoundedRectangle(r, 8.f);
    g.setColour(col::line); g.drawRoundedRectangle(r, 8.f, 1.f);
    g.setColour(tint); g.fillRect(kPad, 12, 3, 12);
    g.setColour(col::ink); g.setFont(sans(14.f, true));
    g.drawText(title, kPad + 9, 6, getWidth() - 2 * kPad, 24, juce::Justification::centredLeft);
}

// ============================== bands ==============================
Bands::Bands(SiltProcessor& p) : proc(p) {
    for (int b = 0; b < 8; ++b) {
        sl[b].setSliderStyle(juce::Slider::LinearVertical);
        sl[b].setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible(sl[b]);
        att[b] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "band" + juce::String(b), sl[b]);
    }
    const juce::var pr = SiltProcessor::bandPresets();
    if (auto* o = pr.getDynamicObject())
        for (auto& prop : o->getProperties()) {
            auto* btn = presets.add(new juce::TextButton(prop.name.toString()));
            const juce::var vals = prop.value;
            btn->onClick = [this, vals] { for (int b = 0; b < 8 && b < vals.size(); ++b)
                if (auto* prm = proc.apvts.getParameter("band" + juce::String(b))) prm->setValueNotifyingHost((float) (double) vals[b]); };
            addAndMakeVisible(btn);
        }
}
void Bands::resized() {
    auto r = getLocalBounds();
    auto btns = r.removeFromBottom(26);
    r.removeFromBottom(18); // centre-frequency labels
    const int w = r.getWidth() / 8;
    for (int b = 0; b < 8; ++b) sl[b].setBounds(r.getX() + b * w, r.getY(), w, r.getHeight());
    const int bw = presets.size() ? (btns.getWidth() - (presets.size() - 1) * 4) / presets.size() : 0;
    for (auto* b : presets) { b->setBounds(btns.removeFromLeft(bw)); btns.removeFromLeft(4); }
}
void Bands::paint(juce::Graphics& g) {
    auto r = getLocalBounds(); r.removeFromBottom(26);
    auto lab = r.removeFromBottom(18);
    const int w = r.getWidth() / 8;
    g.setColour(col::muted); g.setFont(mono(10.5f));
    for (int b = 0; b < 8; ++b) {
        const double c = 1000 * std::pow(2.0, (b - 3.5) * 0.85);
        g.drawText(c >= 1000 ? juce::String(c / 1000, 1) + "k" : juce::String((int) std::lround(c)), lab.getX() + b * w, lab.getY(), w, 16, juce::Justification::centred);
    }
}

// ============================== screens ==============================
Screens::Screens(SiltProcessor& p) : proc(p) { startTimerHz(30); setOpaque(false); }

void Screens::timerCallback() {
    viz = proc.getViz();
    if (++tick % 2 == 0) {
        if (viz.on) { std::array<float, silt::NK> a {}; for (int k = 0; k < silt::NK; ++k) a[(size_t) k] = viz.cur[k]; trace.push_back(a); }
        else trace.push_back(std::nullopt);
        const size_t maxN = (size_t) juce::jmax(10, getWidth() / 4);
        while (trace.size() > maxN) trace.pop_front();
    }
    repaint();
}

void Screens::screen(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& caption) {
    g.setColour(col::screen); g.fillRoundedRectangle(r, 6.f);
    g.setColour(col::grid);
    for (int i = 1; i < 8; ++i) { const float x = r.getX() + r.getWidth() * i / 8.f; g.drawVerticalLine((int) x, r.getY() + 2, r.getBottom() - 2); }
    for (int i = 1; i < 4; ++i) { const float y = r.getY() + r.getHeight() * i / 4.f; g.drawHorizontalLine((int) y, r.getX() + 2, r.getRight() - 2); }
    g.setColour(col::screenInk.withAlpha(0.8f)); g.setFont(mono(10.5f));
    g.drawText(caption, r.reduced(8, 4), juce::Justification::topLeft);
}

void Screens::drawScope(juce::Graphics& g, juce::Rectangle<float> r) {
    screen(g, r, "scope");
    const int w = proc.scopeWrite.load(std::memory_order_acquire);
    for (int i = 0; i < 4096; ++i) tbuf[i] = proc.scopeRing[(w - 4096 + i) & (SiltProcessor::kRing - 1)];
    int s = 1; for (; s < 2048; ++s) if (tbuf[s - 1] < 0 && tbuf[s] >= 0) break; if (s >= 2048) s = 0;
    float pk = 0; for (int i = 0; i < 2048; ++i) pk = juce::jmax(pk, std::abs(tbuf[s + i]));
    if (pk < 1e-5f) { g.setColour(col::screenInk); g.setFont(mono(12.f)); g.drawText("Play a note", r, juce::Justification::centred); return; }
    const float sc = 0.85f / juce::jmax(0.01f, pk);
    juce::Path p;
    for (int i = 0; i < 2048; ++i) { const float x = r.getX() + i / 2047.f * r.getWidth(), y = r.getCentreY() - tbuf[s + i] * sc * r.getHeight() * 0.5f;
        if (i) p.lineTo(x, y); else p.startNewSubPath(x, y); }
    g.setColour(col::trace); g.strokePath(p, juce::PathStrokeType(1.5f));
}

void Screens::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> r) {
    screen(g, r, "spectrum");
    std::fill(fftData.begin(), fftData.end(), 0.f);
    std::copy(tbuf, tbuf + 4096, fftData.begin());
    window.multiplyWithWindowingTable(fftData.data(), 4096);
    fft.performFrequencyOnlyForwardTransform(fftData.data(), true);
    for (int k = 0; k < 2048; ++k) smooth[(size_t) k] = smooth[(size_t) k] * 0.6f + fftData[(size_t) k] / 4096.f * 0.4f * 2.4f;
    const double nyq = proc.sampleRateHz / 2, lo = std::log(30.0), hi = std::log(16000.0);
    juce::Path p; bool started = false;
    for (float x = 0; x <= r.getWidth(); x += 2) {
        const double f = std::exp(lo + (hi - lo) * x / r.getWidth());
        const int bin = juce::jmin(2047, (int) std::lround(f / nyq * 2048));
        const float db = juce::jlimit(-110.f, -10.f, juce::Decibels::gainToDecibels(smooth[(size_t) bin], -200.f));
        const float y = r.getBottom() - (db + 110.f) / 100.f * r.getHeight();
        if (!started) { p.startNewSubPath(r.getX() + x, y); started = true; } else p.lineTo(r.getX() + x, y);
    }
    juce::Path fill(p); fill.lineTo(r.getRight(), r.getBottom()); fill.lineTo(r.getX(), r.getBottom()); fill.closeSubPath();
    g.setColour(col::trace.withAlpha(0.18f)); g.fillPath(fill);
    g.setColour(col::trace); g.strokePath(p, juce::PathStrokeType(1.2f));
}

void Screens::drawCloud(juce::Graphics& g, juce::Rectangle<float> r) {
    screen(g, r, "emitters (layer A)");
    const float cx = r.getCentreX(), cy = r.getCentreY() + 6, rad = juce::jmin(r.getWidth(), r.getHeight()) * 0.34f;
    g.setColour(col::grid.brighter(0.3f)); g.drawEllipse(cx - rad, cy - rad, rad * 2, rad * 2, 1.f);
    if (!viz.on) { g.setColour(col::screenInk); g.setFont(mono(12.f)); g.drawText("Play a note", r, juce::Justification::centred); return; }
    float sx = 0, sy = 0;
    for (int i = 0; i < viz.N; ++i) {
        const float a = viz.th[i] * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi, e = juce::jmin(1.f, viz.e[i]);
        sx += std::cos(a); sy += std::sin(a);
        g.setColour((e > 0.02f ? col::trace : col::screenInk).withAlpha(0.35f + 0.65f * e));
        const float d = 3 + e * 5;
        g.fillEllipse(cx + std::cos(a) * rad - d, cy + std::sin(a) * rad - d, d * 2, d * 2);
    }
    sx /= (float) viz.N; sy /= (float) viz.N;
    g.setColour(col::trace2); g.drawLine(cx, cy, cx + sx * rad, cy + sy * rad, 2.f);
    g.setColour(col::screenInk); g.setFont(mono(10.5f));
    g.drawText("coherence " + juce::String(std::hypot(sx, sy), 2), r.reduced(8, 4), juce::Justification::topRight);
}

void Screens::drawTrace(juce::Graphics& g, juce::Rectangle<float> r) {
    screen(g, r, "motion");
    struct L { int k; juce::Colour c; std::function<float(float)> nrm; };
    const float lo = std::log2(20.f), hi = std::log2(20000.f);
    const L lines[] = { { silt::K_LEVEL, col::trace, [](float v) { return v / 1.3f; } }, { silt::K_COUP, col::trace2, [](float v) { return v; } },
                        { silt::K_CUT, col::screenInk, [lo, hi](float v) { return (v - lo) / (hi - lo); } } };
    for (auto& l : lines) {
        juce::Path p; bool st = false; const int n = (int) trace.size();
        for (int i = 0; i < n; ++i) {
            if (!trace[(size_t) i]) { st = false; continue; }
            const float x = r.getRight() - (n - 1 - i) * 4.f, y = r.getBottom() - 6 - juce::jlimit(0.f, 1.f, l.nrm((*trace[(size_t) i])[(size_t) l.k])) * (r.getHeight() - 12);
            if (!st) { p.startNewSubPath(x, y); st = true; } else p.lineTo(x, y);
        }
        g.setColour(l.c); g.strokePath(p, juce::PathStrokeType(1.4f));
    }
    g.setFont(mono(10.f)); auto t = r.reduced(8, 4);
    g.setColour(col::trace); g.drawText("level", t, juce::Justification::topRight); t.removeFromRight(42);
    g.setColour(col::trace2); g.drawText("coupling", t, juce::Justification::topRight); t.removeFromRight(62);
    g.setColour(col::screenInk); g.drawText("cutoff", t, juce::Justification::topRight);
}

void Screens::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    auto top = r.removeFromTop(r.getHeight() * 0.38f); drawScope(g, top.reduced(0, 3));
    auto mid = r.removeFromTop(r.getHeight() * 0.45f); drawSpectrum(g, mid.reduced(0, 3));
    auto left = r.removeFromLeft(r.getWidth() * 0.45f); drawCloud(g, left.reduced(0, 3).withTrimmedRight(3)); drawTrace(g, r.reduced(0, 3).withTrimmedLeft(3));
}

// ============================== notes ==============================
void Notes::set(const juce::var& n) { notes = n; repaint(); }
juce::AttributedString Notes::text(int) const {
    juce::AttributedString s; s.setWordWrap(juce::AttributedString::byWord);
    static const char* keys[4][2] = { { "target", "Target" }, { "listen", "Listen for" }, { "decides", "It decides" }, { "play", "Try" } };
    if (!notes.isObject()) { s.append("Patch notes belong to the numbered test patches. This is your own patch.", sans(13.f), col::muted); return s; }
    for (auto& k : keys) {
        s.append(juce::String(k[1]).toUpperCase() + "\n", sans(10.5f, true), col::sed);
        s.append(notes[k[0]].toString() + "\n\n", sans(13.5f), col::ink);
    }
    return s;
}
int Notes::heightFor(int w) const { juce::TextLayout t; t.createLayout(text(w), (float) w - 20); return (int) t.getHeight() + 20; }
void Notes::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(col::panel); g.fillRoundedRectangle(r, 8.f); g.setColour(col::line); g.drawRoundedRectangle(r, 8.f, 1.f);
    juce::TextLayout t; t.createLayout(text(getWidth()), (float) getWidth() - 20); t.draw(g, r.reduced(10));
}

// ============================== editor ==============================
static const std::pair<int, int> kKeyMap[] = { { 'A', 0 }, { 'W', 1 }, { 'S', 2 }, { 'E', 3 }, { 'D', 4 }, { 'F', 5 }, { 'T', 6 }, { 'G', 7 }, { 'Y', 8 },
                                               { 'H', 9 }, { 'U', 10 }, { 'J', 11 }, { 'K', 12 }, { 'O', 13 }, { 'L', 14 }, { 'P', 15 }, { ';', 16 } };
static const char* kListenNotes[7] = { "Hearing the full output.",
    "Raw layers: the emitters and mixer only, before resonator, derez, filter and VCA. Auto-levelled.",
    "Pre-filter: after the resonator and derez, before the filter. Auto-levelled.",
    "Voices: after filter, VCA and heat, before feedback, vocoder and space. Auto-levelled.",
    "Vocoder wet only: what the material vocoder adds. Auto-levelled.", "Feedback return only. Auto-levelled.", "Space wet only. Auto-levelled." };

SiltEditor::SiltEditor(SiltProcessor& p)
    : AudioProcessorEditor(p), proc(p), screens(p), keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&look);

    title.setText("Silt", juce::dontSendNotification); title.setFont(sans(24.f, true)); addAndMakeVisible(title);
    addAndMakeVisible(patchBox); patchBox.onChange = [this] { onPatchChosen(); };
    for (auto* b : { &saveBtn, &copyBtn, &pasteBtn, &recBtn, &takesBtn }) addAndMakeVisible(b);
    saveBtn.onClick = [this] { saveAs(); };
    copyBtn.onClick = [this] { copyPatch(); };
    pasteBtn.onClick = [this] { pastePatch(); };
    recBtn.onClick = [this] { toggleRecord(); };
    takesBtn.onClick = [] { auto d = SiltProcessor::takesDir(); d.createDirectory(); d.startAsProcess(); };
    status.setFont(sans(13.f)); status.setColour(juce::Label::textColourId, col::muted); addAndMakeVisible(status);

    addAndMakeVisible(screens);
    readout.setFont(mono(12.f)); readout.setColour(juce::Label::textColourId, col::ink); readout.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(readout);
    listenLbl.setText("Listen", juce::dontSendNotification); tapLbl.setText("Scope shows", juce::dontSendNotification);
    for (auto* l : { &listenLbl, &tapLbl }) { l->setFont(sans(13.f)); l->setColour(juce::Label::textColourId, col::muted); addAndMakeVisible(l); }
    for (auto [box, id] : { std::pair<juce::ComboBox*, const char*> { &listenBox, "listen" }, { &tapBox, "tap" } }) {
        for (auto& d : silt::kParams) if (juce::String(d.id) == id) { auto items = juce::StringArray::fromTokens(d.choices, "|", ""); for (int i = 0; i < items.size(); ++i) box->addItem(items[i], i + 1); }
        addAndMakeVisible(box);
    }
    listenAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "listen", listenBox);
    tapAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, "tap", tapBox);
    listenNote.setFont(sans(12.5f)); listenNote.setColour(juce::Label::textColourId, col::muted); listenNote.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(listenNote);
    for (auto* b : { &storeA, &storeB, &hearA, &hearB, &matchBtn }) addAndMakeVisible(b);
    storeA.onClick = [this] { store(0); }; storeB.onClick = [this] { store(1); };
    hearA.onClick = [this] { hear(0); }; hearB.onClick = [this] { hear(1); };
    matchBtn.setClickingTogglesState(true); matchBtn.setToggleState(true, juce::dontSendNotification);
    matchBtn.onClick = [this] { matchOn = matchBtn.getToggleState(); if (!matchOn) proc.abTrim = 1.f; };
    abNote.setFont(sans(12.5f)); abNote.setColour(juce::Label::textColourId, col::muted); abNote.setJustificationType(juce::Justification::topLeft);
    abNote.setText("Store A, change something, store B, then flip between them while playing.", juce::dontSendNotification);
    addAndMakeVisible(abNote);
    addAndMakeVisible(notes);

    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(10);
    addAndMakeVisible(viewport);
    buildPanels();

    keyboard.clearKeyMappings();
    keyboard.setScrollButtonsVisible(false);
    keyboard.setOctaveForMiddleC(4);
    addAndMakeVisible(keyboard);
    for (auto* b : { &octDown, &octUp, &latchBtn, &panicBtn }) addAndMakeVisible(b);
    octDown.onClick = [this] { setOctave(octave - 1); };
    octUp.onClick = [this] { setOctave(octave + 1); };
    latchBtn.setClickingTogglesState(true);
    latchBtn.onClick = [this] { latchOn = latchBtn.getToggleState(); if (!latchOn) { for (int n : latched) proc.keyboardState.noteOff(1, n, 0.f); latched.clear(); } };
    panicBtn.onClick = [this] { panic(); };
    octLabel.setFont(mono(13.f)); octLabel.setJustificationType(juce::Justification::centred); addAndMakeVisible(octLabel);
    keysHint.setFont(mono(11.5f)); keysHint.setColour(juce::Label::textColourId, col::muted);
    keysHint.setText("Keys  A W S E D F T G Y H U J K O L P ;   Z/X octave   Mod wheel = Allure", juce::dontSendNotification);
    addAndMakeVisible(keysHint);

    refreshPatchMenu();
    releaseFocusTheft(*this);
    setWantsKeyboardFocus(true);
    setResizable(true, true);
    setResizeLimits(1040, 680, 3200, 2200);
    setSize(1400, 880);
    setOctave(3);
    abUi();
    startTimerHz(20);
}

SiltEditor::~SiltEditor() {
    for (auto& [k, n] : keyHeld) proc.keyboardState.noteOff(1, n, 0.f);
    setLookAndFeel(nullptr);
}

void SiltEditor::releaseFocusTheft(juce::Component& c) {
    for (auto* ch : c.getChildren()) {
        if (dynamic_cast<juce::TextEditor*>(ch) == nullptr) { ch->setWantsKeyboardFocus(false); ch->setMouseClickGrabsKeyboardFocus(false); }
        releaseFocusTheft(*ch);
    }
}

void SiltEditor::buildPanels() {
    auto& s = proc.apvts;
    auto layer = [&](const char* pre, const char* name, juce::Colour tint) {
        auto pnl = std::make_unique<ui::Panel>(name, tint);
        const juce::String P(pre);
        pnl->add(s, P + "On", "Layer on");
        if (P == "b") pnl->add(s, "bSync", "Sync to A");
        pnl->add(s, P + "Mat", "Material"); pnl->add(s, P + "Bank", "Bank");
        posRows[P == "a" ? 0 : 1] = &pnl->add(s, P + "Pos", "Position");
        pnl->add(s, P + "Form", "Formant"); pnl->add(s, P + "Cyc", "Cycles"); pnl->add(s, P + "Win", "Window"); pnl->add(s, P + "Phase", "Phase");
        pnl->add(s, P + "N", "Emitters"); pnl->add(s, P + "Det", "Detune spread"); pnl->add(s, P + "Wander", "Wander");
        pnl->add(s, P + "Pan", "Stereo spread"); pnl->add(s, P + "Lvl", "Level");
        if (P == "b") { pnl->add(s, "bSemi", "Pitch"); pnl->add(s, "bFine", "Fine"); }
        panels.push_back(std::move(pnl));
    };
    auto group = [&](const char* name, std::initializer_list<const char*> ids, juce::Colour tint = col::accent) -> ui::Panel& {
        auto pnl = std::make_unique<ui::Panel>(name, tint);
        for (auto* id : ids) for (auto& d : silt::kParams) if (juce::String(d.id) == id) pnl->add(s, id, d.label);
        panels.push_back(std::move(pnl)); return *panels.back();
    };
    layer("a", "Layer A", col::accent);
    layer("b", "Layer B", col::accent);
    for (int st = 0; st < 3; ++st) {
        auto pnl = std::make_unique<ui::Panel>(juce::String("State: ") + silt::kStageNames[st], col::trace2);
        for (int k = 0; k < silt::NK; ++k) pnl->add(s, silt::stateId(st, k), silt::kStateDefs[k].label);
        panels.push_back(std::move(pnl));
    }
    group("Articulation", { "atk", "dec", "rel", "curve", "loopTime", "loop" }, col::trace2);
    group("Motion", { "auth", "stiff", "visc", "temp", "poly", "velAmp" }, col::trace2);
    group("Macros", { "mass", "grainM", "allure", "vol" }, col::sed);
    group("Voice", { "pmAmt", "formTrack", "wanderRate", "derez", "drift", "resTune", "resDecay", "strike" });
    group("Filter and VCA", { "fMode", "reso", "drive", "heat", "keytrack", "velCut" });
    auto& voc = group("Material vocoder", { "vocOn", "vocAuto", "vocBody", "vocSkin", "vocMix", "vocShift", "vocSpread", "vocQ", "vocResp",
                                            "scrapeRate", "scrapeDepth", "dustRate", "dropRate", "dropPitch" }, col::sed);
    bands = std::make_unique<ui::Bands>(proc);
    voc.addExtra(bands.get(), 190);
    group("Feedback", { "fbType", "fbTime", "fbCut", "fbQ", "fbDrive", "fbRegen", "fbMix" }, col::sed);
    group("Space", { "spSize", "spDecay", "spTone", "spMix" }, col::sed);
    for (auto& pn : panels) content.addAndMakeVisible(*pn);
}

void SiltEditor::layoutPanels() {
    const int W = viewport.getWidth() - viewport.getScrollBarThickness() - 4;
    const int gap = 10, ncol = juce::jmax(1, (W + gap) / (300 + gap)), cw = (W - (ncol - 1) * gap) / ncol;
    std::vector<int> h((size_t) ncol, 0);
    for (auto& pn : panels) {
        const size_t c = (size_t) (std::min_element(h.begin(), h.end()) - h.begin());
        const int ph = pn->preferredHeight();
        pn->setBounds((int) c * (cw + gap), h[c], cw, ph);
        h[c] += ph + gap;
    }
    content.setSize(W, *std::max_element(h.begin(), h.end()));
}

void SiltEditor::paint(juce::Graphics& g) {
    g.fillAll(col::bg);
    g.setColour(col::line); g.drawHorizontalLine(52, 0, (float) getWidth());
    g.drawHorizontalLine(getHeight() - 128, 0, (float) getWidth());
}

void SiltEditor::resized() {
    auto r = getLocalBounds();
    auto head = r.removeFromTop(52).reduced(14, 10);
    title.setBounds(head.removeFromLeft(70));
    patchBox.setBounds(head.removeFromLeft(300)); head.removeFromLeft(8);
    for (auto* b : { &saveBtn, &copyBtn, &pasteBtn }) { b->setBounds(head.removeFromLeft(100)); head.removeFromLeft(6); }
    head.removeFromLeft(10);
    recBtn.setBounds(head.removeFromLeft(118)); head.removeFromLeft(6);
    takesBtn.setBounds(head.removeFromLeft(104)); head.removeFromLeft(12);
    status.setBounds(head);

    auto kb = r.removeFromBottom(128).reduced(14, 10);
    auto kbLeft = kb.removeFromLeft(150);
    auto line1 = kbLeft.removeFromTop(28);
    octDown.setBounds(line1.removeFromLeft(72)); line1.removeFromLeft(6); octUp.setBounds(line1);
    kbLeft.removeFromTop(6); octLabel.setBounds(kbLeft.removeFromTop(22));
    kbLeft.removeFromTop(6);
    auto line3 = kbLeft.removeFromTop(28); latchBtn.setBounds(line3.removeFromLeft(72)); line3.removeFromLeft(6); panicBtn.setBounds(line3);
    kb.removeFromLeft(12);
    keysHint.setBounds(kb.removeFromBottom(18));
    keyboard.setBounds(kb.withTrimmedBottom(4));
    keyboard.setKeyWidth((float) kb.getWidth() / 15.f);

    r.reduce(14, 12);
    auto left = r.removeFromLeft(juce::jlimit(360, 460, r.getWidth() / 3));
    r.removeFromLeft(14);
    viewport.setBounds(r);
    layoutPanels();

    screens.setBounds(left.removeFromTop(juce::jmax(300, left.getHeight() / 2 - 30)));
    readout.setBounds(left.removeFromTop(40));
    auto l1 = left.removeFromTop(26);
    listenLbl.setBounds(l1.removeFromLeft(50)); listenBox.setBounds(l1.removeFromLeft(150)); l1.removeFromLeft(10);
    tapLbl.setBounds(l1.removeFromLeft(84)); tapBox.setBounds(l1);
    listenNote.setBounds(left.removeFromTop(34));
    auto ab = left.removeFromTop(28); const int bw = (ab.getWidth() - 4 * 6) / 5;
    for (auto* b : { &storeA, &storeB, &hearA, &hearB, &matchBtn }) { b->setBounds(ab.removeFromLeft(bw)); ab.removeFromLeft(6); }
    abNote.setBounds(left.removeFromTop(34));
    left.removeFromTop(4);
    notes.setBounds(left);
}

// ---------- patches ----------
void SiltEditor::refreshPatchMenu() {
    patchBox.clear(juce::dontSendNotification);
    patchBox.addSectionHeading("Tests and references");
    for (int i = 0; i < proc.getNumPrograms(); ++i) patchBox.addItem(proc.getProgramName(i), i + 1);
    userPatchNames.clear();
    auto dir = SiltProcessor::userPatchDir();
    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.json");
    files.sort();
    if (!files.isEmpty()) {
        patchBox.addSeparator(); patchBox.addSectionHeading("Your patches (Documents/Silt/Patches)");
        for (auto& f : files) { userPatchNames.add(f.getFileNameWithoutExtension()); patchBox.addItem(f.getFileNameWithoutExtension(), 1000 + userPatchNames.size() - 1); }
    }
    int sel = 0;
    for (int i = 0; i < proc.getNumPrograms(); ++i) if (proc.getProgramName(i) == proc.patchName) sel = i + 1;
    const int ui = userPatchNames.indexOf(proc.patchName); if (sel == 0 && ui >= 0) sel = 1000 + ui;
    if (sel) patchBox.setSelectedId(sel, juce::dontSendNotification); else patchBox.setText(proc.patchName.isNotEmpty() ? proc.patchName : "Unsaved patch", juce::dontSendNotification);
}

void SiltEditor::onPatchChosen() {
    const int id = patchBox.getSelectedId();
    if (id <= 0) return;
    if (id < 1000) proc.loadFactory(id - 1);
    else {
        const auto name = userPatchNames[id - 1000];
        const auto f = SiltProcessor::userPatchDir().getChildFile(name + ".json");
        if (!proc.loadPatchJson(f.loadFileAsString(), name, juce::var())) status.setText("That file is not a Silt patch.", juce::dontSendNotification);
    }
    abSlot[0] = abSlot[1] = juce::var(); abCur = -1; abUi();
}

void SiltEditor::afterPatchLoad() {
    notes.set(proc.patchNotes);
    for (auto& pn : panels) for (auto& row : pn->rows) if (row->slider) row->slider->setDoubleClickReturnValue(true, row->slider->getValue());
    refreshPatchMenu();
}

void SiltEditor::saveAs() {
    auto* w = new juce::AlertWindow("Save patch", "Name this patch. It is saved to Documents/Silt/Patches as a .json file you can paste to Claude.", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor("name", proc.patchName.startsWithChar('0') ? juce::String() : proc.patchName, "Name");
    w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int r) {
        if (r != 1) return;
        auto name = juce::File::createLegalFileName(w->getTextEditorContents("name").trim());
        if (name.isEmpty()) { status.setText("Type a name first.", juce::dontSendNotification); return; }
        auto dir = SiltProcessor::userPatchDir(); dir.createDirectory();
        proc.patchName = name; proc.patchNotes = juce::var();
        if (dir.getChildFile(name + ".json").replaceWithText(proc.patchToJson())) status.setText("Saved \"" + name + "\".", juce::dontSendNotification);
        else status.setText("Could not write to Documents/Silt/Patches.", juce::dontSendNotification);
        ++proc.patchSerial;
    }), true);
}

void SiltEditor::copyPatch() {
    juce::SystemClipboard::copyTextToClipboard(proc.patchToJson());
    status.setText("Patch copied. Paste it to Claude in chat, or into the browser lab.", juce::dontSendNotification);
}

void SiltEditor::pastePatch() {
    const auto t = juce::SystemClipboard::getTextFromClipboard();
    if (proc.loadPatchJson(t, {}, juce::var())) status.setText("Patch loaded from the clipboard.", juce::dontSendNotification);
    else status.setText("The clipboard does not hold a Silt patch. Copy one from the lab or from chat first.", juce::dontSendNotification);
    abSlot[0] = abSlot[1] = juce::var(); abCur = -1; abUi();
}

void SiltEditor::toggleRecord() {
    if (!proc.isRecording()) {
        if (proc.startRecording()) { recBtn.setToggleState(true, juce::dontSendNotification); status.setText("Recording...", juce::dontSendNotification); }
        else status.setText("Could not start recording to Documents/Silt/Takes.", juce::dontSendNotification);
    } else {
        auto f = proc.stopRecording();
        recBtn.setToggleState(false, juce::dontSendNotification); recBtn.setButtonText("Record take");
        status.setText("Saved " + f.getFileName() + " and its patch .json in Documents/Silt/Takes. Attach both in chat.", juce::dontSendNotification);
    }
}

// ---------- A/B ----------
void SiltEditor::abUi() {
    hearA.setEnabled(!abSlot[0].isVoid()); hearB.setEnabled(!abSlot[1].isVoid());
    hearA.setToggleState(abCur == 0, juce::dontSendNotification); hearB.setToggleState(abCur == 1, juce::dontSendNotification);
}
void SiltEditor::store(int k) {
    abSlot[k] = juce::JSON::parse(proc.patchToJson()); abLoud[k] = 0; abCur = k; abUi();
    abNote.setText(juce::String("Stored ") + (k ? "B. " : "A. ") + (!abSlot[0].isVoid() && !abSlot[1].isVoid() ? "Flip between Hear A and Hear B while playing." : "Change something, then store the other slot."), juce::dontSendNotification);
}
void SiltEditor::hear(int k) {
    if (abSlot[k].isVoid()) return;
    abCur = k;
    const auto keepName = proc.patchName; const auto keepNotes = proc.patchNotes;
    proc.loadPatchVar(abSlot[k]); proc.patchName = keepName; proc.patchNotes = keepNotes;
    proc.abTrim = 1.f; abUi();
}

// ---------- keyboard ----------
void SiltEditor::setOctave(int o) {
    for (auto& [k, n] : keyHeld) release(n);
    keyHeld.clear();
    octave = juce::jlimit(0, 7, o);
    octLabel.setText("C" + juce::String(octave), juce::dontSendNotification);
    keyboard.setAvailableRange(12 * (octave + 1), 12 * (octave + 1) + 24); // the 25 notes the computer keys reach
}
void SiltEditor::press(int n) {
    if (latchOn) { if (latched.count(n)) { latched.erase(n); proc.keyboardState.noteOff(1, n, 0.f); } else { latched.insert(n); proc.keyboardState.noteOn(1, n, 0.85f); } }
    else proc.keyboardState.noteOn(1, n, 0.85f);
}
void SiltEditor::release(int n) { if (!latchOn) proc.keyboardState.noteOff(1, n, 0.f); }
void SiltEditor::panic() {
    for (int n = 0; n < 128; ++n) proc.keyboardState.noteOff(1, n, 0.f);
    latched.clear(); keyHeld.clear();
    proc.panicRequest = true;
}

static bool typingInTextField() { return dynamic_cast<juce::TextEditor*>(juce::Component::getCurrentlyFocusedComponent()) != nullptr; }

bool SiltEditor::keyPressed(const juce::KeyPress& k) {
    if (typingInTextField()) return false;
    const auto mods = k.getModifiers();
    if (mods.isCommandDown() || mods.isAltDown() || mods.isCtrlDown()) return false;
    const int c = juce::CharacterFunctions::toUpperCase(k.getTextCharacter());
    if (c == 'Z') { setOctave(octave - 1); return true; }
    if (c == 'X') { setOctave(octave + 1); return true; }
    for (auto& m : kKeyMap) if (m.first == c) return true; // handled in keyStateChanged
    return false;
}

bool SiltEditor::keyStateChanged(bool) {
    const bool typing = typingInTextField();
    const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
    bool any = false;
    for (auto& m : kKeyMap) {
        const bool down = juce::KeyPress::isKeyCurrentlyDown(m.first);
        auto it = keyHeld.find(m.first);
        if (down && it == keyHeld.end()) {
            if (typing || mods.isCommandDown() || mods.isAltDown() || mods.isCtrlDown()) continue;
            const int n = 12 * (octave + 1) + m.second;
            keyHeld[m.first] = n; press(n); any = true;
        } else if (!down && it != keyHeld.end()) { release(it->second); keyHeld.erase(it); any = true; }
    }
    return any;
}

// ---------- timer ----------
void SiltEditor::timerCallback() {
    // keep the computer keyboard playing notes whatever was clicked last
    if (juce::ModalComponentManager::getInstance()->getNumModalComponents() == 0 && isShowing()) {
        auto* f = juce::Component::getCurrentlyFocusedComponent();
        // take focus back from anything in this window that is not a text field (the standalone's Options button, for example)
        const bool ours = f != nullptr && (f == this || isParentOf(f));
        const bool sameWindow = f == nullptr || f->getTopLevelComponent() == getTopLevelComponent();
        if (!ours && sameWindow && !typingInTextField() && juce::Process::isForegroundProcess()) grabKeyboardFocus();
    }
    // release notes whose keys went up while focus was elsewhere
    for (auto it = keyHeld.begin(); it != keyHeld.end();) {
        if (!juce::KeyPress::isKeyCurrentlyDown(it->first)) { release(it->second); it = keyHeld.erase(it); } else ++it;
    }
    if (proc.patchSerial.load() != lastSerial) { lastSerial = proc.patchSerial.load(); afterPatchLoad(); }
    for (auto* r : posRows) if (r && r->slider) r->slider->updateText();
    listenNote.setText(kListenNotes[juce::jlimit(0, 6, listenBox.getSelectedItemIndex())], juce::dontSendNotification);
    {
        static const char* stages[5] = { "Onset", "Continuant", "Termination", "Looping", "Tail" };
        const auto v = proc.getViz();
        juce::String t;
        if (v.on) {
            const float f = v.f0;
            t << "Stage " << stages[juce::jlimit(0, 4, v.stage)] << "   Emission " << juce::String(f, f < 10 ? 2 : 1) << " Hz (" << (f < 20 ? "rhythm" : "pitch") << ")\n"
              << "Formant " << juce::String((int) std::lround(v.ff)) << " Hz   Coherence " << juce::String(v.R, 2) << "   Voices " << v.voices;
        } else t << "Ready. Play the computer keys, the on-screen keys or a MIDI controller.";
        readout.setText(t, juce::dontSendNotification);
    }
    if (proc.isRecording()) {
        const double s = proc.recordSeconds();
        recBtn.setButtonText("Stop  " + juce::String((int) s) + " s");
        if (s > 600) toggleRecord();
    }
    // level-matched A/B, as in the lab: the louder slot is trimmed down to the quieter one
    if (++meterTick % 2 == 0 && abCur >= 0 && !abSlot[0].isVoid() && !abSlot[1].isVoid() && proc.activeVoices.load() > 0) {
        const float vol = proc.apvts.getRawParameterValue("vol")->load();
        const float L = proc.meterRms.load() * vol;
        abLoud[abCur] = abLoud[abCur] > 0 ? abLoud[abCur] * 0.85f + L * 0.15f : L;
        if (matchOn && abLoud[0] > 0 && abLoud[1] > 0) {
            const float t = juce::jmin(1.f, juce::jmin(abLoud[0], abLoud[1]) / abLoud[abCur]);
            if (std::abs(t - proc.abTrim.load()) > 0.02f) proc.abTrim = t;
            abNote.setText(juce::String("Hearing ") + (abCur ? "B" : "A") + ". Level match trims it by " + juce::String(20 * std::log10(juce::jmax(1e-4f, t)), 1) + " dB.", juce::dontSendNotification);
        }
    }
}
