#pragma once
#include <cmath>
#include <complex>
#include <JuceHeader.h>

class BiquadFilter {
public:
    BiquadFilter() = default;
    virtual ~BiquadFilter() = default;

    auto prepareToPlay(double sampleRate) -> void {
        this->sampleRate = sampleRate;
        this->reset();
        this->needsUpdate = true;
    }

    auto reset() -> void {
        this->x1 = 0.0f;
        this->x2 = 0.0f;
        this->y1 = 0.0f; 
        this->y2 = 0.0f;
    }

    auto setType(const String& type) -> void {
        this->type = type;
        this->needsUpdate = true;
    }

    auto setFrequency(float frequency) -> void {
        if (!approximatelyEqual(this->cutoff, frequency)) this->needsUpdate = true;
        this->cutoff = frequency;
    }

    auto setResonance(float resonance) -> void {
        if (!approximatelyEqual(this->q, resonance)) this->needsUpdate = true;
        this->q = std::max(0.1f, resonance);
    }

    auto getResponse(float frequency) -> std::complex<float> {
        this->update();

        float w = 2.0f * MathConstants<float>::pi * frequency / static_cast<float>(this->sampleRate);

        std::complex<float> z1(std::cos(w), -std::sin(w));
        std::complex<float> z2(std::cos(2.0f * w), -std::sin(2.0f * w));

        std::complex<float> numerator = b0 + b1 * z1 + b2 * z2;
        std::complex<float> denominator = 1.0f + a1 * z1 + a2 * z2;

        return numerator / denominator;
    }

    auto update() -> void {
        if (!this->needsUpdate) return;

        this->cutoff = std::clamp(this->cutoff, 20.0f, static_cast<float>(this->sampleRate * 0.49));

        float w0 = 2.0f * MathConstants<float>::pi * this->cutoff / static_cast<float>(this->sampleRate);

        float cosw = std::cos(w0);
        float sinw = std::sin(w0);

        float alpha = sinw / (2.0f * this->q);
        float a0 = 1.0f + alpha;

        if (this->type == "lowpass") {
            this->b0 = (1.0f - cosw) * 0.5f / a0;
            this->b1 = (1.0f - cosw) / a0;
            this->b2 = (1.0f - cosw) * 0.5f / a0;

            this->a1 = (-2.0f * cosw) / a0;
            this->a2 = (1.0f - alpha) / a0;

        } else if (this->type == "highpass") {
            this->b0 = (1.0f + cosw) * 0.5f / a0;
            this->b1 = -(1.0f + cosw) / a0;
            this->b2 =  (1.0f + cosw) * 0.5f / a0;

            this->a1 = (-2.0f * cosw) / a0;
            this->a2 = (1.0f - alpha) / a0;

        } else if (this->type == "bandpass") {
            this->b0 = sinw * 0.5f / a0;
            this->b1 = 0.0f;
            this->b2 = -sinw * 0.5f / a0;

            this->a1 = (-2.0f * cosw) / a0;
            this->a2 = (1.0f - alpha) / a0;

        }

        this->needsUpdate = false;
    }

    auto processSample(float x) -> float {
        this->update();

        float y = this->b0 * x + 
            this->b1 * this->x1 + 
            this->b2 * this->x2 - 
            this->a1 * this->y1 - 
            this->a2 * this->y2;

        this->x2 = this->x1;
        this->x1 = x;

        this->y2 = this->y1;
        this->y1 = y;

        return y;
    }

private:
    String type = "lowpass";

    double sampleRate = 44100.0;
    bool needsUpdate = false;

    float cutoff = 20000.0f;
    float q = 0.0f;

    float b0 = 0.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;

    float a1 = 0.0f;
    float a2 = 0.0f;

    float x1 = 0.0f;
    float x2 = 0.0f;

    float y1 = 0.0f;
    float y2 = 0.0f;
};