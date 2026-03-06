#include "PluginProcessor.h"
#include "PluginEditor.h"

#if JUCE_MAC
#include <mach/mach.h>
#elif JUCE_LINUX
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

static double getMemoryUsageInMB() {
#if JUCE_MAC
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS)
        return (double)info.resident_size / (1024.0 * 1024.0);
#elif JUCE_LINUX
    long rss = 0L;
    FILE* fp = NULL;
    if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL ) return 0.0;
    if ( fscanf( fp, "%*s%ld", &rss ) != 1 ) { fclose( fp ); return 0.0; }
    fclose( fp );
    return (double)rss * (double)sysconf(_SC_PAGESIZE) / (1024.0 * 1024.0);
#endif
    return 0.0;
}

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
    g.setFont (juce::FontOptions ("Courier", 12.0f, juce::Font::bold));
    juce::String text;
    if (slider.getName() == "Filter") text = "LPF";
    else text = slider.getComponentID().isEmpty() ? juce::String (juce::roundToInt (sliderPos * 100)) : slider.getComponentID();
    g.drawFittedText (text, bounds.toNearestInt(), juce::Justification::centredBottom, 1);
}

juce::Font CustomLookAndFeel::getTextButtonFont (juce::TextButton&, int) {
    return juce::FontOptions ("Courier", 10.0f, juce::Font::bold);
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
    textEditor.setMultiLine(true); textEditor.setReadOnly(true); textEditor.setScrollbarsShown(true);
    textEditor.setCaretVisible(false); textEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    textEditor.setColour(juce::TextEditor::textColourId, CustomLookAndFeel::Colors::phosphorGreen);
    textEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    textEditor.setFont(juce::FontOptions ("Courier", 14.0f, juce::Font::plain));
    juce::String manual = 
        "=== COSMIC RAYS OPERATING MANUAL (BETA 3-6-2026) ===\n\n"
        "1. CORE CONTROLS\n"
        "- ACTIVITY: Algorithm-specific density/complexity.\n"
        "- TIME: Grain length / repetition rate.\n"
        "- MODE (A-D): Algorithm sub-variations.\n"
        "- REPEATS: Feedback / persistence.\n"
        "- FILTER: 12dB/oct resonant LPF (Modulated by Env Follower).\n"
        "- SPACE: Reverb + Diffusion network.\n\n"
        "2. ADVANCED GRAIN ENGINE\n"
        "- SPRAY: Randomizes grain sampling position.\n"
        "- SPREAD: Spatializes grains across the stereo field.\n"
        "- JITTER: Random pitch offsets (up to +/- 1 octave).\n"
        "- REV PROB: Probability of a grain playing in reverse.\n"
        "- WINDOW: Select grain envelope (Hann, Gauss, etc.).\n\n"
        "3. WORKFLOW & SHORTCUTS\n"
        "- UNDO/REDO: Use [<] [>] buttons or Cmd/Ctrl + Z.\n"
        "- FINE MODE: Hold SHIFT for 10x knob precision.\n"
        "- ALGO SWITCH: Numeric keys 1-9, 0.\n"
        "- NAVIGATION: Arrow keys cycle Algos and Presets.\n\n"
        "4. VISUALIZERS\n"
        "- WAVEFORM: Shows buffer + grain sampling points.\n"
        "- PITCH CLOUD: Grain pitch, length, and age.\n"
        "- DENSITY: Real-time grain engine load.\n"
        "====================================================";
    textEditor.setText(manual);
}

void HelpComponent::paint(juce::Graphics& g) {
    g.setColour(juce::Colours::black.withAlpha(0.95f)); g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    g.setColour(CustomLookAndFeel::Colors::phosphorGreen); g.drawRoundedRectangle(getLocalBounds().toFloat(), 10.0f, 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.03f)); for (int y = 0; y < getHeight(); y += 3) g.drawHorizontalLine(y, 0, getWidth());
}

void HelpComponent::resized() { textEditor.setBounds(getLocalBounds().reduced(20)); }

