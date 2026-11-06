#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    // Paleta bandera de Panamá
    juce::Colour panamaRed()   { return juce::Colour (0xffd21034); }
    juce::Colour panamaBlue()  { return juce::Colour (0xff1766e0); }
    juce::Colour panamaWhite() { return juce::Colours::white; }

    juce::Colour rainbow (float h, float alpha = 1.0f, float sat = 0.85f, float val = 1.0f)
    {
        h = h - std::floor (h);
        return juce::Colour::fromHSV (h, sat, val, alpha);
    }

    void addGlow (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col, float alpha)
    {
        juce::ColourGradient grad (col.withAlpha (juce::jlimit (0.0f, 1.0f, alpha)), c.x, c.y,
                                   col.withAlpha (0.0f), c.x + r, c.y, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    }

    juce::Font boldFont (float size) { return juce::Font (juce::FontOptions (size, juce::Font::bold)); }
    juce::Font plainFont (float size) { return juce::Font (juce::FontOptions (size, juce::Font::plain)); }
}

//==============================================================================
PanamaLookAndFeel::PanamaLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x66000000));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, panamaBlue());
}

void PanamaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float pos, float startA, float endA, juce::Slider&)
{
    using namespace juce;

    auto bounds = Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startA + pos * (endA - startA);
    const float arcR = radius - 4.0f;

    // pista
    Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startA, endA, true);
    g.setColour (Colour (0x44ffffff));
    g.strokePath (track, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // arco de valor: rojo -> blanco -> azul
    Path val;
    val.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startA, jmax (startA + 0.001f, angle), true);
    ColourGradient grad (panamaRed(), centre.x - arcR, centre.y, panamaBlue(), centre.x + arcR, centre.y, false);
    grad.addColour (0.5, Colours::white);
    g.setGradientFill (grad);
    g.strokePath (val, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // cuerpo de la perilla
    const float bodyR = arcR - 10.0f;
    ColourGradient body (Colour (0xfffdfdff), centre.x - bodyR * 0.4f, centre.y - bodyR * 0.6f,
                         Colour (0xff8f9ac0), centre.x + bodyR * 0.6f, centre.y + bodyR * 0.8f, true);
    g.setGradientFill (body);
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (Colour (0xff0b1030));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 2.0f);

    // estrella azul que gira con la perilla
    Path star;
    star.addStar (centre, 5, bodyR * 0.24f, bodyR * 0.58f, angle);
    g.setColour (panamaBlue());
    g.fillPath (star);

    // aguja roja
    Path pointer;
    pointer.addRoundedRectangle (-2.5f, -bodyR + 3.0f, 5.0f, bodyR * 0.36f, 2.0f);
    g.setColour (panamaRed());
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

//==============================================================================
TwoFarEditor::TwoFarEditor (TwoFarProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    busImg   = juce::ImageCache::getFromMemory (BinaryData::bus_jpg,   BinaryData::bus_jpgSize);
    animeImg = juce::ImageCache::getFromMemory (BinaryData::anime_jpg, BinaryData::anime_jpgSize);
    flagImg  = juce::ImageCache::getFromMemory (BinaryData::flag_jpg,  BinaryData::flag_jpgSize);

    for (auto* s : { &bitsSlider, &rateSlider, &mixSlider, &outSlider })
        setupSlider (*s);

    bitsAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "bits",   bitsSlider);
    rateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "rate",   rateSlider);
    mixAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "mix",    mixSlider);
    outAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "output", outSlider);

    for (auto& c : confetti)
    {
        c.x = rng.nextFloat() * 1000.0f;
        c.y = rng.nextFloat() * 680.0f;
        c.vx = (rng.nextFloat() - 0.5f) * 0.8f;
        c.vy = 0.4f + rng.nextFloat() * 1.2f;
        c.rot = rng.nextFloat() * 6.28f;
        c.vr = (rng.nextFloat() - 0.5f) * 0.15f;
        c.size = 4.0f + rng.nextFloat() * 8.0f;
        c.hue = rng.nextFloat();
        c.kind = rng.nextInt (3);
    }

    setOpaque (true);
    setResizable (false, false);
    setSize (1000, 680);
    startTimerHz (30);
}

