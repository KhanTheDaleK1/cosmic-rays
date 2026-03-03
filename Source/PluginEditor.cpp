#include "PluginProcessor.h"
#include "PluginEditor.h"

void CustomLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    g.setColour (juce::Colours::black.withAlpha(0.15f));
    g.fillEllipse (centreX - radius + 2, centreY - radius + 3, radius * 2, radius * 2);

    g.setGradientFill (juce::ColourGradient (Colors::appleBeige.brighter(0.15f), centreX, centreY - radius,
                                           Colors::appleBeige.darker(0.15f), centreX, centreY + radius, false));
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2, radius * 2);
    
    g.setColour (juce::Colours::black.withAlpha(0.2f));
    g.drawEllipse (centreX - radius, centreY - radius, radius * 2, radius * 2, 1.0f);

    g.setColour (juce::Colour (0xFF222222));
    juce::Path p;
    p.addRectangle (-1.2f, -radius, 2.4f, radius * 0.45f);
    g.fillPath (p, juce::AffineTransform::rotation (toAngle).translated (centreX, centreY));

    g.setColour (juce::Colour (0xFF222222));
    g.setFont (juce::Font("Courier", 12.0f, juce::Font::bold));
    juce::String text;
    if (slider.getName() == "Shape") {
        float val = (float)slider.getValue();
        if (val < 0.25f) text = "SINE"; else if (val < 0.5f) text = "TRI";
        else if (val < 0.75f) text = "SAW"; else text = "SQU";
    } else if (slider.getName() == "Filter") {
        float val = (float)slider.getValue();
        if (val < 0.45f) text = "LPF"; else if (val > 0.55f) text = "HPF"; else text = "OFF";
    } else {
        text = juce::String (juce::roundToInt (sliderPos * 100));
    }
    g.drawFittedText (text, bounds.toNearestInt(), juce::Justification::centredBottom, 1);
}

CustomLookAndFeel::CustomLookAndFeel() {
    setColour (juce::Slider::rotarySliderFillColourId, Colors::appleBlue);
    setColour (juce::Slider::rotarySliderOutlineColourId, Colors::appleBeige.darker(0.3f));
    setColour (juce::Label::textColourId, juce::Colour (0xFF333333));
    setColour (juce::ComboBox::backgroundColourId, Colors::appleBeige.darker(0.1f));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xFF222222));
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black.withAlpha(0.4f));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xFF444444));
    setColour (juce::PopupMenu::backgroundColourId, Colors::appleBeige.brighter(0.1f));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xFF222222));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colors::appleBlue);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

HelpComponent::HelpComponent() {
    addAndMakeVisible(textEditor);
    textEditor.setMultiLine(true);
    textEditor.setReadOnly(true);
    textEditor.setScrollbarsShown(true);
    textEditor.setCaretVisible(false);
    textEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    textEditor.setColour(juce::TextEditor::textColourId, CustomLookAndFeel::Colors::phosphorGreen);
    textEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    textEditor.setFont(juce::Font("Courier", 14.0f, juce::Font::plain));

    juce::String manual = 
        "=== COSMIC RAYS OPERATING MANUAL ===\n\n"
        "1. MICRO LOOP BANK\n"
        "- MOSAIC: Overlapping loops at different speeds/octaves.\n"
        "- SEQ: Slices audio into rhythmic sequences.\n"
        "- GLIDE: Pitch-sliding portamento loops.\n\n"
        "2. GRANULES BANK\n"
        "- HAZE: Dense ambient cloud cluster.\n"
        "- TUNNEL: Hypnotic cyclical drone loops.\n"
        "- STRUM: Pointillistic cascading onset chains.\n\n"
        "3. GLITCH BANK\n"
        "- BLOCKS: Rhythmic predictable glitches.\n"
        "- INTERRUPT: Aggressive pitch-shifted bursts.\n"
        "- ARP: Sequenced major scale arpeggios.\n\n"
        "4. MULTIDELAY BANK\n"
        "- PATTERN: 4 distinct rhythmic delay patterns.\n"
        "- WARP: Pitch-modulated alien echoes.\n\n"
        "CONTROLS\n"
        "- ACTIVITY: Macro for density and complexity.\n"
        "- REPEATS: persistence / feedback.\n"
        "- SHAPE: Envelope (Sine/Tri/Saw/Square).\n"
        "- FILTER: 12dB/oct resonant LPF.\n"
        "- SPACE: Global hall reverb.\n"
        "- MOD: Vintage 'Wow & Flutter' vibrato.\n\n"
        "LOOPER\n"
        "- 60s capacity, PRE/POST FX routing.\n"
        "- REC/DUB for unlimited layering.\n\n"
        "GLOBAL\n"
        "- KILL DRY: Parallel mix mode.\n"
        "- TRAILS: Natural fade on bypass.\n"
        "- FREEZE: Instant infinite drone.\n"
        "====================================";
    textEditor.setText(manual);
}