CosmicRaysAudioProcessorEditor::CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), filterVis(p), pitchVis(p), densityMeter(p), waveformVis(p)
{
    addAndMakeVisible(processor.visualiser);
    processor.visualiser.setColours(CustomLookAndFeel::Colors::scopeBg, CustomLookAndFeel::Colors::phosphorGreen.withAlpha(0.7f));
    addAndMakeVisible(filterVis); addAndMakeVisible(pitchVis); addAndMakeVisible(densityMeter); addAndMakeVisible(waveformVis); addChildComponent(helpOverlay);

    auto setupKnob = [&](juce::Slider& s, juce::Label& l, const juce::String& name) {
        addAndMakeVisible (s); s.setName (name); s.setLookAndFeel (&customLookAndFeel);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (l); l.setText (name.toUpperCase(), juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred); 
        l.setFont (juce::FontOptions ("Courier", 13.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colour(0xFF333333));
    };

    setupKnob (activitySlider, activityLabel, "Activity");
    setupKnob (repeatsSlider, repeatsLabel, "Repeats");
    setupKnob (filterSlider, filterLabel, "Filter");
    setupKnob (spaceSlider, spaceLabel, "Space");
    setupKnob (mixSlider, mixLabel, "Mix");
    setupKnob (loopLevelSlider, loopLevelLabel, "Loop");
    setupKnob (gainSlider, gainLabel, "Gain");
    setupKnob (modRateSlider, modRateLabel, "Rate");
    setupKnob (modDepthSlider, modDepthLabel, "Depth");
    setupKnob (spraySlider, sprayLabel, "Spray");
    setupKnob (spreadSlider, spreadLabel, "Spread");
    setupKnob (jitterSlider, jitterLabel, "Jitter");
    setupKnob (revProbSlider, revProbLabel, "RevProb");

    addAndMakeVisible (windowTypeBox); windowTypeBox.addItemList ({"AUTO", "SINE", "TRI", "SAW", "SQUARE", "RAND", "HANN", "HAMMING", "GAUSS", "RECT"}, 1);
    windowTypeBox.setLookAndFeel(&customLookAndFeel);

    addAndMakeVisible (timeBox); timeBox.setText ("TIME / SUBDIV");
    timeBox.setColour (juce::GroupComponent::textColourId, juce::Colour (0xFF333333));
    timeBox.setColour (juce::GroupComponent::outlineColourId, juce::Colours::black.withAlpha (0.2f));

    auto setupTimeBtn = [&](juce::TextButton& b, const juce::String& text, float val) {
        addAndMakeVisible(b); b.setButtonText(text); b.setClickingTogglesState(true);
        b.setRadioGroupId(100); b.setLookAndFeel(&customLookAndFeel);
        b.onClick = [this, val]() { processor.apvts.getParameter("TIME")->setValueNotifyingHost(val); };
    };
    setupTimeBtn(time1_4Button, "1/4", 0.0f); setupTimeBtn(time1_2Button, "1/2", 0.2f); setupTimeBtn(time1xButton,  "1x",  0.4f);
    setupTimeBtn(time2xButton,  "2x",  0.6f);  setupTimeBtn(time4xButton,  "4x",  0.8f);  setupTimeBtn(time8xButton,  "8x",  1.0f);
    time1xButton.setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible (shapeBox); shapeBox.setText ("MODIFIER / MODE");
    shapeBox.setColour (juce::GroupComponent::textColourId, juce::Colour (0xFF333333));
    shapeBox.setColour (juce::GroupComponent::outlineColourId, juce::Colours::black.withAlpha (0.2f));

    auto setupShapeBtn = [&](juce::TextButton& b, const juce::String& text, float val) {
        addAndMakeVisible(b); b.setButtonText(text); b.setClickingTogglesState(true);
        b.setRadioGroupId(200); b.setLookAndFeel(&customLookAndFeel);
        b.onClick = [this, val]() { processor.apvts.getParameter("SHAPE")->setValueNotifyingHost(val); };
    };
    setupShapeBtn(shapeAButton, "A", 0.0f); setupShapeBtn(shapeBButton, "B", 0.33f);
    setupShapeBtn(shapeCButton, "C", 0.66f); setupShapeBtn(shapeDButton, "D", 1.0f);
    shapeAButton.setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible (algoBox); algoBox.addItemList ({"MOSAIC", "SEQ", "GLIDE", "HAZE", "TUNNEL", "STRUM", "BLOCKS", "INTERRUPT", "ARP", "PATTERN", "WARP"}, 1);
    algoBox.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible (undoButton); undoButton.setButtonText ("UNDO");
    undoButton.setLookAndFeel(&customLookAndFeel);
    undoButton.onClick = [this]() { processor.undoManager.undo(); };
    addAndMakeVisible (redoButton); redoButton.setButtonText ("REDO");
    redoButton.setLookAndFeel(&customLookAndFeel);
    redoButton.onClick = [this]() { processor.undoManager.redo(); };
    addAndMakeVisible (looperBox); looperBox.setText ("PHRASE LOOPER");
    looperBox.setColour (juce::GroupComponent::textColourId, juce::Colour (0xFF333333));
    looperBox.setColour (juce::GroupComponent::outlineColourId, juce::Colours::black.withAlpha (0.2f));
    addAndMakeVisible (looperModeBox); looperModeBox.addItemList ({"ONLY", "PARA", "FX->LP", "LP->FX"}, 1);
    looperModeBox.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible (helpButton); helpButton.setButtonText ("?");
    helpButton.setLookAndFeel(&customLookAndFeel);
    helpButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::Colors::appleBeige.darker(0.2f));
    helpButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF222222));
    helpButton.onClick = [this]() { helpOverlay.setVisible(!helpOverlay.isVisible()); if (helpOverlay.isVisible()) helpOverlay.toFront(false); };
    
    addAndMakeVisible (feedbackButton); feedbackButton.setButtonText ("BUG");
    feedbackButton.setLookAndFeel(&customLookAndFeel);
    feedbackButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::Colors::appleBeige.darker(0.2f));
    feedbackButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF222222));
    feedbackButton.onClick = [this]() { 
        juce::String url = "https://github.com/KhanTheDaleK1/cosmic-rays/issues/new?title=Feedback:%20" + currentVersion.replace(" ", "%20") + "&body=Please%20describe%20your%20issue%20or%20feedback%20below:%0A%0A";
        juce::URL(url).launchInDefaultBrowser(); 
    };

    addChildComponent (updateButton); updateButton.setButtonText ("UPDATE!");
    updateButton.setLookAndFeel(&customLookAndFeel);
    updateButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::Colors::appleRed);
    updateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    updateButton.onClick = []() { juce::URL("https://github.com/KhanTheDaleK1/cosmic-rays/releases").launchInDefaultBrowser(); };
    
    juce::Thread::launch([this]() {
        juce::URL url("https://raw.githubusercontent.com/KhanTheDaleK1/cosmic-rays/main/version.txt");
        juce::String latestVersion = url.readEntireTextStream().trim();
        if (latestVersion.isNotEmpty() && latestVersion != currentVersion) {
            juce::MessageManager::callAsync([this]() { updateButton.setVisible(true); });
        }
    });

    addAndMakeVisible (shiftButton); shiftButton.setButtonText ("SHIFT");
    shiftButton.setLookAndFeel(&customLookAndFeel);
    shiftButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::Colors::appleBeige.darker(0.1f));
    shiftButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF222222));
    shiftButton.onClick = [this]() {
        auto* param = processor.apvts.getParameter("TEMPO_MODE");
        param->setValueNotifyingHost(param->getValue() > 0.5f ? 0.0f : 1.0f);
        updateLabels();
    };
    addAndMakeVisible (tapButton); tapButton.setButtonText ("TAP");
    tapButton.setLookAndFeel(&customLookAndFeel);
    tapButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::Colors::appleBeige.darker(0.1f));
    tapButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF222222));
    tapButton.onClick = [this]() { handleTap(); };
    addAndMakeVisible (looperRecButton); looperRecButton.setButtonText ("REC");
    looperRecButton.setLookAndFeel(&customLookAndFeel);
    looperRecButton.setClickingTogglesState(true); looperRecButton.setColour(juce::TextButton::buttonOnColourId, CustomLookAndFeel::Colors::appleRed.withAlpha(0.8f));
    addAndMakeVisible (looperOdubButton); looperOdubButton.setButtonText ("DUB");
    looperOdubButton.setLookAndFeel(&customLookAndFeel);
    looperOdubButton.setClickingTogglesState(true); looperOdubButton.setColour(juce::TextButton::buttonOnColourId, CustomLookAndFeel::Colors::appleOrange.withAlpha(0.8f));
    addAndMakeVisible (quantizeButton); quantizeButton.setButtonText ("QUANT");
    quantizeButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (reverseButton); reverseButton.setButtonText ("REV");
    reverseButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (killDryButton); killDryButton.setButtonText ("KILL DRY");
    killDryButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (trailsButton); trailsButton.setButtonText ("TRAILS");
    trailsButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (freezeButton); freezeButton.setButtonText ("FREEZE");
    freezeButton.setLookAndFeel(&customLookAndFeel);
    freezeButton.setClickingTogglesState(true); freezeButton.setColour(juce::TextButton::buttonOnColourId, CustomLookAndFeel::Colors::appleBlue.withAlpha(0.8f));

    addAndMakeVisible (cpuLabel); cpuLabel.setFont (juce::FontOptions ("Courier", 10.0f, juce::Font::plain));
    cpuLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF444444));
    addAndMakeVisible (ramLabel); ramLabel.setFont (juce::FontOptions ("Courier", 10.0f, juce::Font::plain));
    ramLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF444444));

    activityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "ACTIVITY", activitySlider);
    repeatsAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "REPEATS", repeatsSlider);
    filterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "FILTER", filterSlider);
    spaceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SPACE", spaceSlider);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MIX", mixSlider);
    loopLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "LOOP_LEVEL", loopLevelSlider);
    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "GAIN", gainSlider);
    modRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MOD_RATE", modRateSlider);
    modDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MOD_DEPTH", modDepthSlider);
    sprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SPRAY", spraySlider);
    spreadAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SPREAD", spreadSlider);
    jitterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "PITCH_JITTER", jitterSlider);
    revProbAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "REV_PROB", revProbSlider);
    windowTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "WINDOW_TYPE", windowTypeBox);
    algoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "ALGO", algoBox);
    looperModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "LOOPER_MODE", looperModeBox);
    quantizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_QUANT", quantizeButton);
    reverseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_REV", reverseButton);
    freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "FREEZE", freezeButton);
    looperRecAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_REC", looperRecButton);
    looperOdubAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "LOOPER_ODUB", looperOdubButton);
    killDryAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "KILL_DRY", killDryButton);
    trailsAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "TRAILS", trailsButton);

    startTimerHz(30); setSize (800, 600); updateLabels();
    setWantsKeyboardFocus(true);
}

