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

    // night sky -> black ground
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1023), bounds.getCentreX(), 0.0f,
                                             juce::Colour (0xff030204), bounds.getCentreX(), bounds.getBottom(), false));
    g.fillAll();

    drawMoon (g, { getWidth() - 110.0f, 10.0f, 90.0f, 90.0f });
    drawCobweb (g, { 0.0f, 0.0f });
    drawBats (g, { 20.0f, 20.0f, 220.0f, 60.0f });

    drawTitle (g, { 0.0f, 10.0f, (float) getWidth(), 76.0f });
    drawRiiftyMark (g, { (float) getWidth() * 0.5f - 90.0f, 60.0f, 180.0f, 24.0f });

    // thin divider lines
    g.setColour (juce::Colour (0xff2a2433));
    g.drawHorizontalLine (96, 30.0f, (float) getWidth() - 30.0f);

    drawTombstones (g, { 0.0f, (float) getHeight() - 42.0f, (float) getWidth(), 42.0f });
}

void GraveyardShiftEditor::drawTitle (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Font f (juce::Font::getDefaultSansSerifFontName(), 40.0f, juce::Font::bold);
    g.setFont (f);

    auto r = area.withTrimmedBottom (28.0f).toNearestInt();

    // layered shadow pass for a worn, carved-stone look
    g.setColour (juce::Colour (0xff4a1020));
    g.drawText ("GRAVEYARD SHIFT", r.translated (3, 3), juce::Justification::centred);
    g.setColour (juce::Colour (0xff3d2b5c));
    g.drawText ("GRAVEYARD SHIFT", r.translated (-2, -1), juce::Justification::centred);
    g.setColour (juce::Colour (0xffd8102f));
    g.drawText ("GRAVEYARD SHIFT", r, juce::Justification::centred);

    // a couple of blood drips off the letters
    juce::Random rnd (42);
    const float textWidth = f.getStringWidthFloat ("GRAVEYARD SHIFT");
    const float startX = area.getCentreX() - textWidth * 0.5f;
    g.setColour (juce::Colour (0xffb3203f));
    for (int i = 0; i < 5; ++i)
    {
        const float x = startX + rnd.nextFloat() * textWidth;
        const float dripH = 6.0f + rnd.nextFloat() * 16.0f;
        const float y0 = area.getY() + 36.0f;
        juce::Path drip;
        drip.addRoundedRectangle (x, y0, 2.6f, dripH, 1.3f);
        g.fillPath (drip);
    }

}

void GraveyardShiftEditor::drawRiiftyMark (juce::Graphics& g, juce::Rectangle<float> area)
{
    // small tombstone icon + wordmark, used as the Riifty logo
    const float iconSize = area.getHeight();
    const float textWidth = 120.0f;
    const float totalWidth = iconSize + 8.0f + textWidth;
    auto r = area.withSizeKeepingCentre (totalWidth, area.getHeight());

    auto iconArea = r.removeFromLeft (iconSize).reduced (3.0f);
    r.removeFromLeft (8.0f);

    juce::Path stone;
    stone.addRoundedRectangle (iconArea.getX(), iconArea.getY() + iconArea.getHeight() * 0.3f,
                               iconArea.getWidth(), iconArea.getHeight() * 0.7f, 2.5f);
    stone.addArc (iconArea.getX(), iconArea.getY(), iconArea.getWidth(), iconArea.getHeight() * 0.6f,
                 juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi, true);
    stone.closeSubPath();
    g.setColour (juce::Colour (0xff2a2433));
    g.fillPath (stone);
    g.setColour (juce::Colour (0xffb3203f));
    g.strokePath (stone, juce::PathStrokeType (1.2f));

    g.setColour (juce::Colour (0xffc9bfd8));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), iconArea.getHeight() * 0.5f, juce::Font::bold));
    g.drawText ("R", iconArea, juce::Justification::centred);

    g.setColour (juce::Colour (0xff9a8fb0));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 15.0f, juce::Font::plain));
    g.drawText ("B Y   R I I F T Y", r, juce::Justification::centredLeft);
}

