#include "PresetSelectorOverlay.h"
#include "AviationTheme.h"
#include "../../PluginProcessor.h"
#include "../Cockpit/PresetDisplayUtils.h"

namespace
{
constexpr int kRowHeight = 24;
constexpr char kAllCategory[] = "All";
} // namespace

class PresetSelectorOverlay::PresetSelectorCategoryList : public juce::Component,
                                                          public juce::ListBoxModel
{
public:
    explicit PresetSelectorCategoryList (PresetSelectorOverlay& o)
        : owner (o)
        , list ("selectorCategories", this)
    {
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.setOutlineThickness (0);
        list.setRowHeight (kRowHeight);
        addAndMakeVisible (list);
    }

    void syncFromOwner()
    {
        categories.clear();
        categories.add (kAllCategory);

        auto& pm = owner.processorRef.getPresetManager();
        categories.addArray (pm.getAllCategories());
        list.updateContent();
        list.repaint();
    }

    void resized() override { list.setBounds (getLocalBounds()); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Aviation::panelDarker());
        g.setColour (Aviation::gold().withAlpha (0.15f));
        g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());
    }

    int getNumRows() override { return categories.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override
    {
        if (! juce::isPositiveAndBelow (row, categories.size()))
            return;

        const auto& cat = categories[row];
        const bool active = cat.equalsIgnoreCase (owner.activeCategory);

        if (active)
        {
            g.setColour (Aviation::gold().withAlpha (0.20f));
            g.fillRect (0, 0, w, h);
            g.setColour (Aviation::goldBright());
            g.fillRect (0, 0, 2, h);
        }

        g.setFont (Aviation::body (11.5f));
        g.setColour (active ? Aviation::goldBright() : Aviation::textSecondary());
        g.drawText (owner.categoryLabel (cat), 10, 0, w - 22, h, juce::Justification::centredLeft);

        if (active)
        {
            g.setFont (Aviation::sans (12.0f));
            g.setColour (Aviation::textPrimary().withAlpha (0.6f));
            g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), w - 16, 0, 10, h, juce::Justification::centred);
        }
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        if (! juce::isPositiveAndBelow (row, categories.size()))
            return;

        owner.selectCategory (categories[row]);
    }

    PresetSelectorOverlay& owner;
    juce::StringArray categories;
    juce::ListBox list;
};

class PresetSelectorOverlay::PresetSelectorPresetList : public juce::Component,
                                                        public juce::ListBoxModel
{
public:
    explicit PresetSelectorPresetList (PresetSelectorOverlay& o)
        : owner (o)
        , list ("selectorPresets", this)
    {
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.setOutlineThickness (0);
        list.setRowHeight (kRowHeight);
        addAndMakeVisible (list);
    }

    void syncFromOwner()
    {
        list.updateContent();
        list.repaint();

        const int idx = owner.findCurrentIndexInView();
        if (idx >= 0)
            list.scrollToEnsureRowIsOnscreen (idx);
    }

    void resized() override { list.setBounds (getLocalBounds()); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Aviation::panelDark());
    }

    int getNumRows() override { return owner.presetEntries.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override
    {
        if (! juce::isPositiveAndBelow (row, owner.presetEntries.size()))
            return;

        const auto& entry = owner.presetEntries.getReference (row);
        const bool active = entry.category.equalsIgnoreCase (owner.currentCategory)
                         && entry.name.equalsIgnoreCase (owner.currentPresetName);

        if (active)
        {
            g.setColour (Aviation::cyan().withAlpha (0.10f));
            g.fillRect (0, 0, w, h);
        }

        auto display = PresetDisplayUtils::shortenDisplayName (entry.name);
        if (owner.isFavourited && owner.isFavourited (entry.category, entry.name))
            display = juce::String::fromUTF8 ("\xe2\x98\x85 ") + display;

        const int textX = active ? 24 : 10;
        g.setFont (Aviation::body (11.5f));

        if (active)
        {
            g.setColour (Aviation::cyan());
            g.drawText (juce::String::fromUTF8 ("\xe2\x9c\x93"), 8, 0, 12, h, juce::Justification::centredLeft);
            g.setColour (Aviation::goldBright());
        }
        else
        {
            g.setColour (Aviation::textPrimary().withAlpha (0.88f));
        }

        g.drawText (display, textX, 0, w - textX - 8, h, juce::Justification::centredLeft);

        if (owner.activeCategory.equalsIgnoreCase (kAllCategory))
        {
            g.setFont (Aviation::label (8.5f, 0.06f));
            g.setColour (Aviation::textSecondary().withAlpha (0.7f));
            g.drawText (owner.categoryLabel (entry.category).toUpperCase(),
                        10, 0, w - 14, h, juce::Justification::centredRight);
        }
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        owner.selectPresetAt (row);
        owner.dismiss();
    }

    PresetSelectorOverlay& owner;
    juce::ListBox list;
};