CosmicRaysAudioProcessorEditor::~CosmicRaysAudioProcessorEditor() {
    stopTimer();
    for (auto* s : {&activitySlider, &repeatsSlider, &filterSlider, &spaceSlider, &mixSlider, &loopLevelSlider, &gainSlider, &modRateSlider, &modDepthSlider, &spraySlider, &spreadSlider, &jitterSlider, &revProbSlider})
        s->setLookAndFeel(nullptr);
    algoBox.setLookAndFeel(nullptr); looperModeBox.setLookAndFeel(nullptr);
    windowTypeBox.setLookAndFeel(nullptr);
}

bool CosmicRaysAudioProcessorEditor::keyPressed (const juce::KeyPress& key) {
    auto& apvts = processor.apvts;
    auto* algoParam = apvts.getParameter("ALGO");

    // Undo/Redo (Cmd/Ctrl + Z)
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown()) {
        if (key.getKeyCode() == 'Z') {
            if (key.getModifiers().isShiftDown()) processor.undoManager.redo();
            else processor.undoManager.undo();
            return true;
        }
    }

    // Algorithm selection (1-9, 0)
    auto keyCode = key.getKeyCode();
    if (keyCode >= '0' && keyCode <= '9') {
        int index = (keyCode - '0');
        if (index == 0) index = 10; else index -= 1; // 1 -> 0, 0 -> 9
        if (index < 11) {
            algoParam->setValueNotifyingHost(algoParam->getNormalisableRange().convertTo0to1((float)index));
            return true;
        }
    }

    // Arrow keys for navigation
    if (key.getKeyCode() == juce::KeyPress::upKey) {
        float val = algoParam->getValue() - (1.0f / 10.0f);
        algoParam->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, val));
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::downKey) {
        float val = algoParam->getValue() + (1.0f / 10.0f);
        algoParam->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, val));
        return true;
    }

    return false;
}