void HelpComponent::paint(juce::Graphics& g) {
    g.setColour(juce::Colours::black.withAlpha(0.95f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    g.setColour(CustomLookAndFeel::Colors::phosphorGreen);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 10.0f, 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.03f));
    for (int y = 0; y < getHeight(); y += 3) g.drawHorizontalLine(y, 0, getWidth());
}

void HelpComponent::resized() { textEditor.setBounds(getLocalBounds().reduced(20)); }

CosmicRaysAudioProcessorEditor::CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), filterVis(p), grainShapeVis(p)
{
    addAndMakeVisible(processor.visualiser);
    processor.visualiser.setColours(CustomLookAndFeel::Colors::scopeBg, CustomLookAndFeel::Colors::phosphorGreen.withAlpha(0.7f));
    addAndMakeVisible(filterVis);
    addAndMakeVisible(grainShapeVis);
    addChildComponent(helpOverlay);

    auto setupKnob = [&](juce::Slider& s, juce::Label& l, const juce::String& name) {
        addAndMakeVisible (s); s.setName (name); s.setLookAndFeel (&customLookAndFeel);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (l); l.setText (name.toUpperCase(), juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred); 
        l.setFont (juce::Font ("Courier", 13.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colour(0xFF333333));
    };

    setupKnob (activitySlider, activityLabel, "Activity");
    setupKnob (timeSlider, timeLabel, "Time");
    setupKnob (shapeSlider, shapeLabel, "Shape");
    setupKnob (repeatsSlider, repeatsLabel, "Repeats");
    setupKnob (filterSlider, filterLabel, "Filter");
    setupKnob (spaceSlider, spaceLabel, "Space");
    setupKnob (mixSlider, mixLabel, "Mix");
    setupKnob (loopLevelSlider, loopLevelLabel, "Loop");
    setupKnob (gainSlider, gainLabel, "Gain");
    setupKnob (modRateSlider, modRateLabel, "Rate");
    setupKnob (modDepthSlider, modDepthLabel, "Depth");

    addAndMakeVisible (algoBox); algoBox.addItemList ({"MOSAIC", "SEQ", "GLIDE", "HAZE", "TUNNEL", "STRUM", "BLOCKS", "INTERRUPT", "ARP", "PATTERN", "WARP"}, 1);
    algoBox.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible (looperModeBox); looperModeBox.addItemList ({"PRE", "POST"}, 1);
    looperModeBox.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible (presetBox); presetBox.addItemList ({"P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8", "P9", "P10", "P11", "P12", "P13", "P14", "P15", "P16"}, 1);
    presetBox.setLookAndFeel(&customLookAndFeel);

    addAndMakeVisible (helpButton); helpButton.setButtonText ("?");
    helpButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::Colors::appleBeige.darker(0.2f));
    helpButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF222222));
    helpButton.onClick = [this]() { helpOverlay.setVisible(!helpOverlay.isVisible()); if (helpOverlay.isVisible()) helpOverlay.toFront(false); };

    addAndMakeVisible (shiftButton); shiftButton.setButtonText ("SHIFT");
    shiftButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::Colors::appleBeige.darker(0.1f));
    shiftButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF222222));
    shiftButton.onClick = [this]() {
        auto* param = processor.apvts.getParameter("TEMPO_MODE");
        param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
        updateLabels();
    };

    addAndMakeVisible (tapButton); tapButton.setButtonText ("TAP");
    tapButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::Colors::appleBeige.darker(0.1f));
    tapButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF222222));
    tapButton.onClick = [this]() { handleTap(); };

    addAndMakeVisible (looperRecButton); looperRecButton.setButtonText ("REC");
    looperRecButton.setClickingTogglesState(true);
    looperRecButton.setColour(juce::TextButton::buttonOnColourId, CustomLookAndFeel::Colors::appleRed.withAlpha(0.8f));
    addAndMakeVisible (looperOdubButton); looperOdubButton.setButtonText ("DUB");
    looperOdubButton.setClickingTogglesState(true);
    looperOdubButton.setColour(juce::TextButton::buttonOnColourId, CustomLookAndFeel::Colors::appleOrange.withAlpha(0.8f));

    addAndMakeVisible (quantizeButton); quantizeButton.setButtonText ("QUANT");
    quantizeButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (reverseButton); reverseButton.setButtonText ("REV");
    reverseButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (killDryButton); killDryButton.setButtonText ("KILL DRY");
    killDryButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (trailsButton); trailsButton.setButtonText ("TRAILS");
    trailsButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);

    addAndMakeVisible (freezeButton); freezeButton.setButtonText ("FREEZE");
    freezeButton.setClickingTogglesState(true);
    freezeButton.setColour(juce::TextButton::buttonOnColourId, CustomLookAndFeel::Colors::appleBlue.withAlpha(0.8f));

    activityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "ACTIVITY", activitySlider);
    timeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "TIME", timeSlider);
    shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SHAPE", shapeSlider);
    repeatsAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "REPEATS", repeatsSlider);
    filterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "FILTER", filterSlider);
    spaceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SPACE", spaceSlider);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MIX", mixSlider);
    loopLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "LOOP_LEVEL", loopLevelSlider);
    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "GAIN", gainSlider);
    modRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MOD_RATE", modRateSlider);
    modDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MOD_DEPTH", modDepthSlider);
    algoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "ALGO", algoBox);
    looperModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LOOPER_MODE", looperModeBox);
    presetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "PRESET", presetBox);
    quantizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_QUANT", quantizeButton);
    reverseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_REV", reverseButton);
    freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "FREEZE", freezeButton);
    looperRecAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_REC", looperRecButton);
    looperOdubAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_ODUB", looperOdubButton);
    killDryAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "KILL_DRY", killDryButton);
    trailsAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "TRAILS", trailsButton);

    startTimerHz(30); setSize (800, 600); updateLabels();
}