TwoFarEditor::~TwoFarEditor()
{
    stopTimer();
    for (auto* s : { &bitsSlider, &rateSlider, &mixSlider, &outSlider })
        s->setLookAndFeel (nullptr);
}

void TwoFarEditor::setupSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 110, 22);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                           juce::MathConstants<float>::pi * 2.8f, true);
    s.setLookAndFeel (&laf);
    addAndMakeVisible (s);
}

void TwoFarEditor::resized()
{
    auto r = getLocalBounds();
    headerArea = r.removeFromTop (90);
    tickerArea = r.removeFromBottom (28);
    knobsArea  = r.removeFromBottom (170);

    auto main = r.reduced (14, 8);
    auto left = main.removeFromLeft (270);
    animeArea = left.removeFromTop (202);
    left.removeFromTop (10);
    flagArea  = left.removeFromLeft (158);
    left.removeFromLeft (10);
    meterArea = left;

    // El bus conserva su proporción y se centra en el espacio restante.
    busArea = juce::RectanglePlacement (juce::RectanglePlacement::centred)
                  .appliedTo (juce::Rectangle<float> (0.0f, 0.0f, 550.0f, 409.0f),
                              main.reduced (6, 0).toFloat());

    auto k = knobsArea.reduced (14, 8);
    const int colW = k.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        knobCols[(size_t) i] = (i == 3) ? k : k.removeFromLeft (colW);

    bitsSlider.setBounds (knobCols[0].withTrimmedTop (28).reduced (24, 0));
    rateSlider.setBounds (knobCols[1].withTrimmedTop (28).reduced (24, 0));
    mixSlider .setBounds (knobCols[2].withTrimmedTop (28).reduced (24, 0));
    outSlider .setBounds (knobCols[3].withTrimmedTop (28).reduced (24, 0));
}

//==============================================================================
void TwoFarEditor::timerCallback()
{
    const float dt = 1.0f / 30.0f;
    phase += dt;
    tickerPos += 1.6f;

    const float target = twofar::crushAmount ((float) bitsSlider.getValue(), (float) rateSlider.getValue());
    crush += (target - crush) * 0.2f;

    const float peak = std::sqrt (juce::jlimit (0.0f, 1.0f, proc.outputPeak.load()));
    level = juce::jmax (peak, level * 0.88f);

    const float speed = 0.5f + 2.5f * crush;
    for (auto& c : confetti)
    {
        c.y += c.vy * speed;
        c.x += c.vx + std::sin (phase * 2.0f + c.hue * 10.0f) * 0.4f * crush;
        c.rot += c.vr * (1.0f + 2.0f * crush);
        if (c.y > 690.0f) { c.y = -10.0f; c.x = rng.nextFloat() * 1000.0f; }
        if (c.x < -10.0f) c.x = 1010.0f;
        if (c.x > 1010.0f) c.x = -10.0f;
    }

    repaint();
}

juce::Image TwoFarEditor::makePixelated (const juce::Image& src, juce::Rectangle<float> target, int block)
{
    if (! src.isValid()) return src;

    // recorte centrado con la proporción del destino (para no deformar la imagen)
    const float srcAspect = (float) src.getWidth() / (float) src.getHeight();
    const float dstAspect = target.getWidth() / target.getHeight();
    auto crop = src.getBounds();
    if (srcAspect > dstAspect)
        crop = crop.withSizeKeepingCentre ((int) ((float) src.getHeight() * dstAspect), src.getHeight());
    else
        crop = crop.withSizeKeepingCentre (src.getWidth(), (int) ((float) src.getWidth() / dstAspect));

    auto cropped = src.getClippedImage (crop);
    if (block <= 1) return cropped;

    const int w = juce::jmax (8, (int) (target.getWidth()  / (float) block));
    const int h = juce::jmax (8, (int) (target.getHeight() / (float) block));
    return cropped.rescaled (w, h, juce::Graphics::mediumResamplingQuality);
}

//==============================================================================
void TwoFarEditor::paint (juce::Graphics& g)
{
    drawBackground (g);
    drawHeader (g);
    drawAnime (g);
    drawFlag (g);
    drawMeter (g);
    drawBus (g);
    drawKnobPanel (g);
    drawTicker (g);
}