void CosmicRaysAudioProcessorEditor::modifierKeysChanged (const juce::ModifierKeys& modifiers) {
    isFineMode = modifiers.isShiftDown();
    for (auto* s : {&activitySlider, &repeatsSlider, &filterSlider, &spaceSlider, &mixSlider, &loopLevelSlider, &gainSlider, &modRateSlider, &modDepthSlider}) {
        s->setMouseDragSensitivity(isFineMode ? (int)0.1 : 1);
    }
    updateLabels();
    repaint();
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
        if (freq > filterKnob) y += (freq - filterKnob) * bounds.getHeight() * 2.5f;
        if (std::abs(freq - filterKnob) < 0.04f) y -= (resonance * 0.8f) * 40.0f;
        p.lineTo(x, juce::jlimit(0.0f, bounds.getHeight(), y));
    }
    g.setColour(C::phosphorGreen.withAlpha(0.35f)); g.strokePath(p, juce::PathStrokeType(4.0f));
    g.setColour(C::phosphorGreen); g.strokePath(p, juce::PathStrokeType(1.5f));
}

void PitchVisualizer::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; g.fillAll(C::scopeBg); auto bounds = getLocalBounds().toFloat().reduced(5);
    g.setColour(C::phosphorGreen.withAlpha(0.12f));
    for (float x = 0; x < bounds.getWidth(); x += 25) g.drawVerticalLine((int)(bounds.getX() + x), bounds.getY(), bounds.getBottom());
    for (float y = 0; y < bounds.getHeight(); y += 25) g.drawHorizontalLine((int)(bounds.getY() + y), bounds.getX(), bounds.getRight());
    
    // Draw Octave Guide Lines
    g.setColour(C::phosphorGreen.withAlpha(0.2f));
    float midY = bounds.getCentreY();
    g.drawHorizontalLine((int)midY, bounds.getX(), bounds.getRight()); // 1x
    g.drawHorizontalLine((int)(midY - bounds.getHeight() * 0.25f), bounds.getX(), bounds.getRight()); // 2x
    g.drawHorizontalLine((int)(midY + bounds.getHeight() * 0.25f), bounds.getX(), bounds.getRight()); // 0.5x

    auto& grains = processor.getGranularEngine().getGrains();
    for (auto& grain : grains) {
        if (grain.active) {
            float octave = std::log2(grain.startSpeed);
            float x = bounds.getX() + (grain.currentPos / (5.0f * 44100.0f)) * bounds.getWidth();
            while (x > bounds.getRight()) x -= bounds.getWidth();
            float y = midY - (octave * bounds.getHeight() * 0.25f);
            float alpha = 1.0f - (grain.age / grain.length);
            
            // Draw grain length as a tail
            float grainLenPix = (grain.length / (5.0f * 44100.0f)) * bounds.getWidth();
            g.setColour(C::phosphorGreen.withAlpha(alpha * 0.4f));
            g.drawLine(x, y, x - grainLenPix, y, 2.0f);
            
            // Draw grain head
            g.setColour(C::phosphorGreen.withAlpha(alpha));
            g.fillEllipse(x - 3, y - 3, 6, 6);
            
            // Optional: Draw pan-based offset or color
            g.setColour(C::appleBlue.withAlpha(alpha * 0.3f));
            g.drawEllipse(x - 4, y - 4, 8, 8, 1.0f);
        }
    }
}

