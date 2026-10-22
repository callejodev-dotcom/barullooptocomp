#include "PluginEditor.h"
#include "BinaryData.h"
#include "Presets.h"

namespace
{
    using escoita::U;
    using escoita::numero;
}

namespace ui
{
// ================================================================ tipografía
// Montserrat (títulos e rótulos, en maiúsculas espaciadas coma na web de Escoita) e Inter
// (texto corrido): as da familia Barullo. As dúas, licenza OFL.
static juce::Typeface::Ptr tf (int which)
{
    static juce::Typeface::Ptr t[3] = {
        juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize) };
    return t[which];
}
juce::Font body (float s) { return juce::Font (juce::FontOptions (tf (0)).withHeight (s)); }
juce::Font semi (float s) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s)); }
juce::Font cond (float s) { return juce::Font (juce::FontOptions (tf (2)).withHeight (s)); }
juce::Font caps (float s, float tracking) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s).withKerningFactor (tracking)); }
juce::String upper (const juce::String& t) { return escoita::maiusculas (t); }

void drawCaps (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, float size, float tracking)
{
    g.setFont (caps (size, tracking));
    g.drawText (upper (t), r, j, false);
}

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c)
{
    g.setColour (c.withAlpha (0.16f));
    for (const auto d : { juce::Point<float> (-0.8f, 0.0f), { 0.8f, 0.0f }, { 0.0f, -0.8f }, { 0.0f, 0.8f } })
        g.drawText (t, r.translated (d.x, d.y), j, false);
    g.setColour (c);
    g.drawText (t, r, j, false);
}

// ================================================================ pezas do aparello
void paintPanel (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.11f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f * (float) i).expanded ((float) i), rad + (float) i);
    }
    g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, r.getY(), col::panel2, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (r.getX() + rad, r.getY() + 0.5f, r.getWidth() - 2.0f * rad, 1.0f);
}

void paintWell (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    g.setColour (col::visor.brighter (0.06f));
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.65f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 5.0f, false));
    g.fillRect (r.withHeight (6.0f));
    g.restoreState();
}

void paintGlass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    juce::ColourGradient gr (col::cristal1, r.getCentreX(), r.getY(), col::cristal2, r.getCentreX(), r.getBottom(), false);
    gr.addColour (0.55, juce::Colour (0xff0a1129));
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.16f - 0.03f * (float) i));
        g.drawRoundedRectangle (r.reduced ((float) i + 0.5f), rad, 1.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), r.getX(), r.getY(),
                                             juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.32f, r.getY() + r.getHeight() * 0.55f, false));
    g.fillRect (r);
    g.restoreState();
}

void paintKey (juce::Graphics& g, juce::Rectangle<float> r, bool over, bool down)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 7.0f);
    if (down) r.translate (0.0f, 1.0f);
    const float lift = over && ! down ? 0.10f : 0.0f;
    g.setGradientFill (juce::ColourGradient (col::tecla1.brighter (lift), 0.0f, r.getY(), col::tecla2.brighter (lift * 0.6f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.04f : 0.10f));
    g.fillRect (r.getX() + 6.0f, r.getY() + 0.5f, r.getWidth() - 12.0f, 1.0f);
}

void paintLed (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour luz)
{
    if (on)
    {
        g.setColour (luz.withAlpha (0.16f)); g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
        g.setColour (luz.withAlpha (0.34f)); g.fillEllipse (c.x - 4.6f, c.y - 4.6f, 9.2f, 9.2f);
        g.setColour (luz); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillEllipse (c.x - 1.3f, c.y - 2.0f, 2.2f, 2.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff3a3326)); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f, 0.8f);
    }
}

// rótulo dun grupo: na súa cor e cunha liña debaixo
void groupTitle (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c)
{
    g.setColour (c);
    drawCaps (g, t, r.withHeight (14.0f), juce::Justification::centred, 9.0f, 0.28f);
    g.setColour (c.withAlpha (0.25f));
    g.fillRect (r.getX() + 8.0f, r.getY() + 18.0f, r.getWidth() - 16.0f, 1.0f);
}

// ================================================================ aspecto
Look::Look()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::noite);
    setColour (juce::Label::textColourId, col::crema);
    setColour (juce::TextEditor::backgroundColourId, col::visor);
    setColour (juce::TextEditor::textColourId, col::ouro);
    setColour (juce::TextEditor::outlineColourId, col::lina);
    setColour (juce::TextEditor::focusedOutlineColourId, col::laranxa);
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::crema);
    setColour (juce::PopupMenu::headerTextColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedTextColourId, col::noite);
    setColour (juce::TextButton::buttonColourId, col::tecla2);
    setColour (juce::TextButton::textColourOffId, col::crema2);
    setColour (juce::TextButton::textColourOnId, col::crema);
    setColour (juce::AlertWindow::backgroundColourId, col::panel2);
    setColour (juce::AlertWindow::textColourId, col::crema);
    setColour (juce::AlertWindow::outlineColourId, col::lina);
}

juce::Typeface::Ptr Look::getTypefaceForFont (const juce::Font& f) { return tf (f.isBold() ? 1 : 0); }

