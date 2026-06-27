#include "Parameters.h"
#include "Functions.hpp"

template<typename T>
static auto castParameter(const AudioProcessorValueTreeState& tree, 
    const ParameterID* id, T*& dest) -> void {
    dest = dynamic_cast<T*>(tree.getParameter(id->getParamID()));
    jassert(dest != nullptr);
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterFloat* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = paramObj->getDefaultValue();
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterBool* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = paramObj->getDefaultValue();
}

template <typename T>
static auto resetParameter(const AudioProcessorValueTreeState& tree, 
    const AudioParameterChoice* param, T*& dest) -> void {
    auto* paramObj = tree.getParameter(param->getParameterID());
    if (paramObj) *dest = static_cast<T>(paramObj->getDefaultValue());
}

ParameterIDs Parameters::paramIDs = ParameterIDs::loadFromJSON();

Parameters::Parameters(AudioProcessorValueTreeState& tree) : tree(tree) {
    using FloatPair = std::pair<AudioParameterFloat*&, const ParameterID*>;
    using BoolPair = std::pair<AudioParameterBool*&, const ParameterID*>;
    using ChoicePair = std::pair<AudioParameterChoice*&, const ParameterID*>;

    auto floatParameters = std::vector<FloatPair>{
        {frequencyParam, &paramIDs.frequency},
        {resonanceParam, &paramIDs.resonance},
        {filterLFOAmountParam, &paramIDs.filterLFOAmount},
        {filterLFORateParam, &paramIDs.filterLFORate},
    };

    auto boolParameters = std::vector<BoolPair>{
        {filterLFOInvertParam, &paramIDs.filterLFOInvert}
    };

    auto choiceParameters = std::vector<ChoicePair>{
        {filterParam, &paramIDs.filter},
        {slopeParam, &paramIDs.slope},
        {filterLFOTypeParam, &paramIDs.filterLFOType}
    };

    for (const auto& [param, paramID] : floatParameters) {
        castParameter(tree, paramID, param);
    }

    for (auto& [param, paramID] : boolParameters) {
        castParameter(tree, paramID, param);
    }

    for (auto& [param, paramID] : choiceParameters) {
        castParameter(tree, paramID, param);
    }
}

auto Parameters::createParameterLayout() -> AudioProcessorValueTreeState::ParameterLayout {
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterChoice>(
        paramIDs.filter, "Filter Type", StringArray{"lowpass", "highpass", "bandpass"}, 0
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.frequency, "Frequency", NormalisableRange<float>{200.0f, 20000.0f, 1.0f, 0.5f, true}, 20000.0f, 
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayHz)
        .withValueFromStringFunction(Functions::parseHz)
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.resonance, "Resonance", NormalisableRange<float>{0.01f, 2.0f, 0.01f}, 1.0f
    ));

    layout.add(std::make_unique<AudioParameterChoice>(
        paramIDs.slope, "Slope", StringArray{"12 dB", "24 dB", "36 dB", "48 dB"}, 1
    ));

    layout.add(std::make_unique<AudioParameterChoice>(
        paramIDs.filterLFOType, "Filter LFO Type", StringArray{"sawExp", "saw", "sawLog", "triangle", "sine"}, 0
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.filterLFORate, "Filter LFO Rate", NormalisableRange<float>{0.03125f, 4.0f, 0.0001f}, 0.25f,
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayLFORate)
        .withValueFromStringFunction(Functions::parseLFORate)
    ));

    layout.add(std::make_unique<AudioParameterFloat>(
        paramIDs.filterLFOAmount, "Filter LFO Amount", NormalisableRange<float>{0.0f, 1.0f, 0.01f}, 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction(Functions::displayPercent)
        .withValueFromStringFunction(Functions::parsePercent)
    ));

    layout.add(std::make_unique<AudioParameterBool>(
        paramIDs.filterLFOInvert, "Filter LFO Invert", false
    ));

    return layout;
}

auto Parameters::getDefaultParameter(const Array<var>& args,
    WebBrowserComponent::NativeFunctionCompletion completion) -> void {

    auto paramID = args[0].toString();
    auto* param = this->tree.getParameter(paramID);
    float defaultValue = param->convertFrom0to1(param->getDefaultValue());

    completion(defaultValue);
}

auto Parameters::prepareToPlay(double sampleRate, int blockSize) noexcept -> void {
    this->sampleRate = sampleRate;
    this->blockSize = blockSize;

    double duration = 0.001;

    auto smoothers = std::vector{
        &frequencySmoother,
        &resonanceSmoother,
        &filterLFOAmountSmoother
    };

    for (const auto& smoother : smoothers) {
        smoother->reset(this->sampleRate, duration);
    }

    this->filterLFO.prepareToPlay(this->sampleRate);
}

auto Parameters::reset() noexcept -> void {
    auto paramFloats = std::vector{
        std::pair{frequencyParam, &frequency},
        std::pair{resonanceParam, &resonance}
    };

    for (auto& [param, value] : paramFloats) {
        resetParameter(tree, param, value);
    }
    
    auto smoothers = std::vector{
        std::pair{frequencyParam, &frequencySmoother},
        std::pair{resonanceParam, &resonanceSmoother},
        std::pair{filterLFOAmountParam, &filterLFOAmountSmoother}
    };

    for (const auto& [param, smoother] : smoothers) {
        smoother->setCurrentAndTargetValue(param->get());
    }

    this->filterLFO.reset();
}

auto Parameters::setHostInfo(double bpm, double ppq, const AudioPlayHead::TimeSignature& timeSignature) noexcept -> void {
    this->bpm = bpm;
    this->ppq = ppq;
    this->timeSignature = timeSignature;

    if (ppq > 0.0) {
        this->ppq = ppq;
        this->internalPPQ = ppq;
    } else {
        double ppqPerSample = (this->bpm / 60.0) / this->sampleRate;
        this->internalPPQ += ppqPerSample * this->blockSize; 
        this->ppq = this->internalPPQ;
    }

    this->filterLFO.syncToHost(this->bpm, this->ppq, this->timeSignature);
}

auto Parameters::blockUpdate() noexcept -> void {
    auto smoothers = std::vector{
        std::pair{frequencyParam, &frequencySmoother},
        std::pair{resonanceParam, &resonanceSmoother},
        std::pair{filterLFOAmountParam, &filterLFOAmountSmoother}
    };

    for (const auto& [param, smoother] : smoothers) {
        smoother->setTargetValue(param->get());
    }

    this->filterType = this->filterParam->getCurrentChoiceName();
    this->slope = this->slopeParam->getCurrentChoiceName();

    this->filterLFO.setType(this->filterLFOTypeParam->getCurrentChoiceName());
    this->filterLFO.setSyncedRate(this->filterLFORateParam->get());
    this->filterLFO.setPhaseInvert(this->filterLFOInvertParam->get());
}

auto Parameters::update() noexcept -> void {
    this->frequency = frequencySmoother.getNextValue();
    this->resonance = resonanceSmoother.getNextValue();

    float filterLFOValue = this->filterLFO.getSample();
    float filterLFOAmount = this->filterLFOAmountSmoother.getNextValue();
    this->frequency *= jmap(filterLFOValue, -1.0f, 1.0f, 1.0f - filterLFOAmount, 1.0f);
}