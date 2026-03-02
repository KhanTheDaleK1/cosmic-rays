#include "PluginProcessor.h"
#include "PluginEditor.h"

void CustomLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour (juce::Slider::rotarySliderFillColourId);
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = 4.0f; auto arcRadius = radius - lineW * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (bounds.getCentreX(), bounds.getBottom() - radius, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (outline);
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (slider.isEnabled()) {
        juce::Path valueArc;
        valueArc.addCentredArc (bounds.getCentreX(), bounds.getBottom() - radius, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
        g.setColour (fill);
        g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (juce::Colours::white); g.setFont (11.0f);
    juce::String text;
    if (slider.getName() == "Shape") {
        float val = (float)slider.getValue();
        if (val < 0.2f) text = "Sine"; else if (val < 0.4f) text = "Tri";
        else if (val < 0.6f) text = "Saw"; else if (val < 0.8f) text = "Sq"; else text = "Rand";
    } else if (slider.getName() == "Filter") {
        float val = (float)slider.getValue();
        if (val < 0.45f) text = "LPF"; else if (val > 0.55f) text = "HPF"; else text = "Flat";
    } else if (slider.getName() == "Time") {
        text = slider.getComponentID(); 
    } else {
        text = juce::String (juce::roundToInt (sliderPos * 100)) + "%";
    }
    g.drawFittedText (text, bounds.toNearestInt(), juce::Justification::centred, 1);
}

CosmicRaysAudioProcessorEditor::CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    auto setupKnob = [&](juce::Slider& s, juce::Label& l, const juce::String& name) {
        addAndMakeVisible (s); s.setName (name); s.setLookAndFeel (&customLookAndFeel);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::teal);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::black.withAlpha (0.3f));
        addAndMakeVisible (l); l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred); l.setFont (juce::Font (12.0f, juce::Font::bold));
    };

    setupKnob (activitySlider, activityLabel, "Activity");
    setupKnob (timeSlider, timeLabel, "Time");
    setupKnob (shapeSlider, shapeLabel, "Shape");
    setupKnob (repeatsSlider, repeatsLabel, "Repeats");
    setupKnob (filterSlider, filterLabel, "Filter");
    setupKnob (spaceSlider, spaceLabel, "Space");
    setupKnob (mixSlider, mixLabel, "Mix");
    setupKnob (loopLevelSlider, loopLevelLabel, "Loop Level");
    setupKnob (gainSlider, gainLabel, "Gain");

    addAndMakeVisible (algoBox); algoBox.addItemList ({"Mosaic", "Seq", "Glide", "Haze", "Tunnel", "Strum", "Blocks", "Interrupt", "Arp", "Pattern", "Warp"}, 1);
    addAndMakeVisible (looperModeBox); looperModeBox.addItemList ({"Pre-FX", "Looper Only", "Burst"}, 1);
    addAndMakeVisible (presetBox); presetBox.addItemList ({"P1", "P2", "P3", "P4"}, 1);

    addAndMakeVisible (shiftButton); shiftButton.setButtonText ("Shift / Mode");
    shiftButton.onStateChange = [this]() {
        bool isDown = shiftButton.isMouseButtonDown() || juce::ModifierKeys::getCurrentModifiers().isShiftDown();
        if (isDown != isShiftPressed) { isShiftPressed = isDown; updateLabels(); }
    };
    shiftButton.onClick = [this]() {
        auto* param = processor.apvts.getParameter("TEMPO_MODE");
        param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
        updateLabels();
    };

    addAndMakeVisible (tapButton); tapButton.setButtonText ("Tap Tempo");
    tapButton.onClick = [this]() { handleTap(); };

    addAndMakeVisible (quantizeButton); quantizeButton.setButtonText ("Quantize");
    addAndMakeVisible (reverseButton); reverseButton.setButtonText ("Reverse");

    activityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "ACTIVITY", activitySlider);
    timeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "TIME", timeSlider);
    shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SHAPE", shapeSlider);
    repeatsAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "REPEATS", repeatsSlider);
    filterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "FILTER", filterSlider);
    spaceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SPACE", spaceSlider);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MIX", mixSlider);
    loopLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "LOOP_LEVEL", loopLevelSlider);
    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "GAIN", gainSlider);
    algoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "ALGO", algoBox);
    looperModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LOOPER_MODE", looperModeBox);
    presetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "PRESET", presetBox);
    quantizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_QUANT", quantizeButton);
    reverseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_REV", reverseButton);

    startTimerHz(30); 
    setSize (800, 600); updateLabels();
}

CosmicRaysAudioProcessorEditor::~CosmicRaysAudioProcessorEditor() {
    stopTimer();
    for (auto* s : {&activitySlider, &timeSlider, &shapeSlider, &repeatsSlider, &filterSlider, &spaceSlider, &mixSlider, &loopLevelSlider, &gainSlider})
        s->setLookAndFeel(nullptr);
}