// as teclas: en relevo, cun LED (aceso se está escollida; «led» = a cor da capa) e o texto en
// maiúsculas espaciadas; «gold» = un visor dourado (o selector da combinación)
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        paintWell (g, r, 8.0f);
        if (over) { g.setColour (col::laranxa.withAlpha (0.08f)); g.fillRoundedRectangle (r, 8.0f); }
        return;
    }
    paintKey (g, r, over, down);
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed"))
    {
        const auto luz = b.getProperties().contains ("led") ? juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["led"]) : col::laranxa;
        paintLed (g, { r.getX() + 12.0f, r.getCentreY() }, b.getToggleState(), luz);
    }
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        g.setFont (caps (12.0f, 0.1f));
        glowText (g, upper (b.getButtonText()), r.reduced (14.0f, 0.0f).withTrimmedRight (16.0f), juce::Justification::centredLeft, col::ouro);
        juce::Path arrow;
        const float ax = r.getRight() - 18.0f, ay = r.getCentreY();
        arrow.addTriangle (ax - 5.0f, ay - 2.5f, ax + 5.0f, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (col::ouro); g.fillPath (arrow);
        return;
    }
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed")) r.removeFromLeft (14.0f);
    juce::Colour c = b.getToggleState() || over ? col::crema : col::crema2;
    if (! b.isEnabled()) c = col::crema3.withAlpha (0.6f);
    g.setColour (c);
    const int curve = b.getProperties()["curve"];   // desfacer (−1) e refacer (+1): unha frecha en arco
    if (curve != 0)
    {
        const auto ctr = r.getCentre(); const float rad = 6.5f;
        const float a0 = curve > 0 ? -2.4f : 2.4f, a1 = curve > 0 ? 1.2f : -1.2f;
        juce::Path arc; arc.addCentredArc (ctr.x, ctr.y + 1.0f, rad, rad, 0.0f, a0, a1, true);
        g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const juce::Point<float> tip (ctr.x + rad * std::sin (a1), ctr.y + 1.0f - rad * std::cos (a1));
        juce::Path head; head.addTriangle (tip.x + (float) curve * 4.0f, tip.y, tip.x - (float) curve * 1.5f, tip.y - 4.0f, tip.x - (float) curve * 1.5f, tip.y + 4.0f);
        g.fillPath (head);
        return;
    }
    g.setFont (caps ((float) b.getProperties().getWithDefault ("fontSize", 9.5f), 0.15f));
    g.drawFittedText (upper (b.getButtonText()), r.toNearestInt().reduced (3, 0), juce::Justification::centred, 1, 0.7f);
}

// os mandos: disco mecanizado, anel de LED laranxa e o valor nun visor debaixo
void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
    auto lcd = area.removeFromBottom (17.0f);
    lcd = lcd.withSizeKeepingCentre (juce::jmin (66.0f, area.getWidth() - 2.0f), 17.0f);
    area.removeFromBottom (3.0f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.withSizeKeepingCentre (d, d).getCentre();
    const float ring = d * 0.5f - 4.0f;
    const auto luz = s.getProperties().contains ("led") ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["led"]) : col::laranxa;

    juce::Path track; track.addCentredArc (c.x, c.y, ring, ring, 0.0f, a0, a1, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
    const double zeroAt = bip ? 0.0 : s.getMinimum();
    const float zero = a0 + (float) s.valueToProportionOfLength (zeroAt) * (a1 - a0);
    const float ang = a0 + pos * (a1 - a0);
    if (std::abs (ang - zero) > 0.01f)
    {
        juce::Path val; val.addCentredArc (c.x, c.y, ring, ring, 0.0f, juce::jmin (zero, ang), juce::jmax (zero, ang), true);
        const auto lit = s.isEnabled() ? luz : col::crema3;
        g.setColour (lit.withAlpha (0.12f)); g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit.withAlpha (0.26f)); g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit); g.strokePath (val, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float kr = ring - 7.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr, c.y - kr + 2.5f, kr * 2.0f, kr * 2.0f);
    juce::ColourGradient disc (juce::Colour (0xff4a5577), c.x - kr * 0.3f, c.y - kr * 0.45f, juce::Colour (0xff0e1323), c.x + kr * 0.55f, c.y + kr * 1.05f, true);
    disc.addColour (0.55, juce::Colour (0xff232b45));
    g.setGradientFill (disc);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);
    const juce::Point<float> tip (c.x + kr * 0.82f * std::sin (ang), c.y - kr * 0.82f * std::cos (ang));
    g.setColour (col::crema.withAlpha (s.isEnabled() ? 0.92f : 0.4f));
    g.drawLine ({ c, tip }, 2.0f);

    paintWell (g, lcd, 5.0f);
    g.setFont (caps (9.0f, 0.04f));
    glowText (g, upper (s.getTextFromValue (s.getValue())), lcd, juce::Justification::centred, s.isEnabled() ? col::ouro : col::crema3);
}

// os deslizadores: a pista afundida, a luz laranxa desde abaixo ata o capuchón e o capuchón mecanizado
void Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y, bot = (float) (y + h);
    auto slot = juce::Rectangle<float> (cx - 4.5f, top - 7.0f, 9.0f, bot - top + 14.0f);
    paintWell (g, slot, 4.5f);
    const int steps = s.getMaximum() - s.getMinimum() <= 3.5 ? (int) (s.getMaximum() - s.getMinimum()) : 10;
    g.setColour (col::crema3.withAlpha (0.55f));
    for (int t = 0; t <= steps; ++t)
    {
        const float ty = bot - (bot - top) * (float) t / (float) juce::jmax (1, steps);
        const float len = (t % 5 == 0 || steps <= 3) ? 5.0f : 2.5f;
        g.fillRect (cx - 9.0f - len, ty - 0.5f, len, 1.0f);
        g.fillRect (cx + 9.0f, ty - 0.5f, len, 1.0f);
    }
    if (pos < bot - 0.5f)
    {
        auto lit = juce::Rectangle<float> (cx - 1.5f, pos, 3.0f, bot + 4.0f - pos);
        g.setColour (col::laranxa.withAlpha (0.14f)); g.fillRoundedRectangle (lit.expanded (3.0f, 0.0f), 3.0f);
        g.setColour (col::laranxa.withAlpha (0.30f)); g.fillRoundedRectangle (lit.expanded (1.5f, 0.0f), 2.0f);
        g.setColour (col::laranxa); g.fillRoundedRectangle (lit, 1.5f);
    }
    auto cap = juce::Rectangle<float> (cx - 13.0f, pos - 7.0f, 26.0f, 14.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (cap.translated (0.0f, 2.5f), 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a5577), cap.getX(), cap.getY(), juce::Colour (0xff141a2e), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawRoundedRectangle (cap.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (col::crema); g.fillRect (cap.getX() + 4.0f, pos - 0.8f, cap.getWidth() - 8.0f, 1.6f);
}

void Look::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<int> (w, h).toFloat();
    g.setColour (col::panel2); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::lina); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    juce::AttributedString s; s.setJustification (juce::Justification::topLeft);
    s.append (text, body (13.5f), col::crema);
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