void DensityMeter::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; g.fillAll(C::scopeBg); auto bounds = getLocalBounds().toFloat();
    float activeCount = processor.getGranularEngine().getActiveGrainCount();
    float maxGrains = (float)processor.getGranularEngine().getGrains().size();
    float norm = activeCount / maxGrains;
    
    g.setColour(C::phosphorGreen.withAlpha(0.1f));
    for (float x = 0; x < bounds.getWidth(); x += 10) g.drawVerticalLine((int)x, 0, bounds.getHeight());
    
    g.setColour(C::phosphorGreen.withAlpha(0.6f));
    g.fillRect(bounds.withWidth(bounds.getWidth() * norm));
    
    g.setColour(C::phosphorGreen); g.setFont(juce::FontOptions ("Courier", 10.0f, juce::Font::bold));
    g.drawText("DENSITY", bounds.reduced(5, 0), juce::Justification::centredLeft);
}

void WaveformVisualizer::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; g.fillAll(C::scopeBg); auto bounds = getLocalBounds().toFloat();
    auto& engine = processor.getGranularEngine(); auto& buffer = engine.getDelayBuffer();
    if (buffer.getNumSamples() == 0) return;
    int writeIdx = engine.getWriteIdx();
    g.setColour(C::phosphorGreen.withAlpha(0.12f));
    for (float x = 0; x < bounds.getWidth(); x += 50) g.drawVerticalLine((int)x, 0, bounds.getHeight());
    
    juce::Path wavePath; wavePath.startNewSubPath(0, bounds.getCentreY());
    int numSamples = buffer.getNumSamples(); float step = (float)numSamples / bounds.getWidth();
    for (float x = 0; x < bounds.getWidth(); x += 1.0f) {
        int sampleIdx = (writeIdx + (int)(x * step)) % numSamples;
        float val = buffer.getSample(0, sampleIdx);
        wavePath.lineTo(x, bounds.getCentreY() - val * bounds.getHeight() * 0.45f);
    }
    g.setColour(C::phosphorGreen.withAlpha(0.6f)); g.strokePath(wavePath, juce::PathStrokeType(1.5f));

    // Draw Grain Markers
    auto& grains = engine.getGrains();
    for (auto& grain : grains) {
        if (grain.active) {
            int grainPosInt = (int)grain.currentPos;
            int relativePos = (grainPosInt - writeIdx + numSamples) % numSamples;
            float x = ((float)relativePos / (float)numSamples) * bounds.getWidth();
            float alpha = 1.0f - (grain.age / grain.length);
            g.setColour(C::appleRed.withAlpha(alpha * 0.8f));
            g.drawVerticalLine((int)x, 0, (float)bounds.getHeight());
        }
    }
    g.setColour(C::appleBeige.withAlpha(0.5f)); g.drawVerticalLine(0, 0, bounds.getHeight()); // Current write pos
}