void TwoFarEditor::drawBackground (juce::Graphics& g)
{
    using namespace juce;

    ColourGradient bg (Colour (0xff070b24), 0.0f, 0.0f,
                       Colour (0xff2b0a3d), (float) getWidth(), (float) getHeight(), false);
    bg.addColour (0.55, Colour (0xff0d1a4a));
    g.setGradientFill (bg);
    g.fillAll();

    // reflectores que giran detrás del bus
    if (busArea.getWidth() > 0.0f)
    {
        const Point<float> origin (busArea.getCentreX(), busArea.getBottom() - 10.0f);
        for (int i = 0; i < 5; ++i)
        {
            const float a = -MathConstants<float>::halfPi + std::sin (phase * (0.6f + 0.15f * (float) i) + (float) i * 1.7f) * 1.1f;
            const float len = 520.0f;
            const float spread = 0.09f + 0.05f * crush;

            Path beam;
            beam.startNewSubPath (origin);
            beam.lineTo (origin.x + std::cos (a - spread) * len, origin.y + std::sin (a - spread) * len);
            beam.lineTo (origin.x + std::cos (a + spread) * len, origin.y + std::sin (a + spread) * len);
            beam.closeSubPath();

            auto col = rainbow (phase * 0.2f + (float) i * 0.2f);
            ColourGradient grad (col.withAlpha (0.04f + 0.22f * crush), origin.x, origin.y,
                                 col.withAlpha (0.0f), origin.x + std::cos (a) * len, origin.y + std::sin (a) * len, false);
            g.setGradientFill (grad);
            g.fillPath (beam);
        }
    }

    // confeti: estrellas, círculos y papelitos
    const int visible = 20 + (int) (crush * 50.0f);
    const Colour palette[3] = { panamaRed(), panamaWhite(), panamaBlue() };
    for (int i = 0; i < visible && i < (int) confetti.size(); ++i)
    {
        const auto& c = confetti[(size_t) i];
        auto col = palette[i % 3].interpolatedWith (rainbow (c.hue + phase * 0.2f), crush * 0.9f);
        g.setColour (col.withAlpha (0.25f + 0.35f * crush));

        if (c.kind == 0)
        {
            Path s;
            s.addStar (Point<float> (0.0f, 0.0f), 5, c.size * 0.4f, c.size * 0.9f, 0.0f);
            g.fillPath (s, AffineTransform::rotation (c.rot).translated (c.x, c.y));
        }
        else if (c.kind == 1)
        {
            g.fillEllipse (c.x - c.size * 0.4f, c.y - c.size * 0.4f, c.size * 0.8f, c.size * 0.8f);
        }
        else
        {
            Path r;
            r.addRectangle (-c.size * 0.5f, -c.size * 0.2f, c.size, c.size * 0.4f);
            g.fillPath (r, AffineTransform::rotation (c.rot).translated (c.x, c.y));
        }
    }
}