juce::Rectangle<int> Look::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s; s.append (text, body (13.5f));
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    const int w = (int) tl.getWidth() + 22, h = (int) tl.getHeight() + 16;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h).constrainedWithin (parent);
}

void Look::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panel2);
    g.setColour (col::lina); g.drawRect (0, 0, w, h);
}

// ================================================================ mando
Knob::Knob()
{
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (180);
    addAndMakeVisible (slider);
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (slider.isEnabled() ? col::crema2 : col::crema3);
    drawCaps (g, name, getLocalBounds().removeFromBottom (12).toFloat(), juce::Justification::centred, 7.5f, 0.16f);
}



// ================================================================ utilidades
namespace
{
    const juce::Colour AMBAR { 0xffffb040 }, AMBAR_CLARO { 0xffffd68c };
    constexpr float A0 = juce::MathConstants<float>::pi * 1.25f, A1 = juce::MathConstants<float>::pi * 2.75f;
    juce::Point<float> polar (juce::Point<float> c, float r, float a) { return { c.x + r * std::sin (a), c.y - r * std::cos (a) }; }
    juce::String dbTxt (double db) { return (db >= 0.05 ? "+" : db <= -0.05 ? U ("−") : juce::String()) + numero (std::abs (db), 1) + " dB"; }
}

// ================================================================ mando negro
MandoNegro::MandoNegro()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (A0, A1, true);
    setMouseDragSensitivity (260);
}

void MandoNegro::paint (juce::Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float pos = (float) valueToProportionOfLength (getValue());
    // a escala: marcas cada 10 e os números 0, 50 e 100
    for (int k = 0; k <= 10; ++k)
    {
        const float a = A0 + (float) k / 10.0f * (A1 - A0);
        g.setColour (col::crema3);
        g.drawLine ({ polar (c, 70.0f, a), polar (c, k % 5 == 0 ? 77.0f : 74.0f, a) }, 1.1f);
        if (k % 5 == 0)
        {
            const auto p = polar (c, 85.0f, a);
            g.setFont (caps (7.0f, 0.0f));
            g.drawText (juce::String (k * 10), juce::Rectangle<float> (p.x - 12.0f, p.y - 6.0f, 24.0f, 12.0f), juce::Justification::centred, false);
        }
    }
    // o anel de luces
    for (int k = 0; k < 25; ++k)
    {
        const float u = (float) k / 24.0f;
        const auto p = polar (c, 61.0f, A0 + u * (A1 - A0));
        if (u <= pos + 1e-4f && pos > 0.002f)
        {
            g.setColour (AMBAR.withAlpha (0.18f)); g.fillEllipse (p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);
            g.setColour (AMBAR.withAlpha (0.4f)); g.fillEllipse (p.x - 3.6f, p.y - 3.6f, 7.2f, 7.2f);
            g.setColour (AMBAR_CLARO); g.fillEllipse (p.x - 2.1f, p.y - 2.1f, 4.2f, 4.2f);
        }
        else { g.setColour (juce::Colour (0xff242c4a)); g.fillEllipse (p.x - 2.0f, p.y - 2.0f, 4.0f, 4.0f); }
    }
    // sombra, saia de aluminio moleteada e o mando negro
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (c.x - 52.0f, c.y - 46.0f, 104.0f, 110.0f);
    g.setColour (juce::Colour (0xff969ca9)); g.fillEllipse (c.x - 50.0f, c.y - 50.0f, 100.0f, 100.0f);
    for (int k = 0; k < 60; ++k)
    {
        const float a = (float) k * juce::MathConstants<float>::twoPi / 60.0f;
        g.setColour (k % 2 ? juce::Colour (0xff6e7482) : juce::Colour (0xffbec4ce));
        g.drawLine ({ polar (c, 44.0f, a), polar (c, 50.0f, a) }, 1.4f);
    }
    juce::ColourGradient tapa (juce::Colour (0xff484a54), c.x - 14.0f, c.y - 18.0f, juce::Colour (0xff0e0f14), c.x + 30.0f, c.y + 40.0f, true);
    g.setGradientFill (tapa);
    g.fillEllipse (c.x - 40.0f, c.y - 40.0f, 80.0f, 80.0f);
    juce::Path brillo; brillo.addCentredArc (c.x, c.y, 38.0f, 38.0f, 0.0f, -1.9f, -0.4f, true);
    g.setColour (juce::Colours::white.withAlpha (0.16f)); g.strokePath (brillo, juce::PathStrokeType (1.0f));
    const float a = A0 + pos * (A1 - A0);
    g.setColour (col::crema); g.drawLine ({ polar (c, 12.0f, a), polar (c, 36.0f, a) }, 3.0f);
}