CosmicRaysAudioProcessorEditor::~CosmicRaysAudioProcessorEditor() {
    stopTimer();
    for (auto* s : {&activitySlider, &timeSlider, &shapeSlider, &repeatsSlider, &filterSlider, &spaceSlider, &mixSlider, &loopLevelSlider, &gainSlider, &modRateSlider, &modDepthSlider})
        s->setLookAndFeel(nullptr);
    algoBox.setLookAndFeel(nullptr); looperModeBox.setLookAndFeel(nullptr); presetBox.setLookAndFeel(nullptr);
}

void FilterVisualizer::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; g.fillAll(C::scopeBg); auto bounds = getLocalBounds().toFloat();
    g.setColour(C::phosphorGreen.withAlpha(0.12f));
    for (float x = 0; x < bounds.getWidth(); x += 25) g.drawVerticalLine((int)x, 0, bounds.getHeight());
    for (float y = 0; y < bounds.getHeight(); y += 25) g.drawHorizontalLine((int)y, 0, bounds.getWidth());
    float filterKnob = processor.apvts.getRawParameterValue("FILTER")->load();
    float resonance = processor.apvts.getRawParameterValue("RESONANCE")->load();
    juce::Path p; p.startNewSubPath(0, bounds.getHeight() * 0.5f);
    for (float x = 0; x < bounds.getWidth(); x += 1.0f) {
        float freq = x / bounds.getWidth(), y = bounds.getHeight() * 0.5f;
        if (filterKnob < 0.45f) {
            float cutoff = filterKnob / 0.45f;
            if (freq > cutoff) y += (freq - cutoff) * bounds.getHeight() * 2.5f;
            if (std::abs(freq - cutoff) < 0.04f) y -= (resonance * 0.8f) * 40.0f;
        } else if (filterKnob > 0.55f) {
            float cutoff = (filterKnob - 0.55f) / 0.45f;
            if (freq < cutoff) y += (cutoff - freq) * bounds.getHeight() * 2.5f;
            if (std::abs(freq - cutoff) < 0.04f) y -= (resonance * 0.8f) * 40.0f;
        }
        p.lineTo(x, juce::jlimit(0.0f, bounds.getHeight(), y));
    }
    g.setColour(C::phosphorGreen.withAlpha(0.35f)); g.strokePath(p, juce::PathStrokeType(4.0f));
    g.setColour(C::phosphorGreen); g.strokePath(p, juce::PathStrokeType(1.5f));
}