void TwoFarEditor::drawHeader (juce::Graphics& g)
{
    using namespace juce;

    auto r = headerArea.toFloat();

    g.setColour (Colour (0x66000000));
    g.fillRect (r);

    // franja tricolor
    auto bar = r.removeFromBottom (6.0f);
    const float third = bar.getWidth() / 3.0f;
    g.setColour (panamaRed());   g.fillRect (bar.getX(),             bar.getY(), third, bar.getHeight());
    g.setColour (panamaWhite()); g.fillRect (bar.getX() + third,     bar.getY(), third, bar.getHeight());
    g.setColour (panamaBlue());  g.fillRect (bar.getX() + third * 2, bar.getY(), third, bar.getHeight());

    // título que baila con el bitcrush
    const String title ("2FAR BITCRUSHER");
    const auto font = boldFont (52.0f);
    g.setFont (font);
    float x = 28.0f;
    const float baseY = 64.0f;
    for (int i = 0; i < title.length(); ++i)
    {
        const String ch = title.substring (i, i + 1);
        const float w = font.getStringWidthFloat (ch);
        const float dy = std::sin (phase * 6.0f + (float) i * 0.55f) * (1.0f + 7.0f * crush);

        g.setColour (panamaRed().withAlpha (0.95f));
        g.drawSingleLineText (ch, roundToInt (x + 3.0f), roundToInt (baseY + dy + 3.0f));
        g.setColour (panamaBlue().withAlpha (0.9f));
        g.drawSingleLineText (ch, roundToInt (x - 2.0f), roundToInt (baseY + dy - 2.0f));
        g.setColour (Colours::white.interpolatedWith (rainbow (phase * 0.6f + (float) i * 0.07f), crush * 0.9f));
        g.drawSingleLineText (ch, roundToInt (x), roundToInt (baseY + dy));
        x += w + 2.0f;
    }

    // etiqueta "2far made this"
    const float pulseK = 1.0f + 0.04f * std::sin (phase * 5.0f) * (0.3f + crush);
    auto pill = Rectangle<float> ((float) getWidth() - 280.0f, 22.0f, 252.0f, 46.0f);
    pill = pill.withSizeKeepingCentre (pill.getWidth() * pulseK, pill.getHeight() * pulseK);

    ColourGradient pg (panamaRed(), pill.getX(), pill.getY(), panamaBlue(), pill.getRight(), pill.getBottom(), false);
    g.setGradientFill (pg);
    g.fillRoundedRectangle (pill, 23.0f);
    g.setColour (Colours::white);
    g.drawRoundedRectangle (pill, 23.0f, 2.5f);

    for (int side = 0; side < 2; ++side)
    {
        Path s;
        const float cx = side == 0 ? pill.getX() + 24.0f : pill.getRight() - 24.0f;
        s.addStar (Point<float> (cx, pill.getCentreY()), 5, 4.5f, 10.0f, phase * (side == 0 ? 1.5f : -1.5f));
        g.setColour (Colours::white);
        g.fillPath (s);
    }

    g.setFont (boldFont (23.0f));
    g.setColour (Colours::white);
    g.drawText ("2far made this", pill, Justification::centred);
}

void TwoFarEditor::drawAnime (juce::Graphics& g)
{
    using namespace juce;
    if (! animeImg.isValid()) return;

    auto r = animeArea.toFloat();

    // El panel se "bitcrushea" visualmente: más bitcrush = píxeles más grandes.
    const int block = 1 + (int) (crush * 12.0f);
    if (block != animeBlock) { animePix = makePixelated (animeImg, r, block); animeBlock = block; }

    g.saveState();
    Path clip;
    clip.addRoundedRectangle (r, 14.0f);
    g.reduceClipRegion (clip);
    g.setImageResamplingQuality (block > 1 ? Graphics::lowResamplingQuality : Graphics::highResamplingQuality);
    g.drawImage (animePix, r, RectanglePlacement::stretchToFit);

    if (crush > 0.05f)
    {
        g.setColour (Colours::black.withAlpha (0.22f * crush));
        for (float yy = r.getY(); yy < r.getBottom(); yy += 3.0f)
            g.fillRect (r.getX(), yy, r.getWidth(), 1.0f);

        g.setColour (rainbow (phase * 0.5f).withAlpha (0.10f + 0.18f * crush * (0.5f + level)));
        g.fillRect (r);
    }
    g.restoreState();

    // marco neón
    g.setColour (rainbow (phase * 0.4f).withAlpha (0.15f + 0.6f * crush));
    g.drawRoundedRectangle (r.expanded (2.0f), 16.0f, 4.0f);
    g.setColour (Colours::white.withAlpha (0.85f));
    g.drawRoundedRectangle (r, 14.0f, 2.0f);
}

void TwoFarEditor::drawFlag (juce::Graphics& g)
{
    using namespace juce;
    if (! flagImg.isValid()) return;

    auto r = flagArea.toFloat();

    const int block = 1 + (int) (crush * 12.0f);
    if (block != flagBlock) { flagPix = makePixelated (flagImg, r, block); flagBlock = block; }

    g.saveState();
    Path clip;
    clip.addRoundedRectangle (r, 14.0f);
    g.reduceClipRegion (clip);
    g.setImageResamplingQuality (block > 1 ? Graphics::lowResamplingQuality : Graphics::highResamplingQuality);
    g.drawImage (flagPix, r, RectanglePlacement::stretchToFit);

    if (crush > 0.05f)
    {
        g.setColour (Colours::black.withAlpha (0.22f * crush));
        for (float yy = r.getY(); yy < r.getBottom(); yy += 3.0f)
            g.fillRect (r.getX(), yy, r.getWidth(), 1.0f);
    }

    // cinta inferior
    auto banner = r.removeFromBottom (26.0f);
    g.setColour (Colour (0xcc000000));
    g.fillRect (banner);
    g.setColour (Colours::white);
    g.setFont (boldFont (14.0f));
    g.drawText (String::fromUTF8 ("VIVA PANAM\xC3\x81"), banner, Justification::centred);
    g.restoreState();

    // marco tricolor
    auto rr = flagArea.toFloat();
    ColourGradient frame (panamaRed(), rr.getX(), rr.getY(), panamaBlue(), rr.getRight(), rr.getBottom(), false);
    frame.addColour (0.5, Colours::white);
    g.setGradientFill (frame);
    g.drawRoundedRectangle (rr, 14.0f, 3.5f);
}

