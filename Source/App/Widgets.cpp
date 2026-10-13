#include "PluginEditor.h"
#include "Format.h"
#include "Help.h"

using namespace ui;

static juce::Font wmono(float h) { return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain)); }
static juce::Font wsans(float h, bool bold = false) { return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain)); }

// ============================== articulation view ==============================
ArticulationView::ArticulationView(SiltProcessor& p) : proc(p) {
    for (int k = 0; k < silt::NK; ++k) field.addItem(silt::kStateDefs[k].label, k + 1);
    field.setSelectedId(silt::K_COUP + 1, juce::dontSendNotification);
    field.setTooltip("A third articulation field to draw alongside level and cutoff.");
    field.onChange = [this] { trail.clear(); repaint(); };
    setTooltip("Faint lines: the shape you set with the states and times. Bright lines: what the playing voice actually does, with the physics and modulation included.");
    addAndMakeVisible(field);
}
void ArticulationView::resized() { field.setBounds(getWidth() - 132, 2, 130, 22); }

// x positions of the segment boundaries for a plot rectangle
static void artGeom(const SiltProcessor& p, juce::Rectangle<float> plot, float xs[6], bool& loop) {
    auto raw = [&](const char* id) { return p.apvts.getRawParameterValue(id)->load(); };
    const float atk = raw("atk"), dec = raw("dec"), rel = raw("rel"), tailP = raw("tail");
    loop = raw("loop") > 0.5f;
    const float tail = tailP > 0.001f ? tailP : rel * 0.35f * 6.9f;
    auto wOf = [](float t) { return std::log1p(t * 8.f) + 0.08f; };
    const float wA = wOf(atk), wD = wOf(dec), wH = 0.9f, wR = wOf(rel), wT = wOf(tail), W = wA + wD + wH + wR + wT;
    const float x0 = plot.getX(), sx = plot.getWidth() / W;
    xs[0] = x0; xs[1] = x0 + wA * sx; xs[2] = xs[1] + wD * sx; xs[3] = xs[2] + wH * sx; xs[4] = xs[3] + wR * sx; xs[5] = plot.getRight();
}
static juce::Rectangle<float> artPlot(juce::Rectangle<float> local) { return local.withTrimmedTop(28).reduced(8, 6).withTrimmedBottom(16); }

void ArticulationView::update(const silt::VizSnapshot& v) {
    using namespace silt;
    viz = v;
    if (v.on) {
        float xs[6]; bool lp; artGeom(proc, artPlot(getLocalBounds().toFloat()), xs, lp);
        float x = xs[0];
        switch (v.stage) {
            case 0: x = xs[0] + v.prog * (xs[1] - xs[0]); break;
            case 1: x = v.prog < 1 ? xs[1] + v.prog * (xs[2] - xs[1]) : juce::jmin(xs[3] - 2, lastStage == 1 && lastX >= xs[2] ? lastX + 0.6f : xs[2]); break;
            case 3: x = xs[2] - v.prog * (xs[2] - xs[1]); break;
            case 2: x = xs[3] + v.prog * (xs[4] - xs[3]); break;
            default: x = xs[4] + v.prog * (xs[5] - xs[4]); break;
        }
        if (v.note != lastNote || (v.stage == 0 && lastStage != 0 && lastStage != 3)) trail.clear();
        lastNote = v.note; lastStage = v.stage; lastX = x;
        const int fk = field.getSelectedId() - 1;
        const float lv = v.stage == 4 ? v.cur[K_LEVEL] * std::exp(-6.9f * v.prog) : v.cur[K_LEVEL];
        trail.push_back({ x, lv, std::pow(2.f, v.cur[K_CUT]), v.cur[juce::jlimit(0, NK - 1, fk)] });
        if (trail.size() > 600) trail.erase(trail.begin());
    }
    repaint();
}