void CosmicRaysAudioProcessorEditor::handleTap() {
    double now = juce::Time::getMillisecondCounterHiRes();
    if (tapTimes.size() > 0 && (now - tapTimes.getLast()) > 2000.0) tapTimes.clear();
    tapTimes.add(now); if (tapTimes.size() > 4) tapTimes.remove(0);
    if (tapTimes.size() >= 2) processor.apvts.getParameter("TEMPO_MODE")->setValueNotifyingHost(1.0f);
}

void CosmicRaysAudioProcessorEditor::timerCallback() {
    float shapeVal = processor.apvts.getRawParameterValue("SHAPE")->load();
    shapeAButton.setToggleState(shapeVal < 0.25f, juce::dontSendNotification);
    shapeBButton.setToggleState(shapeVal >= 0.25f && shapeVal < 0.5f, juce::dontSendNotification);
    shapeCButton.setToggleState(shapeVal >= 0.5f && shapeVal < 0.75f, juce::dontSendNotification);
    shapeDButton.setToggleState(shapeVal >= 0.75f, juce::dontSendNotification);

    float timeVal = processor.apvts.getRawParameterValue("TIME")->load();
    time1_4Button.setToggleState(timeVal < 0.1f, juce::dontSendNotification);
    time1_2Button.setToggleState(timeVal >= 0.1f && timeVal < 0.3f, juce::dontSendNotification);
    time1xButton.setToggleState(timeVal >= 0.3f && timeVal < 0.5f, juce::dontSendNotification);
    time2xButton.setToggleState(timeVal >= 0.5f && timeVal < 0.7f, juce::dontSendNotification);
    time4xButton.setToggleState(timeVal >= 0.7f && timeVal < 0.9f, juce::dontSendNotification);
    time8xButton.setToggleState(timeVal >= 0.9f, juce::dontSendNotification);

    cpuLabel.setText ("CPU: " + juce::String (processor.getCPUUsage(), 1) + "%", juce::dontSendNotification);
    ramLabel.setText ("RAM: " + juce::String (juce::roundToInt(getMemoryUsageInMB())) + "MB", juce::dontSendNotification);

    filterVis.repaint(); pitchVis.repaint(); densityMeter.repaint(); waveformVis.repaint(); repaint();
}

void CosmicRaysAudioProcessorEditor::updateLabels() {
    if (isFineMode) {
        filterLabel.setText("RES", juce::dontSendNotification); 
        repeatsLabel.setText("DEPTH", juce::dontSendNotification); 
        spaceLabel.setText("MODE", juce::dontSendNotification);
        algoLabel.setText("RATE", juce::dontSendNotification);
    } else {
        filterLabel.setText("FILTER", juce::dontSendNotification); 
        repeatsLabel.setText("REPEATS", juce::dontSendNotification); 
        spaceLabel.setText("SPACE", juce::dontSendNotification);
        algoLabel.setText("ALGO", juce::dontSendNotification);
    }
}