// ================================================================ VU
float Vu::posicion (double db)
{
    const double lo = std::pow (10.0, -20.0 / 20.0), hi = std::pow (10.0, 3.0 / 20.0);
    return (float) juce::jlimit (-0.04, 1.06, (std::pow (10.0, juce::jlimit (-30.0, 6.0, db) / 20.0) - lo) / (hi - lo));
}

void Vu::tick (double dt)
{
    // o que mide: a redución (0 sen comprimir, cara á esquerda canto máis) ou a saída (0 VU = −18 dBFS)
    double db;
    if (proc.vu == 0) db = proc.reducion.load();
    else db = 20.0 * std::log10 (std::max (1e-6, proc.saidaMedia.load() * 1.11 / 0.1259));
    const double obx = posicion (db);
    // a agulla: un resorte amortecido coma o dun VU (chega en ~300 ms, cun chisco de pasada)
    const double w = 2 * juce::MathConstants<double>::pi * 2.1, z = 0.78;
    for (int k = 0; k < 4; ++k)
    {
        const double h = dt / 4.0;
        vel += (w * w * (obx - pos) - 2.0 * z * w * vel) * h;
        pos += vel * h;
    }
    repaint();
}

void Vu::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::visor); g.fillRoundedRectangle (r, 14.0f);
    auto cara = r.reduced (7.0f);
    juce::ColourGradient fondo (juce::Colour (0xfff6e8be), 0.0f, cara.getY(), juce::Colour (0xffdcc694), 0.0f, cara.getBottom(), false);
    g.setGradientFill (fondo); g.fillRoundedRectangle (cara, 10.0f);
    // a luz cálida por detrás
    juce::ColourGradient luz (juce::Colour (0x66ffd078), cara.getCentreX(), cara.getCentreY(), juce::Colour (0x00ffd078), cara.getRight(), cara.getCentreY(), true);
    g.setGradientFill (luz); g.fillRoundedRectangle (cara, 10.0f);

    const juce::Point<float> c (cara.getCentreX(), cara.getBottom() - 2.0f);
    const float rr = cara.getHeight() * 0.76f;
    auto ang = [] (float u) { return juce::degreesToRadians (-50.0f + 100.0f * u); };
    const juce::Colour tinta (0xff28221c), roxo (0xffc83228);
    juce::Path arco;
    for (int k = 0; k <= 60; ++k) { const auto p = polar (c, rr, ang (posicion (-20.0 + 23.0 * k / 60.0))); if (k == 0) arco.startNewSubPath (p); else arco.lineTo (p); }
    g.setColour (tinta); g.strokePath (arco, juce::PathStrokeType (1.4f));
    juce::Path vermello;
    for (int k = 0; k <= 30; ++k) { const auto p = polar (c, rr, ang (posicion (3.0 * k / 30.0))); if (k == 0) vermello.startNewSubPath (p); else vermello.lineTo (p); }
    g.setColour (roxo); g.strokePath (vermello, juce::PathStrokeType (5.0f));
    const int marcas[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    for (int m : marcas)
    {
        const float a = ang (posicion (m));
        g.setColour (m > 0 ? roxo : tinta);
        g.drawLine ({ polar (c, rr, a), polar (c, rr + 9.0f, a) }, 1.5f);
        const auto p = polar (c, rr + 20.0f, a);
        g.setFont (semi (9.0f));
        g.drawText (juce::String (std::abs (m)), juce::Rectangle<float> (p.x - 12.0f, p.y - 7.0f, 24.0f, 14.0f), juce::Justification::centred, false);
    }
    g.setFont (cond (14.0f)); g.setColour (tinta);
    g.drawText (U ("−"), juce::Rectangle<float> (cara.getX() + 8.0f, cara.getY() + cara.getHeight() * 0.55f, 20.0f, 20.0f), juce::Justification::centred, false);
    g.setColour (roxo);
    g.drawText ("+", juce::Rectangle<float> (cara.getRight() - 28.0f, cara.getY() + cara.getHeight() * 0.55f, 20.0f, 20.0f), juce::Justification::centred, false);
    g.setColour (tinta); g.setFont (cond (20.0f));
    g.drawText ("VU", juce::Rectangle<float> (c.x - 40.0f, cara.getY() + cara.getHeight() * 0.48f, 80.0f, 24.0f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xff6e5c46));
    drawCaps (g, proc.vu == 0 ? U ("Redución") : U ("Saída"), juce::Rectangle<float> (c.x - 60.0f, cara.getY() + cara.getHeight() * 0.68f, 120.0f, 12.0f), juce::Justification::centred, 6.5f, 0.3f);
    // a agulla
    const float a = ang ((float) juce::jlimit (-0.05, 1.07, pos));
    g.setColour (juce::Colours::black.withAlpha (0.18f)); g.drawLine ({ c.translated (3.0f, 2.0f), polar (c, rr + 4.0f, a).translated (3.0f, 2.0f) }, 1.8f);
    g.setColour (juce::Colour (0xff141210)); g.drawLine ({ c, polar (c, rr + 4.0f, a) }, 1.8f);
    g.setColour (juce::Colour (0xff1e1c1a)); g.fillEllipse (c.x - 9.0f, c.y - 9.0f, 18.0f, 18.0f);
    // o cristal
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), cara.getX(), cara.getY(), juce::Colours::transparentWhite, cara.getX() + cara.getWidth() * 0.45f, cara.getBottom(), false));
    g.fillRoundedRectangle (cara, 10.0f);
}

// ================================================================ célula
void Celula::tick()
{
    luz = juce::jmax (proc.brillo.load(), luz * 0.9f);
    red = proc.reducion.load();
    repaint();
}

