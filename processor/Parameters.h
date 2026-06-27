#pragma once
#include <JuceHeader.h>
#include "ParameterIDs.hpp"
#include "LFO.hpp"

class Parameters {
public:
    Parameters(AudioProcessorValueTreeState& tree);
    ~Parameters() = default;

    static auto createParameterLayout() -> AudioProcessorValueTreeState::ParameterLayout;

    auto prepareToPlay(double sampleRate, int blockSize) noexcept -> void;
    auto reset() noexcept -> void;
    auto init() noexcept -> void;
    auto blockUpdate() noexcept -> void;
    auto update() noexcept -> void;
    auto setHostInfo(double bpm, double ppq, const AudioPlayHead::TimeSignature& timeSignature) noexcept -> void;

    auto getDefaultParameter(const Array<var>& args, 
        WebBrowserComponent::NativeFunctionCompletion completion) -> void;

    static ParameterIDs paramIDs;

    String filterType = "lowpass";
    AudioParameterChoice* filterParam;

    float frequency = 20000.0f;
    AudioParameterFloat* frequencyParam;

    float resonance = 0.6f;
    AudioParameterFloat* resonanceParam;

    String slope = "24 dB";
    AudioParameterChoice* slopeParam;

    LFO filterLFO;
    AudioParameterChoice* filterLFOTypeParam;
    AudioParameterFloat* filterLFORateParam;
    AudioParameterFloat*  filterLFOAmountParam;
    AudioParameterBool*  filterLFOInvertParam;
private:
    AudioProcessorValueTreeState& tree;

    LinearSmoothedValue<float> frequencySmoother;
    LinearSmoothedValue<float> resonanceSmoother;
    LinearSmoothedValue<float> filterLFOAmountSmoother;

    double sampleRate = 44100.0;
    int blockSize = 512;
    double bpm = 150.0;
    double ppq = 0.0;
    double internalPPQ = 0.0;
    AudioPlayHead::TimeSignature timeSignature{4, 4};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Parameters)
};