void CosmicRaysAudioProcessorEditor::paint (juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; g.fillAll (C::appleBeige); auto bounds = getLocalBounds().toFloat();
    auto headerArea = bounds.removeFromTop (80.0f);
    auto stripeWidth = bounds.getWidth(), stripeHeight = 5.0f, stripeY = headerArea.getBottom() - 5;
    juce::Colour colors[] = { C::appleGreen, C::appleYellow, C::appleOrange, C::appleRed, C::applePurple, C::appleBlue };
    float rectW = stripeWidth / 6.0f;
    for (int i = 0; i < 6; ++i) { g.setColour (colors[i]); g.fillRect (i * rectW, stripeY, rectW, stripeHeight); }
    g.setColour (juce::Colour (0xFF222222)); g.setFont (juce::FontOptions ("Helvetica", 30.0f, juce::Font::bold));
    g.drawFittedText ("COSMIC RAYS", headerArea.withTrimmedLeft(20).withTrimmedBottom(25).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setFont (juce::FontOptions ("Courier", 12.0f, juce::Font::plain));
    g.drawFittedText ("BETA 3-6-2026", headerArea.withTrimmedRight(20).withTrimmedBottom(25).toNearestInt(), juce::Justification::centredRight, 1);
    g.setColour(juce::Colour(0xFF444444)); g.setFont(juce::FontOptions ("Courier", 10.0f, juce::Font::bold));
    g.drawText("SYNC", shiftButton.getX(), shiftButton.getY() - 10, 50, 10, juce::Justification::centred);
    g.drawText("BEAT", tapButton.getX(), tapButton.getY() - 10, 50, 10, juce::Justification::centred);
    bool tempoMode = processor.apvts.getRawParameterValue("TEMPO_MODE")->load() > 0.5f;
    g.setColour(tempoMode ? C::appleRed : C::appleGreen); g.fillEllipse(shiftButton.getRight() + 3, shiftButton.getY() + 10, 8, 8);
    g.setColour(ledOn ? juce::Colours::white : juce::Colours::darkgrey); g.fillEllipse(tapButton.getRight() + 3, tapButton.getY() + 10, 8, 8);

    if (isFineMode) {
        g.setColour(C::appleBlue); g.setFont(juce::FontOptions ("Courier", 14.0f, juce::Font::bold));
        g.drawText("FINE", getWidth() - 60, 10, 50, 20, juce::Justification::centredRight);
    }
}

void CosmicRaysAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced (20); auto header = area.removeFromTop (80); auto footer = area.removeFromBottom (100);
    
    // Header right buttons (Help, Bug, Update)
    helpButton.setBounds(header.removeFromRight(25).removeFromTop(20).translated(0, 35));
    header.removeFromRight(5); // gap
    feedbackButton.setBounds(header.removeFromRight(35).removeFromTop(20).translated(0, 35));
    header.removeFromRight(5); // gap
    updateButton.setBounds(header.removeFromRight(65).removeFromTop(20).translated(0, 35));

    auto controlHeader = area.removeFromTop(40);
    algoBox.setBounds (controlHeader.removeFromLeft (160).reduced (5, 5));
    undoButton.setBounds (controlHeader.removeFromLeft (60).reduced (2, 5));
    redoButton.setBounds (controlHeader.removeFromLeft (60).reduced (2, 5));
    windowTypeBox.setBounds (controlHeader.removeFromLeft (80).reduced (5, 5));
    freezeButton.setBounds (controlHeader.removeFromLeft (80).reduced (5, 5));
    
    tapButton.setBounds (controlHeader.removeFromRight (80).reduced (5, 5));
    shiftButton.setBounds (controlHeader.removeFromRight (80).reduced (5, 5));
    auto statsArea = header.removeFromRight(100).reduced(5, 10);
    cpuLabel.setBounds(statsArea.removeFromTop(20));
    ramLabel.setBounds(statsArea.removeFromTop(20));
    
    auto visArea = area.removeFromTop(120).reduced(5);
    processor.visualiser.setBounds(visArea.removeFromLeft(visArea.getWidth() / 3).reduced(5));
    waveformVis.setBounds(visArea.removeFromLeft(visArea.getWidth() / 2).reduced(5));
    auto rightVis = visArea;
    filterVis.setBounds(rightVis.removeFromTop(rightVis.getHeight() * 0.4f).reduced(2));
    densityMeter.setBounds(rightVis.removeFromTop(20).reduced(2));
    pitchVis.setBounds(rightVis.reduced(2));
    
    auto footerLeft = footer.removeFromLeft(420); 
    looperBox.setBounds(footerLeft.reduced(5, 5));
    auto looperArea = looperBox.getBounds().reduced(10, 20);
    looperModeBox.setBounds (looperArea.removeFromLeft(80).reduced(0, 5));
    looperRecButton.setBounds (looperArea.removeFromLeft(60).reduced(5, 5));
    looperOdubButton.setBounds (looperArea.removeFromLeft(60).reduced(5, 5));
    quantizeButton.setBounds (looperArea.removeFromLeft(80).reduced(5, 5));
    reverseButton.setBounds (looperArea.removeFromLeft(80).reduced(5, 5));
    
    auto footerRight = footer;
    gainSlider.setBounds (footerRight.removeFromRight (70).reduced (5, 10));
    gainLabel.setBounds (gainSlider.getBounds().withTop(gainSlider.getBottom() - 15));
    spreadSlider.setBounds (footerRight.removeFromRight (55).reduced (2, 10));
    spreadLabel.setBounds (spreadSlider.getBounds().withTop(spreadSlider.getBottom() - 15));
    spraySlider.setBounds (footerRight.removeFromRight (55).reduced (2, 10));
    sprayLabel.setBounds (spraySlider.getBounds().withTop(spraySlider.getBottom() - 15));
    revProbSlider.setBounds (footerRight.removeFromRight (55).reduced (2, 10));
    revProbLabel.setBounds (revProbSlider.getBounds().withTop(revProbSlider.getBottom() - 15));
    jitterSlider.setBounds (footerRight.removeFromRight (55).reduced (2, 10));
    jitterLabel.setBounds (jitterSlider.getBounds().withTop(jitterSlider.getBottom() - 15));
    modDepthSlider.setBounds (footerRight.removeFromRight (55).reduced (2, 10));
    modDepthLabel.setBounds (modDepthSlider.getBounds().withTop(modDepthSlider.getBottom() - 15));
    modRateSlider.setBounds (footerRight.removeFromRight (55).reduced (2, 10));
    modRateLabel.setBounds (modRateSlider.getBounds().withTop(modRateSlider.getBottom() - 15));
    
    auto globalRow = footerRight.reduced(5, 10);
    killDryButton.setBounds (globalRow.removeFromLeft(80));
    trailsButton.setBounds (globalRow.removeFromLeft(80));
    int kw = area.getWidth() / 4, kh = area.getHeight() / 2; auto r1 = area.removeFromTop (kh);
    auto setSliderBounds = [&](juce::Slider& s, juce::Label& l, juce::Rectangle<int> rect) {
        s.setBounds (rect.withTrimmedBottom (20)); l.setBounds (rect.withTop (rect.getBottom() - 20));
    };
    setSliderBounds (activitySlider, activityLabel, r1.removeFromLeft (kw).reduced (10));
    auto timeArea = r1.removeFromLeft (kw).reduced (10);
    timeBox.setBounds(timeArea);
    auto tb = timeBox.getBounds().reduced(10, 20);
    int bh = tb.getHeight() / 2, bw = tb.getWidth() / 3;
    auto t_row1 = tb.removeFromTop(bh);
    time1_4Button.setBounds(t_row1.removeFromLeft(bw).reduced(2));
    time1_2Button.setBounds(t_row1.removeFromLeft(bw).reduced(2));
    time1xButton.setBounds(t_row1.reduced(2));
    auto t_row2 = tb;
    time2xButton.setBounds(t_row2.removeFromLeft(bw).reduced(2));
    time4xButton.setBounds(t_row2.removeFromLeft(bw).reduced(2));
    time8xButton.setBounds(t_row2.reduced(2));
    auto shapeArea = r1.removeFromLeft (kw).reduced (10);
    shapeBox.setBounds(shapeArea);
    auto sb = shapeBox.getBounds().reduced(10, 20);
    int sbh = sb.getHeight() / 2, sbw = sb.getWidth() / 2;
    auto s_row1 = sb.removeFromTop(sbh);
    shapeAButton.setBounds(s_row1.removeFromLeft(sbw).reduced(2));
    shapeBButton.setBounds(s_row1.reduced(2));
    auto s_row2 = sb;
    shapeCButton.setBounds(s_row2.removeFromLeft(sbw).reduced(2));
    shapeDButton.setBounds(s_row2.reduced(2));
    setSliderBounds (repeatsSlider, repeatsLabel, r1.reduced (10));
    auto r2 = area;
    setSliderBounds (filterSlider, filterLabel, r2.removeFromLeft (kw).reduced (10));
    setSliderBounds (spaceSlider, spaceLabel, r2.removeFromLeft (kw).reduced (10));
    setSliderBounds (mixSlider, mixLabel, r2.removeFromLeft (kw).reduced (10));
    setSliderBounds (loopLevelSlider, loopLevelLabel, r2.reduced (10));
    helpOverlay.setBounds(getLocalBounds().reduced(40, 60));
}