void Celula::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::visor); g.fillRoundedRectangle (r, 12.0f);
    g.setColour (col::lina); g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
    g.setColour (col::crema3);
    drawCaps (g, U ("A célula"), r.withHeight (28.0f), juce::Justification::centred, 6.5f, 0.3f);
    // o panel: apagado, gris; acendido, ámbar e con luz arredor
    auto panel = juce::Rectangle<float> (r.getX() + 22.0f, r.getY() + 34.0f, r.getWidth() - 44.0f, 92.0f);
    const float l = juce::jlimit (0.0f, 1.0f, luz);
    {
        juce::ColourGradient halo (AMBAR.withAlpha (0.5f * l), panel.getCentreX(), panel.getCentreY(), AMBAR.withAlpha (0.0f), panel.getCentreX() + panel.getWidth() * 0.9f, panel.getCentreY(), true);
        g.setGradientFill (halo);
        g.fillEllipse (panel.expanded (panel.getWidth() * 0.45f, panel.getHeight() * 0.25f));
    }
    const auto apagado = juce::Colour (0xff3a3426), alto = juce::Colour (0xfffff0c8), baixo = juce::Colour (0xffffa848);
    g.setGradientFill (juce::ColourGradient (apagado.interpolatedWith (alto, l), 0.0f, panel.getY(), apagado.interpolatedWith (baixo, l), 0.0f, panel.getBottom(), false));
    g.fillRoundedRectangle (panel, 6.0f);
    // a fotorresistencia (a serpentina de sulfuro de cadmio)
    auto f = juce::Rectangle<float> (panel.getX() + 6.0f, panel.getBottom() + 14.0f, panel.getWidth() - 12.0f, 30.0f);
    g.setColour (juce::Colour (0xff7a3c28)); g.fillRoundedRectangle (f, 5.0f);
    juce::Path s;
    const float x0 = f.getX() + 5.0f, x1 = f.getRight() - 5.0f;
    for (int k = 0; k < 8; ++k)
    {
        const float y = f.getY() + 4.0f + (float) k * 3.1f;
        if (k == 0) s.startNewSubPath (x0, y);
        s.lineTo (k % 2 == 0 ? x1 : x0, y);
        s.lineTo (k % 2 == 0 ? x1 : x0, y + 3.1f);
    }
    g.setColour (juce::Colour (0xffe6c37a).interpolatedWith (AMBAR_CLARO, l)); g.strokePath (s, juce::PathStrokeType (1.1f));
    g.setColour (AMBAR);
    g.setFont (cond (11.0f));
    g.drawText (dbTxt (red), r.withTop (r.getBottom() - 26.0f), juce::Justification::centred, false);
}

