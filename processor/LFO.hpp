#pragma once
#include <cmath>
#include <JuceHeader.h>

using TimeSignature = AudioPlayHead::TimeSignature;

class LFO {
public:
    LFO() = default;
    virtual ~LFO() = default;

    auto prepareToPlay(double sampleRate) -> void {
        this->sampleRate = sampleRate;
        reset();
    }

    auto reset() -> void {
        this->phase = 0.0f;
    }

    auto setType(const String& type) -> void {
        this->type = type;
    }

    auto setHzRate(float frequency) -> void {
        this->frequency = frequency;
    }

    auto setPhaseInvert(bool phaseInvert) -> void {
        this->phaseInvert = phaseInvert;
    }

    auto setSyncedRate(float syncedRate) -> void {
        float timeScale = static_cast<float>(this->timeSignature.numerator) / static_cast<float>(this->timeSignature.denominator);
        this->syncedBeats = static_cast<float>(syncedRate) * 4.0f * timeScale;
    }

    auto syncToHost(double bpm, double ppq, const TimeSignature& timeSignature) -> void {
        this->bpm = bpm;
        this->timeSignature = timeSignature;

        this->syncPPQ(ppq);
    }

    auto syncPPQ(double ppq) -> void {
        if (syncedBeats > 0.0f) {
            double positionInCycle = std::fmod(ppq, syncedBeats);
            phase = static_cast<float>(positionInCycle / syncedBeats);
        }
    }

    auto getSample() -> float {
        float value = renderWaveform(this->phase);
        if (this->phaseInvert) value *= -1.0f;

        return value;
    }

    auto renderWaveform(float pos) -> float {
        float tension = 5.0f;

        if (this->type == "saw") {
            return 1.0f - 2.0f * pos;
        } else if (this->type == "sawLog") {
            float expT = (std::exp(tension * pos) - 1.0f) / (std::exp(tension) - 1.0f);
            return 1.0f - 2.0f * expT;
        } else if (this->type == "sawExp") {
            float logT = std::log(1.0f + (std::exp(tension) - 1.0f) * pos) / tension;
            return 1.0f - 2.0f * logT;
        } else if (this->type == "triangle") {
            return 4.0f * std::abs(pos - 0.5f) - 1.0f;
        } else if (this->type == "sine") {
            return std::sin(pos * MathConstants<float>::twoPi);
        }

        return 0.0f;
    }

private:
    String type = "saw";

    double sampleRate = 44100.0;
    double bpm = 150.0;
    TimeSignature timeSignature{4, 4};

    float frequency = 1.0f;
    float syncedBeats = 1.0f;
    float phase = 0.0f;
    bool phaseInvert = false;
};