#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

// Platform-specific headers for memory usage reporting
#if JUCE_MAC
#include <mach/mach.h>
#elif JUCE_LINUX
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

/** 
 * Returns current resident memory usage in Megabytes.
 */
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

/**
 * Custom LookAndFeel implementation for the 80s aesthetic.
 */
void CustomLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Shadow
    g.setColour (juce::Colours::black.withAlpha(0.2f));
    g.fillEllipse (centreX - radius + 2, centreY - radius + 3, radius * 2, radius * 2);
    
    // Knob base
    g.setGradientFill (juce::ColourGradient (Colors::puttyGrey.brighter(0.1f), centreX, centreY - radius,
                                           Colors::puttyGrey.darker(0.1f), centreX, centreY + radius, false));
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2, radius * 2);
    
    // Rim
    g.setColour (juce::Colours::black.withAlpha(0.3f));
    g.drawEllipse (centreX - radius, centreY - radius, radius * 2, radius * 2, 1.2f);
    
    // Indicator
    g.setColour (juce::Colour (0xFF111111));
    juce::Path p;
    p.addRoundedRectangle (-1.5f, -radius, 3.0f, radius * 0.4f, 1.0f);
    g.fillPath (p, juce::AffineTransform::rotation (toAngle).translated (centreX, centreY));
    
    // Label
    g.setColour (juce::Colour (0xFF111111));
    g.setFont (juce::FontOptions ("Courier", 11.0f, juce::Font::bold));
    juce::String text;
    if (slider.getName() == "Filter") text = "LPF";
    else text = slider.getComponentID().isEmpty() ? juce::String (juce::roundToInt (sliderPos * 100)) : slider.getComponentID();
    g.drawFittedText (text, bounds.toNearestInt(), juce::Justification::centredBottom, 1);
}

juce::Font CustomLookAndFeel::getTextButtonFont (juce::TextButton&, int) {
    return juce::FontOptions ("Courier", 12.0f, juce::Font::bold);
}

void CustomLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto isDown = shouldDrawButtonAsDown || button.getToggleState();
    
    // 0. The "Socket": Draw the recessed hole the button sits in.
    // This ensures no 'black holes' appear, and gives a mechanical depth.
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRect(bounds.reduced(0.5f));

    // 1. Mechanical "Shift": If the button is 'pressed', we shift the drawing down
    // to simulate the short-travel mechanical depth.
    auto buttonFace = bounds.reduced(1.0f);
    if (isDown) buttonFace = buttonFace.translated(0, 1.2f); 

    // 2. The Keycap Body (Sharp Edges, slight bevel)
    // We use puttyGrey directly to ensure it NEVER turns black.
    g.setColour(Colors::puttyGrey); 
    g.fillRect(buttonFace);

    // 3. Two-Shot Molded Edge (High-contrast border)
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawRect(buttonFace, 1.0f);

    // 4. LED Indicator (Top Right Corner)
    auto text = button.getButtonText();
    bool shouldShowLed = (button.getClickingTogglesState() || text == "TAP") 
                         && text != "?" && text != "BUG" && text != "UNDO" && text != "REDO" && text != "UPDATE!";

    if (shouldShowLed)
    {
        auto ledArea = buttonFace.removeFromTop(6).removeFromRight(6).translated(-3, 3);
        bool ledOn = button.getToggleState();
        
        if (ledOn) {
            juce::Graphics::ScopedSaveState s(g);
            g.setColour(juce::Colours::red.withAlpha(0.3f));
            g.fillEllipse(ledArea.expanded(2.0f));
        }
        
        g.setColour(ledOn ? juce::Colours::red.brighter() : juce::Colours::black.withAlpha(0.2f));
        g.fillEllipse(ledArea);
    }

    // 5. High-Contrast Bottom Shadow (Only visible when button is UP)
    if (!isDown) {
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRect(buttonFace.getX(), buttonFace.getBottom() - 1.0f, buttonFace.getWidth(), 1.0f);
    }
}

void CustomLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                       bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto fontSize = juce::jmin (13.0f, bounds.getHeight() * 0.6f);
    auto tickArea = bounds.removeFromLeft (20).reduced (3);
    
    g.setColour (Colors::puttyGrey.darker (0.1f));
    g.fillRect (tickArea);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawRect (tickArea, 1.0f);
    
    if (button.getToggleState())
    {
        g.setColour (Colors::appleGreen);
        g.fillRect (tickArea.reduced (3));
    }
    
    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.setFont (juce::FontOptions ("Courier", fontSize, juce::Font::bold));
    g.drawText (button.getButtonText(), bounds, juce::Justification::centredLeft, true);
}

CustomLookAndFeel::CustomLookAndFeel() {
    setColour (juce::Slider::rotarySliderFillColourId, Colors::appleBlue);
    setColour (juce::Slider::rotarySliderOutlineColourId, Colors::puttyGrey.darker(0.3f));
    setColour (juce::Label::textColourId, juce::Colour (0xFF222222));
    setColour (juce::ComboBox::backgroundColourId, Colors::puttyGrey.darker(0.1f));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xFF111111));
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black.withAlpha(0.4f));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xFF333333));
    setColour (juce::PopupMenu::backgroundColourId, Colors::puttyGrey.brighter(0.1f));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xFF111111));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colors::appleBlue);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    
    setColour (juce::TextButton::buttonColourId, Colors::puttyGrey);
    setColour (juce::TextButton::buttonOnColourId, Colors::puttyGrey); // MUST set this to avoid black toggle
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF111111));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));
}

HelpComponent::HelpComponent(const juce::String& version) {
    addAndMakeVisible(textEditor);
    textEditor.setMultiLine(true); textEditor.setReadOnly(true); textEditor.setScrollbarsShown(true);
    textEditor.setCaretVisible(false); textEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
    textEditor.setColour(juce::TextEditor::textColourId, CustomLookAndFeel::Colors::phosphorGreen);
    textEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    textEditor.setFont(juce::FontOptions ("Courier", 14.0f, juce::Font::plain));
    juce::String manual = 
        "=== COSMIC RAYS OPERATING MANUAL (" + version + ") ===\n\n"
        "1. CORE CONTROLS\n"
        "- ACTIVITY: Algorithm-specific density/complexity.\n"
        "- TIME: Grain length / repetition rate.\n"
        "- MODE (A-D): Algorithm sub-variations.\n"
        "- REPEATS: Feedback / persistence.\n"
        "- FILTER: 12dB/oct resonant LPF.\n"
        "- SPACE: Reverb + Diffusion network.\n\n"
        "2. ADVANCED GRAIN ENGINE\n"
        "- SPRAY: Randomizes grain sampling position.\n"
        "- SPREAD: Spatializes grains across field.\n"
        "- JITTER: Random pitch offsets.\n"
        "- REV PROB: Reverse grain probability.\n"
        "====================================================";
    textEditor.setText(manual);
}