void ArticulationView::paint(juce::Graphics& g) {
    using namespace silt;
    auto r = getLocalBounds().toFloat().withTrimmedTop(28);
    g.setColour(col::screen); g.fillRoundedRectangle(r, 6.f);
    auto plot = artPlot(getLocalBounds().toFloat());
    float xs[6]; bool loop; artGeom(proc, plot, xs, loop);
    const float curve = raw("curve");
    // segment grid and names
    g.setFont(wmono(9.5f));
    const char* seg[5] = { "attack", "decay", loop ? "held (looping)" : "held", "release", "tail" };
    for (int i = 0; i < 5; ++i) {
        g.setColour(col::grid); if (i) g.drawVerticalLine((int) xs[i], plot.getY(), plot.getBottom());
        g.setColour(col::screenInk.withAlpha(0.7f)); g.drawText(seg[i], juce::Rectangle<float>(xs[i], plot.getBottom() + 2, xs[i + 1] - xs[i], 12), juce::Justification::centred);
    }
    g.setColour(col::sed.withAlpha(0.85f)); g.setFont(wmono(9.5f));
    g.drawText("onset", juce::Rectangle<float>(xs[1] - 30, plot.getY() - 2, 60, 11), juce::Justification::centred);
    g.drawText("continuant", juce::Rectangle<float>(xs[2] - 34, plot.getY() - 2, 68, 11), juce::Justification::centred);
    g.drawText("termination", juce::Rectangle<float>(xs[4] - 36, plot.getY() - 2, 72, 11), juce::Justification::centred);
    // authored paths
    const int fk = field.getSelectedId() - 1;
    auto st = [&](int s, int k) { return raw(stateId(s, k)); };
    auto norm = [&](int k, float v) {
        const SDef& d = kStateDefs[k];
        if (d.log) return (std::log2(juce::jlimit(d.min, d.max, v)) - std::log2(d.min)) / (std::log2(d.max) - std::log2(d.min));
        return (v - d.min) / (d.max - d.min);
    };
    const double c = curve * 9;
    auto shape = [&](double u) { return std::abs(c) < 0.01 ? u : c > 0 ? (1 - std::exp(-c * u)) / (1 - std::exp(-c)) : (std::exp(-c * u) - 1) / (std::exp(-c) - 1); };
    auto pathFor = [&](int k, bool isLevel) {
        juce::Path p;
        const float fcut = k == K_CUT ? raw("fCut") : 0.f;
        auto val = [&](float v) { return k == K_CUT ? v * std::pow(2.f, fcut) : v; };
        const float v0 = isLevel ? 0.f : val(st(0, k)), v1 = val(st(0, k)), v2 = val(st(1, k)), v3 = val(st(2, k));
        auto Y = [&](float v) { return plot.getBottom() - juce::jlimit(0.f, 1.f, norm(k, v)) * plot.getHeight(); };
        p.startNewSubPath(xs[0], Y(v0));
        auto seg2 = [&](float xa, float xb, float a, float b) { for (int i = 1; i <= 24; ++i) { const double u = i / 24.0; p.lineTo(xa + float(u) * (xb - xa), Y(float(a + (b - a) * shape(u)))); } };
        seg2(xs[0], xs[1], v0, v1); seg2(xs[1], xs[2], v1, v2);
        p.lineTo(xs[3], Y(v2));
        seg2(xs[3], xs[4], v2, v3);
        if (isLevel) { for (int i = 1; i <= 24; ++i) { const float u = i / 24.f; p.lineTo(xs[4] + u * (xs[5] - xs[4]), Y(v3 * std::exp(-6.9f * u))); } }
        else p.lineTo(xs[5], Y(v3));
        return p;
    };
    struct Ln { int k; juce::Colour c; const char* name; };
    const Ln lines[3] = { { K_CUT, col::screenInk, "cutoff" }, { fk, col::trace2, kStateDefs[fk].label }, { K_LEVEL, col::trace, "level" } };
    for (auto& l : lines) { g.setColour(l.c.withAlpha(0.45f)); g.strokePath(pathFor(l.k, l.k == K_LEVEL), juce::PathStrokeType(1.3f)); }
    auto Yk = [&](int k, float v) { return plot.getBottom() - juce::jlimit(0.f, 1.f, norm(k, v)) * plot.getHeight(); };
    if (trail.size() > 1) {
        for (auto& l : lines) {
            juce::Path p; bool started = false;
            for (auto& t : trail) { const float v = l.k == K_LEVEL ? t.lv : l.k == K_CUT ? t.cut : t.fd; const float y = Yk(l.k, v);
                if (!started) { p.startNewSubPath(t.x, y); started = true; } else p.lineTo(t.x, y); }
            g.setColour(l.c); g.strokePath(p, juce::PathStrokeType(2.f));
        }
        const auto& t = trail.back();
        g.setColour(col::ink); g.fillEllipse(t.x - 3, Yk(K_LEVEL, t.lv) - 3, 6, 6);
        g.setColour(col::ink.withAlpha(0.25f)); g.drawVerticalLine((int) t.x, plot.getY(), plot.getBottom());
    }
    // legend
    g.setFont(wmono(10.f)); float lx = 2;
    for (int i = 2; i >= 0; --i) { g.setColour(lines[i].c); const juce::String n = lines[i].name; g.drawText(n, juce::Rectangle<float>(lx, 4, 90, 16), juce::Justification::centredLeft); lx += wmono(10.f).getStringWidthFloat(n) + 12; }
}

