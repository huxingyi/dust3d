/*
 *  Copyright (c) 2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <algorithm>
#include <cmath>
#include <cstring>
#include <dust3d/animation/sound_generator.h>

namespace dust3d {

SurfaceSynthParams getSurfaceSynthParams(SurfaceMaterial material)
{
    SurfaceSynthParams p;
    switch (material) {
    case SurfaceMaterial::Stone:
        // Hard flat surface — sharp heel strike thud, minimal deformation, solid ground modes
        p.filterCutoff = 4500.0f;
        p.resonance = 1.2f;
        p.brightness = 0.5f;
        p.pitchVariation = 0.05f;
        p.heelStrikeAttack = 0.002f;
        p.heelStrikeDecay = 0.020f;
        p.heelStrikeCutoff = 2200.0f;
        p.heelStrikeWeight = 0.65f;
        p.deformationOnset = 0.005f;
        p.deformationDuration = 0.03f;
        p.deformationCutoff = 3500.0f;
        p.deformationGranularity = 0.1f;
        p.deformationWeight = 0.2f;
        p.weightTransferDuration = 0.06f;
        p.weightTransferLowFreq = 70.0f;
        p.weightTransferWeight = 0.4f;
        p.toeOffDelay = 0.0f;
        p.toeOffWeight = 0.0f;
        p.granularDensity = 0.15f;
        p.granularDecay = 0.025f;
        p.granularBandLow = 1500.0f;
        p.granularBandHigh = 5000.0f;
        p.earlyReflectionDelay = 0.0005f;
        p.earlyReflectionGain = 0.25f;
        p.groundModeCount = 3;
        p.groundModes[0] = { 70.0f, 1.0f, 0.05f, 2.5f };
        p.groundModes[1] = { 180.0f, 0.5f, 0.03f, 3.0f };
        p.groundModes[2] = { 400.0f, 0.2f, 0.02f, 4.0f };
        p.massScale = 1.0f;
        p.padSoftness = 0.3f;
        break;
    case SurfaceMaterial::Wood:
        // Hollow resonant surface — distinct thud with tonal ring, slight creak
        p.filterCutoff = 3000.0f;
        p.resonance = 1.8f;
        p.brightness = 0.35f;
        p.pitchVariation = 0.08f;
        p.heelStrikeAttack = 0.002f;
        p.heelStrikeDecay = 0.030f;
        p.heelStrikeCutoff = 1800.0f;
        p.heelStrikeWeight = 0.55f;
        p.deformationOnset = 0.006f;
        p.deformationDuration = 0.04f;
        p.deformationCutoff = 2500.0f;
        p.deformationGranularity = 0.08f;
        p.deformationWeight = 0.15f;
        p.weightTransferDuration = 0.08f;
        p.weightTransferLowFreq = 55.0f;
        p.weightTransferWeight = 0.35f;
        p.toeOffDelay = 0.0f;
        p.toeOffWeight = 0.0f;
        p.granularDensity = 0.08f;
        p.granularDecay = 0.03f;
        p.granularBandLow = 1000.0f;
        p.granularBandHigh = 3500.0f;
        p.earlyReflectionDelay = 0.0008f;
        p.earlyReflectionGain = 0.30f; // wood reflects well
        p.groundModeCount = 4;
        p.groundModes[0] = { 55.0f, 0.8f, 0.08f, 4.0f }; // hollow resonance
        p.groundModes[1] = { 150.0f, 0.6f, 0.06f, 5.0f };
        p.groundModes[2] = { 330.0f, 0.35f, 0.04f, 6.0f };
        p.groundModes[3] = { 600.0f, 0.15f, 0.025f, 4.0f };
        p.massScale = 1.0f;
        p.padSoftness = 0.3f;
        break;
    case SurfaceMaterial::Sand:
        // Loose granular — slow compression, heavy crunch, muffled thud
        p.filterCutoff = 2000.0f;
        p.resonance = 0.4f;
        p.brightness = 0.15f;
        p.pitchVariation = 0.18f;
        p.heelStrikeAttack = 0.006f; // slow — foot sinks in
        p.heelStrikeDecay = 0.040f;
        p.heelStrikeCutoff = 1200.0f;
        p.heelStrikeWeight = 0.35f;
        p.deformationOnset = 0.003f;
        p.deformationDuration = 0.10f; // long compression
        p.deformationCutoff = 2500.0f;
        p.deformationGranularity = 0.9f; // very crunchy
        p.deformationWeight = 0.55f; // dominant layer
        p.weightTransferDuration = 0.10f;
        p.weightTransferLowFreq = 45.0f;
        p.weightTransferWeight = 0.2f;
        p.toeOffDelay = 0.12f;
        p.toeOffDuration = 0.06f;
        p.toeOffCutoff = 2000.0f;
        p.toeOffWeight = 0.15f; // sand scrape on lift
        p.granularDensity = 0.95f;
        p.granularDecay = 0.10f;
        p.granularBandLow = 600.0f;
        p.granularBandHigh = 3500.0f;
        p.earlyReflectionDelay = 0.002f;
        p.earlyReflectionGain = 0.05f; // sand absorbs
        p.groundModeCount = 2;
        p.groundModes[0] = { 40.0f, 0.4f, 0.06f, 1.5f };
        p.groundModes[1] = { 120.0f, 0.15f, 0.03f, 1.5f };
        p.massScale = 1.0f;
        p.padSoftness = 0.5f;
        break;
    case SurfaceMaterial::Metal:
        // Hard resonant — sharp click, long ring, bright
        p.filterCutoff = 8000.0f;
        p.resonance = 3.0f;
        p.brightness = 0.8f;
        p.pitchVariation = 0.03f;
        p.heelStrikeAttack = 0.001f;
        p.heelStrikeDecay = 0.015f;
        p.heelStrikeCutoff = 3500.0f;
        p.heelStrikeWeight = 0.5f;
        p.deformationOnset = 0.003f;
        p.deformationDuration = 0.02f;
        p.deformationCutoff = 5000.0f;
        p.deformationGranularity = 0.05f;
        p.deformationWeight = 0.1f;
        p.weightTransferDuration = 0.05f;
        p.weightTransferLowFreq = 80.0f;
        p.weightTransferWeight = 0.3f;
        p.toeOffDelay = 0.0f;
        p.toeOffWeight = 0.0f;
        p.granularDensity = 0.05f;
        p.granularDecay = 0.015f;
        p.granularBandLow = 2000.0f;
        p.granularBandHigh = 8000.0f;
        p.earlyReflectionDelay = 0.0003f;
        p.earlyReflectionGain = 0.35f;
        p.groundModeCount = 4;
        p.groundModes[0] = { 200.0f, 1.0f, 0.20f, 15.0f }; // metal rings long
        p.groundModes[1] = { 530.0f, 0.6f, 0.15f, 20.0f };
        p.groundModes[2] = { 1100.0f, 0.35f, 0.10f, 18.0f };
        p.groundModes[3] = { 2400.0f, 0.15f, 0.06f, 12.0f };
        p.massScale = 1.0f;
        p.padSoftness = 0.2f;
        break;
    case SurfaceMaterial::Grass:
        // Soft organic — gentle thud, rustling texture, quiet
        p.filterCutoff = 1800.0f;
        p.resonance = 0.3f;
        p.brightness = 0.12f;
        p.pitchVariation = 0.22f;
        p.heelStrikeAttack = 0.005f;
        p.heelStrikeDecay = 0.035f;
        p.heelStrikeCutoff = 1000.0f;
        p.heelStrikeWeight = 0.4f;
        p.deformationOnset = 0.004f;
        p.deformationDuration = 0.08f;
        p.deformationCutoff = 2000.0f;
        p.deformationGranularity = 0.5f; // grass blade rustle
        p.deformationWeight = 0.4f;
        p.weightTransferDuration = 0.10f;
        p.weightTransferLowFreq = 40.0f;
        p.weightTransferWeight = 0.25f;
        p.toeOffDelay = 0.10f;
        p.toeOffDuration = 0.05f;
        p.toeOffCutoff = 1800.0f;
        p.toeOffWeight = 0.12f; // subtle grass displacement on lift
        p.granularDensity = 0.65f;
        p.granularDecay = 0.08f;
        p.granularBandLow = 500.0f;
        p.granularBandHigh = 3000.0f;
        p.earlyReflectionDelay = 0.003f;
        p.earlyReflectionGain = 0.05f; // grass absorbs
        p.groundModeCount = 2;
        p.groundModes[0] = { 35.0f, 0.3f, 0.06f, 1.5f };
        p.groundModes[1] = { 100.0f, 0.1f, 0.03f, 1.5f };
        p.massScale = 1.0f;
        p.padSoftness = 0.6f;
        break;
    case SurfaceMaterial::Water:
        // Liquid — splash, bubble, slosh
        p.filterCutoff = 3500.0f;
        p.resonance = 1.2f;
        p.brightness = 0.4f;
        p.pitchVariation = 0.3f;
        p.heelStrikeAttack = 0.003f;
        p.heelStrikeDecay = 0.025f;
        p.heelStrikeCutoff = 1500.0f;
        p.heelStrikeWeight = 0.35f;
        p.deformationOnset = 0.002f;
        p.deformationDuration = 0.12f; // splash sustain
        p.deformationCutoff = 3000.0f;
        p.deformationGranularity = 0.6f; // bubble/droplet texture
        p.deformationWeight = 0.45f;
        p.weightTransferDuration = 0.15f;
        p.weightTransferLowFreq = 50.0f;
        p.weightTransferWeight = 0.25f;
        p.toeOffDelay = 0.10f;
        p.toeOffDuration = 0.08f;
        p.toeOffCutoff = 2500.0f;
        p.toeOffWeight = 0.2f; // water displacement on lift
        p.granularDensity = 0.5f;
        p.granularDecay = 0.12f;
        p.granularBandLow = 800.0f;
        p.granularBandHigh = 5000.0f;
        p.earlyReflectionDelay = 0.001f;
        p.earlyReflectionGain = 0.2f;
        p.groundModeCount = 3;
        p.groundModes[0] = { 60.0f, 0.5f, 0.10f, 2.0f };
        p.groundModes[1] = { 180.0f, 0.3f, 0.06f, 3.0f };
        p.groundModes[2] = { 500.0f, 0.15f, 0.03f, 2.5f };
        p.massScale = 1.0f;
        p.padSoftness = 0.5f;
        break;
    case SurfaceMaterial::Dirt:
        // Packed earth — moderate thud, some grit, earthy
        p.filterCutoff = 2500.0f;
        p.resonance = 0.6f;
        p.brightness = 0.22f;
        p.pitchVariation = 0.12f;
        p.heelStrikeAttack = 0.003f;
        p.heelStrikeDecay = 0.030f;
        p.heelStrikeCutoff = 1400.0f;
        p.heelStrikeWeight = 0.5f;
        p.deformationOnset = 0.005f;
        p.deformationDuration = 0.06f;
        p.deformationCutoff = 2800.0f;
        p.deformationGranularity = 0.45f;
        p.deformationWeight = 0.35f;
        p.weightTransferDuration = 0.08f;
        p.weightTransferLowFreq = 50.0f;
        p.weightTransferWeight = 0.3f;
        p.toeOffDelay = 0.08f;
        p.toeOffDuration = 0.04f;
        p.toeOffCutoff = 2200.0f;
        p.toeOffWeight = 0.1f;
        p.granularDensity = 0.5f;
        p.granularDecay = 0.06f;
        p.granularBandLow = 700.0f;
        p.granularBandHigh = 3500.0f;
        p.earlyReflectionDelay = 0.001f;
        p.earlyReflectionGain = 0.15f;
        p.groundModeCount = 3;
        p.groundModes[0] = { 50.0f, 0.6f, 0.06f, 2.0f };
        p.groundModes[1] = { 140.0f, 0.3f, 0.04f, 2.5f };
        p.groundModes[2] = { 350.0f, 0.12f, 0.025f, 2.0f };
        p.massScale = 1.0f;
        p.padSoftness = 0.45f;
        break;
    case SurfaceMaterial::Snow:
        // Compressible crystalline — crunch + muffled thud, very soft
        p.filterCutoff = 1400.0f;
        p.resonance = 0.2f;
        p.brightness = 0.08f;
        p.pitchVariation = 0.25f;
        p.heelStrikeAttack = 0.008f; // very slow — snow compresses
        p.heelStrikeDecay = 0.050f;
        p.heelStrikeCutoff = 800.0f;
        p.heelStrikeWeight = 0.3f;
        p.deformationOnset = 0.003f;
        p.deformationDuration = 0.12f; // long crunch
        p.deformationCutoff = 2000.0f;
        p.deformationGranularity = 0.85f; // ice crystal crunch
        p.deformationWeight = 0.55f; // dominant = crunch
        p.weightTransferDuration = 0.12f;
        p.weightTransferLowFreq = 30.0f;
        p.weightTransferWeight = 0.15f;
        p.toeOffDelay = 0.12f;
        p.toeOffDuration = 0.06f;
        p.toeOffCutoff = 1500.0f;
        p.toeOffWeight = 0.1f;
        p.granularDensity = 0.9f;
        p.granularDecay = 0.10f;
        p.granularBandLow = 400.0f;
        p.granularBandHigh = 2500.0f;
        p.earlyReflectionDelay = 0.004f;
        p.earlyReflectionGain = 0.03f; // snow absorbs everything
        p.groundModeCount = 1;
        p.groundModes[0] = { 30.0f, 0.2f, 0.08f, 1.2f };
        p.massScale = 1.0f;
        p.padSoftness = 0.7f;
        break;
    }
    return p;
}

std::string surfaceMaterialName(SurfaceMaterial material)
{
    switch (material) {
    case SurfaceMaterial::Stone:
        return "Stone";
    case SurfaceMaterial::Wood:
        return "Wood";
    case SurfaceMaterial::Sand:
        return "Sand";
    case SurfaceMaterial::Metal:
        return "Metal";
    case SurfaceMaterial::Grass:
        return "Grass";
    case SurfaceMaterial::Water:
        return "Water";
    case SurfaceMaterial::Dirt:
        return "Dirt";
    case SurfaceMaterial::Snow:
        return "Snow";
    }
    return "Stone";
}

SurfaceMaterial surfaceMaterialFromName(const std::string& name)
{
    if (name == "Wood")
        return SurfaceMaterial::Wood;
    if (name == "Sand")
        return SurfaceMaterial::Sand;
    if (name == "Metal")
        return SurfaceMaterial::Metal;
    if (name == "Grass")
        return SurfaceMaterial::Grass;
    if (name == "Water")
        return SurfaceMaterial::Water;
    if (name == "Dirt")
        return SurfaceMaterial::Dirt;
    if (name == "Snow")
        return SurfaceMaterial::Snow;
    return SurfaceMaterial::Stone;
}

// Simple xorshift32 PRNG for deterministic noise
static uint32_t xorshift32(uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static float randomFloat(uint32_t& state)
{
    return static_cast<float>(xorshift32(state)) / static_cast<float>(UINT32_MAX);
}

void SoundGenerator::synthesizeImpact(
    std::vector<float>& buffer,
    int sampleRate,
    float eventTime,
    float intensity,
    const SurfaceSynthParams& params,
    uint32_t seed)
{
    constexpr float PI = 3.14159265358979f;
    constexpr float TWO_PI = 6.28318530717959f;
    uint32_t rng = seed;

    float pitchOffset = 1.0f + (randomFloat(rng) - 0.5f) * 2.0f * params.pitchVariation;
    float cutoff = params.filterCutoff * pitchOffset;
    float dt = 1.0f / sampleRate;

    int startSample = static_cast<int>(eventTime * sampleRate);
    if (startSample < 0)
        startSample = 0;

    // Compute total duration from all phases + ground mode ring
    float totalTime = params.heelStrikeAttack + params.heelStrikeDecay;
    float deformEnd = params.deformationOnset + params.deformationDuration;
    if (deformEnd > totalTime)
        totalTime = deformEnd;
    float weightEnd = deformEnd + params.weightTransferDuration;
    if (weightEnd > totalTime)
        totalTime = weightEnd;
    float toeEnd = params.toeOffDelay + params.toeOffDuration;
    if (toeEnd > totalTime)
        totalTime = toeEnd;
    // Add ground mode ring-out
    float maxModeDecay = 0.0f;
    for (int m = 0; m < params.groundModeCount; ++m) {
        if (params.groundModes[m].decay > maxModeDecay)
            maxModeDecay = params.groundModes[m].decay;
    }
    totalTime += maxModeDecay * 3.0f;
    totalTime = std::max(totalTime, 0.05f);

    int totalSamples = static_cast<int>(totalTime * sampleRate);
    int endSample = std::min(startSample + totalSamples, static_cast<int>(buffer.size()));
    if (endSample <= startSample)
        return;

    // Softness factor from pad softness — slows attack, lowers brightness
    float softFactor = 1.0f + params.padSoftness * 2.0f; // 1.0 = hard, 3.0 = very soft

    // ---- Early reflection delay line ----
    int reflectionDelaySamples = std::max(1, static_cast<int>(params.earlyReflectionDelay * sampleRate));
    std::vector<float> delayLine(reflectionDelaySamples, 0.0f);
    int delayWritePos = 0;

    // ---- State-variable filter (shared across phases) ----
    float svfLow = 0.0f, svfBand = 0.0f;
    float svfF = 2.0f * sinf(PI * std::min(cutoff, sampleRate * 0.49f) / sampleRate);
    float svfQ = 1.0f / std::max(params.resonance, 0.1f);

    // ---- Ground mode resonators ----
    struct ModalState {
        float y1 = 0.0f, y2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float gain = 0.0f;
    };
    ModalState groundModeStates[4];
    for (int m = 0; m < params.groundModeCount && m < 4; ++m) {
        float freq = params.groundModes[m].frequency * pitchOffset;
        float bw = freq / std::max(params.groundModes[m].q, 1.0f);
        float r = expf(-PI * bw * dt);
        float theta = TWO_PI * freq * dt;
        groundModeStates[m].a1 = 2.0f * r * cosf(theta);
        groundModeStates[m].a2 = -(r * r);
        groundModeStates[m].gain = params.groundModes[m].amplitude * (1.0f - r);
    }

    // ---- Velvet noise schedule for granular texture (deformation + toe-off) ----
    std::vector<std::pair<int, float>> velvetImpulses;
    if (params.granularDensity > 0.0f) {
        float density = 300.0f + params.granularDensity * 3000.0f;
        float granDuration = params.deformationDuration + params.granularDecay;
        int granSamples = static_cast<int>(granDuration * sampleRate);
        int numImpulses = static_cast<int>(density * granDuration);
        velvetImpulses.reserve(numImpulses);
        uint32_t vrng = seed ^ 0xDEADBEEF;
        for (int j = 0; j < numImpulses; ++j) {
            int pos = static_cast<int>(randomFloat(vrng) * granSamples);
            float sign = (randomFloat(vrng) > 0.5f) ? 1.0f : -1.0f;
            float amp = sign * (0.5f + 0.5f * randomFloat(vrng));
            velvetImpulses.push_back({ pos, amp });
        }
    }

    // ---- Bandpass filter state for granular texture ----
    float bpLow = 0.0f, bpBand = 0.0f;
    float bpF = 2.0f * sinf(PI * std::min(params.granularBandHigh, sampleRate * 0.49f) / sampleRate);

    // ---- Phase timing (in samples relative to start) ----
    int heelAttackSamples = std::max(1, static_cast<int>(params.heelStrikeAttack * softFactor * sampleRate));
    int heelDecaySamples = static_cast<int>(params.heelStrikeDecay * sampleRate);
    int deformOnsetSample = static_cast<int>(params.deformationOnset * sampleRate);
    int deformDurationSamples = static_cast<int>(params.deformationDuration * sampleRate);
    int weightStartSample = deformOnsetSample + deformDurationSamples / 2;
    int weightDurationSamples = static_cast<int>(params.weightTransferDuration * sampleRate);
    int toeOffStartSample = static_cast<int>(params.toeOffDelay * sampleRate);
    int toeOffDurationSamples = static_cast<int>(params.toeOffDuration * sampleRate);

    // ---- Filter states for deformation, toe-off, air absorption ----
    float deformLp = 0.0f;
    float toeLp = 0.0f;
    float absLp = 0.0f;

    // ---- Per-sample synthesis ----
    for (int i = startSample; i < endSample; ++i) {
        float t = static_cast<float>(i - startSample) * dt;
        int ls = i - startSample; // local sample
        float output = 0.0f;

        // == PHASE 1: HEEL/PAW STRIKE ==
        // Soft onset burst — the initial contact of pad/hoof/paw with ground
        {
            float heelEnv = 0.0f;
            if (ls < heelAttackSamples) {
                // Smooth rise (sine curve for organic feel)
                float phase = static_cast<float>(ls) / heelAttackSamples;
                heelEnv = sinf(phase * PI * 0.5f);
            } else if (ls < heelAttackSamples + heelDecaySamples) {
                float decayT = static_cast<float>(ls - heelAttackSamples) / sampleRate;
                heelEnv = expf(-decayT / params.heelStrikeDecay * 4.0f);
            }
            if (heelEnv > 0.001f) {
                float noise = randomFloat(rng) * 2.0f - 1.0f;
                // Lowpass at heel strike cutoff for muffled paw thud
                float hCut = params.heelStrikeCutoff * pitchOffset / softFactor;
                float hF = 2.0f * sinf(PI * std::min(hCut, sampleRate * 0.49f) / sampleRate);
                svfLow += hF * svfBand;
                float svfHigh = noise - svfLow - svfQ * svfBand;
                svfBand += hF * svfHigh;
                output += svfLow * heelEnv * params.heelStrikeWeight;
            }
        }

        // == PHASE 2: GROUND DEFORMATION ==
        // Granular crunch/rustle as the surface compresses under the foot
        {
            int deformLocal = ls - deformOnsetSample;
            if (deformLocal >= 0 && deformLocal < deformDurationSamples) {
                float deformPhase = static_cast<float>(deformLocal) / deformDurationSamples;
                // Bell-shaped envelope (peaks in middle of compression)
                float deformEnv = sinf(deformPhase * PI);

                // Granular texture via velvet noise
                float velvetSum = 0.0f;
                for (const auto& imp : velvetImpulses) {
                    if (imp.first == deformLocal) {
                        velvetSum += imp.second;
                    }
                }

                // Bandpass the granular texture
                float granNoise = velvetSum * params.deformationGranularity;
                bpLow += bpF * bpBand;
                float bpHigh = granNoise - bpLow - 0.5f * bpBand;
                bpBand += bpF * bpHigh;
                float bandpassed = bpBand;

                // Also add filtered broadband for body of deformation
                float bodyNoise = randomFloat(rng) * 2.0f - 1.0f;
                float dCut = params.deformationCutoff * pitchOffset;
                float dF = 2.0f * sinf(PI * std::min(dCut, sampleRate * 0.49f) / sampleRate);
                // Simple one-pole lowpass for deformation body
                deformLp += dF * (bodyNoise - deformLp);

                output += (bandpassed + deformLp * (1.0f - params.deformationGranularity))
                    * deformEnv * params.deformationWeight;
            }
        }

        // == PHASE 3: WEIGHT TRANSFER ==
        // Deep sub-bass rumble as body mass settles onto the foot
        {
            int weightLocal = ls - weightStartSample;
            if (weightLocal >= 0 && weightLocal < weightDurationSamples) {
                float weightPhase = static_cast<float>(weightLocal) / weightDurationSamples;
                // Smooth envelope with slow release
                float weightEnv = sinf(weightPhase * PI) * (1.0f - weightPhase * 0.5f);

                // Sub-bass oscillation
                float subFreq = params.weightTransferLowFreq * params.massScale;
                float subOsc = sinf(TWO_PI * subFreq * weightLocal * dt);
                // Add second harmonic for body
                float subOsc2 = sinf(TWO_PI * subFreq * 2.0f * weightLocal * dt) * 0.3f;

                output += (subOsc + subOsc2) * weightEnv * params.weightTransferWeight;
            }
        }

        // == PHASE 4: TOE-OFF / LIFT ==
        // Subtle scrape or displacement as the foot lifts away
        if (params.toeOffWeight > 0.001f) {
            int toeLocal = ls - toeOffStartSample;
            if (toeLocal >= 0 && toeLocal < toeOffDurationSamples) {
                float toePhase = static_cast<float>(toeLocal) / toeOffDurationSamples;
                // Fade-out envelope
                float toeEnv = 1.0f - toePhase;
                toeEnv *= toeEnv; // quadratic fade

                float toeNoise = randomFloat(rng) * 2.0f - 1.0f;
                // Highpass for scrape character
                float toeCut = params.toeOffCutoff * pitchOffset;
                float toeF = 2.0f * sinf(PI * std::min(toeCut, sampleRate * 0.49f) / sampleRate);
                toeLp += toeF * (toeNoise - toeLp);
                float toeHp = toeNoise - toeLp;

                output += toeHp * toeEnv * params.toeOffWeight;
            }
        }

        // == GROUND MODE RESONANCE ==
        // Excited by the heel strike impulse, rings according to surface material
        for (int m = 0; m < params.groundModeCount && m < 4; ++m) {
            auto& ms = groundModeStates[m];
            float excitation = (ls == 0) ? intensity : 0.0f;
            float y = ms.a1 * ms.y1 + ms.a2 * ms.y2 + excitation * ms.gain;
            ms.y2 = ms.y1;
            ms.y1 = y;
            float modeEnv = expf(-t / params.groundModes[m].decay);
            output += y * modeEnv;
        }

        // == AIR ABSORPTION (high-frequency rolloff over time) ==
        if (params.airAbsorption > 0.0f) {
            float absCut = cutoff * expf(-params.airAbsorption * t * 10.0f);
            absCut = std::max(absCut, 200.0f);
            float absF = 2.0f * sinf(PI * std::min(absCut, sampleRate * 0.49f) / sampleRate);
            absLp += absF * (output - absLp);
            output = absLp * (1.0f - params.airAbsorption * 0.3f) + output * params.airAbsorption * 0.3f;
        }

        // == EARLY REFLECTION ==
        float withReflection = output;
        if (params.earlyReflectionGain > 0.0f) {
            int readPos = (delayWritePos - reflectionDelaySamples + static_cast<int>(delayLine.size())) % static_cast<int>(delayLine.size());
            withReflection += delayLine[readPos] * params.earlyReflectionGain;
            delayLine[delayWritePos] = output;
            delayWritePos = (delayWritePos + 1) % static_cast<int>(delayLine.size());
        }

        // Final output scaled by intensity and mass
        buffer[i] += withReflection * intensity * params.massScale * 0.8f;
    }
}

void SoundGenerator::synthesizeUnderwater(
    std::vector<float>& buffer,
    int sampleRate,
    float eventTime,
    float intensity,
    uint32_t seed)
{
    // Underwater sound synthesis for fish/aquatic creatures
    // Key characteristics: heavily lowpassed, water displacement whoosh,
    // bubble resonances, muffled body swoosh — no sharp transients
    constexpr float PI = 3.14159265358979f;
    constexpr float TWO_PI = 6.28318530717959f;
    uint32_t rng = seed;

    auto randF = [&rng]() -> float {
        return static_cast<float>(xorshift32(rng)) / static_cast<float>(UINT32_MAX);
    };

    float dt = 1.0f / sampleRate;
    int startSample = static_cast<int>(eventTime * sampleRate);
    if (startSample < 0)
        startSample = 0;

    // Underwater sounds are longer and smoother
    float duration = 0.15f + intensity * 0.2f;
    int totalSamples = static_cast<int>(duration * sampleRate);
    int endSample = std::min(startSample + totalSamples, static_cast<int>(buffer.size()));
    if (endSample <= startSample)
        return;

    // Pitch variation for organic feel
    float pitchMod = 1.0f + (randF() - 0.5f) * 0.3f;

    // ---- Underwater lowpass filter (very aggressive, ~600 Hz) ----
    float waterCutoff = (400.0f + intensity * 300.0f) * pitchMod;
    float waterF = 2.0f * sinf(PI * std::min(waterCutoff, sampleRate * 0.49f) / sampleRate);
    float waterLp = 0.0f;
    float waterLp2 = 0.0f; // second-order for steeper rolloff

    // ---- Bubble resonators (2-3 random bubble pops) ----
    struct Bubble {
        float freq;
        float y1 = 0.0f, y2 = 0.0f;
        float a1, a2, gain;
        int onset; // sample offset for bubble start
        float decay;
    };
    int bubbleCount = 2 + (static_cast<int>(randF() * 2.5f));
    if (bubbleCount > 4)
        bubbleCount = 4;
    Bubble bubbles[4];
    for (int b = 0; b < bubbleCount; ++b) {
        float freq = (200.0f + randF() * 600.0f) * pitchMod;
        float bw = freq / (2.0f + randF() * 3.0f);
        float r = expf(-PI * bw * dt);
        float theta = TWO_PI * freq * dt;
        bubbles[b].freq = freq;
        bubbles[b].a1 = 2.0f * r * cosf(theta);
        bubbles[b].a2 = -(r * r);
        bubbles[b].gain = (0.3f + randF() * 0.5f) * (1.0f - r);
        bubbles[b].onset = static_cast<int>(randF() * totalSamples * 0.6f);
        bubbles[b].decay = 0.02f + randF() * 0.04f;
    }

    // ---- Water displacement whoosh (broadband, heavily filtered) ----
    // Smooth envelope: slow attack, sustained, slow decay
    int attackSamples = static_cast<int>(0.02f * sampleRate);
    int sustainEnd = totalSamples - static_cast<int>(0.04f * sampleRate);

    for (int i = startSample; i < endSample; ++i) {
        float t = static_cast<float>(i - startSample) * dt;
        int ls = i - startSample;
        float output = 0.0f;

        // == WATER DISPLACEMENT WHOOSH ==
        // Smooth envelope
        float whooshEnv;
        if (ls < attackSamples) {
            float phase = static_cast<float>(ls) / attackSamples;
            whooshEnv = sinf(phase * PI * 0.5f); // smooth rise
        } else if (ls < sustainEnd) {
            whooshEnv = 1.0f;
        } else {
            float decayPhase = static_cast<float>(ls - sustainEnd) / static_cast<float>(totalSamples - sustainEnd);
            whooshEnv = cosf(decayPhase * PI * 0.5f); // smooth fall
        }

        // Broadband noise, double lowpassed for muffled underwater character
        float noise = randF() * 2.0f - 1.0f;
        waterLp += waterF * (noise - waterLp);
        waterLp2 += waterF * (waterLp - waterLp2); // second pass
        output += waterLp2 * whooshEnv * 0.6f;

        // == BUBBLE RESONANCES ==
        for (int b = 0; b < bubbleCount; ++b) {
            auto& bub = bubbles[b];
            if (ls < bub.onset)
                continue;
            int bubLocal = ls - bub.onset;
            float excitation = (bubLocal == 0) ? intensity * bub.gain : 0.0f;
            float y = bub.a1 * bub.y1 + bub.a2 * bub.y2 + excitation;
            bub.y2 = bub.y1;
            bub.y1 = y;
            float bubEnv = expf(-static_cast<float>(bubLocal) * dt / bub.decay);
            output += y * bubEnv * 0.4f;
        }

        // == LOW-FREQUENCY BODY SWOOSH ==
        // Very low sine modulation for the "weight" of water moving
        float swooshFreq = (30.0f + intensity * 20.0f) * pitchMod;
        float swoosh = sinf(TWO_PI * swooshFreq * t) * whooshEnv * 0.25f;
        output += swoosh;

        buffer[i] += output * intensity * 0.7f;
    }
}

void SoundGenerator::synthesizeWhoosh(
    std::vector<float>& buffer,
    int sampleRate,
    float eventTime,
    float intensity,
    float whooshDuration,
    uint32_t seed)
{
    // Air displacement whoosh for fast-moving body parts (head charge, tail whip).
    // Modeled after FMOD/Wwise whoosh design: bandpass-filtered noise with a
    // velocity-driven pitch sweep.  Faster motion = higher center frequency
    // and brighter spectrum.  The envelope follows an asymmetric bell: fast
    // attack (leading edge of the moving object) and slower decay (turbulent
    // wake trailing behind).
    //
    // Signal chain:
    //   White noise → Bandpass (center sweeps with velocity) → Amplitude envelope → Output
    //
    // The bandpass center frequency rises from ~200 Hz to a peak proportional
    // to intensity (up to ~2500 Hz for maximum velocity), then falls back.
    // This creates the characteristic rising-then-falling pitch of a whoosh.

    constexpr float PI = 3.14159265358979f;
    uint32_t rng = seed;

    float dt = 1.0f / sampleRate;
    int startSample = static_cast<int>(eventTime * sampleRate);
    if (startSample < 0)
        startSample = 0;

    if (whooshDuration < 0.02f)
        whooshDuration = 0.15f;

    int totalSamples = static_cast<int>(whooshDuration * sampleRate);
    int endSample = std::min(startSample + totalSamples, static_cast<int>(buffer.size()));
    if (endSample <= startSample)
        return;

    // Pitch variation for organic variation
    float pitchMod = 1.0f + (static_cast<float>(xorshift32(rng)) / static_cast<float>(UINT32_MAX) - 0.5f) * 0.15f;

    // Frequency range: scales with intensity (higher velocity = brighter whoosh)
    float freqLow = 200.0f * pitchMod;
    float freqPeak = (800.0f + intensity * 1700.0f) * pitchMod; // 800-2500 Hz at peak
    float bandwidth = (300.0f + intensity * 500.0f) * pitchMod; // wider bandwidth at higher velocity

    // Asymmetric envelope: attack is 30% of duration, decay is 70%
    // This matches real aeroacoustics — the leading edge is sharp,
    // the turbulent wake trails off.
    float attackFraction = 0.3f;
    int attackSamples = static_cast<int>(attackFraction * totalSamples);
    int decaySamples = totalSamples - attackSamples;

    // State-variable bandpass filter
    float bpLow = 0.0f, bpBand = 0.0f;

    // Second bandpass for wider, more natural sound
    float bp2Low = 0.0f, bp2Band = 0.0f;

    for (int i = startSample; i < endSample; ++i) {
        int ls = i - startSample;
        float phase = static_cast<float>(ls) / static_cast<float>(totalSamples);

        // Amplitude envelope: asymmetric bell
        float env;
        if (ls < attackSamples) {
            // Fast sine rise
            float t = static_cast<float>(ls) / static_cast<float>(attackSamples);
            env = sinf(t * PI * 0.5f);
            env *= env; // sharpen the attack
        } else {
            // Slower exponential decay with cosine shaping
            float t = static_cast<float>(ls - attackSamples) / static_cast<float>(std::max(1, decaySamples));
            env = cosf(t * PI * 0.5f);
            env *= expf(-t * 2.0f); // exponential falloff for natural turbulence decay
        }

        // Center frequency sweep: rises to peak during attack, falls during decay
        // Uses a bell curve centered at attackFraction
        float freqPhase = (phase - attackFraction) / (1.0f - attackFraction);
        float freqEnv;
        if (phase < attackFraction) {
            float t = phase / attackFraction;
            freqEnv = t * t; // quadratic rise
        } else {
            freqEnv = expf(-freqPhase * freqPhase * 4.0f); // Gaussian falloff
        }
        float centerFreq = freqLow + (freqPeak - freqLow) * freqEnv;

        // SVF bandpass coefficients (recomputed per sample for sweep)
        float bpF = 2.0f * sinf(PI * std::min(centerFreq, sampleRate * 0.49f) / sampleRate);
        float bpQ = 1.0f / std::max(centerFreq / bandwidth, 0.5f);

        // Second band offset higher for width
        float centerFreq2 = centerFreq * 1.6f;
        float bp2F = 2.0f * sinf(PI * std::min(centerFreq2, sampleRate * 0.49f) / sampleRate);
        float bp2Q = 1.0f / std::max(centerFreq2 / (bandwidth * 1.3f), 0.5f);

        // White noise source
        float noise = static_cast<float>(xorshift32(rng)) / static_cast<float>(UINT32_MAX) * 2.0f - 1.0f;

        // Primary bandpass
        bpLow += bpF * bpBand;
        float bpHigh = noise - bpLow - bpQ * bpBand;
        bpBand += bpF * bpHigh;

        // Secondary bandpass (higher octave, mixed quieter)
        bp2Low += bp2F * bp2Band;
        float bp2High = noise - bp2Low - bp2Q * bp2Band;
        bp2Band += bp2F * bp2High;

        float output = bpBand * 0.7f + bp2Band * 0.3f;
        output *= env * intensity * 0.6f;

        buffer[i] += output;
    }
}

AnimationSoundData SoundGenerator::generate(
    const std::vector<SoundEvent>& events,
    float durationSeconds,
    SurfaceMaterial material,
    float volumeScale)
{
    AnimationSoundData result;
    result.sampleRate = 44100;
    result.channels = 1;
    result.durationSeconds = durationSeconds;

    int totalSamples = static_cast<int>(durationSeconds * result.sampleRate);
    if (totalSamples <= 0) {
        return result;
    }

    std::vector<float> floatBuffer(totalSamples, 0.0f);

    SurfaceSynthParams params = getSurfaceSynthParams(material);

    for (size_t i = 0; i < events.size(); ++i) {
        const auto& event = events[i];
        // Use event index + time as seed for deterministic variation
        uint32_t seed = static_cast<uint32_t>(i * 2654435761u + static_cast<uint32_t>(event.timeSeconds * 10000));
        if (seed == 0)
            seed = 1;

        if (event.isWhoosh) {
            synthesizeWhoosh(floatBuffer, result.sampleRate, event.timeSeconds,
                event.intensity, event.whooshDuration, seed);
        } else if (event.isUnderwater) {
            synthesizeUnderwater(floatBuffer, result.sampleRate, event.timeSeconds,
                event.intensity, seed);
        } else {
            synthesizeImpact(floatBuffer, result.sampleRate, event.timeSeconds,
                event.intensity, params, seed);
        }
    }

    // Normalize and convert to int16
    float maxVal = 0.0f;
    for (float s : floatBuffer) {
        float abs = s < 0 ? -s : s;
        if (abs > maxVal)
            maxVal = abs;
    }

    float normalize = 1.0f;
    if (maxVal > 0.001f) {
        normalize = 0.9f / maxVal;
    }
    normalize *= volumeScale;

    result.pcmSamples.resize(totalSamples);
    for (int i = 0; i < totalSamples; ++i) {
        float s = floatBuffer[i] * normalize;
        s = std::max(-1.0f, std::min(1.0f, s));
        result.pcmSamples[i] = static_cast<int16_t>(s * 32767.0f);
    }

    return result;
}

std::vector<uint8_t> SoundGenerator::encodeWav(const AnimationSoundData& data)
{
    uint32_t dataSize = static_cast<uint32_t>(data.pcmSamples.size() * 2);
    uint32_t fileSize = 36 + dataSize;

    std::vector<uint8_t> wav;
    wav.reserve(44 + dataSize);

    auto writeU32 = [&](uint32_t v) {
        wav.push_back(v & 0xFF);
        wav.push_back((v >> 8) & 0xFF);
        wav.push_back((v >> 16) & 0xFF);
        wav.push_back((v >> 24) & 0xFF);
    };
    auto writeU16 = [&](uint16_t v) {
        wav.push_back(v & 0xFF);
        wav.push_back((v >> 8) & 0xFF);
    };
    auto writeStr = [&](const char* s) {
        for (int i = 0; s[i]; ++i)
            wav.push_back(static_cast<uint8_t>(s[i]));
    };

    // RIFF header
    writeStr("RIFF");
    writeU32(fileSize);
    writeStr("WAVE");

    // fmt chunk
    writeStr("fmt ");
    writeU32(16); // chunk size
    writeU16(1); // PCM format
    writeU16(static_cast<uint16_t>(data.channels));
    writeU32(static_cast<uint32_t>(data.sampleRate));
    writeU32(static_cast<uint32_t>(data.sampleRate * data.channels * 2)); // byte rate
    writeU16(static_cast<uint16_t>(data.channels * 2)); // block align
    writeU16(16); // bits per sample

    // data chunk
    writeStr("data");
    writeU32(dataSize);

    for (int16_t sample : data.pcmSamples) {
        wav.push_back(static_cast<uint8_t>(sample & 0xFF));
        wav.push_back(static_cast<uint8_t>((sample >> 8) & 0xFF));
    }

    return wav;
}

} // namespace dust3d
