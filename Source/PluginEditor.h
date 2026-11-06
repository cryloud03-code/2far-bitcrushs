#pragma once
#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"

// Perillas con colores de la bandera de Panamá (rojo / blanco / azul) y una estrella que gira.
class PanamaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PanamaLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

class TwoFarEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit TwoFarEditor (TwoFarProcessor&);
    ~TwoFarEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Confetti { float x, y, vx, vy, rot, vr, size, hue; int kind; };

    void timerCallback() override;

    void drawBackground (juce::Graphics&);
    void drawHeader (juce::Graphics&);
    void drawAnime (juce::Graphics&);
    void drawFlag (juce::Graphics&);
    void drawMeter (juce::Graphics&);
    void drawBus (juce::Graphics&);
    void drawKnobPanel (juce::Graphics&);
    void drawTicker (juce::Graphics&);

    static juce::Image makePixelated (const juce::Image& src, juce::Rectangle<float> target, int block);
    void setupSlider (juce::Slider&);

    TwoFarProcessor& proc;

    // OJO con el orden: el look-and-feel debe vivir más que los sliders,
    // y los attachments deben morir antes que los sliders.
    PanamaLookAndFeel laf;
    juce::Slider bitsSlider, rateSlider, mixSlider, outSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitsAtt, rateAtt, mixAtt, outAtt;

    juce::Image busImg, animeImg, flagImg;
    juce::Image animePix, flagPix;
    int animeBlock = -1, flagBlock = -1;

    std::array<Confetti, 70> confetti;
    juce::Random rng;

    float phase = 0.0f;        // reloj de animación (segundos)
    float crush = 0.0f;        // 0..1, cuánto bitcrush hay (suavizado)
    float level = 0.0f;        // 0..1, nivel de audio con caída suave
    float tickerPos = 0.0f;

    juce::Rectangle<int> headerArea, tickerArea, knobsArea, animeArea, flagArea, meterArea;
    juce::Rectangle<float> busArea;
    std::array<juce::Rectangle<int>, 4> knobCols;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TwoFarEditor)
};