// ================================================================ vista principal
MainView::MainView (BOProcessor& p) : proc (p), vu (p), celula (p)
{
    setLookAndFeel (&look);
    auto key = [this] (juce::TextButton& b, const juce::String& tip) { b.setTooltip (tip); b.setWantsKeyboardFocus (false); addAndMakeVisible (b); };
    auto ambar = [] (juce::TextButton& b) { b.getProperties().set ("led", (juce::int64) AMBAR.getARGB()); };

    // cabeceira: o preset, os presets propios, o A/B e desfacer
    presetBtn.getProperties().set ("gold", true);
    presetBtn.onClick = [this] { presetsMenu(); };
    key (presetBtn, U ("Os presets de fábrica: voces, baixo, batería, acústica, piano, a mestura e o limitador."));
    prevBtn.setButtonText (U ("‹")); nextBtn.setButtonText (U ("›"));
    for (auto* b : { &prevBtn, &nextBtn, &presetsBtn, &undoBtn, &redoBtn }) b->getProperties().set ("noLed", true);
    prevBtn.onClick = [this] { proc.estado.novoPaso(); proc.cargarPreset ((proc.getPreset() + opto::NUM_PRESETS - 1) % opto::NUM_PRESETS); refreshAll(); };
    nextBtn.onClick = [this] { proc.estado.novoPaso(); proc.cargarPreset ((proc.getPreset() + 1) % opto::NUM_PRESETS); refreshAll(); };
    key (prevBtn, U ("O preset anterior."));
    key (nextBtn, U ("O preset seguinte."));
    presetsBtn.onClick = [this] { showPresetsMenu(); };
    key (presetsBtn, U ("Os teus presets (gardar, abrir a carpeta, copiar entre A e B) e os avisos das versións novas."));
    for (int i = 0; i < 2; ++i)
    {
        abBtn[i].setButtonText (i == 0 ? "A" : "B");
        ambar (abBtn[i]);
        abBtn[i].onClick = [this, i] { if (i != proc.estado.lado()) proc.estado.cambiarLado(); refreshAll(); };
        key (abBtn[i], U ("A/B: dous axustes para comparar. Cambiar de lado garda o actual."));
    }
    undoBtn.getProperties().set ("curve", -1); redoBtn.getProperties().set ("curve", 1);
    undoBtn.setTitle (U ("Desfacer")); redoBtn.setTitle (U ("Refacer"));
    undoBtn.onClick = [this] { proc.undo.undo(); refreshAll(); };
    redoBtn.onClick = [this] { proc.undo.redo(); refreshAll(); };
    key (undoBtn, U ("Desfacer o último cambio. Cada clic é un paso."));
    key (redoBtn, U ("Refacer."));

    // os dous mandos grandes
    auto mando = [this] (MandoNegro& m, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& att, const char* id, const juce::String& tip)
    {
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, m);
        m.setTooltip (tip);
        m.onValueChange = [this] { repaint(); };
        addAndMakeVisible (m);
    };
    mando (reducion, reducionAtt, "reducion", U ("Redución: canto escoita o detector. Máis, máis comprime (a 0, nada)."));
    mando (ganancia, gananciaAtt, "ganancia", U ("Ganancia: o volume despois da célula, pola etapa de válvulas. Cun pouco de máis, as válvulas engaden harmónicos."));

    // comprimir / limitar
    {
        auto* prm = proc.apvts.getParameter ("modo");
        modoAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v)
        {
            for (int i = 0; i < 2; ++i) modoBtn[i].setToggleState (i == (int) std::lround (v), juce::dontSendNotification);
        });
        const juce::String nomes[2] = { "Comprimir", "Limitar" };
        const juce::String axudas[2] = { U ("Comprimir: suave, uns 3:1 nos picos. Para voces, baixos e mesturas."),
                                         U ("Limitar: o detector escoita máis e a curva é máis dura (uns 7:1). Para os picos.") };
        for (int i = 0; i < 2; ++i)
        {
            modoBtn[i].setButtonText (nomes[i]); ambar (modoBtn[i]);
            modoBtn[i].onClick = [this, i] { modoAtt->setValueAsCompleteGesture ((float) i); };
            key (modoBtn[i], axudas[i]);
        }
        modoAtt->sendInitialUpdate();
    }
    // o que amosa o VU
    {
        const juce::String nomes[2] = { U ("Redución"), U ("Saída") };
        for (int i = 0; i < 2; ++i)
        {
            vuBtn[i].setButtonText (nomes[i]); ambar (vuBtn[i]);
            vuBtn[i].onClick = [this, i] { proc.vu = i; for (int k = 0; k < 2; ++k) vuBtn[k].setToggleState (k == i, juce::dontSendNotification); vu.repaint(); };
            vuBtn[i].setToggleState (i == proc.vu, juce::dontSendNotification);
            key (vuBtn[i], i == 0 ? U ("O VU amosa canto comprime: en 0, nada; cara á esquerda, os dB que baixa.")
                                  : U ("O VU amosa o volume de saída (0 VU = −18 dBFS)."));
        }
    }
    // «Máis»: os agudos e o corte do detector, a mestura e a célula
    maisBtn.setButtonText (U ("Máis")); ambar (maisBtn);
    maisBtn.onClick = [this] { mostrarMais (! mais); };
    key (maisBtn, U ("O detector (agudos e corte), a mestura en paralelo e a célula."));
    auto knob = [this] (Knob& kn, const char* pid, const juce::String& name, const juce::String& tip, std::function<juce::String (double)> fmt)
    {
        kn.name = name;
        kn.slider.setTooltip (tip);
        kn.slider.getProperties().set ("led", (juce::int64) AMBAR.getARGB());
        kn.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, pid, kn.slider);
        kn.slider.textFromValueFunction = std::move (fmt);
        addChildComponent (kn);
    };
    auto pc = [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; };
    knob (agudos, "agudos", "Agudos no detector", U ("O detector escoita máis os agudos: baixan antes os «s» e os pratos (coma o axuste de atrás dos de antes)."), pc);
    knob (corte, "corteDetector", "Corte do detector", U ("O detector non escoita por debaixo: o baixo e o bombo xa non fan bombear o resto."), [] (double v) { return juce::String (juce::roundToInt (v)) + " Hz"; });
    knob (mestura, "mestura", "Mestura", U ("Compresión en paralelo: canto do comprimido fronte ao seco (os dous coa mesma ganancia)."), pc);
    {
        auto* prm = proc.apvts.getParameter ("celula");
        celulaAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v)
        {
            for (int i = 0; i < 3; ++i) celulaBtn[i].setToggleState (i == (int) std::lround (v), juce::dontSendNotification);
        });
        const juce::String nomes[3] = { U ("Rápida"), U ("Clásica"), "Lenta" };
        const juce::String axudas[3] = { U ("Unha célula rápida: colle e solta antes. Para batería."),
                                         U ("A célula clásica: colle en ~10 ms, solta a metade en ~100 ms e o resto amodo, máis amodo canto máis tempo comprimiu."),
                                         U ("Unha célula vella e lenta: moi suave, para baixos e mesturas.") };
        for (int i = 0; i < 3; ++i)
        {
            celulaBtn[i].setButtonText (nomes[i]); ambar (celulaBtn[i]);
            celulaBtn[i].onClick = [this, i] { celulaAtt->setValueAsCompleteGesture ((float) i); };
            celulaBtn[i].setTooltip (axudas[i]); celulaBtn[i].setWantsKeyboardFocus (false);
            addChildComponent (celulaBtn[i]);
        }
        celulaAtt->sendInitialUpdate();
    }
    // a luz de acendido é o bypass
    {
        auto* prm = proc.apvts.getParameter ("bypass");
        bypassAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v) { acendido = v < 0.5f; repaint (luzArea); });
        bypassAtt->sendInitialUpdate();
    }

    addAndMakeVisible (vu);
    addAndMakeVisible (celula);
    vu.setTooltip (U ("O VU: a redución ou a saída, escóllese debaixo."));
    newVersion.getProperties().set ("noLed", true);
    newVersion.onClick = [] { juce::URL (escoita::Actualizacions::downloadsUrl()).launchInDefaultBrowser(); };
    newVersion.setWantsKeyboardFocus (false);
    addChildComponent (newVersion);

    juce::Desktop::getInstance().addGlobalMouseListener (this);
    setSize (W, H);
    mostrarMais (false);
    refreshAll();
    startTimerHz (60);
}

MainView::~MainView()
{
    juce::Desktop::getInstance().removeGlobalMouseListener (this);
    stopTimer();
    setLookAndFeel (nullptr);
}

void MainView::mouseDown (const juce::MouseEvent& e)
{
    proc.estado.novoPaso();
    // un clic na luz: bypass
    if (e.eventComponent == this && luzArea.contains (e.getPosition()))
        bypassAtt->setValueAsCompleteGesture (acendido ? 1.0f : 0.0f);
}