void TwoFarEditor::drawMeter (juce::Graphics& g)
{
    using namespace juce;

    auto r = meterArea.toFloat();
    g.setColour (Colour (0xcc0a0f2c));
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.5f);

    auto top = r.removeFromTop (24.0f);
    auto bottom = r.removeFromBottom (30.0f);

    g.setColour (Colours::white);
    g.setFont (boldFont (14.0f));
    g.drawText ("PARTY", top, Justification::centred);

    g.setFont (boldFont (20.0f));
    g.setColour (Colours::white.interpolatedWith (rainbow (phase), crush));
    g.drawText (String (roundToInt (crush * 100.0f)) + "%", bottom, Justification::centred);

    auto bars = r.reduced (16.0f, 2.0f);
    const int n = 14;
    const float segH = bars.getHeight() / (float) n;
    const int lit = roundToInt ((float) n * jmax (level, crush * 0.5f));

    for (int i = 0; i < n; ++i)
    {
        const float yy = bars.getBottom() - (float) (i + 1) * segH;
        Colour c = i < 5 ? panamaBlue() : (i < 9 ? panamaWhite() : panamaRed());
        c = c.interpolatedWith (rainbow (phase * 0.8f + (float) i * 0.08f), crush * 0.7f);
        g.setColour (i < lit ? c : c.withAlpha (0.14f));
        g.fillRoundedRectangle (bars.getX(), yy + 1.0f, bars.getWidth(), segH - 2.0f, 3.0f);
    }
}