// ============================== meter, buttons ==============================
void Meter::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    const float v = juce::jlimit(0.f, 1.f, get ? get() : 0.f);
    g.setColour(col::muted); g.setFont(wsans(12.5f));
    g.drawText(name, r.removeFromLeft(118), juce::Justification::centredLeft);
    auto bar = r.reduced(0, 8).withTrimmedRight(60);
    g.setColour(col::line); g.fillRoundedRectangle(bar, 3.f);
    g.setColour(colour); g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * v), 3.f);
    g.setColour(col::ink); g.setFont(wmono(11.f));
    g.drawText(text ? text() : juce::String(juce::roundToInt(v * 100)) + "%", r.removeFromRight(56), juce::Justification::centredRight);
}

void ButtonBar::addButton(const juce::String& text, std::function<void()> fn) {
    auto* b = buttons.add(new juce::TextButton(text)); b->onClick = std::move(fn); addAndMakeVisible(b);
}
void ButtonBar::resized() {
    auto r = getLocalBounds().reduced(0, 2);
    const int n = buttons.size(); if (!n) return;
    const int w = (r.getWidth() - (n - 1) * 6) / n;
    for (auto* b : buttons) { b->setBounds(r.removeFromLeft(w)); r.removeFromLeft(6); }
}

// ============================== mod matrix row ==============================
ModRow::ModRow(SiltProcessor& p, int s) : proc(p), slot(s) {
    const juce::String pre = "m" + juce::String(s);
    for (int i = 0; i < (int) silt::modSourceNames().size(); ++i) src.addItem(juce::String::fromUTF8(silt::modSourceNames()[(size_t) i].c_str()), i + 1);
    std::string lastGroup;
    const auto& d = silt::modDests();
    for (size_t i = 0; i < d.size(); ++i) {
        const std::string lab = d[i].label; const auto colon = lab.find(": ");
        const std::string grp = colon == std::string::npos ? "" : lab.substr(0, colon), item = colon == std::string::npos ? lab : lab.substr(colon + 2);
        if (grp != lastGroup && !grp.empty()) { dst.addSectionHeading(juce::String::fromUTF8(grp.c_str())); lastGroup = grp; }
        dst.addItem(juce::String::fromUTF8(item.c_str()), (int) i + 1);
    }
    amt.setSliderStyle(juce::Slider::LinearHorizontal); amt.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 20);
    for (juce::Component* c : { (juce::Component*) &src, (juce::Component*) &dst, (juce::Component*) &amt }) addAndMakeVisible(c);
    sAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, pre + "Src", src);
    dAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts, pre + "Dst", dst);
    aAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, pre + "Amt", amt);
    amt.setDoubleClickReturnValue(true, 0.0); amt.setScrollWheelEnabled(false);
    src.setTooltip("Modulation source"); dst.setTooltip("What it moves"); amt.setTooltip("How much, and which way. Double-click for zero.");
}
void ModRow::resized() {
    auto r = getLocalBounds().reduced(0, 2);
    auto top = r.removeFromTop(24);
    src.setBounds(top.removeFromLeft(top.getWidth() * 2 / 5)); top.removeFromLeft(4); dst.setBounds(top);
    r.removeFromTop(3); r.removeFromLeft(26); amt.setBounds(r.removeFromTop(22));
}
void ModRow::paint(juce::Graphics& g) {
    auto r = getLocalBounds().reduced(0, 2); r.removeFromTop(27);
    auto lamp = r.removeFromLeft(22).removeFromTop(22).toFloat().reduced(4);
    g.setColour(col::line); g.fillEllipse(lamp);
    g.setColour(col::trace.withAlpha(juce::jlimit(0.f, 1.f, std::abs(live)))); g.fillEllipse(lamp.reduced(1));
    g.setColour(col::muted); g.setFont(wmono(9.f)); g.drawText(juce::String(slot + 1), lamp.translated(0, 0).toNearestInt(), juce::Justification::centred);
}

