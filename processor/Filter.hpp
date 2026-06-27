#pragma once
#include <JuceHeader.h>
#include "BiquadFilter.hpp"

class Filter {
public:
    Filter() = default;
    virtual ~Filter() = default;

    auto prepareToPlay(double sampleRate) -> void {
        auto prepareFilters = [sampleRate](std::array<BiquadFilter, 4>& filters, 
            std::optional<String> type = std::nullopt) -> void {
            for (auto& filter : filters) {
                if (type) filter.setType(*type);
                filter.prepareToPlay(sampleRate);
            }
        };

        prepareFilters(lowpassFilters, "lowpass");
        prepareFilters(highpassFilters, "highpass");
        prepareFilters(bandpassFilters, "bandpass");
    }

    auto reset() -> void {
        auto resetFilters = [](std::array<BiquadFilter, 4>& filters) -> void {
            for (auto& filter : filters) {
                filter.reset();
            }
        };

        resetFilters(lowpassFilters);
        resetFilters(highpassFilters);
        resetFilters(bandpassFilters);
    }

    auto setType(const String& type) -> void {
        this->type = type;
    }

    auto setSlope(const String& slope) -> void {
        if (slope == "12 dB") {
            this->numStages = 1;
        } else if (slope == "24 dB") {
            this->numStages = 2;
        } else if (slope == "36 dB") {
            this->numStages = 3;
        } else {
            this->numStages = 4;
        }
    }

    auto setFrequency(float frequency) -> void {
        auto setFilterFrequency = [frequency](std::array<BiquadFilter, 4>& filters) -> void {
            for (auto& filter : filters) {
                filter.setFrequency(frequency);
            }
        };

        setFilterFrequency(lowpassFilters);
        setFilterFrequency(highpassFilters);
        setFilterFrequency(bandpassFilters);
    }

    auto setResonance(float resonance) -> void {
        auto setFilterResonance = [resonance](std::array<BiquadFilter, 4>& filters) -> void {
            for (auto& filter : filters) {
                filter.setResonance(resonance);
            }
        };

        setFilterResonance(lowpassFilters);
        setFilterResonance(highpassFilters);
        setFilterResonance(bandpassFilters);
    }

    auto getChainResponse(std::array<BiquadFilter, 4>& filters, 
        float frequency) -> std::complex<float> {
        std::complex<float> response{1.0f, 0.0f};

        for (size_t i = 0; i < this->numStages; i++) {
            response *= filters[i].getResponse(frequency);
        }

        return response;
    }

    auto getMagnitude(float frequency) -> float {
        if (type == "lowpass") {
            return std::abs(getChainResponse(lowpassFilters, frequency));
        } else if (type == "highpass") {
            return std::abs(getChainResponse(highpassFilters, frequency));
        } else if (type == "bandpass") {
            return std::abs(getChainResponse(bandpassFilters, frequency));
        }

        return 1.0f;
    }

    auto processChain(std::array<BiquadFilter, 4>& filters, float x) -> float {
        float y = x;

        for (size_t i = 0; i < this->numStages; i++) {
            y = filters[i].processSample(y);
        }

        return y;
    }

    auto processSample(float x) -> float {
        if (type == "lowpass") {
            return this->processChain(lowpassFilters, x);
        } else if (type == "highpass") {
            return this->processChain(highpassFilters, x);
        } else if (type == "bandpass") {
            return this->processChain(bandpassFilters, x);
        }

        return x;
    }

private:
    String type = "lowpass";
    size_t numStages = 1;

    std::array<BiquadFilter, 4> lowpassFilters;
    std::array<BiquadFilter, 4> highpassFilters;
    std::array<BiquadFilter, 4> bandpassFilters;
};