void MainView::mostrarMais (bool s)
{
    mais = s;
    maisBtn.setToggleState (s, juce::dontSendNotification);
    vu.setVisible (! s); celula.setVisible (! s);
    for (auto& b : vuBtn) b.setVisible (! s);
    for (auto* k : { &agudos, &corte, &mestura }) k->setVisible (s);
    for (auto& b : celulaBtn) b.setVisible (s);
    repaint();
}

void MainView::presetsMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Presets"));
    for (int i = 0; i < opto::NUM_PRESETS; ++i)
        m.addItem (100 + i, U (opto::PRESETS[i].nome), true, i == proc.getPreset() && proc.nomePreset() == U (opto::PRESETS[i].nome));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [this] (int r)
    {
        if (r < 100) return;
        proc.estado.novoPaso();
        proc.cargarPreset (r - 100);
        refreshAll();
    });
}

void MainView::showPresetsMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Os teus presets"));
    proc.estado.engadirAoMenu (m, [sp = juce::Component::SafePointer<MainView> (this)] { if (sp != nullptr) sp->refreshAll(); });
    m.addSeparator();
    m.addSectionHeader (U ("Función"));
    m.addItem (U ("Avisar das versións novas"), true, versions->enabled(), [this] { versions->setEnabled (! versions->enabled()); });
    m.addItem (U ("escoita.link"), [] { juce::URL ("https://www.escoita.link").launchInDefaultBrowser(); });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetsBtn));
}

void MainView::refreshState()
{
    for (int i = 0; i < 2; ++i)
        if (abBtn[i].getToggleState() != (i == proc.estado.lado())) abBtn[i].setToggleState (i == proc.estado.lado(), juce::dontSendNotification);
    if (undoBtn.isEnabled() != proc.undo.canUndo()) undoBtn.setEnabled (proc.undo.canUndo());
    if (redoBtn.isEnabled() != proc.undo.canRedo()) redoBtn.setEnabled (proc.undo.canRedo());
    const auto t = proc.nomePreset() + (proc.presetEditado() ? U (" *") : juce::String());
    if (presetBtn.getButtonText() != t) presetBtn.setButtonText (t);
}

void MainView::refreshAll() { refreshState(); repaint(); }

void MainView::timerCallback()
{
    refreshState();
    if (! mais) { vu.tick (1.0 / 60.0); celula.tick(); }
    const auto nv = versions->newVersion();
    if (nv.isNotEmpty() != newVersion.isVisible())
    {
        newVersion.setButtonText (U ("Hai unha versión nova: ") + nv + U (" · descargar"));
        newVersion.setVisible (nv.isNotEmpty());
        repaint();
    }
}