void GrainShapeVisualizer::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; g.fillAll(C::scopeBg); auto bounds = getLocalBounds().toFloat().reduced(5);
    g.setColour(C::phosphorGreen.withAlpha(0.12f));
    for (float x = 0; x < bounds.getWidth(); x += 25) g.drawVerticalLine((int)(bounds.getX() + x), bounds.getY(), bounds.getBottom());
    for (float y = 0; y < bounds.getHeight(); y += 25) g.drawHorizontalLine((int)(bounds.getY() + y), bounds.getX(), bounds.getRight());
    float shape = processor.apvts.getRawParameterValue("SHAPE")->load();
    juce::Path p; p.startNewSubPath(bounds.getX(), bounds.getBottom());
    int type = shape < 0.25f ? 0 : (shape < 0.5f ? 1 : (shape < 0.75f ? 2 : 3));
    for (float x = 0; x <= bounds.getWidth(); x += 1.0f) {
        float phase = x / bounds.getWidth(), window = 0.0f;
        if (type == 0) window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
        else if (type == 1) window = 1.0f - std::abs(2.0f * phase - 1.0f);
        else if (type == 2) window = 1.0f - phase;
        else window = phase < 0.05f ? phase * 20.0f : (phase > 0.95f ? (1.0f - phase) * 20.0f : 1.0f);
        p.lineTo(bounds.getX() + x, bounds.getBottom() - (window * bounds.getHeight()));
    }
    g.setColour(C::phosphorGreen.withAlpha(0.35f)); g.strokePath(p, juce::PathStrokeType(4.0f));
    g.setColour(C::phosphorGreen); g.strokePath(p, juce::PathStrokeType(1.5f));
}

void CosmicRaysAudioProcessorEditor::handleTap() {
    double now = juce::Time::getMillisecondCounterHiRes();
    if (tapTimes.size() > 0 && (now - tapTimes.getLast()) > 2000.0) tapTimes.clear();
    tapTimes.add(now); if (tapTimes.size() > 4) tapTimes.remove(0);
    if (tapTimes.size() >= 2) {
        double avgDiff = (tapTimes.getLast() - tapTimes.getFirst()) / (tapTimes.size() - 1);
        float bpm = juce::jlimit(40.0f, 240.0f, 60000.0f / (float)avgDiff);
        processor.apvts.getParameter("TIME")->setValueNotifyingHost((bpm - 40.0f) / 200.0f);
        processor.apvts.getParameter("TEMPO_MODE")->setValueNotifyingHost(1.0f);
        updateLabels();
    }
}