// ============================== sequencer graph ==============================
juce::Point<float> SeqGraph::node(int i) const {
    const auto c = getLocalBounds().toFloat().withTrimmedBottom(20).getCentre();
    const float rad = juce::jmin(getWidth(), getHeight() - 20) * 0.40f;
    const float a = juce::MathConstants<float>::twoPi * i / 16.f - juce::MathConstants<float>::halfPi;
    return { c.x + std::cos(a) * rad, c.y + std::sin(a) * rad };
}
int SeqGraph::hit(juce::Point<float> p) const { for (int i = 0; i < 16; ++i) if (node(i).getDistanceFrom(p) < 14) return i; return -1; }
void SeqGraph::set(const juce::String& id, float v) { if (auto* p = proc.apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(v)); }

static void arrow(juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Point<float> centre, juce::Colour c, bool dashed, float bend) {
    if (a.getDistanceFrom(b) < 1) { g.setColour(c); g.drawEllipse(a.x - 6, a.y - 26, 12, 14, 1.4f); return; }
    // bend toward the centre so cords across the ring stay readable
    auto mid = (a + b) * 0.5f; auto toC = centre - mid;
    const auto ctrl = mid + toC * bend;
    juce::Path shortP; bool started = false; juce::Point<float> prev = a, end = a;
    for (int i = 0; i <= 32; ++i) {
        const float t = i / 32.f, u = 1 - t;
        const juce::Point<float> q = a * (u * u) + ctrl * (2 * u * t) + b * (t * t);
        if (q.getDistanceFrom(a) < 12 || q.getDistanceFrom(b) < 14) continue;
        if (!started) { shortP.startNewSubPath(q); started = true; } else { shortP.lineTo(q); prev = end; }
        end = q;
    }
    if (!started || prev == end) return;
    g.setColour(c);
    if (dashed) { const float dl[2] = { 5.f, 4.f }; juce::Path d; juce::PathStrokeType(1.5f).createDashedStroke(d, shortP, dl, 2); g.fillPath(d); }
    else g.strokePath(shortP, juce::PathStrokeType(1.6f));
    const float ang = std::atan2(end.y - prev.y, end.x - prev.x);
    juce::Path head; head.addTriangle(0, 0, -8, -4, -8, 4);
    g.fillPath(head, juce::AffineTransform::rotation(ang).translated(end));
}