void HelpComponent::paint(juce::Graphics& g) {
    g.setColour(juce::Colours::black.withAlpha(0.95f)); g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    g.setColour(CustomLookAndFeel::Colors::phosphorGreen); g.drawRoundedRectangle(getLocalBounds().toFloat(), 10.0f, 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.03f)); for (int y = 0; y < getHeight(); y += 3) g.drawHorizontalLine(y, 0, getWidth());
}

void HelpComponent::resized() { textEditor.setBounds(getLocalBounds().reduced(20)); }

/**
 * Creates the high-performance orange-peel texture.
 */
void CosmicRaysAudioProcessorEditor::createPlasticTexture()
{
    plasticTexture = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
    juce::Graphics g(plasticTexture);
    juce::Random r;

    // Generate 15,000 tiny highlight/shadow blobs for a deep orange-peel look
    for (int i = 0; i < 15000; ++i)
    {
        float x = r.nextFloat() * getWidth();
        float y = r.nextFloat() * getHeight();
        float size = 1.0f + r.nextFloat() * 3.0f;
        float alpha = 0.04f + r.nextFloat() * 0.06f;
        
        if (r.nextBool())
            g.setColour(juce::Colours::white.withAlpha(alpha));
        else
            g.setColour(juce::Colours::black.withAlpha(alpha));
            
        g.fillEllipse(x, y, size, size);
    }
}

CosmicRaysAudioProcessorEditor::CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor& p)
    : AudioProcessorEditor (&p), currentVersion(PROJECT_VERSION_STRING), audioProcessor (p), filterVis(p), pitchVis(p), densityMeter(p), waveformVis(p), helpOverlay(currentVersion)
{
    addAndMakeVisible(audioProcessor.visualiser);
    audioProcessor.visualiser.setColours(CustomLookAndFeel::Colors::scopeBg, CustomLookAndFeel::Colors::phosphorGreen.withAlpha(0.7f));
    addAndMakeVisible(filterVis); addAndMakeVisible(pitchVis); addAndMakeVisible(densityMeter); addAndMakeVisible(waveformVis); addChildComponent(helpOverlay);

    auto setupVisLabel = [&](juce::Label& l, const juce::String& text) {
        addAndMakeVisible(l);
        l.setText(text.toUpperCase(), juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::FontOptions("Courier", 13.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colour(0xFF222222));
    };

    setupVisLabel(filterVisLabel, "Spectrum");
    setupVisLabel(pitchVisLabel, "Pitch Cloud");
    setupVisLabel(waveformVisLabel, "Buffer");

    auto setupKnob = [&](juce::Slider& s, juce::Label& l, const juce::String& name) {
        addAndMakeVisible (s); s.setName (name); s.setLookAndFeel (&customLookAndFeel);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (l); l.setText (name.toUpperCase(), juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred); 
        l.setFont (juce::FontOptions ("Courier", 13.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colour(0xFF222222));
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
    timeBox.setColour (juce::GroupComponent::textColourId, juce::Colour (0xFF222222));
    timeBox.setColour (juce::GroupComponent::outlineColourId, juce::Colours::black.withAlpha (0.2f));

    auto setupTimeBtn = [&](juce::TextButton& b, const juce::String& text, float val) {
        addAndMakeVisible(b); b.setButtonText(text); b.setClickingTogglesState(true);
        b.setRadioGroupId(100); b.setLookAndFeel(&customLookAndFeel);
        b.onClick = [this, val]() { audioProcessor.apvts.getParameter("TIME")->setValueNotifyingHost(val); };
    };
    setupTimeBtn(time1_4Button, "1/4", 0.0f); setupTimeBtn(time1_2Button, "1/2", 0.2f); setupTimeBtn(time1xButton,  "1x",  0.4f);
    setupTimeBtn(time2xButton,  "2x",  0.6f);  setupTimeBtn(time4xButton,  "4x",  0.8f);  setupTimeBtn(time8xButton,  "8x",  1.0f);
    time1xButton.setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible (shapeBox); shapeBox.setText ("MODIFIER / MODE");
    shapeBox.setColour (juce::GroupComponent::textColourId, juce::Colour (0xFF222222));
    shapeBox.setColour (juce::GroupComponent::outlineColourId, juce::Colours::black.withAlpha (0.2f));

    auto setupShapeBtn = [&](juce::TextButton& b, const juce::String& text, float val) {
        addAndMakeVisible(b); b.setButtonText(text); b.setClickingTogglesState(true);
        b.setRadioGroupId(200); b.setLookAndFeel(&customLookAndFeel);
        b.onClick = [this, val]() { audioProcessor.apvts.getParameter("SHAPE")->setValueNotifyingHost(val); };
    };
    setupShapeBtn(shapeAButton, "A", 0.0f); setupShapeBtn(shapeBButton, "B", 0.33f);
    setupShapeBtn(shapeCButton, "C", 0.66f); setupShapeBtn(shapeDButton, "D", 1.0f);
    shapeAButton.setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible (algoBox); algoBox.addItemList ({"MOSAIC", "SEQ", "GLIDE", "HAZE", "TUNNEL", "STRUM", "BLOCKS", "INTERRUPT", "ARP", "PATTERN", "WARP"}, 1);
    algoBox.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible (undoButton); undoButton.setButtonText ("UNDO");
    undoButton.setLookAndFeel(&customLookAndFeel);
    undoButton.onClick = [this]() { audioProcessor.undoManager.undo(); };
    addAndMakeVisible (redoButton); redoButton.setButtonText ("REDO");
    redoButton.setLookAndFeel(&customLookAndFeel);
    redoButton.onClick = [this]() { audioProcessor.undoManager.redo(); };
    addAndMakeVisible (looperBox); looperBox.setText ("PHRASE LOOPER");
    looperBox.setColour (juce::GroupComponent::textColourId, juce::Colour (0xFF222222));
    looperBox.setColour (juce::GroupComponent::outlineColourId, juce::Colours::black.withAlpha (0.2f));
    addAndMakeVisible (looperModeBox); looperModeBox.addItemList ({"ONLY", "PARA", "FX->LP", "LP->FX"}, 1);
    looperModeBox.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible (helpButton); helpButton.setButtonText ("?");
    helpButton.setLookAndFeel(&customLookAndFeel);
    helpButton.onClick = [this]() { helpOverlay.setVisible(!helpOverlay.isVisible()); if (helpOverlay.isVisible()) helpOverlay.toFront(false); };
    
    addAndMakeVisible (feedbackButton); feedbackButton.setButtonText ("BUG");
    feedbackButton.setLookAndFeel(&customLookAndFeel);
    feedbackButton.onClick = [this]() { 
        juce::String url = "https://github.com/KhanTheDaleK1/cosmic-rays/issues/new?title=Feedback:%20" + currentVersion.replace(" ", "%20") + "&body=Please%20describe%20your%20issue%20or%20feedback%20below:%0A%0A";
        juce::URL(url).launchInDefaultBrowser(); 
    };

    addChildComponent (updateButton); updateButton.setButtonText ("UPDATE!");
    updateButton.setLookAndFeel(&customLookAndFeel);
    updateButton.onClick = []() { juce::URL("https://github.com/KhanTheDaleK1/cosmic-rays/releases").launchInDefaultBrowser(); };
    
    juce::Thread::launch([this]() {
        juce::URL url("https://raw.githubusercontent.com/KhanTheDaleK1/cosmic-rays/main/version.txt");
        juce::String latestVersion = url.readEntireTextStream().trim();
        if (latestVersion.isNotEmpty() && latestVersion != currentVersion) {
            juce::MessageManager::callAsync([this]() { updateButton.setVisible(true); });
        }
    });

    addAndMakeVisible (shiftButton); shiftButton.setButtonText ("SYNC");
    shiftButton.setLookAndFeel(&customLookAndFeel);
    shiftButton.setClickingTogglesState(true);
    shiftButton.onClick = [this]() {
        auto* param = audioProcessor.apvts.getParameter("TEMPO_MODE");
        param->setValueNotifyingHost(shiftButton.getToggleState() ? 1.0f : 0.0f);
        updateLabels();
    };
    addAndMakeVisible (tapButton); tapButton.setButtonText ("TAP");
    tapButton.setLookAndFeel(&customLookAndFeel);
    tapButton.onClick = [this]() { handleTap(); };
    addAndMakeVisible (looperRecButton); looperRecButton.setButtonText ("REC");
    looperRecButton.setLookAndFeel(&customLookAndFeel);
    looperRecButton.setClickingTogglesState(true); 
    addAndMakeVisible (looperOdubButton); looperOdubButton.setButtonText ("DUB");
    looperOdubButton.setLookAndFeel(&customLookAndFeel);
    looperOdubButton.setClickingTogglesState(true); 
    
    addAndMakeVisible (quantizeButton); quantizeButton.setButtonText ("QUANT");
    quantizeButton.setLookAndFeel(&customLookAndFeel);
    quantizeButton.setClickingTogglesState(true); 
    
    addAndMakeVisible (reverseButton); reverseButton.setButtonText ("REV");
    reverseButton.setLookAndFeel(&customLookAndFeel);
    reverseButton.setClickingTogglesState(true); 
    
    addAndMakeVisible (killDryButton); killDryButton.setButtonText ("KILL DRY");
    killDryButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (trailsButton); trailsButton.setButtonText ("TRAILS");
    trailsButton.setColour(juce::ToggleButton::textColourId, juce::Colours::black);
    addAndMakeVisible (freezeButton); freezeButton.setButtonText ("FREEZE");
    freezeButton.setLookAndFeel(&customLookAndFeel);
    freezeButton.setClickingTogglesState(true); 

    addAndMakeVisible (cpuLabel); cpuLabel.setFont (juce::FontOptions ("Courier", 10.0f, juce::Font::plain));
    cpuLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF444444));
    addAndMakeVisible (ramLabel); ramLabel.setFont (juce::FontOptions ("Courier", 10.0f, juce::Font::plain));
    ramLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF444444));

    activityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "ACTIVITY", activitySlider);
    repeatsAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "REPEATS", repeatsSlider);
    filterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "FILTER", filterSlider);
    spaceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "SPACE", spaceSlider);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "MIX", mixSlider);
    loopLevelAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "LOOP_LEVEL", loopLevelSlider);
    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "GAIN", gainSlider);
    modRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "MOD_RATE", modRateSlider);
    modDepthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "MOD_DEPTH", modDepthSlider);
    sprayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "SPRAY", spraySlider);
    spreadAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "SPREAD", spreadSlider);
    jitterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "PITCH_JITTER", jitterSlider);
    revProbAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "REV_PROB", revProbSlider);
    windowTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "WINDOW_TYPE", windowTypeBox);
    algoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "ALGO", algoBox);
    looperModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "LOOPER_MODE", looperModeBox);
    quantizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "LOOPER_QUANT", quantizeButton);
    reverseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "LOOPER_REV", reverseButton);
    freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "FREEZE", freezeButton);
    looperRecAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "LOOPER_REC", looperRecButton);
    looperOdubAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "LOOPER_ODUB", looperOdubButton);
    killDryAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "KILL_DRY", killDryButton);
    trailsAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "TRAILS", trailsButton);

    startTimerHz(30); 
    setSize (800, 600); 
    createPlasticTexture();
    logoImage = juce::ImageCache::getFromMemory(BinaryData::Logo_png, BinaryData::Logo_pngSize);
    updateLabels();
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
    auto& apvts = audioProcessor.apvts;
    auto* algoParam = apvts.getParameter("ALGO");
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown()) {
        if (key.getKeyCode() == 'Z') {
            if (key.getModifiers().isShiftDown()) audioProcessor.undoManager.redo();
            else audioProcessor.undoManager.undo();
            return true;
        }
    }
    auto keyCode = key.getKeyCode();
    if (keyCode >= '0' && keyCode <= '9') {
        int index = (keyCode - '0');
        if (index == 0) index = 10; else index -= 1;
        if (index < 11) {
            algoParam->setValueNotifyingHost(algoParam->getNormalisableRange().convertTo0to1((float)index));
            return true;
        }
    }
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