void TwoFarEditor::drawBus (juce::Graphics& g)
{
    using namespace juce;
    if (! busImg.isValid() || busArea.isEmpty()) return;

    const auto area = busArea;
    const float scale = area.getWidth() / (float) busImg.getWidth();
    auto map = [&] (float ox, float oy) { return Point<float> (area.getX() + ox * scale, area.getY() + oy * scale); };

    const float intensity = crush;
    const float pulse = 0.55f + 0.45f * level;      // el audio hace "latir" las luces

    // neón en el piso
    {
        const Point<float> c (area.getCentreX(), area.getBottom() - 4.0f);
        g.saveState();
        g.addTransform (AffineTransform::scale (1.0f, 0.16f, c.x, c.y));
        addGlow (g, c, area.getWidth() * 0.62f, rainbow (phase * 0.5f), 0.15f + 0.6f * intensity * pulse);
        g.restoreState();
    }

    // el bus rebota y se inclina al ritmo cuando hay bitcrush
    const float bounceY = intensity * (1.0f + 3.0f * level) * std::sin (phase * 10.0f) * 1.6f;
    const float tilt = intensity * 0.012f * std::sin (phase * 5.0f) * (0.5f + level);

    g.saveState();
    g.addTransform (AffineTransform::rotation (tilt, area.getCentreX(), area.getBottom()).translated (0.0f, bounceY));

    // foto del bus
    g.saveState();
    {
        Path clip;
        clip.addRoundedRectangle (area, 18.0f);
        g.reduceClipRegion (clip);
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImage (busImg, area, RectanglePlacement::stretchToFit);
    }
    g.restoreState();

    // ventanas (coordenadas en la imagen original de 550x409)
    Path winL, winR, winS;
    { auto a = map (84, 115),  b = map (192, 118), c = map (190, 176), d = map (83, 170);
      winL.addQuadrilateral (a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y); }
    { auto a = map (198, 118), b = map (309, 127), c = map (309, 182), d = map (198, 179);
      winR.addQuadrilateral (a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y); }
    { auto a = map (340, 120), b = map (503, 105), c = map (503, 154), d = map (340, 179);
      winS.addQuadrilateral (a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y); }

    auto party = [&] (const Path& p, float seed, bool dancers)
    {
        g.saveState();
        g.reduceClipRegion (p);
        const auto b = p.getBounds();

        // sin bitcrush las ventanas se ven apagadas
        g.setColour (Colours::black.withAlpha (0.28f * (1.0f - intensity)));
        g.fillRect (b);

        if (intensity > 0.01f)
        {
            // lavado de color que barre la ventana
            ColourGradient wash (rainbow (phase * 0.35f + seed), b.getX(), b.getY(),
                                 rainbow (phase * 0.35f + seed + 0.5f), b.getRight(), b.getBottom(), false);
            wash.addColour (0.5, rainbow (phase * 0.35f + seed + 0.25f));
            wash.multiplyOpacity (0.20f + 0.55f * intensity * pulse);
            g.setGradientFill (wash);
            g.fillRect (b);

            // luces de discoteca que se mueven por dentro
            const int blobs = 3 + (int) (intensity * 9.0f);
            for (int i = 0; i < blobs; ++i)
            {
                const float f = (float) i + seed * 7.0f;
                const float bx = b.getX() + b.getWidth()  * (0.5f + 0.5f * std::sin (phase * (1.3f + 0.37f * f) + f * 2.1f));
                const float by = b.getY() + b.getHeight() * (0.5f + 0.5f * std::sin (phase * (1.7f + 0.23f * f) + f * 1.3f));
                const float rad = b.getHeight() * 0.6f * (0.65f + 0.35f * std::sin (phase * 3.0f + f));
                addGlow (g, Point<float> (bx, by), rad, rainbow (phase * 0.5f + f * 0.13f), 0.9f * intensity * pulse);
            }

            // siluetas bailando dentro del bus
            if (dancers)
            {
                const int n = 2 + (int) (intensity * 7.0f);
                for (int i = 0; i < n; ++i)
                {
                    const float cx = b.getX() + b.getWidth() * (((float) i + 0.5f) / (float) n);
                    const float bob = std::abs (std::sin (phase * (5.0f + 0.7f * (float) i) + (float) i)) * 14.0f * scale * pulse;
                    const float headR = 5.0f * scale;
                    const float headY = b.getBottom() - 22.0f * scale - bob;
                    const auto sil = Colour (0xff07070f).withAlpha (jlimit (0.0f, 0.8f, intensity * 1.6f));

                    g.setColour (sil);
                    g.fillEllipse (cx - headR, headY - headR, headR * 2.0f, headR * 2.0f);
                    g.fillRoundedRectangle (cx - headR * 1.1f, headY + headR, headR * 2.2f, 30.0f * scale, 3.0f);

                    // brazos arriba, alternando
                    const float armUp = std::sin (phase * 7.0f + (float) i * 1.9f) * 7.0f * scale;
                    g.drawLine (cx - headR, headY + headR * 1.6f, cx - headR * 2.6f, headY - headR * 0.8f + armUp, 2.5f * scale);
                    g.drawLine (cx + headR, headY + headR * 1.6f, cx + headR * 2.6f, headY - headR * 0.8f - armUp, 2.5f * scale);
                }
            }

            // flash estroboscópico con los picos de audio
            if (level > 0.65f && ((int) (phase * 14.0f)) % 2 == 0)
            {
                g.setColour (Colours::white.withAlpha (0.20f * intensity));
                g.fillRect (b);
            }
        }
        g.restoreState();
    };

    party (winL, 0.00f, false);
    party (winR, 0.33f, false);
    party (winS, 0.66f, true);

    // domos rosados del techo
    const Point<float> domes[3] = { map (244, 48), map (313, 42), map (380, 33) };
    for (int i = 0; i < 3; ++i)
    {
        const auto col = Colour (0xffff4fd8).interpolatedWith (rainbow (phase * 0.8f + (float) i * 0.3f), intensity * 0.85f);
        const float flick = 0.5f + 0.5f * std::sin (phase * 9.0f + (float) i * 2.0f);
        addGlow (g, domes[i], (26.0f + 34.0f * intensity * pulse) * scale * 1.6f, col, 0.18f + 0.65f * intensity * (0.4f + 0.6f * flick));
    }

    // faros amarillos
    const Point<float> heads[4] = { map (99, 279), map (128, 279), map (158, 280), map (183, 281) };
    for (int i = 0; i < 4; ++i)
    {
        const auto col = Colour (0xffffd21a).interpolatedWith (rainbow (phase * 0.7f + (float) i * 0.2f), intensity * 0.6f);
        const float flick = 0.5f + 0.5f * std::sin (phase * 8.0f + (float) i * 1.5f);
        addGlow (g, heads[i], (14.0f + 22.0f * intensity) * scale * 1.5f, col, 0.18f + 0.7f * intensity * pulse * (0.5f + 0.5f * flick));
    }

    // lamparitas rojas y focos del frente
    const Point<float> lamps[8] = { map (97, 115), map (110, 115), map (279, 121), map (293, 121),
                                    map (97, 92),  map (120, 94),  map (271, 99),  map (295, 100) };
    for (int i = 0; i < 8; ++i)
    {
        const float flick = 0.5f + 0.5f * std::sin (phase * 11.0f + (float) i * 1.3f);
        addGlow (g, lamps[i], (8.0f + 12.0f * intensity) * scale * 1.5f,
                 rainbow (phase * 0.9f + (float) i * 0.125f), intensity * 0.9f * flick);
    }

    g.restoreState();
}

