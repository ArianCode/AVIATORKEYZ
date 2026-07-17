#include "ModAssignCallout.h"
#include "../../DSP/ModMatrix.h"

ModAssignCallout::ModAssignCallout (juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& targetParamId,
                                    std::function<void()> onDone)
    : apvtsRef (apvts)
    , paramId (targetParamId)
    , doneCallback (std::move (onDone))
{
    sourceBox.addItemList (ModMatrix::sourceNames(), 1);
    sourceBox.setSelectedId (2, juce::dontSendNotification);
    sourceBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0d1f33));
    sourceBox.setColour (juce::ComboBox::textColourId, AviatorTokens::textPrimary());
    addAndMakeVisible (sourceBox);

    amountSlider.setRange (0.0, 1.0, 0.01);
    amountSlider.setValue (0.35);
    amountSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    amountSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 18);
    addAndMakeVisible (amountSlider);

    applyButton.onClick = [this]
    {
        const auto dest = ModRoutingHub::modDestForParam (paramId);
        if (! dest.has_value())
            return;

        int row = 0;
        if (auto existing = ModRoutingHub::rowForDest (apvtsRef, *dest))
            row = *existing;
        else
        {
            for (int r = 0; r < ModRoutingHub::kNumRows; ++r)
            {
                if (apvtsRef.getRawParameterValue (ModRoutingHub::kRows[r].on)->load() < 0.5f
                    || ModRoutingHub::stateForParam (apvtsRef, paramId).active == false)
                {
                    row = r;
                    break;
                }
            }
        }

        ModRoutingHub::assignRow (apvtsRef, row, sourceBox.getSelectedId() - 1,
                                *dest, static_cast<float> (amountSlider.getValue()), true);
        if (doneCallback)
            doneCallback();
    };

    clearButton.onClick = [this]
    {
        if (auto row = ModRoutingHub::rowForParam (apvtsRef, paramId))
        {
            const auto& ids = ModRoutingHub::kRows[*row];
            if (auto* p = apvtsRef.getParameter (ids.on))
                p->setValueNotifyingHost (0.f);
        }
        if (doneCallback)
            doneCallback();
    };

    addAndMakeVisible (applyButton);
    addAndMakeVisible (clearButton);
    setSize (220, 110);
}

void ModAssignCallout::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0a1628));
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.4f));
    g.drawRect (getLocalBounds(), 1);
    g.setFont (AviatorTokens::hudBold (10.f));
    g.setColour (AviatorTokens::instrumentCyan());
    g.drawText ("Assign Modulation", 8, 4, getWidth() - 16, 14, juce::Justification::centredLeft);
}

void ModAssignCallout::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (16);
    sourceBox.setBounds (area.removeFromTop (22));
    area.removeFromTop (4);
    amountSlider.setBounds (area.removeFromTop (22));
    area.removeFromTop (6);
    auto buttons = area.removeFromTop (22);
    applyButton.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2 - 2));
    buttons.removeFromLeft (4);
    clearButton.setBounds (buttons);
}

void ModAssignCallout::showForKnob (juce::Component& anchor,
                                    juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& paramId)
{
    if (! ModRoutingHub::modDestForParam (paramId).has_value())
        return;

    auto* hostParent = anchor.findParentComponentOfClass<juce::Component>();
    while (hostParent != nullptr && hostParent->getParentComponent() != nullptr
           && hostParent->getParentComponent()->getWidth() <= hostParent->getWidth())
        hostParent = hostParent->getParentComponent();

    if (hostParent == nullptr)
        hostParent = anchor.getTopLevelComponent();

    if (hostParent == nullptr)
        return;

    for (int i = hostParent->getNumChildComponents() - 1; i >= 0; --i)
    {
        if (auto* child = hostParent->getChildComponent (i);
            child != nullptr && child->getComponentID() == "modAssignPopoverHost")
            hostParent->removeChildComponent (child);
    }

    struct PopoverHost : juce::Component
    {
        ModAssignCallout callout;

        PopoverHost (juce::AudioProcessorValueTreeState& a, const juce::String& id)
            : callout (a, id, [this] { delete this; })
        {
            setComponentID ("modAssignPopoverHost");
            setInterceptsMouseClicks (true, true);
            addAndMakeVisible (callout);
            setSize (callout.getWidth(), callout.getHeight());
        }

        void resized() override { callout.setBounds (getLocalBounds()); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (! callout.getBounds().contains (e.getPosition()))
                delete this;
        }
    };

    const int popW = 220;
    const int popH = 110;
    auto anchorScreen = anchor.getScreenBounds();
    auto localPos = hostParent->getLocalPoint (nullptr, anchorScreen.getBottomLeft());

    auto bounds = juce::Rectangle<int> (localPos.x, localPos.y, popW, popH);
    if (bounds.getBottom() > hostParent->getHeight())
        bounds.translate (0, anchorScreen.getHeight() - popH - anchorScreen.getHeight());
    bounds = bounds.constrainedWithin (hostParent->getLocalBounds());

    auto* host = new PopoverHost (apvts, paramId);
    host->setBounds (bounds);
    hostParent->addAndMakeVisible (host);
    host->toFront (true);
}