void MainView::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff161e33), 0.0f, 0.0f, juce::Colour (0xff0b101e), 0.0f, (float) H, false));
    g.fillAll();
    // o aparello
    const auto panel = juce::Rectangle<float> (20.0f, 16.0f, (float) W - 40.0f, 380.0f);
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (panel.translated (0.0f, 2.0f * (float) i).expanded ((float) i), 18.0f + (float) i);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1f2947), 0.0f, panel.getY(), juce::Colour (0xff0f152a), 0.0f, panel.getBottom(), false));
    g.fillRoundedRectangle (panel, 18.0f);
    g.setColour (juce::Colour (0xff323e64)); g.drawRoundedRectangle (panel, 18.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (panel.getX() + 20.0f, panel.getY() + 0.5f, panel.getWidth() - 40.0f, 1.0f);
    for (auto p : { panel.getTopLeft().translated (16, 16), panel.getTopRight().translated (-16, 16), panel.getBottomLeft().translated (16, -16), panel.getBottomRight().translated (-16, -16) })
    {
        g.setColour (juce::Colour (0xff0a0e1c)); g.fillEllipse (p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f);
        g.setColour (juce::Colour (0xff3c4870)); g.drawLine (p.x - 2.4f, p.y - 2.4f, p.x + 2.4f, p.y + 2.4f, 0.9f);
    }
    // o nome
    g.setColour (col::crema3);
    drawCaps (g, "BARULLO", { 52.0f, 40.0f, 140.0f, 12.0f }, juce::Justification::centredLeft, 8.0f, 0.42f);
    g.setColour (col::crema); g.setFont (cond (24.0f));
    g.drawText ("OPTO COMP", juce::Rectangle<float> (50.0f, 52.0f, 300.0f, 30.0f), juce::Justification::centredLeft, false);
    g.setColour (col::crema3);
    drawCaps (g, U ("Compresor óptico · célula, válvulas e VU"), { 52.0f, 84.0f, 340.0f, 12.0f }, juce::Justification::centredLeft, 7.0f, 0.3f);
    // a luz (o bypass)
    {
        const auto c = luzArea.toFloat().getCentre();
        if (acendido)
        {
            g.setColour (AMBAR.withAlpha (0.18f)); g.fillEllipse (c.x - 10.0f, c.y - 10.0f, 20.0f, 20.0f);
            g.setColour (AMBAR.withAlpha (0.4f)); g.fillEllipse (c.x - 6.5f, c.y - 6.5f, 13.0f, 13.0f);
            g.setColour (AMBAR_CLARO); g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9.0f, 9.0f);
        }
        else { g.setColour (juce::Colour (0xff2a2f44)); g.fillEllipse (c.x - 4.5f, c.y - 4.5f, 9.0f, 9.0f); }
    }
    auto well = [&] (juce::Component& a, juce::Component& b) { paintWell (g, a.getBounds().getUnion (b.getBounds()).expanded (4).toFloat(), 10.0f); };
    well (prevBtn, nextBtn);
    well (presetsBtn, redoBtn);
    if (! mais)
    {
        g.setColour (col::crema3);
        drawCaps (g, "O VU AMOSA", vuBtn[0].getBounds().getUnion (vuBtn[1].getBounds()).toFloat().translated (0.0f, -18.0f).withHeight (12.0f), juce::Justification::centred, 6.5f, 0.3f);
    }
    else
    {
        paintWell (g, juce::Rectangle<float> (54.0f, 116.0f, 452.0f, 206.0f), 14.0f);
        groupTitle (g, juce::Rectangle<float> (70.0f, 126.0f, 420.0f, 20.0f), U ("O detector e a mestura"), AMBAR);
        g.setColour (col::crema3);
        drawCaps (g, U ("A célula"), celulaBtn[0].getBounds().getUnion (celulaBtn[2].getBounds()).toFloat().translated (0.0f, -18.0f).withHeight (12.0f), juce::Justification::centred, 6.5f, 0.3f);
    }
    // os rótulos e os valores dos mandos
    auto valor = [&] (MandoNegro& m, const juce::String& rotulo, const juce::String& v)
    {
        const auto b = m.getBounds().toFloat();
        g.setColour (col::crema2);
        drawCaps (g, rotulo, { b.getX(), b.getBottom() + 4.0f, b.getWidth(), 12.0f }, juce::Justification::centred, 8.0f, 0.3f);
        const auto p = juce::Rectangle<float> (b.getCentreX() - 38.0f, b.getBottom() + 20.0f, 76.0f, 20.0f);
        paintWell (g, p, 6.0f);
        g.setFont (cond (10.5f));
        glowText (g, v, p, juce::Justification::centred, AMBAR);
    };
    valor (reducion, U ("Redución"), juce::String (juce::roundToInt (reducion.getValue())));
    valor (ganancia, "Ganancia", dbTxt (opto::gananciaDb (ganancia.getValue() / 100.0)));

    // o pé
    const auto r = juce::Rectangle<float> (24.0f, (float) H - 30.0f, (float) W - 48.0f, 14.0f);
    g.setColour (col::crema3);
    const float we = juce::GlyphArrangement::getStringWidth (caps (8.0f, 0.3f), "ESCOITA");
    drawCaps (g, "ESCOITA", r, juce::Justification::centredLeft, 8.0f, 0.3f);
    g.setColour (col::laranxa);
    drawCaps (g, ".", r.withTrimmedLeft (we - 2.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
    g.setColour (col::crema3);
    drawCaps (g, U (" Laboratorio creativo · v") + escoita::Actualizacions::currentVersion(), r.withTrimmedLeft (we + 3.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
    drawCaps (g, U ("42°14′N 8°43′W"), r, juce::Justification::centredRight, 8.0f, 0.3f);
}

void MainView::resized()
{
    // cabeceira, á dereita
    {
        int x = 412;
        const int y = 46, h = 30;
        presetBtn.setBounds (x, y, 150, h); x += 160;
        prevBtn.setBounds (x, y, 28, h); x += 32;
        nextBtn.setBounds (x, y, 28, h); x += 42;
        presetsBtn.setBounds (x, y, 66, h); x += 70;
        abBtn[0].setBounds (x, y, 30, h); x += 34;
        abBtn[1].setBounds (x, y, 30, h); x += 40;
        undoBtn.setBounds (x, y, 28, h); x += 32;
        redoBtn.setBounds (x, y, 28, h);
        luzArea = { 222, 55, 24, 24 };   // ao carón do nome
    }
    vu.setBounds (54, 124, 300, 190);
    vuBtn[0].setBounds (98, 352, 104, 26);
    vuBtn[1].setBounds (210, 352, 104, 26);
    celula.setBounds (392, 124, 110, 190);
    reducion.setBounds (620 - 86, 210 - 86, 172, 172);
    ganancia.setBounds (792 - 86, 210 - 86, 172, 172);
    modoBtn[0].setBounds (530, 352, 104, 26);
    modoBtn[1].setBounds (642, 352, 104, 26);
    maisBtn.setBounds (776, 352, 76, 26);
    // «Máis»
    agudos.setBounds (80, 156, 110, 130);
    corte.setBounds (200, 156, 110, 130);
    mestura.setBounds (320, 156, 110, 130);
    for (int i = 0; i < 3; ++i) celulaBtn[i].setBounds (98 + i * 100, 352, 94, 26);
    newVersion.setBounds (W / 2 - 170, H - 34, 340, 22);
}
} // namespace ui

// ================================================================ xanela (escalable)
BOEditor::BOEditor (BOProcessor& p) : AudioProcessorEditor (&p), proc (p), main (p)
{
    addAndMakeVisible (main);
    setResizable (true, true);
    setResizeLimits (ui::MainView::W * 6 / 10, ui::MainView::H * 6 / 10, ui::MainView::W * 2, ui::MainView::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::MainView::W / ui::MainView::H);
    float s = proc.uiScale;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto ua = d->userArea;
        const float fit = juce::jmin ((float) (ua.getWidth() - 40) / (float) ui::MainView::W, (float) (ua.getHeight() - 90) / (float) ui::MainView::H);
        if (s > fit && fit > 0.4f) s = fit;
    }
    setSize (juce::roundToInt (ui::MainView::W * s), juce::roundToInt (ui::MainView::H * s));
}

void BOEditor::resized()
{
    const float s = (float) getWidth() / (float) ui::MainView::W;
    main.setBounds (0, 0, ui::MainView::W, ui::MainView::H);
    main.setTransform (juce::AffineTransform::scale (s));
    proc.uiScale = s;
}