/**
 * Helper to draw the round chassis with a silver ring around a black screen.
 */
static void drawRoundScope(juce::Graphics& g, juce::Rectangle<float> bounds) {
    using C = CustomLookAndFeel::Colors;
    auto centre = bounds.getCentre();
    auto fullRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto circleBounds = juce::Rectangle<float>(centre.x - fullRadius, centre.y - fullRadius, fullRadius * 2.0f, fullRadius * 2.0f);
    
    // 1. Chrome Bezel (Simulating top-left light source)
    float bezelThickness = 6.0f;
    juce::ColourGradient chrome(juce::Colours::white.withAlpha(0.8f), circleBounds.getX(), circleBounds.getY(),
                                 juce::Colours::black.withAlpha(0.5f), circleBounds.getRight(), circleBounds.getBottom(), false);
    chrome.addColour(0.3, C::puttyGrey.brighter(0.3f));
    chrome.addColour(0.5, C::puttyGrey);
    chrome.addColour(0.7, C::puttyGrey.darker(0.4f));

    g.setGradientFill(chrome);
    g.drawEllipse(circleBounds.reduced(bezelThickness/2.0f), bezelThickness);
                  
    // Top-left shiny highlight
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawEllipse(circleBounds.reduced(0.5f), 1.0f);
    
    // Bottom-right deep shadow (inner ring)
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawEllipse(circleBounds.reduced(bezelThickness - 0.5f), 1.0f);

    // 2. Inner Black Screen
    float screenRadius = fullRadius - bezelThickness;
    g.setColour(C::scopeBg);
    g.fillEllipse(centre.x - screenRadius, centre.y - screenRadius, screenRadius * 2.0f, screenRadius * 2.0f);
    
    // 3. Softer inner shadow for a natural glass edge
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.drawEllipse(centre.x - screenRadius, centre.y - screenRadius, screenRadius * 2.0f, screenRadius * 2.0f, 2.0f);
}

