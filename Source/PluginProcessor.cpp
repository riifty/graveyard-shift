#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Knobs (all 0..1):  drive, dark, crush, wobble, echo, echotime, space, width, mix
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GraveyardShiftProcessor::createLayout()
{
    static const char* names[numParams] = { "Drive", "Dark", "Crush", "Wobble", "Echo",
                                            "Echo Time", "Space", "Width", "Mix" };
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < numParams; ++i)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { GSPresets::paramIds[i], 1 },
            names[i],
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
            GSPresets::all[0].v[i]));

    return layout;
}

GraveyardShiftProcessor::GraveyardShiftProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < numParams; ++i)
        p[i] = apvts.getRawParameterValue (GSPresets::paramIds[i]);
}

bool GraveyardShiftProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void GraveyardShiftProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    lowpass.prepare (spec);
    lowpass.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowpass.setResonance (0.6f);

    chorus.prepare (spec);
    chorus.setCentreDelay (7.0f);
    chorus.setFeedback (0.15f);

    delay.prepare (spec);
    delay.reset();

    reverb.reset();
    reverb.setSampleRate (sampleRate);

    sDrive.reset (sampleRate, 0.03);    sDrive.setCurrentAndTargetValue (*p[0]);
    sDark.reset (sampleRate, 0.03);     sDark.setCurrentAndTargetValue (*p[1]);
    sEchoTime.reset (sampleRate, 0.25); sEchoTime.setCurrentAndTargetValue (*p[5]);
    sWidth.reset (sampleRate, 0.03);    sWidth.setCurrentAndTargetValue (*p[7]);
    sMix.reset (sampleRate, 0.03);      sMix.setCurrentAndTargetValue (*p[8]);

    dryBuffer.setSize (2, samplesPerBlock);

    for (int c = 0; c < 2; ++c) { heldValue[c] = 0.f; holdCount[c] = 0; damp[c] = 0.f; }
}

//==============================================================================
void GraveyardShiftProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    // keep an untouched copy for the dry/wet mix
    dryBuffer.setSize (2, n, false, false, true);
    dryBuffer.copyFrom (0, 0, buffer, 0, 0, n);
    dryBuffer.copyFrom (1, 0, buffer, 1, 0, n);

    // read the knobs
    const float drive    = p[0]->load();
    const float dark     = p[1]->load();
    const float crush    = p[2]->load();
    const float wobble   = p[3]->load();
    const float echo     = p[4]->load();
    const float echoTime = p[5]->load();
    const float space    = p[6]->load();
    const float width    = p[7]->load();
    const float mix      = p[8]->load();

    sDrive.setTargetValue (drive);
    sDark.setTargetValue (dark);
    sEchoTime.setTargetValue (juce::jmap (echoTime, 0.08f, 0.70f) * (float) sr);
    sWidth.setTargetValue (width);
    sMix.setTargetValue (mix);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    float* ch[2] = { L, R };

    // ---- STAGE 1: CRUSH -> DRIVE -> DARK (low-pass filter) ----
    const bool  doCrush = crush > 0.005f;
    const int   hold    = 1 + (int) (crush * crush * 14.0f);
    const float levels  = std::pow (2.0f, 16.0f - crush * 11.0f);

    for (int i = 0; i < n; ++i)
    {
        const float drv  = sDrive.getNextValue();
        const float dk   = sDark.getNextValue();
        const float gain = 1.0f + drv * 24.0f;
        const float comp = 1.0f - 0.35f * drv;

        // dark: 0 = open (20 kHz), 1 = muffled (400 Hz)
        lowpass.setCutoffFrequency (20000.0f * std::pow (0.02f, dk));

        for (int c = 0; c < 2; ++c)
        {
            float x = ch[c][i];

            if (doCrush)
            {
                if (holdCount[c] <= 0)
                {
                    heldValue[c] = std::round (x * levels) / levels;
                    holdCount[c] = hold;
                }
                --holdCount[c];
                x = heldValue[c];
            }

            x = std::tanh (x * gain) * comp;
            x = lowpass.processSample (c, x);
            ch[c][i] = x;
        }
    }

    // ---- STAGE 2: WOBBLE (chorus) ----
    chorus.setRate (0.15f + wobble * 1.2f);
    chorus.setDepth (0.25f + 0.5f * wobble);
    chorus.setMix (wobble * 0.6f);
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        chorus.process (context);
    }

    // ---- STAGE 3: ECHO (dark, ping-pong-ish delay) ----
    const float feedback = 0.2f + 0.5f * echo;
    const float echoAmt  = echo * 0.9f;

    for (int i = 0; i < n; ++i)
    {
        const float dt = sEchoTime.getNextValue();

        for (int c = 0; c < 2; ++c)
        {
            const float x = ch[c][i];
            const float d = delay.popSample (c, juce::jmin (c == 0 ? dt : dt * 1.12f, 190000.0f), true);

            damp[c] += 0.25f * (d - damp[c]);   // each repeat gets darker
            delay.pushSample (c, x + damp[c] * feedback);

            ch[c][i] = x + d * echoAmt;
        }
    }

    // ---- STAGE 4: SPACE (reverb) ----
    {
        juce::Reverb::Parameters rp;
        rp.roomSize   = 0.55f + 0.40f * space;
        rp.damping    = 0.65f;
        rp.wetLevel   = space * 0.5f;
        rp.dryLevel   = 1.0f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        reverb.processStereo (L, R, n);
    }

    // ---- STAGE 5: WIDTH + MIX ----
    const float* dryL = dryBuffer.getReadPointer (0);
    const float* dryR = dryBuffer.getReadPointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float w = sWidth.getNextValue() * 2.0f;   // 0..2 (1 = untouched)
        const float m = 0.5f * (L[i] + R[i]);
        const float s = 0.5f * (L[i] - R[i]) * w;
        const float wetL = m + s;
        const float wetR = m - s;

        const float mx = sMix.getNextValue();
        L[i] = dryL[i] * (1.0f - mx) + wetL * mx;
        R[i] = dryR[i] * (1.0f - mx) + wetR * mx;
    }
}

//==============================================================================
void GraveyardShiftProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, GSPresets::count - 1, index);
    currentPreset = index;

    for (int i = 0; i < numParams; ++i)
        if (auto* prm = apvts.getParameter (GSPresets::paramIds[i]))
            prm->setValueNotifyingHost (prm->convertTo0to1 (GSPresets::all[index].v[i]));
}

const juce::String GraveyardShiftProcessor::getProgramName (int index)
{
    if (index < 0 || index >= GSPresets::count)
        return {};
    return GSPresets::all[index].name;
}

//==============================================================================
void GraveyardShiftProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GraveyardShiftProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, GSPresets::count - 1, (int) tree.getProperty ("preset", 0));
            apvts.replaceState (tree);
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* GraveyardShiftProcessor::createEditor()
{
    return new GraveyardShiftEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GraveyardShiftProcessor();
}