void TwoFarEditor::drawKnobPanel (juce::Graphics& g)
{
    using namespace juce;

    auto panel = knobsArea.toFloat().reduced (10.0f, 4.0f);
    g.setColour (Colour (0xaa050818));
    g.fillRoundedRectangle (panel, 16.0f);

    ColourGradient edge (panamaRed(), panel.getX(), panel.getY(), panamaBlue(), panel.getRight(), panel.getY(), false);
    edge.addColour (0.5, Colours::white);
    g.setGradientFill (edge);
    g.drawRoundedRectangle (panel, 16.0f, 2.5f);

    static const char* names[4] = { "BITS", "RATE", "MIX", "OUTPUT" };
    static const char* subs[4]  = { "profundidad", "sample hold", "seco / mojado", "volumen" };

    for (int i = 0; i < 4; ++i)
    {
        auto col = knobCols[(size_t) i].toFloat();
        g.setColour (i % 2 == 0 ? panamaRed().brighter (0.3f) : panamaBlue().brighter (0.3f));
        g.setFont (boldFont (18.0f));
        g.drawText (names[i], col.removeFromTop (18.0f), Justification::centred);
        g.setColour (Colours::white.withAlpha (0.55f));
        g.setFont (plainFont (11.0f));
        g.drawText (subs[i], col.removeFromTop (12.0f), Justification::centred);

        if (i > 0)
        {
            g.setColour (Colours::white.withAlpha (0.12f));
            const auto b = knobCols[(size_t) i].toFloat();
            g.drawLine (b.getX(), b.getY() + 6.0f, b.getX(), b.getBottom() - 6.0f, 1.0f);
        }
    }
}

void TwoFarEditor::drawTicker (juce::Graphics& g)
{
    using namespace juce;

    auto r = tickerArea.toFloat();
    ColourGradient bg (panamaRed().darker (0.4f), r.getX(), r.getY(), panamaBlue().darker (0.4f), r.getRight(), r.getY(), false);
    bg.addColour (0.5, Colour (0xff1b1030));
    g.setGradientFill (bg);
    g.fillRect (r);

    g.saveState();
    g.reduceClipRegion (tickerArea);

    const auto font = boldFont (16.0f);
    g.setFont (font);
    const String unit ("2FAR MADE THIS  -  2FAR BITCRUSHER  -  DIABLO ROJO EDITION");
    const float textW = font.getStringWidthFloat (unit);
    const float unitW = textW + 70.0f;

    for (float x = -std::fmod (tickerPos, unitW); x < r.getRight(); x += unitW)
    {
        g.setColour (Colours::white);
        g.drawText (unit, Rectangle<float> (x, r.getY(), textW + 4.0f, r.getHeight()), Justification::centredLeft);

        Path s;
        s.addStar (Point<float> (x + textW + 35.0f, r.getCentreY()), 5, 3.5f, 8.0f, phase * 2.0f);
        g.setColour (rainbow (phase * 0.5f + x * 0.002f).interpolatedWith (Colour (0xffffd21a), 0.4f));
        g.fillPath (s);
    }
    g.restoreState();
}