void FilterVisualizer::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; 
    auto bounds = getLocalBounds().toFloat();
    drawRoundScope(g, bounds);
    
    // Define clipping path to strictly stay inside the silver ring
    float bezelThickness = 6.0f;
    auto centre = bounds.getCentre();
    float screenRadius = (juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f) - bezelThickness;
    auto inner = juce::Rectangle<float>(centre.x - screenRadius, centre.y - screenRadius, screenRadius * 2.0f, screenRadius * 2.0f);
    
    juce::Path clipPath; clipPath.addEllipse(inner);
    g.saveState();
    g.reduceClipRegion(clipPath);
    
    g.setColour(C::phosphorGreen.withAlpha(0.12f));
    for (float x = inner.getX(); x < inner.getRight(); x += inner.getWidth()/8.0f) g.drawVerticalLine((int)x, inner.getY(), inner.getBottom());
    for (float y = inner.getY(); y < inner.getBottom(); y += inner.getHeight()/8.0f) g.drawHorizontalLine((int)y, inner.getX(), inner.getRight());
    
#if ENABLE_ENHANCED_VISUALIZERS
    // 1. Live Spectrum Analyzer - Responsive to TOTAL OUTPUT audio
    g.setColour(C::phosphorGreen.withAlpha(0.25f));
    juce::Random r;
    juce::Path spectrum;
    
    float filterKnob = audioProcessor.apvts.getRawParameterValue("FILTER")->load();
    float resonance = audioProcessor.apvts.getRawParameterValue("RESONANCE")->load();
    
    // Get actual total output peak from engine
    float outputPeak = audioProcessor.getGranularEngine().getOutputFollower();
    
    // Smooth and ARTIFICIALLY BOOST for display (log-style scaling)
    static float smoothedPeak = 0.0f;
    smoothedPeak = smoothedPeak * 0.7f + outputPeak * 0.3f;
    
    // Use square root to bring up low levels visually (Display Gain)
    float visualGain = std::sqrt(smoothedPeak); 

    spectrum.startNewSubPath(inner.getX(), inner.getBottom());
    for (float x = 0; x < inner.getWidth(); x += 2.0f) {
        float freq = x / inner.getWidth();
        
        // Base energy distribution driven by BOOSTED output level
        float baseAmp = std::pow(1.0f - freq, 1.2f) * inner.getHeight() * 1.5f * visualGain;
        
        // --- APPLY FILTER EFFECT TO SPECTRUM ---
        float attenuation = 1.0f;
        if (freq > filterKnob) {
            attenuation = std::exp(-5.0f * (freq - filterKnob)); 
        }
        
        float resBoost = 0.0f;
        float distanceToCutoff = std::abs(freq - filterKnob);
        if (distanceToCutoff < 0.05f) {
            resBoost = (1.0f - (distanceToCutoff / 0.05f)) * resonance * 35.0f * visualGain;
        }
        
        float jitter = r.nextFloat() * 15.0f * (0.3f + visualGain);
        float amp = (baseAmp * attenuation) + resBoost + jitter;
        
        spectrum.lineTo(inner.getX() + x, inner.getBottom() - juce::jlimit(0.0f, inner.getHeight(), amp));
    }
    spectrum.lineTo(inner.getRight(), inner.getBottom());
    g.fillPath(spectrum);
#endif

    juce::Path p; p.startNewSubPath(inner.getX(), inner.getCentreY());
    for (float x = 0; x < inner.getWidth(); x += 1.0f) {
        float freq = x / inner.getWidth(), y = inner.getCentreY();
        if (freq > filterKnob) y += (freq - filterKnob) * inner.getHeight() * 2.5f;
        if (std::abs(freq - filterKnob) < 0.04f) y -= (resonance * 0.8f) * 40.0f;
        p.lineTo(inner.getX() + x, juce::jlimit(inner.getY(), inner.getBottom(), y));
    }
    g.setColour(C::phosphorGreen.withAlpha(0.35f)); g.strokePath(p, juce::PathStrokeType(4.0f));
    g.setColour(C::phosphorGreen); g.strokePath(p, juce::PathStrokeType(1.5f));

    g.setColour(C::phosphorGreen.withAlpha(0.8f));
    g.setFont(juce::FontOptions ("Courier", 11.0f, juce::Font::bold));
#if !ENABLE_ENHANCED_VISUALIZERS
    g.drawText(juce::String(juce::roundToInt(filterKnob * 100)) + "%", inner.withTrimmedBottom(12), juce::Justification::centredBottom);
#else
    g.drawText("FILTER: " + juce::String(juce::roundToInt(filterKnob * 100)) + "%", inner.withTrimmedBottom(12), juce::Justification::centredBottom);
#endif
    g.restoreState();
}

void PitchVisualizer::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors;
    auto bounds = getLocalBounds().toFloat();
    drawRoundScope(g, bounds);
    
    float bezelThickness = 6.0f;
    auto centre = bounds.getCentre();
    float screenRadius = (juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f) - bezelThickness;
    auto inner = juce::Rectangle<float>(centre.x - screenRadius, centre.y - screenRadius, screenRadius * 2.0f, screenRadius * 2.0f);
    
    juce::Path clipPath; clipPath.addEllipse(inner);
    g.saveState();
    g.reduceClipRegion(clipPath);

    g.setColour(C::phosphorGreen.withAlpha(0.12f));
    for (float x = inner.getX(); x < inner.getRight(); x += inner.getWidth()/8.0f) g.drawVerticalLine((int)x, inner.getY(), inner.getBottom());
    for (float y = inner.getY(); y < inner.getBottom(); y += inner.getHeight()/8.0f) g.drawHorizontalLine((int)y, inner.getX(), inner.getRight());
    
#if ENABLE_ENHANCED_VISUALIZERS
    // Octave markers for better reading
    g.setColour(C::phosphorGreen.withAlpha(0.2f));
    g.setFont(juce::FontOptions ("Courier", 9.0f, juce::Font::plain));
    float octHeights[] = { 0.15f, 0.5f, 0.85f };
    juce::String octLabels[] = { "+2", "0", "-2" };
    for (int i = 0; i < 3; ++i) {
        float y = inner.getY() + (inner.getHeight() * octHeights[i]);
        g.drawHorizontalLine((int)y, inner.getX(), inner.getRight());
        g.drawText(octLabels[i], inner.getX() + 5, (int)y - 10, 20, 10, juce::Justification::left);
    }
