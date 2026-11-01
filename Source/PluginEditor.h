#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Dark, spooky colours for all the controls
class GraveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GraveLookAndFeel()
    {
        const auto blood  = juce::Colour (0xffb3203f);
        const auto violet = juce::Colour (0xff6b4fa0);
        const auto ink    = juce::Colour (0xff0d0b12);

        setColour (juce::Slider::rotarySliderFillColourId,    blood);
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2a2433));
        setColour (juce::Slider::thumbColourId,               juce::Colour (0xffe8e0f0));
        setColour (juce::Slider::textBoxTextColourId,         juce::Colour (0xffc9bfd8));
        setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId,                 juce::Colour (0xff9a8fb0));

        setColour (juce::ComboBox::backgroundColourId,        ink.brighter (0.1f));
        setColour (juce::ComboBox::textColourId,              juce::Colour (0xffe8e0f0));
        setColour (juce::ComboBox::outlineColourId,           violet);
        setColour (juce::ComboBox::arrowColourId,             blood);
        setColour (juce::PopupMenu::backgroundColourId,       ink);
        setColour (juce::PopupMenu::textColourId,             juce::Colour (0xffe8e0f0));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, blood);
        setColour (juce::PopupMenu::headerTextColourId,       violet.brighter (0.4f));

        setColour (juce::TextButton::buttonColourId,          ink.brighter (0.1f));
        setColour (juce::TextButton::textColourOffId,         juce::Colour (0xffe8e0f0));
    }
};

class GraveyardShiftEditor : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit GraveyardShiftEditor (GraveyardShiftProcessor&);
    ~GraveyardShiftEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void selectPreset (int index);

    GraveyardShiftProcessor& proc;
    GraveLookAndFeel laf;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::array<Knob, 9> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraveyardShiftEditor)
};
