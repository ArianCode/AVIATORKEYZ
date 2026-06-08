#pragma once

#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace AdvancedWidgets
{
inline juce::Colour kGold()      { return juce::Colour (0xffC8A84B); }
inline juce::Colour kCyanLabel() { return juce::Colour (0xff4DB8D4); }
inline juce::Colour kCyanWave()  { return juce::Colour (0xff4DB8D4); }
inline juce::Colour kNavyBg()    { return juce::Colour (0xff060D1A); }
inline juce::Colour kDimBorder() { return juce::Colour (0xff1a2a40); }

inline void paintSectionHeader (juce::Graphics& g,
                                juce::Rectangle<int> bounds,
                                const juce::String& title)
{
    g.setColour (kCyanWave().withAlpha (0.85f));
    g.fillRect (bounds.getX(), bounds.getY() + 1, 2, bounds.getHeight() - 2);
    g.setFont (AviatorTokens::hudBold (9.f));
    g.setColour (kGold());
    g.drawText (title.toUpperCase(), bounds.withTrimmedLeft (7), juce::Justification::centredLeft);
}

inline void paintDivider (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour (kCyanWave().withAlpha (0.2f));
    g.fillRect (bounds.withHeight (1));
}

inline void stylePillButton (juce::TextButton& btn, bool on)
{
    btn.setClickingTogglesState (true);
    btn.setToggleState (on, juce::dontSendNotification);
    btn.setColour (juce::TextButton::buttonColourId,
                   on ? kGold().withAlpha (0.15f) : juce::Colours::transparentBlack);
    btn.setColour (juce::TextButton::buttonOnColourId,
                   on ? kGold().withAlpha (0.15f) : juce::Colours::transparentBlack);
    btn.setColour (juce::TextButton::textColourOffId,
                   on ? kGold() : AviatorTokens::textMuted());
    btn.setColour (juce::TextButton::textColourOnId, kGold());
}

inline void styleCombo (juce::ComboBox& box)
{
    box.setColour (juce::ComboBox::backgroundColourId, kNavyBg());
    box.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    box.setColour (juce::ComboBox::outlineColourId, kDimBorder());
    box.setColour (juce::ComboBox::arrowColourId, kCyanLabel());
}

class ChoiceToggleRow : public juce::Component,
                          private juce::AudioProcessorValueTreeState::Listener
{
public:
    ChoiceToggleRow (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramId,
                     const juce::StringArray& labels,
                     int radioGroupId)
        : apvtsRef (apvts), choiceId (paramId)
    {
        for (int i = 0; i < labels.size(); ++i)
        {
            auto btn = std::make_unique<juce::TextButton> (labels[i]);
            btn->setRadioGroupId (radioGroupId);
            btn->setClickingTogglesState (true);
            btn->onClick = [this, i] { setChoiceIndex (i); };
            addAndMakeVisible (*btn);
            buttons.push_back (std::move (btn));
        }
        apvtsRef.addParameterListener (choiceId, this);
        syncFromParam();
    }

    ~ChoiceToggleRow() override { apvtsRef.removeParameterListener (choiceId, this); }

    void resized() override
    {
        const int n = (int) buttons.size();
        if (n == 0) return;
        const int gap = 1;
        const int w = (getWidth() - gap * (n - 1)) / n;
        int x = 0;
        for (auto& b : buttons)
        {
            b->setBounds (x, 0, w, getHeight());
            x += w + gap;
        }
    }

private:
    void parameterChanged (const juce::String& id, float) override
    {
        if (id == choiceId)
            syncFromParam();
    }

    void setChoiceIndex (int index)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (choiceId)))
        {
            const int n = p->choices.size();
            const float norm = n <= 1 ? 0.f : static_cast<float> (index) / static_cast<float> (n - 1);
            p->setValueNotifyingHost (norm);
        }
        syncFromParam();
    }

    void syncFromParam()
    {
        int idx = 0;
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (choiceId)))
            idx = p->getIndex();

        for (int i = 0; i < (int) buttons.size(); ++i)
            stylePillButton (*buttons[(size_t) i], i == idx);
        repaint();
    }

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String choiceId;
    std::vector<std::unique_ptr<juce::TextButton>> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceToggleRow)
};

class FlatToggle : public juce::TextButton
{
public:
    FlatToggle (juce::AudioProcessorValueTreeState& apvts,
                const juce::String& paramId,
                const juce::String& onText,
                const juce::String& offText)
        : onLabel (onText), offLabel (offText)
    {
        setClickingTogglesState (true);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, paramId, *this);
        onClick = [this] { syncStyle(); };
        syncStyle();
    }

private:
    void syncStyle()
    {
        setButtonText (getToggleState() ? onLabel : offLabel);
        stylePillButton (*this, getToggleState());
    }

    juce::String onLabel;
    juce::String offLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

class FxEnableButton : public juce::TextButton
{
public:
    FxEnableButton (juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& paramId,
                    const juce::String& effectName)
        : name (effectName)
    {
        setClickingTogglesState (true);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, paramId, *this);
        onClick = [this] { syncStyle(); };
        syncStyle();
    }

private:
    void syncStyle()
    {
        setButtonText (name);
        stylePillButton (*this, getToggleState());
    }

    juce::String name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

} // namespace AdvancedWidgets