#endif

    float midY = inner.getCentreY();
    auto& engine = audioProcessor.getGranularEngine();
    auto& grains = engine.getGrains();
    auto& buffer = engine.getDelayBuffer();
    int numSamples = buffer.getNumSamples();
    
    int activeCount = 0;
    if (numSamples > 0) {
        for (auto& grain : grains) {
            if (grain.active) {
                float octave = std::log2(grain.currentSpeed);
                // X position based on actual buffer size
                float xNorm = grain.currentPos / (float)numSamples;
                float x = inner.getX() + xNorm * inner.getWidth();
                
                // Wrap X inside the circular scope display
                while (x > inner.getRight()) x -= inner.getWidth();
                while (x < inner.getX()) x += inner.getWidth();
                
                // Y position scaled to fit +/- 3 octaves comfortably
                float y = midY - (octave * inner.getHeight() * 0.15f);
                float alpha = 1.0f - (grain.age / grain.length);
                
                g.setColour(C::phosphorGreen.withAlpha(alpha));
                g.fillEllipse(x - 3, y - 3, 6, 6);
                activeCount++;
            }
        }
    }

    g.setColour(C::phosphorGreen.withAlpha(0.8f));
    g.setFont(juce::FontOptions ("Courier", 11.0f, juce::Font::bold));
#if !ENABLE_ENHANCED_VISUALIZERS
    g.drawText(juce::String(activeCount), inner.withTrimmedBottom(12), juce::Justification::centredBottom);
#else
    g.drawText("GRAINS: " + juce::String(activeCount), inner.withTrimmedBottom(12), juce::Justification::centredBottom);
#endif
    g.restoreState();
}

void DensityMeter::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors;
    auto bounds = getLocalBounds().toFloat();
    
    // 0. Protrusion Shadow (Outer)
    for (int i = 1; i <= 4; ++i) {
        g.setColour(juce::Colours::black.withAlpha(0.12f / (float)i));
        g.fillRoundedRectangle(bounds.translated(0.0f, (float)i * 1.0f).expanded((float)i * 0.4f), 4.0f);
    }

    // 1. Protruding Beveled Frame
    juce::ColourGradient frameGrad(C::puttyGrey.brighter(0.5f), 0, 0, C::puttyGrey.darker(0.4f), 0, bounds.getHeight(), false);
    g.setGradientFill(frameGrad);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Shiny Top Highlight
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    // 2. Paper Surface (Slightly reduced to show the frame edge)
    auto inner = bounds.reduced(6);
    auto paperCol = juce::Colour(0xFFEEE5CC);
    g.setColour(paperCol);
    g.fillRoundedRectangle(inner, 2.0f);
    
    // Subtle shadow around the paper to sit it into the frame
    g.setColour(juce::Colours::black.withAlpha(0.15f));
    g.drawRoundedRectangle(inner, 2.0f, 1.0f);

    auto arcBounds = inner.reduced(10).translated(0, 15);
    float centerX = arcBounds.getCentreX();
    float centerY = arcBounds.getBottom() + 12;
    float outerRad = arcBounds.getWidth() * 0.75f;
    
    // 3. The Scale Arc
    juce::Path arc;
    arc.addCentredArc(centerX, centerY, outerRad, outerRad, 0.0f, -juce::MathConstants<float>::pi * 0.35f, juce::MathConstants<float>::pi * 0.35f, true);
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.strokePath(arc, juce::PathStrokeType(1.5f));
    
    // Label
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.setFont(juce::FontOptions ("Courier", 13.0f, juce::Font::bold));
    g.drawText("DENSITY", inner.removeFromBottom(25), juce::Justification::centred);
    
    // Ticks
    for (int i = 0; i <= 10; ++i) {
        float angle = -juce::MathConstants<float>::pi * 0.35f + (float)i/10.0f * juce::MathConstants<float>::pi * 0.7f;
        auto pivot = juce::Point<float>(centerX, centerY);
        auto p1 = pivot.getPointOnCircumference(outerRad, angle);
        auto p2 = pivot.getPointOnCircumference(outerRad + 6.0f, angle);
        g.setColour(i > 7 ? juce::Colours::red : juce::Colours::black);
        g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.5f);
    }
    
    // 4. The Needle
    float activeCount = audioProcessor.getGranularEngine().getActiveGrainCount();
    // Maximum capacity is now 256
    float maxGrains = 256.0f; 
    float norm = juce::jlimit(0.0f, 1.0f, activeCount / maxGrains);
    float needleAngle = -juce::MathConstants<float>::pi * 0.35f + norm * juce::MathConstants<float>::pi * 0.7f;
    
    auto needlePivot = juce::Point<float>(centerX, centerY);
    auto needleTip = needlePivot.getPointOnCircumference(outerRad * 0.96f, needleAngle);
    
    // Needle Shadow
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawLine(needlePivot.x + 1.5f, needlePivot.y - 13.5f, needleTip.x + 1.5f, needleTip.y + 1.5f, 2.0f);
    
    // Main Needle Body
    g.setColour(juce::Colours::red.darker(0.2f));
    g.drawLine(needlePivot.x, needlePivot.y - 15.0f, needleTip.x, needleTip.y, 2.0f);
    
    // Needle Hub
    g.setColour(juce::Colours::black);
    g.fillEllipse(needlePivot.x - 5, needlePivot.y - 20, 10, 10);
    g.setColour(C::puttyGrey);
    g.fillEllipse(needlePivot.x - 2, needlePivot.y - 17, 4, 4);

    // 5. Glass Lens Reflection
    juce::Path glassPath;
    glassPath.addRoundedRectangle(inner, 2.0f);
    g.saveState();
    g.reduceClipRegion(glassPath);
    
    juce::ColourGradient glassGrad(juce::Colours::white.withAlpha(0.2f), inner.getX(), inner.getY(),
                                   juce::Colours::transparentWhite, inner.getRight(), inner.getBottom(), false);
    glassGrad.addColour(0.4, juce::Colours::white.withAlpha(0.08f));
    glassGrad.addColour(0.5, juce::Colours::transparentWhite);
    g.setGradientFill(glassGrad);
    g.fillAll();
    
    g.restoreState();
}