void CosmicRaysAudioProcessorEditor::handleTap() {
    double now = juce::Time::getMillisecondCounterHiRes();
    tapTimes.add(now);
    if (tapTimes.size() > 4) tapTimes.remove(0);
    if (tapTimes.size() >= 2) {
        double avgDiff = (tapTimes.getLast() - tapTimes.getFirst()) / (tapTimes.size() - 1);
        float bpm = 60000.0f / (float)avgDiff;
        float normalizedTime = (bpm - 40.0f) / 200.0f;
        processor.apvts.getParameter("TIME")->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normalizedTime));
        processor.apvts.getParameter("TEMPO_MODE")->setValueNotifyingHost(1.0f);
    }
}

void CosmicRaysAudioProcessorEditor::timerCallback() {
    float timeVal = processor.apvts.getRawParameterValue("TIME")->load();
    bool tempoMode = processor.apvts.getRawParameterValue("TEMPO_MODE")->load() > 0.5f;
    float bpm = tempoMode ? (40.0f + timeVal * 200.0f) : 120.0f;
    float msPerBeat = 60000.0f / bpm;
    
    uint32_t now = juce::Time::getMillisecondCounter();
    if (now - lastBlinkTime > msPerBeat) {
        ledOn = true;
        lastBlinkTime = now;
    } else if (now - lastBlinkTime > 100) {
        ledOn = false;
    }

    if (!isShiftPressed) {
        if (tempoMode) {
            timeSlider.setComponentID(juce::String(juce::roundToInt(bpm)) + " BPM");
        } else {
            juce::StringArray divs; divs.add("1/1"); divs.add("1/2"); divs.add("1/4"); divs.add("1/8"); divs.add("1/16");
            int idx = juce::jlimit(0, 4, juce::roundToInt(timeVal * 4.0f));
            timeSlider.setComponentID(divs[idx]);
            processor.apvts.getParameter("SUBDIV")->setValueNotifyingHost((float)idx / 4.0f);
        }
    }
    repaint();
}

void CosmicRaysAudioProcessorEditor::updateLabels() {
    if (isShiftPressed) {
        filterLabel.setText("Resonance", juce::dontSendNotification);
        timeLabel.setText("Mod Rate", juce::dontSendNotification);
        repeatsLabel.setText("Mod Depth", juce::dontSendNotification);
        spaceLabel.setText("Space Mode", juce::dontSendNotification);
    } else {
        filterLabel.setText("Filter", juce::dontSendNotification);
        timeLabel.setText("Time", juce::dontSendNotification);
        repeatsLabel.setText("Repeats", juce::dontSendNotification);
        spaceLabel.setText("Space", juce::dontSendNotification);
    }
}

void CosmicRaysAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll (juce::Colour (0xFF1A1A1A));
    auto bounds = getLocalBounds().toFloat();
    auto headerArea = bounds.removeFromTop (60.0f);
    g.setColour (juce::Colours::teal);
    g.setFont (juce::Font ("Arial", 32.0f, juce::Font::bold | juce::Font::italic));
    g.drawFittedText ("COSMIC RAYS", headerArea.withTrimmedBottom(20).toNearestInt(), juce::Justification::centred, 1);
    
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::Font ("Arial", 14.0f, juce::Font::plain));
    g.drawFittedText ("Beta 3/2/2026", headerArea.withTrimmedTop(40).toNearestInt(), juce::Justification::centred, 1);
    
    bool tempoMode = processor.apvts.getRawParameterValue("TEMPO_MODE")->load() > 0.5f;
    g.setColour(tempoMode ? juce::Colours::red : juce::Colours::green);
    g.fillEllipse(shiftButton.getRight() + 5, shiftButton.getY() + 15, 12, 12);

    g.setColour(ledOn ? juce::Colours::white : juce::Colours::darkgrey);
    g.fillEllipse(tapButton.getRight() + 5, tapButton.getY() + 15, 12, 12);
}

void CosmicRaysAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced (20);
    auto header = area.removeFromTop (60);
    auto footer = area.removeFromBottom (80);
    algoBox.setBounds (header.removeFromLeft (200).reduced (5));
    presetBox.setBounds (header.removeFromLeft (100).reduced (5));
    shiftButton.setBounds (header.removeFromRight (150).reduced (5));
    tapButton.setBounds (shiftButton.getBounds().translated(-160, 0));
    
    auto looperArea = footer.removeFromLeft (400);
    looperModeBox.setBounds (looperArea.removeFromLeft(120).reduced(5, 20));
    quantizeButton.setBounds (looperArea.removeFromLeft(120).reduced(5, 20));
    reverseButton.setBounds (looperArea.reduced(5, 20));
    
    int kw = area.getWidth() / 4, kh = area.getHeight() / 2;
    auto r1 = area.removeFromTop (kh);
    
    auto setSliderBounds = [&](juce::Slider& s, juce::Label& l, juce::Rectangle<int> rect) {
        s.setBounds (rect.withTrimmedBottom (20));
        l.setBounds (rect.withTop (rect.getBottom() - 20));
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
    
    gainSlider.setBounds (footer.removeFromRight (150).reduced (10, 20));
    gainLabel.setBounds (gainSlider.getBounds().withTop(gainSlider.getBottom() - 10));
}
