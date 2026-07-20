#include "PresetBrowser.h"
#include "JetsonicIcons.h"
#include "JetsonicTheme.h"

namespace
{
constexpr int kRowH      = 33;
constexpr int kHeaderH   = 40;
constexpr int kSearchH   = 30;
constexpr int kStarZoneW = 34;
} // namespace

// Slim bronze scrollbar for the preset list.
class PresetBrowserPanel::BrowserLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h,
                        bool vertical, int thumbStart, int thumbSize,
                        bool mouseOver, bool mouseDown) override
    {
        g.setColour (juce::Colour (0xff060d15));
        g.fillRect (x, y, w, h);

        juce::Rectangle<float> thumb = vertical
            ? juce::Rectangle<float> ((float) x + 1.0f, (float) thumbStart, (float) w - 2.0f, (float) thumbSize)
            : juce::Rectangle<float> ((float) thumbStart, (float) y + 1.0f, (float) thumbSize, (float) h - 2.0f);

        g.setColour (Jetsonic::bronze().withAlpha (mouseDown ? 1.0f : (mouseOver ? 0.85f : 0.65f)));
        g.fillRoundedRectangle (thumb, 2.5f);
    }
};

PresetBrowserPanel::PresetBrowserPanel()
    : lookAndFeel (std::make_unique<BrowserLookAndFeel>())
{
    searchBox.setMultiLine (false);
    searchBox.setReturnKeyStartsNewLine (false);
    searchBox.setFont (Jetsonic::body (13.0f));
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff050c13));
    searchBox.setColour (juce::TextEditor::textColourId, Jetsonic::textPrimary());
    searchBox.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff1a2a38));
    searchBox.setColour (juce::TextEditor::focusedOutlineColourId, Jetsonic::cyan().withAlpha (0.55f));
    searchBox.setColour (juce::CaretComponent::caretColourId, Jetsonic::cyan());
    searchBox.setTextToShowWhenEmpty ("Search presets...", Jetsonic::textSecondary().withAlpha (0.8f));
    searchBox.setIndents (10, 6);
    searchBox.onTextChange = [this] { applyFilter(); };
    addAndMakeVisible (searchBox);

    listBox.setModel (this);
    listBox.setRowHeight (kRowH);
    listBox.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    listBox.getViewport()->setScrollBarThickness (6);
    listBox.setLookAndFeel (lookAndFeel.get());
    addAndMakeVisible (listBox);
}

PresetBrowserPanel::~PresetBrowserPanel()
{
    listBox.setLookAndFeel (nullptr);
}

void PresetBrowserPanel::setPresets (const juce::String& cat, const juce::StringArray& names)
{
    category = cat;
    allPresets = names;
    applyFilter();
}

void PresetBrowserPanel::setSelectedPreset (const juce::String& name)
{
    selectedName = name;
    const int row = filtered.indexOf (name);
    if (row >= 0)
    {
        listBox.selectRow (row);
        listBox.scrollToEnsureRowIsOnscreen (row);
    }
    else
    {
        listBox.deselectAllRows();
    }
    listBox.repaint();
}

void PresetBrowserPanel::applyFilter()
{
    const auto needle = searchBox.getText().trim();
    filtered.clearQuick();
    for (const auto& name : allPresets)
        if (needle.isEmpty() || name.containsIgnoreCase (needle))
            filtered.add (name);

    listBox.updateContent();
    const int row = filtered.indexOf (selectedName);
    if (row >= 0)
        listBox.selectRow (row);
    listBox.repaint();
}

void PresetBrowserPanel::resized()
{
    auto r = getLocalBounds().reduced (10, 0);
    r.removeFromTop (kHeaderH);
    searchBox.setBounds (r.removeFromTop (kSearchH));
    r.removeFromTop (10);
    r.removeFromBottom (8);
    listBox.setBounds (r);
}

int PresetBrowserPanel::getNumRows()
{
    return filtered.size();
}

void PresetBrowserPanel::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= filtered.size())
        return;

    const auto& name = filtered[row];

    if (selected)
    {
        g.setColour (juce::Colour (0xff0d2233));
        g.fillRect (0, 0, width, height);
        // cyan indicator rail on the left
        g.setColour (Jetsonic::cyan());
        g.fillRect (0, 2, 3, height - 4);
        g.setColour (Jetsonic::cyan().withAlpha (0.25f));
        g.fillRect (3, 2, 2, height - 4);
    }

    const bool fav = isFavourited != nullptr && isFavourited (name);

    g.setFont (Jetsonic::body (12.5f));
    g.setColour (selected ? Jetsonic::cyanBright() : Jetsonic::textSecondary().brighter (0.25f));

    const juce::Rectangle<int> textArea (12, 0, width - kStarZoneW - 12, height);
    const auto width_f = [&g] (const juce::String& s)
    { return juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), s); };
    auto fitted = name;
    if (width_f (fitted) > (float) textArea.getWidth())
    {
        while (fitted.isNotEmpty() && width_f (fitted + "...") > (float) textArea.getWidth())
            fitted = fitted.dropLastCharacters (1);
        fitted += "...";
    }
    g.drawText (fitted, textArea, juce::Justification::centredLeft);

    if (fav || selected)
    {
        const juce::Rectangle<float> starArea ((float) width - 26.0f, (float) height * 0.5f - 8.0f, 16.0f, 16.0f);
        if (fav)
            JetsonicIcons::fill (g, JetsonicIcons::star(), starArea, Jetsonic::goldBright());
        else
            JetsonicIcons::stroke (g, JetsonicIcons::star(), starArea, Jetsonic::gold().withAlpha (0.8f), 1.1f);
    }

    // hairline row separator
    g.setColour (juce::Colour (0x14405a70));
    g.fillRect (6, height - 1, width - 12, 1);
}

void PresetBrowserPanel::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= filtered.size())
        return;

    const auto& name = filtered[row];

    // Star zone toggles favorite without loading the preset.
    if (e.x >= listBox.getWidth() - kStarZoneW)
    {
        if (onFavouriteToggled && isFavourited)
            onFavouriteToggled (name, ! isFavourited (name));
        listBox.repaint();
        return;
    }

    if (onPresetChosen)
        onPresetChosen (name);
}

void PresetBrowserPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // rounded dark metallic panel
    Jetsonic::fillMetalPanel (g, r, 9.0f, juce::Colour (0xff0a141e), juce::Colour (0xff050b12));
    g.setColour (juce::Colour (0xff1b2c3c).withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);
    Jetsonic::topSpecular (g, r, 0.06f);

    // header: CATEGORY / PRESETS
    g.setFont (Jetsonic::label (12.5f, 0.14f));
    g.setColour (Jetsonic::textPrimary().withAlpha (0.92f));
    g.drawText (category.toUpperCase() + " / PRESETS", 14, 6, getWidth() - 28, kHeaderH - 8,
                juce::Justification::centredLeft);

}

void PresetBrowserPanel::paintOverChildren (juce::Graphics& g)
{
    // magnifier icon inside the search box (children paint before this)
    const auto sb = searchBox.getBounds().toFloat();
    JetsonicIcons::fill (g, JetsonicIcons::magnifier(),
                         { sb.getRight() - 24.0f, sb.getCentreY() - 7.0f, 14.0f, 14.0f },
                         Jetsonic::textSecondary());
}
