#pragma once

#include <JuceHeader.h>

class DiodeClipper {
public:
  DiodeClipper() = default;

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = static_cast<float>(spec.sampleRate);
    state.resize(spec.numChannels, 0.0f);
  }

  void reset() { std::fill(state.begin(), state.end(), 0.0f); }

  template <typename ProcessContext>
  void process(const ProcessContext &context) {
    auto &inputBlock = context.getInputBlock();
    auto &outputBlock = context.getOutputBlock();
    auto numChannels = outputBlock.getNumChannels();
    auto numSamples = outputBlock.getNumSamples();

    for (size_t ch = 0; ch < numChannels; ++ch) {
      auto *input = inputBlock.getChannelPointer(ch);
      auto *output = outputBlock.getChannelPointer(ch);

      for (size_t i = 0; i < numSamples; ++i) {
        output[i] = processSample(input[i], ch);
      }
    }
  }

private:
  float sampleRate = 44100.0f;
  std::vector<float> state; // Capacitor voltage for each channel

  // Circuit parameters
  const float R = 2200.0f;     // 2.2k resistor
  const float C = 0.00000001f; // 10nF capacitor

  // Diode parameters (1N4148 approximation)
  const float Is = 2.52e-9f; // Saturation current
  const float Vt = 0.026f;   // Thermal voltage at room temp
  const float eta = 1.752f;  // Ideality factor

  // Diode current (Shockley equation)
  float diodeCurrent(float v) const {
    if (v < -10.0f * eta * Vt)
      return -Is; // Avoid underflow
    if (v > 10.0f * eta * Vt)
      return Is * std::exp(v / (eta * Vt)); // Avoid overflow in exp
    return Is * (std::exp(v / (eta * Vt)) - 1.0f);
  }

  // Derivative of diode current (for Newton-Raphson)
  float diodeCurrentDerivative(float v) const {
    if (v < -10.0f * eta * Vt || v > 10.0f * eta * Vt)
      return 0.0f;
    return (Is / (eta * Vt)) * std::exp(v / (eta * Vt));
  }

  float processSample(float Vin, size_t channel) {
    float dt = 1.0f / sampleRate;
    float Vout = state[channel]; // Initial guess from previous sample

    // Newton-Raphson iterations to solve nonlinear circuit equation
    for (int iter = 0; iter < 8; ++iter) {
      // Net diode current: forward diode - reverse diode
      float Id = diodeCurrent(Vout) - diodeCurrent(-Vout);
      float dId = diodeCurrentDerivative(Vout) + diodeCurrentDerivative(-Vout);

      // KCL: (Vin - Vout)/R = C * dVout/dt + Id
      // Rearranged: f(Vout) = (Vin - Vout)/R - C*(Vout - state)/dt - Id = 0
      float f = (Vin - Vout) / R - C * (Vout - state[channel]) / dt - Id;

      // Derivative of f with respect to Vout
      float df = -1.0f / R - C / dt - dId;

      // Newton-Raphson update
      float delta = f / df;
      Vout = Vout - delta;

      // Check convergence
      if (std::abs(delta) < 1e-6f)
        break;
    }

    state[channel] = Vout;
    return Vout;
  }
};