void CosmicRaysAudioProcessorEditor::timerCallback() {
    float timeVal = processor.apvts.getRawParameterValue("TIME")->load();
    bool tempoMode = processor.apvts.getRawParameterValue("TEMPO_MODE")->load() > 0.5f;
    float bpm = tempoMode ? (40.0f + timeVal * 200.0f) : 120.0f;
    uint32_t now = juce::Time::getMillisecondCounter();
    if (now - lastBlinkTime > (60000.0f / bpm)) { ledOn = true; lastBlinkTime = now; }
    else if (now - lastBlinkTime > 100) ledOn = false;
    if (!isShiftPressed) {
        if (tempoMode) timeSlider.setComponentID(juce::String(juce::roundToInt(bpm)));
        else {
            juce::StringArray divs = {"1/1", "1/2", "1/4", "1/8", "1/16"};
            int idx = juce::jlimit(0, 4, juce::roundToInt(timeVal * 4.0f));
            timeSlider.setComponentID(divs[idx]);
            processor.apvts.getParameter("SUBDIV")->setValueNotifyingHost((float)idx / 4.0f);
        }
    }
    filterVis.repaint(); grainShapeVis.repaint(); repaint();
}

void CosmicRaysAudioProcessorEditor::updateLabels() {
    if (isShiftPressed) {
        filterLabel.setText("RES", juce::dontSendNotification); timeLabel.setText("DRIFT", juce::dontSendNotification);
        repeatsLabel.setText("SNAP", juce::dontSendNotification); spaceLabel.setText("MODE", juce::dontSendNotification);
    } else {
        filterLabel.setText("FILTER", juce::dontSendNotification); timeLabel.setText("TIME", juce::dontSendNotification);
        repeatsLabel.setText("REPEATS", juce::dontSendNotification); spaceLabel.setText("SPACE", juce::dontSendNotification);
    }
}