void GraveyardShiftEditor::drawMoon (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Path moon;
    moon.addEllipse (area);

    juce::Path bite;
    bite.addEllipse (area.translated (area.getWidth() * 0.28f, -area.getHeight() * 0.12f));
    moon.setUsingNonZeroWinding (true);

    juce::Path crescent (moon);
    crescent.addPath (bite);
    crescent.setUsingNonZeroWinding (false);

    g.setColour (juce::Colour (0x33e8d9ff));
    g.fillEllipse (area.expanded (18.0f));   // soft glow
    g.setColour (juce::Colour (0xffe8e0f0));
    g.fillPath (crescent);
}

void GraveyardShiftEditor::drawCobweb (juce::Graphics& g, juce::Point<float> corner)
{
    g.setColour (juce::Colour (0x40c9bfd8));
    const float R = 70.0f;

    for (int i = 0; i <= 5; ++i)
    {
        const float angle = juce::jmap ((float) i, 0.0f, 5.0f, 0.0f, juce::MathConstants<float>::halfPi);
        g.drawLine (corner.x, corner.y, corner.x + R * std::sin (angle), corner.y + R * std::cos (angle), 1.0f);
    }
    for (int ring = 1; ring <= 3; ++ring)
    {
        juce::Path arc;
        const float r = R * ring / 3.5f;
        arc.addCentredArc (corner.x, corner.y, r, r, 0.0f, 0.0f, juce::MathConstants<float>::halfPi, true);
        g.strokePath (arc, juce::PathStrokeType (1.0f));
    }
}

void GraveyardShiftEditor::drawBats (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Random rnd (7);
    g.setColour (juce::Colour (0xff1a1420));

    for (int i = 0; i < 3; ++i)
    {
        const float x = area.getX() + rnd.nextFloat() * area.getWidth();
        const float y = area.getY() + rnd.nextFloat() * area.getHeight();
        const float s = 7.0f + rnd.nextFloat() * 5.0f;

        juce::Path bat;
        bat.startNewSubPath (x - s, y);
        bat.quadraticTo (x - s * 0.3f, y - s * 0.6f, x, y);
        bat.quadraticTo (x + s * 0.3f, y - s * 0.6f, x + s, y);
        bat.quadraticTo (x + s * 0.3f, y - s * 0.1f, x, y + s * 0.15f);
        bat.quadraticTo (x - s * 0.3f, y - s * 0.1f, x - s, y);
        bat.closeSubPath();
        g.fillPath (bat);
    }
}

void GraveyardShiftEditor::drawTombstones (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (juce::Colour (0xff0a0710));
    g.fillRect (area);

    juce::Random rnd (99);
    const int count = 10;
    const float step = area.getWidth() / count;

    g.setColour (juce::Colour (0xff1c1726));
    for (int i = 0; i < count; ++i)
    {
        const float w = step * 0.42f;
        const float h = 16.0f + rnd.nextFloat() * 20.0f;
        const float x = area.getX() + i * step + step * 0.3f;
        const float y = area.getBottom() - h;

        juce::Path stone;
        if (i % 3 == 0)
        {
            // cross
            stone.addRoundedRectangle (x + w * 0.4f, y, w * 0.2f, h, 2.0f);
            stone.addRoundedRectangle (x + w * 0.15f, y + h * 0.3f, w * 0.7f, w * 0.2f, 2.0f);
        }
        else
        {
            stone.addRoundedRectangle (x, y + h * 0.25f, w, h * 0.75f, 3.0f);
            stone.addArc (x, y, w, h * 0.5f, juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi, true);
            stone.closeSubPath();
        }
        g.fillPath (stone);
    }

    // fog drifting across the graves
    g.setGradientFill (juce::ColourGradient (juce::Colour (0x22c9bfd8), area.getCentreX(), area.getY(),
                                             juce::Colours::transparentBlack, area.getCentreX(), area.getBottom(), false));
    g.fillRect (area);
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
