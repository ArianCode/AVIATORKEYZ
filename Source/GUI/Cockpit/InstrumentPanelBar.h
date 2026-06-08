#pragma once

#include "AviationGauge.h"
#include <memory>
#include <vector>

/** Bottom instrument strip — 8 aviation gauges in one row. */
class InstrumentPanelBar : public juce::Component
{
public:
    static constexpr int kDesignHeight = 158;

    InstrumentPanelBar() = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::vector<std::unique_ptr<AviationGauge>>& getGauges() { return gauges; }

private:
    std::vector<std::unique_ptr<AviationGauge>> gauges;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentPanelBar)
};
