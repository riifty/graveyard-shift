#include "PluginEditor.h"

GraveyardShiftEditor::GraveyardShiftEditor (GraveyardShiftProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (780, 440);

    // ---- preset picker (with category headings) ----
    juce::String lastCategory;
    for (int i = 0; i < GSPresets::count; ++i)
    {
        const juce::String full (GSPresets::all[i].name);
        const juce::String category = full.upToFirstOccurrenceOf (" - ", false, false);
        if (category != lastCategory)
        {
            if (i > 0) presetBox.addSeparator();
            presetBox.addSectionHeading (category);
            lastCategory = category;
        }
        presetBox.addItem (full, i + 1);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { selectPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    prevButton.onClick = [this] { selectPreset (proc.getCurrentProgram() - 1); };
    nextButton.onClick = [this] { selectPreset (proc.getCurrentProgram() + 1); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    // ---- the nine knobs ----
    static const char* labels[9] = { "DRIVE", "DARK", "CRUSH", "WOBBLE", "ECHO",
                                     "ECHO TIME", "SPACE", "WIDTH", "MIX" };

    for (int i = 0; i < 9; ++i)
    {
        auto& k = knobs[(size_t) i];

        k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
        k.slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)); };
        k.slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue() / 100.0; };
        addAndMakeVisible (k.slider);

        k.label.setText (labels[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, GSPresets::paramIds[i], k.slider);
    }

    startTimerHz (5);
}

GraveyardShiftEditor::~GraveyardShiftEditor()
{
    setLookAndFeel (nullptr);
}

void GraveyardShiftEditor::selectPreset (int index)
{
    index = juce::jlimit (0, GSPresets::count - 1, index);
    proc.setCurrentProgram (index);
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
}

// keeps the preset box in sync if the DAW changes the preset
void GraveyardShiftEditor::timerCallback()
{
    const int want = proc.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != want)
        presetBox.setSelectedId (want, juce::dontSendNotification);
}

void GraveyardShiftEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff17101f), bounds.getCentreX(), 0.0f,
                                             juce::Colour (0xff050407), bounds.getCentreX(), bounds.getBottom(), false));
    g.fillAll();

    // title
    g.setColour (juce::Colour (0xffb3203f));
    g.setFont (40.0f);
    g.drawText ("GRAVEYARD SHIFT", 0, 14, getWidth(), 48, juce::Justification::centred);

    g.setColour (juce::Colour (0xff8a7fa8));
    g.setFont (16.0f);
    g.drawText ("B Y   R I I F T Y", 0, 62, getWidth(), 22, juce::Justification::centred);

    // thin divider lines
    g.setColour (juce::Colour (0xff2a2433));
    g.drawHorizontalLine (96, 30.0f, (float) getWidth() - 30.0f);
    g.drawHorizontalLine (getHeight() - 36, 30.0f, (float) getWidth() - 30.0f);

    g.setColour (juce::Colour (0xff6b5f80));
    g.setFont (13.0f);
    g.drawText ("111 presets  |  pick a sound, then tweak the knobs",
                0, getHeight() - 30, getWidth(), 20, juce::Justification::centred);
}

void GraveyardShiftEditor::resized()
{
    auto area = getLocalBounds();

    // preset row
    auto presetRow = juce::Rectangle<int> (area.getCentreX() - 230, 108, 460, 34);
    prevButton.setBounds (presetRow.removeFromLeft (44));
    nextButton.setBounds (presetRow.removeFromRight (44));
    presetBox.setBounds (presetRow.reduced (6, 0));

    // knob row
    const int margin = 20;
    auto row = juce::Rectangle<int> (margin, 175, getWidth() - 2 * margin, 220);
    const int cell = row.getWidth() / 9;

    for (int i = 0; i < 9; ++i)
    {
        auto c = row.removeFromLeft (cell);
        auto& k = knobs[(size_t) i];
        k.label.setBounds (c.removeFromTop (22));
        k.slider.setBounds (c.reduced (2, 0));
    }
}