void WaveformVisualizer::paint(juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; 
    auto bounds = getLocalBounds().toFloat();
    drawRoundScope(g, bounds);
    
    float bezelThickness = 6.0f;
    auto centre = bounds.getCentre();
    float screenRadius = (juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f) - bezelThickness;
    auto inner = juce::Rectangle<float>(centre.x - screenRadius, centre.y - screenRadius, screenRadius * 2.0f, screenRadius * 2.0f);
    
    juce::Path clipPath; clipPath.addEllipse(inner);
    g.saveState();
    g.reduceClipRegion(clipPath);

    g.setColour(C::phosphorGreen.withAlpha(0.12f));
    for (float x = inner.getX(); x < inner.getRight(); x += inner.getWidth()/8.0f) g.drawVerticalLine((int)x, inner.getY(), inner.getBottom());
    for (float y = inner.getY(); y < inner.getBottom(); y += inner.getHeight()/8.0f) g.drawHorizontalLine((int)y, inner.getX(), inner.getRight());

#if ENABLE_ENHANCED_VISUALIZERS
#endif

    auto& engine = audioProcessor.getGranularEngine(); auto& buffer = engine.getDelayBuffer();
    if (buffer.getNumSamples() > 0) {
        int writeIdx = engine.getWriteIdx();
        juce::Path wavePath; wavePath.startNewSubPath(inner.getX(), inner.getCentreY());
        int numSamples = buffer.getNumSamples(); float step = (float)numSamples / inner.getWidth();
        
#if ENABLE_ENHANCED_VISUALIZERS
        // Dynamic Normalization: find max peak to scale waveform
        float maxPeak = 0.01f;
        for (float x = 0; x < inner.getWidth(); x += 4.0f) {
             int sampleIdx = (writeIdx + (int)(x * step)) % numSamples;
             maxPeak = juce::jmax(maxPeak, std::abs(buffer.getSample(0, sampleIdx)));
        }
        float scaler = 0.45f / juce::jmax(0.1f, maxPeak);
#else
        float scaler = 0.45f;
#endif

        for (float x = 0; x < inner.getWidth(); x += 1.0f) {
            int sampleIdx = (writeIdx + (int)(x * step)) % numSamples;
            float val = buffer.getSample(0, sampleIdx);
            wavePath.lineTo(inner.getX() + x, inner.getCentreY() - val * inner.getHeight() * scaler);
        }
        
#if ENABLE_ENHANCED_VISUALIZERS
        // Glow effect
        g.setColour(C::phosphorGreen.withAlpha(0.2f));
        g.strokePath(wavePath, juce::PathStrokeType(4.0f));
#endif
        g.setColour(C::phosphorGreen.withAlpha(0.6f)); g.strokePath(wavePath, juce::PathStrokeType(1.5f));
    }

    g.setColour(C::phosphorGreen.withAlpha(0.8f));
    g.setFont(juce::FontOptions ("Courier", 11.0f, juce::Font::bold));
#if !ENABLE_ENHANCED_VISUALIZERS
    g.drawText("BUFFER", inner.withTrimmedBottom(12), juce::Justification::centredBottom);
#else
    g.drawText("BUFFER", inner.withTrimmedBottom(12), juce::Justification::centredBottom);
#endif
    g.restoreState();
}

void CosmicRaysAudioProcessorEditor::handleTap() {
    // If Host Sync is active, ignore manual taps entirely
    bool isSyncOn = audioProcessor.apvts.getRawParameterValue("TEMPO_MODE")->load() > 0.5f;
    if (isSyncOn) return;

    double now = juce::Time::getMillisecondCounterHiRes();
    
    // Clear taps if it's been more than 2 seconds since the last one
    if (tapTimes.size() > 0 && (now - tapTimes.getLast()) > 2000.0) 
        tapTimes.clear();
        
    tapTimes.add(now); 
    
    // Keep only the last 4 taps for moving average
    if (tapTimes.size() > 4) 
        tapTimes.remove(0);

    if (tapTimes.size() >= 2) {
        double totalInterval = 0;
        for (int i = 1; i < tapTimes.size(); ++i)
            totalInterval += (tapTimes[i] - tapTimes[i-1]);
            
        double avgInterval = totalInterval / (double)(tapTimes.size() - 1);
        float bpm = (float)(60000.0 / avgInterval);
        
        auto* bpmParam = audioProcessor.apvts.getParameter("BPM");
        bpmParam->setValueNotifyingHost(bpmParam->getNormalisableRange().convertTo0to1(juce::jlimit(30.0f, 300.0f, bpm)));
        
        // Also enable Manual Tempo Mode (Tempo Sync Off)
        audioProcessor.apvts.getParameter("TEMPO_MODE")->setValueNotifyingHost(0.0f);
        updateLabels();
    }
}