void SeqGraph::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(col::screen); g.fillRoundedRectangle(r, 6.f);
    const auto centre = r.withTrimmedBottom(20).getCentre();
    const int len = juce::jlimit(1, 16, (int) raw("seqLen"));
    // cords
    for (int i = 0; i < 16; ++i) {
        const juce::String p = "st" + juce::String(i);
        const int to1 = (int) raw(p + "To1"), to2 = (int) raw(p + "To2");
        if (to1 == 0) { if (i < len) arrow(g, node(i), node((i + 1) % len), centre, col::grid.brighter(0.6f), false, 0.0f); }
        else arrow(g, node(i), node(to1 - 1), centre, col::accent, false, 0.35f);
        if (to2 > 0) arrow(g, node(i), node(to2 - 1), centre, col::sed, true, 0.55f);
    }
    if (dragging && dragFrom >= 0) { g.setColour(col::ink.withAlpha(0.6f)); g.drawLine(juce::Line<float>(node(dragFrom), dragPos), 1.5f); }
    // nodes
    for (int i = 0; i < 16; ++i) {
        const juce::String p = "st" + juce::String(i);
        const bool on = raw(p + "On") > 0.5f, inLen = i < len;
        const auto c = node(i);
        juce::Rectangle<float> b(c.x - 12, c.y - 12, 24, 24);
        g.setColour(on ? col::accent.withAlpha(inLen ? 0.35f : 0.18f) : col::bg); g.fillEllipse(b);
        if (heads[0] == i) { g.setColour(col::trace); g.fillEllipse(b); }
        if (heads[1] == i) { g.setColour(col::trace2); g.drawEllipse(b.expanded(3), 2.5f); }
        g.setColour(i == selected ? col::sed : col::line.brighter(0.4f)); g.drawEllipse(b, i == selected ? 2.2f : 1.f);
        const int note = (int) raw(p + "Note");
        g.setColour(heads[0] == i ? col::accentInk : (on ? col::ink : col::muted)); g.setFont(wmono(10.f));
        g.drawText(note == 0 ? juce::String(i + 1) : (note > 0 ? "+" : "") + juce::String(note), b.toNearestInt(), juce::Justification::centred);
        if (raw(p + "Cut") > 0.5f) { g.setColour(col::warn); g.fillEllipse(c.x + 7, c.y - 12, 5, 5); }
    }
    g.setColour(col::screenInk.withAlpha(0.75f)); g.setFont(wmono(9.5f));
    g.drawText("drag step to step: cord   right-click: clear cords   double-click: on/off", r.removeFromBottom(18), juce::Justification::centred);
}
void SeqGraph::mouseDown(const juce::MouseEvent& e) {
    const int h = hit(e.position);
    if (h < 0) return;
    if (e.mods.isPopupMenu()) { set("st" + juce::String(h) + "To1", 0); set("st" + juce::String(h) + "To2", 0); repaint(); return; }
    selected = h; if (onSelect) onSelect(h);
    dragFrom = h; dragPos = e.position; dragging = false; repaint();
}
void SeqGraph::mouseDrag(const juce::MouseEvent& e) { if (dragFrom < 0) return; dragPos = e.position; dragging = e.getDistanceFromDragStart() > 6; repaint(); }
void SeqGraph::mouseUp(const juce::MouseEvent& e) {
    if (dragging && dragFrom >= 0) {
        const int to = hit(e.position);
        if (to >= 0) {
            const juce::String p = "st" + juce::String(dragFrom);
            if ((int) raw(p + "To1") == 0) set(p + "To1", float(to + 1)); else set(p + "To2", float(to + 1));
        }
    }
    dragFrom = -1; dragging = false; repaint();
}
void SeqGraph::mouseDoubleClick(const juce::MouseEvent& e) {
    const int h = hit(e.position); if (h < 0) return;
    const juce::String id = "st" + juce::String(h) + "On"; set(id, raw(id) > 0.5f ? 0.f : 1.f); repaint();
}
