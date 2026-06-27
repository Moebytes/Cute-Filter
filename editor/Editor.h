#pragma once
#pragma clang diagnostic ignored "-Wshadow-field"
#include <JuceHeader.h>
#include "Processor.h"
#include "EventEmitter.hpp"

class Editor : public AudioProcessorEditor, public EventEmitter::Listener, private Timer {
public:
    Editor(Processor& p);
    ~Editor() override;
    
    auto resized() -> void override;

    auto getResource(const String& url) -> std::optional<WebBrowserComponent::Resource>;
    auto webviewOptions() -> WebBrowserComponent::Options;
    auto getWebviewFileBytes(const String& resourceStr) -> std::vector<std::byte>;
    
    auto openFilterSelector(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    auto handleEvent(const String& name, const var& payload) -> void override;
    auto handleThemeChange(const String& theme) -> void;

    auto timerCallback() -> void override;
        
private:
    Processor& processor;
    ComponentBoundsConstrainer constrainer;

    WebComboBoxRelay filterRelay {Parameters::paramIDs.filter.getParamID()};
    WebComboBoxParameterAttachment filterAttachment {*this->processor.parameters.filterParam, filterRelay, nullptr};

    WebSliderRelay frequencyRelay {Parameters::paramIDs.frequency.getParamID()};
    WebSliderParameterAttachment frequencyAttachment {*this->processor.parameters.frequencyParam, frequencyRelay, nullptr};

    WebSliderRelay resonanceRelay {Parameters::paramIDs.resonance.getParamID()};
    WebSliderParameterAttachment resonanceAttachment {*this->processor.parameters.resonanceParam, resonanceRelay, nullptr};

    WebComboBoxRelay slopeRelay {Parameters::paramIDs.slope.getParamID()};
    WebComboBoxParameterAttachment slopeAttachment {*this->processor.parameters.slopeParam, slopeRelay, nullptr};

    WebComboBoxRelay filterLFOTypeRelay {Parameters::paramIDs.filterLFOType.getParamID()};
    WebComboBoxParameterAttachment filterLFOTypeAttachment {*this->processor.parameters.filterLFOTypeParam, filterLFOTypeRelay, nullptr};
    WebSliderRelay filterLFORateRelay {Parameters::paramIDs.filterLFORate.getParamID()};
    WebSliderParameterAttachment filterLFORateAttachment {*this->processor.parameters.filterLFORateParam, filterLFORateRelay, nullptr};
    WebSliderRelay filterLFOAmountRelay {Parameters::paramIDs.filterLFOAmount.getParamID()};
    WebSliderParameterAttachment filterLFOAmountAttachment {*this->processor.parameters.filterLFOAmountParam, filterLFOAmountRelay, nullptr};
    WebToggleButtonRelay filterLFOInvertRelay {Parameters::paramIDs.filterLFOInvert.getParamID()};
    WebToggleButtonParameterAttachment filterLFOInvertAttachment {*this->processor.parameters.filterLFOInvertParam, filterLFOInvertRelay, nullptr};

    WebBrowserComponent webview;
    bool webviewBackendReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};