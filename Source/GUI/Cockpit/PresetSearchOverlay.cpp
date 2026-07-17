#include "PresetSearchOverlay.h"
#include "../../PluginProcessor.h"

namespace
{
constexpr int kPanelDesignW = 600;
constexpr int kPanelDesignH = 500;
} // namespace

class PresetSearchOverlay::CategoryList : public juce::Component,
                                          public juce::ListBoxModel
{
public:
    explicit CategoryList (PresetSearchOverlay& o)
        : owner (o)
        , list ("overlayCategories", this)
    {
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.setOutlineThickness (0);
        list.setRowHeight (24);
        addAndMakeVisible (list);
    }

    void setCategories (const juce::StringArray& cats, const juce::String& active)
    {
        categories = cats;
        activeCategory = active;
        list.updateContent();
        list.repaint();
    }

    void resized() override { list.setBounds (getLocalBounds()); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff0a1220));
    }

    int getNumRows() override { return categories.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override
    {
        if (! juce::isPositiveAndBelow (row, categories.size()))
            return;

        const bool active = categories[row].equalsIgnoreCase (activeCategory);
        if (active)
        {
            g.setColour (AviatorTokens::champagneGold().withAlpha (0.22f));
            g.fillRect (0, 0, w, h);
        }

        g.setFont (AviatorTokens::hud (11.f));
        g.setColour (active ? AviatorTokens::champagneGold() : AviatorTokens::textMuted().brighter (0.25f));
        g.drawText (categories[row].toUpperCase(), 10, 0, w - 12, h, juce::Justification::centredLeft);
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        if (! juce::isPositiveAndBelow (row, categories.size()))
            return;

        owner.activeCategory = categories[row];
        activeCategory = categories[row];
        owner.rebuildPresetList();
        list.repaint();
    }

    PresetSearchOverlay& owner;
    juce::StringArray categories;
    juce::String activeCategory;
    juce::ListBox list;
};

class PresetSearchOverlay::PresetList : public juce::Component,
                                      public juce::ListBoxModel
{
public:
    explicit PresetList (PresetSearchOverlay& o)
        : owner (o)
        , list ("overlayPresets", this)
    {
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.setOutlineThickness (0);
        list.setRowHeight (24);
        addAndMakeVisible (list);
    }

    void syncFromOwner()
    {
        names = owner.filteredNames;
        activeName = owner.currentPresetName;
        list.updateContent();
        list.repaint();
    }

    void resized() override { list.setBounds (getLocalBounds()); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff0a1628));
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.15f));
        g.drawVerticalLine (0, 0.f, (float) getHeight());
    }

    int getNumRows() override { return names.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override
    {
        if (! juce::isPositiveAndBelow (row, names.size()))
            return;

        const bool active = names[row].equalsIgnoreCase (activeName);
        if (active)
        {
            g.setFont (AviatorTokens::hudBold (11.f));
            g.setColour (AviatorTokens::instrumentCyan());
            g.drawText ("✓", 10, 0, 16, h, juce::Justification::centredLeft);
            g.setColour (AviatorTokens::champagneGold());
            g.drawText (names[row], 26, 0, w - 30, h, juce::Justification::centredLeft);
        }
        else
        {
            g.setFont (AviatorTokens::hud (11.f));
            g.setColour (AviatorTokens::textPrimary().withAlpha (0.85f));
            g.drawText (names[row], 10, 0, w - 12, h, juce::Justification::centredLeft);
        }
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        if (! juce::isPositiveAndBelow (row, names.size()))
            return;

        auto& pm = owner.processorRef.getPresetManager();
        pm.loadPreset (owner.activeCategory, names[row]);
        owner.dismiss();
    }

    PresetSearchOverlay& owner;
    juce::StringArray names;
    juce::String activeName;
    juce::ListBox list;
};

PresetSearchOverlay::PresetSearchOverlay (AviatorKeyzProcessor& p)
    : processorRef (p)
    , categoryList (std::make_unique<CategoryList> (*this))
    , presetList (std::make_unique<PresetList> (*this))
{
    setVisible (false);
    setInterceptsMouseClicks (true, true);

    searchField.setTextToShowWhenEmpty ("Search presets...", AviatorTokens::textMuted());
    searchField.setFont (AviatorTokens::hud (12.f));
    searchField.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0a1628));
    searchField.setColour (juce::TextEditor::outlineColourId, AviatorTokens::instrumentCyan().withAlpha (0.25f));
    searchField.setColour (juce::TextEditor::textColourId, AviatorTokens::textPrimary());
    searchField.setColour (juce::CaretComponent::caretColourId, AviatorTokens::instrumentCyan());
    searchField.setIndents (8, 6);
    searchField.onTextChange = [this] { rebuildPresetList(); };
    addAndMakeVisible (searchField);
    addAndMakeVisible (*categoryList);
    addAndMakeVisible (*presetList);
}

PresetSearchOverlay::~PresetSearchOverlay() = default;

void PresetSearchOverlay::showForCategory (const juce::String& category)
{
    auto& pm = processorRef.getPresetManager();
    activeCategory = category.isEmpty() ? pm.getCurrentCategory() : category;
    currentPresetName = pm.getCurrentPresetName();
    searchField.clear();

    categoryList->setCategories (pm.getAllCategories(), activeCategory);
    rebuildPresetList();
    setVisible (true);
    toFront (true);
    searchField.grabKeyboardFocus();
}

void PresetSearchOverlay::dismiss()
{
    setVisible (false);
    if (onDismiss)
        onDismiss();
}

void PresetSearchOverlay::rebuildPresetList()
{
    filteredNames.clear();
    auto& pm = processorRef.getPresetManager();
    const auto query = searchField.getText().trim().toLowerCase();

    for (const auto& name : pm.getPresetsForCategory (activeCategory))
    {
        if (query.isEmpty() || name.toLowerCase().contains (query))
            filteredNames.add (name);
    }

    currentPresetName = pm.getCurrentPresetName();
    presetList->syncFromOwner();
}

juce::Rectangle<int> PresetSearchOverlay::getPanelBounds() const
{
    const int panelW = juce::jmin (AviatorTokens::scaledFor (*this, kPanelDesignW), getWidth() - 40);
    const int panelH = juce::jmin (AviatorTokens::scaledFor (*this, kPanelDesignH), getHeight() - 60);
    return { getWidth() / 2 - panelW / 2,
             getHeight() / 2 - panelH / 2,
             panelW,
             panelH };
}

void PresetSearchOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0x66000000));

    const auto panel = getPanelBounds().toFloat();
    g.setColour (juce::Colour (0xe60a1628));
    g.fillRoundedRectangle (panel, 10.f);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.35f));
    g.drawRoundedRectangle (panel, 10.f, 1.2f);
}

void PresetSearchOverlay::resized()
{
    auto inner = getPanelBounds().reduced (14);
    searchField.setBounds (inner.removeFromTop (28));
    inner.removeFromTop (10);

    const int catW = juce::roundToInt ((float) inner.getWidth() * 0.34f);
    categoryList->setBounds (inner.removeFromLeft (catW));
    presetList->setBounds (inner);
}

void PresetSearchOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (! getPanelBounds().contains (e.getPosition()))
        dismiss();
}
