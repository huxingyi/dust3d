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

#ifndef DUST3D_ANIMATION_SOUND_GENERATOR_H_
#define DUST3D_ANIMATION_SOUND_GENERATOR_H_

#include <cstdint>
#include <string>
#include <vector>

namespace dust3d {

enum class SurfaceMaterial {
    Stone,
    Wood,
    Sand,
    Metal,
    Grass,
    Water,
    Dirt,
    Snow
};

struct SoundEvent {
    float timeSeconds = 0.0f;
    float intensity = 1.0f; // 0.0 – 1.0, derived from impact velocity
    std::string boneName;
    bool isUnderwater = false; // true for fish/aquatic creatures
};

struct SurfaceSynthParams {
    // === Ground material properties ===
    float filterCutoff = 4000.0f; // Hz – overall spectral ceiling
    float resonance = 1.0f; // Q factor
    float brightness = 0.5f; // HF content 0-1
    float pitchVariation = 0.1f; // random variation per step

    // === Biomechanical footstep phases ===
    // Phase 1: Heel/paw strike — initial contact of foot with ground
    float heelStrikeAttack = 0.003f; // seconds – soft onset (not a hard crack)
    float heelStrikeDecay = 0.025f; // seconds – thud body
    float heelStrikeCutoff = 1800.0f; // Hz – low frequency thud
    float heelStrikeWeight = 0.6f; // mix contribution

    // Phase 2: Ground deformation — material compression/displacement
    float deformationOnset = 0.008f; // seconds – delay after heel strike
    float deformationDuration = 0.06f; // seconds – squish/crunch duration
    float deformationCutoff = 3000.0f; // Hz
    float deformationGranularity = 0.3f; // 0-1 crunchiness (sand=high, stone=low)
    float deformationWeight = 0.4f; // mix contribution

    // Phase 3: Weight transfer — body mass settling onto foot
    float weightTransferDuration = 0.08f; // seconds
    float weightTransferLowFreq = 60.0f; // Hz – sub-bass thump
    float weightTransferWeight = 0.3f; // mix contribution

    // Phase 4: Toe-off / lift (subtle scrape as foot departs)
    float toeOffDelay = 0.0f; // seconds after event (0 = disabled)
    float toeOffDuration = 0.04f; // seconds
    float toeOffCutoff = 2500.0f; // Hz
    float toeOffWeight = 0.0f; // mix (0 = no toe-off sound)

    // === Surface texture layer ===
    // Velvet noise for granular materials (sand, gravel, snow, leaves)
    float granularDensity = 0.0f; // impulses/sec factor (0 = off)
    float granularDecay = 0.08f; // seconds
    float granularBandLow = 800.0f; // Hz – bandpass low edge
    float granularBandHigh = 4000.0f; // Hz – bandpass high edge

    // === Ground resonance (modal) ===
    // Low-frequency ground coupling — the "weight" of the step
    struct GroundMode {
        float frequency = 80.0f; // Hz
        float amplitude = 1.0f;
        float decay = 0.06f; // seconds
        float q = 3.0f; // low Q = soft thud, high Q = hollow
    };
    GroundMode groundModes[4] = {};
    int groundModeCount = 0;

    // === Spatial / environment ===
    float earlyReflectionDelay = 0.0008f; // seconds
    float earlyReflectionGain = 0.15f; // subtle ground reflection
    float airAbsorption = 0.0f; // HF rolloff for distance (0-1)

    // === Character (animal weight) ===
    // These scale the output to simulate different-sized creatures
    float massScale = 1.0f; // <1 = light creature, >1 = heavy
    float padSoftness = 0.5f; // 0 = hard hoof, 1 = soft paw pad
};

SurfaceSynthParams getSurfaceSynthParams(SurfaceMaterial material);

std::string surfaceMaterialName(SurfaceMaterial material);
SurfaceMaterial surfaceMaterialFromName(const std::string& name);

struct AnimationSoundData {
    std::vector<int16_t> pcmSamples; // mono 16-bit signed
    int sampleRate = 44100;
    int channels = 1;
    float durationSeconds = 0.0f;
};

class SoundGenerator {
public:
    static AnimationSoundData generate(
        const std::vector<SoundEvent>& events,
        float durationSeconds,
        SurfaceMaterial material,
        float volumeScale = 1.0f);

    static std::vector<uint8_t> encodeWav(const AnimationSoundData& data);

private:
    static void synthesizeImpact(
        std::vector<float>& buffer,
        int sampleRate,
        float eventTime,
        float intensity,
        const SurfaceSynthParams& params,
        uint32_t seed);

    static void synthesizeUnderwater(
        std::vector<float>& buffer,
        int sampleRate,
        float eventTime,
        float intensity,
        uint32_t seed);
};

} // namespace dust3d

#endif