void CosmicRaysAudioProcessorEditor::timerCallback() {
    float shapeVal = audioProcessor.apvts.getRawParameterValue("SHAPE")->load();
    shapeAButton.setToggleState(shapeVal < 0.25f, juce::dontSendNotification);
    shapeBButton.setToggleState(shapeVal >= 0.25f && shapeVal < 0.5f, juce::dontSendNotification);
    shapeCButton.setToggleState(shapeVal >= 0.5f && shapeVal < 0.75f, juce::dontSendNotification);
    shapeDButton.setToggleState(shapeVal >= 0.75f, juce::dontSendNotification);
    float timeVal = audioProcessor.apvts.getRawParameterValue("TIME")->load();
    time1_4Button.setToggleState(timeVal < 0.1f, juce::dontSendNotification);
    time1_2Button.setToggleState(timeVal >= 0.1f && timeVal < 0.3f, juce::dontSendNotification);
    time1xButton.setToggleState(timeVal >= 0.3f && timeVal < 0.5f, juce::dontSendNotification);
    time2xButton.setToggleState(timeVal >= 0.5f && timeVal < 0.7f, juce::dontSendNotification);
    time4xButton.setToggleState(timeVal >= 0.7f && timeVal < 0.9f, juce::dontSendNotification);
    time8xButton.setToggleState(timeVal >= 0.9f, juce::dontSendNotification);

    // --- Tap Button LED Pulse ---
    float bpm = audioProcessor.apvts.getRawParameterValue("BPM")->load();
    bool isSyncOn = audioProcessor.apvts.getRawParameterValue("TEMPO_MODE")->load() > 0.5f;
    
    if (!isSyncOn) {
        double msPerBeat = 60000.0 / bpm;
        double now = juce::Time::getMillisecondCounterHiRes();
        double phase = std::fmod(now, msPerBeat);
        
        // 100ms flash at start of each beat
        tapButton.setToggleState(phase < 100.0, juce::dontSendNotification);
    } else {
        tapButton.setToggleState(false, juce::dontSendNotification);
    }

    float tempoMode = audioProcessor.apvts.getRawParameterValue("TEMPO_MODE")->load();
    shiftButton.setToggleState(tempoMode > 0.5f, juce::dontSendNotification);

    cpuLabel.setText ("CPU: " + juce::String (audioProcessor.getCPUUsage(), 1) + "%", juce::dontSendNotification);
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

static void drawTastefulCracks(juce::Graphics& g, juce::Rectangle<float> bounds) {
    juce::Random r(12345);
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    for (int i = 0; i < 8; ++i) {
        float x = r.nextFloat() * bounds.getWidth();
        float y = r.nextFloat() * bounds.getHeight();
        float len = 25.0f + r.nextFloat() * 70.0f;
        float ang = r.nextFloat() * juce::MathConstants<float>::twoPi;
        juce::Path crack;
        crack.startNewSubPath(x, y);
        float curX = x, curY = y;
        for (int j = 0; j < 10; ++j) {
            curX += std::cos(ang) * (len/10.0f) + (r.nextFloat() - 0.5f) * 10.0f;
            curY += std::sin(ang) * (len/10.0f) + (r.nextFloat() - 0.5f) * 10.0f;
            crack.lineTo(curX, curY);
        }
        g.strokePath(crack, juce::PathStrokeType(1.0f));
    }
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    for (int i = 0; i < 20; ++i) {
        float x = r.nextFloat() * bounds.getWidth();
        float y = r.nextFloat() * bounds.getHeight();
        g.drawLine(x, y, x + 15 + r.nextFloat() * 30, y + (r.nextFloat() - 0.5f) * 8, 0.6f);
    }
}

void CosmicRaysAudioProcessorEditor::paint (juce::Graphics& g) {
    using C = CustomLookAndFeel::Colors; 
    g.fillAll (C::agedPlastic); 
    auto bounds = getLocalBounds().toFloat();
    if (plasticTexture.isValid()) {
        g.setOpacity (1.0f);
        g.drawImageAt (plasticTexture, 0, 0);
    }
    drawTastefulCracks(g, bounds);
    auto headerArea = bounds.removeFromTop (80.0f);
    auto stripeWidth = bounds.getWidth(), stripeHeight = 5.0f, stripeY = headerArea.getBottom() - 5;
    juce::Colour colors[] = { C::appleGreen, C::appleYellow, C::appleOrange, C::appleRed, C::applePurple, C::appleBlue };
    float rectW = stripeWidth / 6.0f;
    for (int i = 0; i < 6; ++i) { g.setColour (colors[i]); g.fillRect (i * rectW, stripeY, rectW, stripeHeight); }
    
    g.setColour (juce::Colour (0xFF111111)); // Set color for both Logo (if fallback) and Version Text
    if (logoImage.isValid()) {
        auto logoBounds = juce::Rectangle<float>(15, 8, 231, 58); // 5% larger (original was 220x55)
        
        // 1. Subtle Drop Shadow for depth
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(logoBounds.translated(1.5f, 2.0f), 6.0f);
        
        // 2. Main Logo with Rounded Corners
        juce::Path logoClip;
        logoBounds = logoBounds.reduced(0.5f); // Tiny reduction to hide edge artifacts
        logoClip.addRoundedRectangle(logoBounds, 6.0f);
        
        g.saveState();
        g.reduceClipRegion(logoClip);
        g.drawImageWithin(logoImage, (int)logoBounds.getX(), (int)logoBounds.getY(), (int)logoBounds.getWidth(), (int)logoBounds.getHeight(), 
                          juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid | juce::RectanglePlacement::fillDestination);
        g.restoreState();
        
        // 3. Bolted-on Bezel/Edge highlight
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawRoundedRectangle(logoBounds, 6.0f, 1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawRoundedRectangle(logoBounds.translated(0, 0.5f), 6.0f, 0.5f);
    } else {
        g.setFont (juce::FontOptions ("Helvetica", 30.0f, juce::Font::bold));
        g.drawFittedText ("COSMIC RAYS", headerArea.withTrimmedLeft(20).withTrimmedBottom(25).toNearestInt(), juce::Justification::centredLeft, 1);
    }
    
    g.setFont (juce::FontOptions ("Courier", 12.0f, juce::Font::plain));
    g.drawFittedText (currentVersion, headerArea.withTrimmedRight(20).withTrimmedBottom(25).toNearestInt(), juce::Justification::centredRight, 1);
    g.setColour(juce::Colour(0xFF222222)); g.setFont(juce::FontOptions ("Courier", 10.0f, juce::Font::bold));
    g.drawText("SYNC", shiftButton.getX(), shiftButton.getY() - 10, 50, 10, juce::Justification::centred);
    g.drawText("BEAT", tapButton.getX(), tapButton.getY() - 10, 50, 10, juce::Justification::centred);

    // --- High-Contrast Functional Labeling (Screen-printed look) ---
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.setFont(juce::FontOptions ("Courier", 10.0f, juce::Font::bold));

    auto recBounds = looperRecButton.getBounds();
    g.drawText("OUT", recBounds.getX(), recBounds.getBottom() + 2, recBounds.getWidth(), 10, juce::Justification::centred);
    g.drawText("IN", recBounds.getX(), recBounds.getY() - 12, recBounds.getWidth(), 10, juce::Justification::centred);
    
    if (isFineMode) {
        g.setColour(C::appleBlue); g.setFont(juce::FontOptions ("Courier", 14.0f, juce::Font::bold));
        g.drawText("FINE", getWidth() - 60, 10, 50, 20, juce::Justification::centredRight);
    }

    // --- Global Chrome Border ---
    float borderThickness = 4.0f;
    auto fullBounds = getLocalBounds().toFloat();
    juce::ColourGradient chrome(juce::Colours::white.withAlpha(0.8f), 0, 0,
                                 juce::Colours::black.withAlpha(0.6f), fullBounds.getWidth(), fullBounds.getHeight(), false);
    chrome.addColour(0.2, C::puttyGrey.brighter(0.4f));
    chrome.addColour(0.45, juce::Colours::white);
    chrome.addColour(0.55, C::puttyGrey.darker(0.3f));
    chrome.addColour(0.8, C::puttyGrey.brighter(0.2f));
    
    g.setGradientFill(chrome);
    g.drawRect(fullBounds, borderThickness);
    
    // Inner bevel line for extra "machined" detail
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRect(fullBounds.reduced(borderThickness - 0.5f), 1.0f);
}

void CosmicRaysAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced (15); 
    auto header = area.removeFromTop (80); 
    auto footer = area.removeFromBottom (110);
    
    // 1. Header Buttons
    helpButton.setBounds(header.removeFromRight(30).removeFromTop(25).translated(0, 35));
    header.removeFromRight(5);
    feedbackButton.setBounds(header.removeFromRight(40).removeFromTop(25).translated(0, 35));
    header.removeFromRight(5);
    updateButton.setBounds(header.removeFromRight(75).removeFromTop(25).translated(0, 35));
    
    auto statsArea = header.removeFromRight(110).reduced(5, 10);
    cpuLabel.setBounds(statsArea.removeFromTop(20));
    ramLabel.setBounds(statsArea.removeFromTop(20));

    // 2. Utility Row (Sequential subtraction from 'area' to prevent overlap)
    auto utilityRow = area.removeFromTop(40);
    
    // Left Group (Tools)
    undoButton.setBounds (utilityRow.removeFromLeft (60).reduced (2, 5));
    redoButton.setBounds (utilityRow.removeFromLeft (60).reduced (2, 5));
    utilityRow.removeFromLeft(5);
    
    // Calculations for Column alignment
    int colW = area.getWidth() / 4;
    
    // Center-aligned with Column 2
    utilityRow.removeFromLeft(colW - 125); // Skip Col 1 width minus undo/redo
    algoBox.setBounds(utilityRow.removeFromLeft(colW).reduced(5, 5));
    
    // Right utility group
    windowTypeBox.setBounds (utilityRow.removeFromLeft (95).reduced (2, 5));
    freezeButton.setBounds (utilityRow.removeFromLeft (85).reduced (2, 5));
    // looperOdubButton removed from here
    
    tapButton.setBounds (utilityRow.removeFromRight (65).reduced (2, 5));
    shiftButton.setBounds (utilityRow.removeFromRight (75).reduced (2, 5));

    // 3. Clear space between utility buttons and scopes
    area.removeFromTop(20); 
    auto mainArea = area; // Remaining space is exclusively for the channel strips

    auto setColComponent = [&](juce::Component& c, juce::Label& l, juce::Rectangle<int>& col, int height) {
        auto area = col.removeFromTop(height);
        l.setBounds(area.removeFromTop(20));
        c.setBounds(area.reduced(5));
    };
    
    auto setupSliderInCol = [&](juce::Slider& s, juce::Label& l, juce::Rectangle<int> rect) {
        s.setBounds (rect.withTrimmedBottom (20)); 
        l.setBounds (rect.withTop (rect.getBottom() - 20));
    };

    // Column 1: TIME / BUFFER
    auto col1 = mainArea.removeFromLeft(colW);
    setColComponent(waveformVis, waveformVisLabel, col1, 150);
    auto tBoxRect = col1.removeFromTop(90).reduced(5);
    timeBox.setBounds(tBoxRect);
    auto tb = tBoxRect.reduced(10, 15);
    int bh = tb.getHeight() / 2, bw = tb.getWidth() / 3;
    auto t_row1 = tb.removeFromTop(bh);
    time1_4Button.setBounds(t_row1.removeFromLeft(bw).reduced(2));
    time1_2Button.setBounds(t_row1.removeFromLeft(bw).reduced(2));
    time1xButton.setBounds(t_row1.reduced(2));
    auto t_row2 = tb;
    time2xButton.setBounds(t_row2.removeFromLeft(bw).reduced(2));
    time4xButton.setBounds(t_row2.removeFromLeft(bw).reduced(2));
    time8xButton.setBounds(t_row2.reduced(2));
    setupSliderInCol(activitySlider, activityLabel, col1.removeFromTop(90));

    // Column 2: ALGO / GRAINS
    auto col2 = mainArea.removeFromLeft(colW);
    setColComponent(pitchVis, pitchVisLabel, col2, 150);
    auto sBoxRect = col2.removeFromTop(90).reduced(5);
    shapeBox.setBounds(sBoxRect);
    auto sb = sBoxRect.reduced(10, 15);
    int sbh = sb.getHeight() / 2, sbw = sb.getWidth() / 2;
    auto s_row1 = sb.removeFromTop(sbh);
    shapeAButton.setBounds(s_row1.removeFromLeft(sbw).reduced(2));
    shapeBButton.setBounds(s_row1.reduced(2));
    auto s_row2 = sb;
    shapeCButton.setBounds(s_row2.removeFromLeft(sbw).reduced(2));
    shapeDButton.setBounds(s_row2.reduced(2));
    setupSliderInCol(loopLevelSlider, loopLevelLabel, col2.removeFromTop(90));

    // Column 3: FILTER / FEEDBACK
    auto col3 = mainArea.removeFromLeft(colW);
    setColComponent(filterVis, filterVisLabel, col3, 150);
    setupSliderInCol(filterSlider, filterLabel, col3.removeFromTop(90));
    setupSliderInCol(repeatsSlider, repeatsLabel, col3.removeFromTop(90));

    // Column 4: OUTPUT / DENSITY
    auto col4 = mainArea;
    setColComponent(densityMeter, resLabel, col4, 150); // Using resLabel as placeholder for Density title if needed
    setupSliderInCol(spaceSlider, spaceLabel, col4.removeFromTop(90));
    setupSliderInCol(mixSlider, mixLabel, col4.removeFromTop(90));

    // 4. Footer Area
    auto footerLeft = footer.removeFromLeft(320); 
    looperBox.setBounds(footerLeft.reduced(5, 5));
    auto looperArea = looperBox.getBounds().reduced(10, 20);
    looperModeBox.setBounds (looperArea.removeFromLeft(75).reduced(0, 5));
    looperRecButton.setBounds (looperArea.removeFromLeft(55).reduced(5, 5));
    looperOdubButton.setBounds (looperArea.removeFromLeft(55).reduced(5, 5)); 
    
    // Stack Quant and Rev buttons to ensure readability
    auto toggleArea = looperArea.removeFromLeft(60);
    quantizeButton.setBounds (toggleArea.removeFromTop(toggleArea.getHeight()/2).reduced(2));
    reverseButton.setBounds (toggleArea.reduced(2));
    
    auto footerRight = footer;
    gainSlider.setBounds (footerRight.removeFromRight (85).reduced (5, 10)); 
    gainLabel.setBounds (gainSlider.getBounds().withTop(gainSlider.getBottom() - 15));
    
    // Width calibrated to fit exactly 6 knobs in remaining space
    const int smallKnobW = 58;
    const int spacer = 4;
    
    spreadSlider.setBounds (footerRight.removeFromRight (smallKnobW).reduced (spacer, 15));
    spreadLabel.setBounds (spreadSlider.getBounds().withTop(spreadSlider.getBottom() - 12));
    spraySlider.setBounds (footerRight.removeFromRight (smallKnobW).reduced (spacer, 15));
    sprayLabel.setBounds (spraySlider.getBounds().withTop(spraySlider.getBottom() - 12));
    revProbSlider.setBounds (footerRight.removeFromRight (smallKnobW).reduced (spacer, 15));
    revProbLabel.setBounds (revProbSlider.getBounds().withTop(revProbSlider.getBottom() - 12));
    jitterSlider.setBounds (footerRight.removeFromRight (smallKnobW).reduced (spacer, 15));
    jitterLabel.setBounds (jitterSlider.getBounds().withTop(jitterSlider.getBottom() - 12));
    modDepthSlider.setBounds (footerRight.removeFromRight (smallKnobW).reduced (spacer, 15));
    modDepthLabel.setBounds (modDepthSlider.getBounds().withTop(modDepthSlider.getBottom() - 12));
    modRateSlider.setBounds (footerRight.removeFromRight (smallKnobW).reduced (spacer, 15));
    modRateLabel.setBounds (modRateSlider.getBounds().withTop(modRateSlider.getBottom() - 12));
    
    auto globalRow = footerRight.reduced(5, 10);
    killDryButton.setBounds (globalRow.removeFromLeft(90));
    trailsButton.setBounds (globalRow.removeFromLeft(90));

    helpOverlay.setBounds(getLocalBounds().reduced(40, 60));
}