PresetSelectorOverlay::PresetSelectorOverlay (AviatorKeyzProcessor& processor)
    : processorRef (processor)
    , categoryList (std::make_unique<PresetSelectorCategoryList> (*this))
    , presetList (std::make_unique<PresetSelectorPresetList> (*this))
{
    setVisible (false);
    setInterceptsMouseClicks (true, true);
    setWantsKeyboardFocus (true);

    addAndMakeVisible (*categoryList);
    addAndMakeVisible (*presetList);
}

PresetSelectorOverlay::~PresetSelectorOverlay() = default;

void PresetSelectorOverlay::showOverlay()
{
    auto& pm = processorRef.getPresetManager();
    activeCategory = pm.getCurrentCategory();
    refreshFromPresetManager();
    categoryList->syncFromOwner();
    setVisible (true);
    toFront (true);
    grabKeyboardFocus();
}

void PresetSelectorOverlay::dismiss()
{
    setVisible (false);
    if (onDismiss)
        onDismiss();
}

juce::String PresetSelectorOverlay::categoryLabel (const juce::String& category) const
{
    return category.equalsIgnoreCase (kAllCategory) ? kAllCategory : category;
}

void PresetSelectorOverlay::refreshFromPresetManager()
{
    auto& pm = processorRef.getPresetManager();
    currentCategory = pm.getCurrentCategory();
    currentPresetName = pm.getCurrentPresetName();

    rebuildPresetEntries();
    presetList->syncFromOwner();
    resized();
    repaint();
}

void PresetSelectorOverlay::selectCategory (const juce::String& category)
{
    activeCategory = category;
    rebuildPresetEntries();
    categoryList->repaint();
    presetList->syncFromOwner();
}

void PresetSelectorOverlay::rebuildPresetEntries()
{
    presetEntries.clear();
    auto& pm = processorRef.getPresetManager();

    if (activeCategory.equalsIgnoreCase (kAllCategory))
    {
        for (const auto& category : pm.getAllCategories())
            for (const auto& name : pm.getPresetsForCategory (category))
                presetEntries.add ({ category, name });
    }
    else
    {
        for (const auto& name : pm.getPresetsForCategory (activeCategory))
            presetEntries.add ({ activeCategory, name });
    }
}

int PresetSelectorOverlay::findCurrentIndexInView() const
{
    for (int i = 0; i < presetEntries.size(); ++i)
    {
        const auto& e = presetEntries.getReference (i);
        if (e.category.equalsIgnoreCase (currentCategory)
            && e.name.equalsIgnoreCase (currentPresetName))
            return i;
    }
    return -1;
}

void PresetSelectorOverlay::selectPresetAt (int index)
{
    if (! juce::isPositiveAndBelow (index, presetEntries.size()))
        return;

    const auto& entry = presetEntries.getReference (index);
    processorRef.getPresetManager().loadPreset (entry.category, entry.name);
    refreshFromPresetManager();
}

juce::Rectangle<int> PresetSelectorOverlay::getPanelBounds() const
{
    const int panelW = juce::jmin (kPanelDesignW, getWidth() - 24);
    const int panelH = juce::jmin (kPanelDesignH, getHeight() - Aviation::categoryTabsBounds().getBottom() - 20);

    const int top = Aviation::categoryTabsBounds().getBottom() + 2;
    const int centerX = Aviation::presetHeaderBounds().getCentreX();

    return { centerX - panelW / 2, top, panelW, panelH };
}

juce::Rectangle<int> PresetSelectorOverlay::getBodyBounds() const
{
    return getPanelBounds().reduced (8, 8);
}

void PresetSelectorOverlay::resized()
{
    auto body = getBodyBounds();
    const int catW = juce::roundToInt ((float) body.getWidth() * 0.32f);
    categoryList->setBounds (body.removeFromLeft (catW));
    presetList->setBounds (body);
}

void PresetSelectorOverlay::paint (juce::Graphics& g)
{
    const auto panel = getPanelBounds().toFloat().reduced (0.5f);

    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.fillRoundedRectangle (panel.translated (0.0f, 2.0f), 8.0f);

    juce::ColourGradient grad (juce::Colour (0xf008121c), panel.getX(), panel.getY(),
                               juce::Colour (0xf0040a11), panel.getX(), panel.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (panel, 8.0f);

    Aviation::strokeGoldEdge (g, panel, 8.0f, 0.55f, 1.0f);
    Aviation::topSpecular (g, panel, 0.05f);
}

void PresetSelectorOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (! getPanelBounds().contains (e.getPosition()))
        dismiss();
}

bool PresetSelectorOverlay::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        dismiss();
        return true;
    }

    return Component::keyPressed (key);
}