void CosmicRaysAudioProcessorEditor::paint (juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; g.fillAll (C::appleBeige); auto bounds = getLocalBounds().toFloat();
    auto headerArea = bounds.removeFromTop (80.0f);
    auto stripeWidth = bounds.getWidth(), stripeHeight = 5.0f, stripeY = headerArea.getBottom() - 5;
    juce::Colour colors[] = { C::appleGreen, C::appleYellow, C::appleOrange, C::appleRed, C::applePurple, C::appleBlue };
    float rectW = stripeWidth / 6.0f;
    for (int i = 0; i < 6; ++i) { g.setColour (colors[i]); g.fillRect (i * rectW, stripeY, rectW, stripeHeight); }
    g.setColour (juce::Colour (0xFF222222)); g.setFont (juce::Font ("Helvetica", 30.0f, juce::Font::bold));
    g.drawFittedText ("COSMIC RAYS", headerArea.withTrimmedLeft(20).withTrimmedBottom(25).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setFont (juce::Font ("Courier", 12.0f, juce::Font::plain));
    g.drawFittedText ("BETA 3-3-2026", headerArea.withTrimmedRight(20).withTrimmedBottom(25).toNearestInt(), juce::Justification::centredRight, 1);
    g.setColour(juce::Colour(0xFF444444)); g.setFont(juce::Font("Courier", 10.0f, juce::Font::bold));
    g.drawText("SYNC", shiftButton.getX(), shiftButton.getY() - 10, 50, 10, juce::Justification::centred);
    g.drawText("BEAT", tapButton.getX(), tapButton.getY() - 10, 50, 10, juce::Justification::centred);
    bool tempoMode = processor.apvts.getRawParameterValue("TEMPO_MODE")->load() > 0.5f;
    g.setColour(tempoMode ? C::appleRed : C::appleGreen); g.fillEllipse(shiftButton.getRight() + 3, shiftButton.getY() + 10, 8, 8);
    g.setColour(ledOn ? juce::Colours::white : juce::Colours::darkgrey); g.fillEllipse(tapButton.getRight() + 3, tapButton.getY() + 10, 8, 8);
}

void CosmicRaysAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced (20); auto header = area.removeFromTop (80); auto footer = area.removeFromBottom (100);
    auto controlHeader = area.removeFromTop(40);
    algoBox.setBounds (controlHeader.removeFromLeft (160).reduced (5, 5));
    presetBox.setBounds (controlHeader.removeFromLeft (80).reduced (5, 5));
    tapButton.setBounds (controlHeader.removeFromRight (80).reduced (5, 5));
    shiftButton.setBounds (controlHeader.removeFromRight (80).reduced (5, 5));
    helpButton.setBounds(header.removeFromRight(40).reduced(5, 20));
    auto visArea = area.removeFromTop(120).reduced(5);
    processor.visualiser.setBounds(visArea.removeFromLeft(visArea.getWidth() / 2).reduced(5));
    auto rightVis = visArea;
    filterVis.setBounds(rightVis.removeFromTop(rightVis.getHeight() / 2).reduced(2));
    grainShapeVis.setBounds(rightVis.reduced(2));
    auto footerLeft = footer.removeFromLeft(450); auto looperRow = footerLeft.removeFromTop(footerLeft.getHeight() / 2);
    looperModeBox.setBounds (looperRow.removeFromLeft(80).reduced(5, 10));
    looperRecButton.setBounds (looperRow.removeFromLeft(60).reduced(5, 10));
    looperOdubButton.setBounds (looperRow.removeFromLeft(60).reduced(5, 10));
    quantizeButton.setBounds (looperRow.removeFromLeft(80).reduced(5, 10));
    reverseButton.setBounds (looperRow.removeFromLeft(80).reduced(5, 10));
    freezeButton.setBounds (looperRow.reduced(5, 10));
    auto globalRow = footerLeft;
    killDryButton.setBounds (globalRow.removeFromLeft(120).reduced(5, 10));
    trailsButton.setBounds (globalRow.removeFromLeft(100).reduced(5, 10));
    gainSlider.setBounds (footer.removeFromRight (100).reduced (10, 10));
    gainLabel.setBounds (gainSlider.getBounds().withTop(gainSlider.getBottom() - 15));
    modDepthSlider.setBounds (footer.removeFromRight (70).reduced (5, 10));
    modDepthLabel.setBounds (modDepthSlider.getBounds().withTop(modDepthSlider.getBottom() - 15));
    modRateSlider.setBounds (footer.removeFromRight (70).reduced (5, 10));
    modRateLabel.setBounds (modRateSlider.getBounds().withTop(modRateSlider.getBottom() - 15));
    int kw = area.getWidth() / 4, kh = area.getHeight() / 2; auto r1 = area.removeFromTop (kh);
    auto setSliderBounds = [&](juce::Slider& s, juce::Label& l, juce::Rectangle<int> rect) {
        s.setBounds (rect.withTrimmedBottom (20)); l.setBounds (rect.withTop (rect.getBottom() - 20));
    };
    setSliderBounds (activitySlider, activityLabel, r1.removeFromLeft (kw).reduced (10));
    setSliderBounds (timeSlider, timeLabel, r1.removeFromLeft (kw).reduced (10));
    setSliderBounds (shapeSlider, shapeLabel, r1.removeFromLeft (kw).reduced (10));
    setSliderBounds (repeatsSlider, repeatsLabel, r1.reduced (10));
    auto r2 = area;
    setSliderBounds (filterSlider, filterLabel, r2.removeFromLeft (kw).reduced (10));
    setSliderBounds (spaceSlider, spaceLabel, r2.removeFromLeft (kw).reduced (10));
    setSliderBounds (mixSlider, mixLabel, r2.removeFromLeft (kw).reduced (10));
    setSliderBounds (loopLevelSlider, loopLevelLabel, r2.reduced (10));
    helpOverlay.setBounds(getLocalBounds().reduced(40, 60));
}
