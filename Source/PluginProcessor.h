#pragma once
#include <JuceHeader.h>
#include "Presets.h"

class GraveyardShiftProcessor : public juce::AudioProcessor
{
public:
    GraveyardShiftProcessor();
    ~GraveyardShiftProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GRAVEYARD SHIFT BY RIIFTY"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // Presets show up as "programs" so the DAW can browse them too
    int getNumPrograms() override { return GSPresets::count; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr int numParams = 9;
    std::atomic<float>* p[numParams] {};
    int currentPreset = 0;

    double sr = 44100.0;

    // sound engine pieces
    juce::dsp::StateVariableTPTFilter<float> lowpass;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 192000 };
    juce::Reverb reverb;

    juce::SmoothedValue<float> sDrive, sDark, sEchoTime, sWidth, sMix;
    juce::AudioBuffer<float> dryBuffer;

    float heldValue[2] { 0.f, 0.f };
    int holdCount[2] { 0, 0 };
    float damp[2] { 0.f, 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraveyardShiftProcessor)
};
