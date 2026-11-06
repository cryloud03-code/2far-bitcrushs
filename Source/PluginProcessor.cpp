#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout TwoFarProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // Los textos se definen aquí para que la perilla y el host muestren lo mismo.
    auto parseFloat = [] (const String& t) { return t.getFloatValue(); };

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "bits", 1 }, "Bits",
        NormalisableRange<float> (1.0f, 16.0f, 0.01f), 8.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " bit"; })
            .withValueFromStringFunction (parseFloat)));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "rate", 1 }, "Rate",
        NormalisableRange<float> (1.0f, 64.0f, 0.01f, 0.4f), 4.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " div"; })
            .withValueFromStringFunction (parseFloat)));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; })
            .withValueFromStringFunction ([] (const String& t) { return jlimit (0.0f, 1.0f, t.getFloatValue() / 100.0f); })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })
            .withValueFromStringFunction (parseFloat)));

    return { params.begin(), params.end() };
}

TwoFarProcessor::TwoFarProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    bitsParam   = apvts.getRawParameterValue ("bits");
    rateParam   = apvts.getRawParameterValue ("rate");
    mixParam    = apvts.getRawParameterValue ("mix");
    outputParam = apvts.getRawParameterValue ("output");
}

void TwoFarProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& c : crushers) c.reset();
    mixSmooth.reset (sampleRate, 0.02);
    gainSmooth.reset (sampleRate, 0.02);
    mixSmooth.setCurrentAndTargetValue (mixParam->load());
    gainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
}

bool TwoFarProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void TwoFarProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int n      = buffer.getNumSamples();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, n);

    const float bits = bitsParam->load();
    const float rate = rateParam->load();
    mixSmooth.setTargetValue (mixParam->load());
    gainSmooth.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float mix = mixSmooth.getNextValue();
        const float gain = gainSmooth.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            const float dry = data[i];
            const float wet = crushers[ch].process (dry, bits, rate);
            const float out = (dry * (1.0f - mix) + wet * mix) * gain;
            data[i] = out;
            peak = juce::jmax (peak, std::abs (out));
        }
    }

    outputPeak.store (peak);
}

juce::AudioProcessorEditor* TwoFarProcessor::createEditor()
{
    return new TwoFarEditor (*this);
}

void TwoFarProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TwoFarProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TwoFarProcessor();
}
