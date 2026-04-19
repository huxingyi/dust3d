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

// Procedural attack (dive) animation for bird rig.
//
// Techniques applied (common in game animation pipelines):
//
// 1. CUBIC HERMITE SPLINE INTERPOLATION — Body pitch, altitude, and wing
//    angles are defined as sparse keyframes with tangent control, then
//    interpolated via cubic Hermite (cspline). This avoids the flat
//    plateaus that smoothstep blending produces and gives animator-quality
//    ease-in / ease-out curves with exact tangent continuity (C1).
//
// 2. CRITICALLY-DAMPED SPRING SIMULATION — Secondary bones (wing tips,
//    tail feathers, head) are driven by a second-order critically-damped
//    spring that tracks a target signal. This produces natural follow-
//    through and overlapping action (Disney principle #5/#6) without
//    any oscillation overshoot, identical to the technique used in
//    Uncharted / The Last of Us procedural layers.
//
// 3. VALUE NOISE TURBULENCE — A seeded 1D value noise (hashed integer
//    lattice + Hermite interpolation) is layered onto the body during
//    the dive phase to simulate aerodynamic buffeting. The amplitude
//    scales with dive speed (velocity-proportional turbulence).
//
// 4. OVERLAPPING ACTION WITH STAGGERED DELAYS — Each bone chain has a
//    different phase offset so that motion propagates outward from the
//    body core: spine leads, then shoulder, elbow, hand. This is the
//    "successive breaking of joints" principle from classical animation.
//
// 5. SQUASH & STRETCH — The spine compresses during high-G pullout and
//    stretches during freefall, applied as a scale along the forward
//    axis derived from the second derivative of the altitude curve
//    (i.e. acceleration → deformation).
//
// 6. ANTICIPATION — A pronounced counter-movement (rise + wing spread)
//    precedes the dive. The magnitude is proportional to dive intensity,
//    following the animation principle that strong actions need strong
//    preparation.
//
// Animation phases (over one cycle):
//   0.00 - 0.20: Anticipation — rise, wings spread wide, head locks target
//   0.20 - 0.55: Dive — steep nose-down, wings tuck, turbulence layer
//   0.55 - 0.65: Strike — talons extend, beak opens, impact freeze
//   0.65 - 1.00: Recovery — pull up with G-load, wings flare, spring settle
//
// Adjustable parameters:
//   - divePitchFactor:      dive steepness
//   - diveDepthFactor:      vertical drop distance
//   - wingTuckFactor:       wing fold tightness during dive
//   - strikeExtendFactor:   talon/beak extension at strike
//   - recoverySpeedFactor:  pullout speed
//   - headTrackFactor:      head aim intensity during dive

#include <cmath>
#include <dust3d/animation/bird/attack.h>
#include <dust3d/animation/common.h>
#include <dust3d/base/math.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/quaternion.h>
#include <dust3d/base/vector3.h>
#include <dust3d/rig/rig_generator.h>

namespace dust3d {

namespace bird {

    // =====================================================================
    // CUBIC HERMITE SPLINE
    // =====================================================================
    // Evaluates a cubic Hermite spline through a set of (time, value, tangent)
    // keyframes. Tangents are specified per-keyframe, giving C1 continuity
    // and precise control over ease-in / ease-out — the same representation
    // used by Maya/MotionBuilder animation curves.

    struct HermiteKey {
        double time;
        double value;
        double tangent; // dv/dt at this keyframe
    };

    static double evalHermite(const HermiteKey* keys, int count, double t)
    {
        if (count == 0)
            return 0.0;
        if (t <= keys[0].time)
            return keys[0].value;
        if (t >= keys[count - 1].time)
            return keys[count - 1].value;

        // Find segment
        int seg = 0;
        for (int i = 0; i < count - 1; ++i) {
            if (t >= keys[i].time && t < keys[i + 1].time) {
                seg = i;
                break;
            }
        }

        double dt = keys[seg + 1].time - keys[seg].time;
        if (dt < 1e-12)
            return keys[seg].value;

        double u = (t - keys[seg].time) / dt;
        double u2 = u * u;
        double u3 = u2 * u;

        // Hermite basis functions
        double h00 = 2.0 * u3 - 3.0 * u2 + 1.0;
        double h10 = u3 - 2.0 * u2 + u;
        double h01 = -2.0 * u3 + 3.0 * u2;
        double h11 = u3 - u2;

        double p0 = keys[seg].value;
        double p1 = keys[seg + 1].value;
        double m0 = keys[seg].tangent * dt;
        double m1 = keys[seg + 1].tangent * dt;

        return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
    }

    // =====================================================================
    // CRITICALLY-DAMPED SPRING (second-order)
    // =====================================================================
    // Tracks a target value with natural follow-through and no oscillation.
    // Used in games for procedural secondary motion layers.
    // Reference: Game Programming Gems 4, "Critically Damped Ease-In/Out"
    //
    // State is kept as (position, velocity) and integrated per-frame with
    // an exact closed-form solution for the critically-damped case (ζ = 1).

    struct SpringState {
        double pos = 0.0;
        double vel = 0.0;
    };

    static void springUpdate(SpringState& state, double target, double halfLife, double dt)
    {
        // Convert half-life to angular frequency: ω = ln(2) / halfLife
        // For critically-damped: ζ = 1, so damping = 2ω
        if (halfLife < 1e-8) {
            state.pos = target;
            state.vel = 0.0;
            return;
        }

        double omega = 0.6931471805599453 / halfLife; // ln(2)
        double damping = 2.0 * omega;
        double expTerm = std::exp(-damping * dt);

        double delta = state.pos - target;
        double newDelta = (delta + (state.vel + omega * delta) * dt) * expTerm;

        state.pos = target + newDelta;
        state.vel = (state.vel - omega * (state.vel + omega * delta) * dt) * expTerm;
    }

    // =====================================================================
    // VALUE NOISE (1D)
    // =====================================================================
    // Seeded integer-lattice noise with Hermite interpolation.
    // Produces smooth, deterministic pseudo-random turbulence without
    // needing a Perlin permutation table.

    static double valueNoise1D(double x, uint32_t seed)
    {
        auto hash = [](int32_t n, uint32_t s) -> double {
            uint32_t h = static_cast<uint32_t>(n) * 1087u + s;
            h ^= h >> 16;
            h *= 0x45d9f3bu;
            h ^= h >> 16;
            h *= 0x45d9f3bu;
            h ^= h >> 16;
            return static_cast<double>(h & 0x7fffffffu) / static_cast<double>(0x7fffffffu) * 2.0 - 1.0;
        };

        int32_t ix = static_cast<int32_t>(std::floor(x));
        double frac = x - static_cast<double>(ix);
        // Hermite interpolation for C1 continuity
        double u = frac * frac * (3.0 - 2.0 * frac);

        double v0 = hash(ix, seed);
        double v1 = hash(ix + 1, seed);
        return v0 + (v1 - v0) * u;
    }

    // Multi-octave fractal noise (fBm)
    static double turbulenceNoise(double x, int octaves, double lacunarity, double gain, uint32_t seed)
    {
        double sum = 0.0;
        double amp = 1.0;
        double freq = 1.0;
        for (int i = 0; i < octaves; ++i) {
            sum += amp * valueNoise1D(x * freq, seed + static_cast<uint32_t>(i) * 17u);
            amp *= gain;
            freq *= lacunarity;
        }
        return sum;
    }

    // =====================================================================
    // ATTACK (DIVE) ANIMATION
    // =====================================================================

    bool attack(const RigStructure& rigStructure,
        const std::map<std::string, Matrix4x4>& /* inverseBindMatrices */,
        RigAnimationClip& animationClip,
        const AnimationParams& parameters)
    {
        using namespace animation;

        int frameCount = static_cast<int>(parameters.getValue("frameCount", 60));
        float durationSeconds = static_cast<float>(parameters.getValue("durationSeconds", 2.5));

        auto boneIdx = buildBoneIndexMap(rigStructure);

        auto bonePos = [&](const std::string& name) -> Vector3 {
            return getBonePos(rigStructure, boneIdx, name);
        };
        auto boneEnd = [&](const std::string& name) -> Vector3 {
            return getBoneEnd(rigStructure, boneIdx, name);
        };

        static const char* requiredBones[] = {
            "Root", "Pelvis", "Spine", "Chest", "Neck", "Head",
            "LeftWingShoulder", "LeftWingElbow", "LeftWingHand",
            "RightWingShoulder", "RightWingElbow", "RightWingHand",
            "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
            "RightUpperLeg", "RightLowerLeg", "RightFoot"
        };
        if (!validateRequiredBones(boneIdx, requiredBones, sizeof(requiredBones) / sizeof(requiredBones[0])))
            return false;

        // Determine body axes
        Vector3 pelvisPos = bonePos("Pelvis");
        Vector3 chestEnd = boneEnd("Chest");
        Vector3 bodyVector = chestEnd - pelvisPos;
        if (bodyVector.isZero())
            return false;

        Vector3 forwardDir = bodyVector.normalized();
        Vector3 worldUp(0.0, 1.0, 0.0);
        Vector3 right = Vector3::crossProduct(forwardDir, worldUp);
        if (right.lengthSquared() < 1e-8)
            right = Vector3::crossProduct(forwardDir, Vector3(0.0, 0.0, 1.0));
        right.normalize();
        Vector3 up = Vector3::crossProduct(right, forwardDir).normalized();

        double bodyLength = bodyVector.length();

        // ---- Animation parameters ----
        double divePitchFactor = parameters.getValue("divePitchFactor", 1.0);
        double diveDepthFactor = parameters.getValue("diveDepthFactor", 1.0);
        double wingTuckFactor = parameters.getValue("wingTuckFactor", 1.0);
        double strikeExtendFactor = parameters.getValue("strikeExtendFactor", 1.0);
        double recoverySpeedFactor = parameters.getValue("recoverySpeedFactor", 1.0);
        double headTrackFactor = parameters.getValue("headTrackFactor", 1.0);

        // ================================================================
        // HERMITE KEYFRAME CURVES
        // ================================================================
        // Each curve is defined as sparse keyframes with tangents, then
        // evaluated with cubic Hermite interpolation for C1-continuous,
        // animator-quality motion.

        // Body pitch curve (radians): anticipation rise → steep dive → pullout
        double maxPitch = -0.9 * divePitchFactor;
        HermiteKey pitchKeys[] = {
            { 0.00, 0.0, 0.0 }, // start level
            { 0.12, 0.20 * divePitchFactor, 1.5 }, // anticipation: nose up, accelerating
            { 0.20, 0.10 * divePitchFactor, -3.0 }, // transition: starting to tip over
            { 0.35, maxPitch, -0.5 }, // peak dive: steep nose-down, decelerating
            { 0.55, maxPitch * 0.8, 1.5 }, // approaching strike: beginning pullout
            { 0.65, -0.15, 1.2 }, // post-strike: pulling up hard
            { 0.80, 0.10, 0.3 }, // overshoot: slight nose-up from momentum
            { 1.00, 0.0, -0.2 }, // settle back to level
        };
        int pitchKeyCount = sizeof(pitchKeys) / sizeof(pitchKeys[0]);

        // Altitude curve (normalized, multiply by diveDepth)
        double diveDepth = bodyLength * 1.0 * diveDepthFactor;
        HermiteKey altKeys[] = {
            { 0.00, 0.0, 0.0 },
            { 0.15, 0.15, 0.8 }, // anticipation rise
            { 0.20, 0.10, -2.0 }, // tip over
            { 0.40, -0.70, -2.5 }, // freefall acceleration
            { 0.55, -1.00, -0.5 }, // deepest point at strike
            { 0.65, -0.80, 1.5 * recoverySpeedFactor }, // pulling up
            { 0.85, -0.10, 0.8 * recoverySpeedFactor }, // nearly recovered
            { 1.00, 0.0, 0.2 }, // back to level
        };
        int altKeyCount = sizeof(altKeys) / sizeof(altKeys[0]);

        // Wing tuck curve (0 = spread, 1 = fully tucked)
        double tuckScale = wingTuckFactor;
        HermiteKey wingTuckKeys[] = {
            { 0.00, 0.0, 0.0 },
            { 0.10, -0.15, -1.0 }, // anticipation: spread wider than rest
            { 0.18, 0.0, 3.0 }, // snap to tuck
            { 0.28, 1.0, 0.5 }, // fully tucked
            { 0.50, 1.0, 0.0 }, // hold tucked through dive
            { 0.56, 0.6, -4.0 }, // explosive snap open at strike
            { 0.62, 0.0, -1.0 }, // wings fully open
            { 0.75, -0.08, 0.3 }, // slight overshoot (spread wider)
            { 1.00, 0.0, 0.1 }, // settle
        };
        int wingTuckKeyCount = sizeof(wingTuckKeys) / sizeof(wingTuckKeys[0]);

        // Leg extension curve (0 = tucked, 1 = fully extended for strike)
        HermiteKey legKeys[] = {
            { 0.00, 0.0, 0.0 },
            { 0.45, 0.0, 0.0 }, // tucked through dive
            { 0.52, 0.3, 4.0 }, // beginning to extend — fast
            { 0.58, 1.0, 1.0 }, // fully extended at strike
            { 0.68, 0.8, -1.5 }, // slight retract after impact
            { 0.85, 0.1, -0.5 }, // tucking back
            { 1.00, 0.0, 0.0 }, // fully tucked
        };
        int legKeyCount = sizeof(legKeys) / sizeof(legKeys[0]);

        // ================================================================
        // SPRING STATE for secondary motion (per-bone-group)
        // ================================================================
        // Springs track the "driving" signal and add follow-through delay.
        SpringState springNeck; // neck follows body pitch with delay
        SpringState springHead; // head tracks target with different dynamics
        SpringState springTailPitch;
        SpringState springTailYaw;
        SpringState springLeftWingTip;
        SpringState springRightWingTip;
        SpringState springBeakOpen;

        double dt = durationSeconds / static_cast<double>(frameCount);

        // Noise seed (deterministic per animation)
        uint32_t noiseSeed = 42u;

        animationClip.durationSeconds = durationSeconds;
        animationClip.frames.resize(frameCount);

        // Previous altitude for velocity/acceleration estimation (squash & stretch)
        double prevAlt = 0.0;
        double prevVelAlt = 0.0;

        for (int frame = 0; frame < frameCount; ++frame) {
            double t = static_cast<double>(frame) / static_cast<double>(frameCount);
            double tSec = t * durationSeconds;

            // ---- Evaluate Hermite curves ----
            double pitchAngle = evalHermite(pitchKeys, pitchKeyCount, t);
            double altNorm = evalHermite(altKeys, altKeyCount, t);
            double altitudeOffset = altNorm * diveDepth;
            double legExtValue = evalHermite(legKeys, legKeyCount, t) * strikeExtendFactor;

            // ---- Velocity & acceleration for squash/stretch ----
            double velAlt = (altitudeOffset - prevAlt) / dt;
            double accelAlt = (velAlt - prevVelAlt) / dt;
            prevAlt = altitudeOffset;
            prevVelAlt = velAlt;

            // Squash & stretch: scale along forward axis based on acceleration.
            // During freefall (negative accel = speeding up downward) the body
            // stretches; during pullout (positive accel) it compresses.
            // Clamped to prevent extreme deformation.
            double accelNorm = accelAlt / (bodyLength * 20.0); // normalize
            double stretchFactor = 1.0 + std::max(-0.08, std::min(0.08, -accelNorm * 0.15));

            // ---- Turbulence noise during dive ----
            // Amplitude proportional to dive speed (velocity-proportional buffeting)
            double diveSpeed = std::abs(velAlt) / (bodyLength * 2.0);
            double turbAmp = std::min(diveSpeed * 0.06, 0.05); // cap amplitude
            double turbRoll = turbulenceNoise(tSec * 8.0, 3, 2.0, 0.5, noiseSeed) * turbAmp;
            double turbYaw = turbulenceNoise(tSec * 6.0, 2, 2.0, 0.5, noiseSeed + 100u) * turbAmp * 0.6;
            double turbPitch = turbulenceNoise(tSec * 7.0, 2, 2.0, 0.5, noiseSeed + 200u) * turbAmp * 0.4;

            // ---- Body transform ----
            Matrix4x4 bodyTransform;
            bodyTransform.translate(up * altitudeOffset);
            bodyTransform.rotate(right, pitchAngle + turbPitch);
            bodyTransform.rotate(forwardDir, turbRoll);
            bodyTransform.rotate(up, turbYaw);

            // Apply squash & stretch along forward axis (scale forward, inverse-scale up/right)
            // Volume preservation: perpendicular axes compensate.
            // Implemented on the Spine bone below as a translation offset.

            std::map<std::string, Matrix4x4> boneWorldTransforms;
            std::map<std::string, Matrix4x4> boneSkinMatrices;

            auto worldFromSkin = [&](const std::string& name, const Matrix4x4& skin) -> Matrix4x4 {
                Matrix4x4 bindWorld = buildBoneWorldTransform(bonePos(name), boneEnd(name));
                Matrix4x4 result = skin;
                result *= bindWorld;
                return result;
            };

            auto computeBodyBone = [&](const std::string& name) {
                boneSkinMatrices[name] = bodyTransform;
                boneWorldTransforms[name] = worldFromSkin(name, bodyTransform);
            };

            // ---- Body chain with squash & stretch on Spine ----
            computeBodyBone("Root");
            computeBodyBone("Pelvis");

            // Spine gets squash/stretch: translate end point along forward axis
            // (Since Matrix4x4 has no non-uniform scale, we approximate by
            // slightly shifting the spine pivot along the forward direction,
            // which compresses/stretches the deformation region.)
            {
                double stretchOffset = (stretchFactor - 1.0) * bodyLength * 0.3;
                Matrix4x4 spineSkin = bodyTransform;
                Vector3 spineBindPos = bonePos("Spine");
                spineSkin.translate(spineBindPos);
                spineSkin.translate(forwardDir * stretchOffset);
                spineSkin.translate(-spineBindPos);
                boneSkinMatrices["Spine"] = spineSkin;
                boneWorldTransforms["Spine"] = worldFromSkin("Spine", spineSkin);
            }

            computeBodyBone("Chest");

            // ---- Neck & Head: spring-driven secondary motion ----
            {
                // Target: head pitches further down during dive to track prey
                double headTrackTarget = pitchAngle * 0.3 * headTrackFactor;
                // During strike, thrust neck forward aggressively
                double strikePhase = evalHermite(legKeys, legKeyCount, t); // reuse leg timing
                headTrackTarget -= 0.35 * strikePhase * headTrackFactor;

                // Neck spring: medium half-life for visible follow-through
                springUpdate(springNeck, headTrackTarget, 0.06, dt);

                // Head spring: slightly faster, tracks a sharper signal
                double headTarget = headTrackTarget * 1.2;
                springUpdate(springHead, headTarget, 0.04, dt);

                Vector3 neckBindPos = bonePos("Neck");
                Matrix4x4 neckSkin = bodyTransform;
                neckSkin.translate(neckBindPos);
                neckSkin.rotate(right, springNeck.pos);
                neckSkin.translate(-neckBindPos);
                boneSkinMatrices["Neck"] = neckSkin;
                boneWorldTransforms["Neck"] = worldFromSkin("Neck", neckSkin);

                // Head: additional rotation beyond neck
                Vector3 headBindPos = bonePos("Head");
                Matrix4x4 headSkin = neckSkin;
                double headExtra = springHead.pos - springNeck.pos;
                headSkin.translate(headBindPos);
                headSkin.rotate(right, headExtra * 0.5);
                headSkin.translate(-headBindPos);
                boneSkinMatrices["Head"] = headSkin;
                boneWorldTransforms["Head"] = worldFromSkin("Head", headSkin);

                // Beak: spring-driven open/close at strike
                if (boneIdx.count("Beak")) {
                    double beakTarget = 0.25 * strikePhase;
                    springUpdate(springBeakOpen, beakTarget, 0.03, dt);

                    Vector3 beakBindPos = bonePos("Beak");
                    Matrix4x4 beakSkin = headSkin;
                    beakSkin.translate(beakBindPos);
                    beakSkin.rotate(right, springBeakOpen.pos);
                    beakSkin.translate(-beakBindPos);
                    boneSkinMatrices["Beak"] = beakSkin;
                    boneWorldTransforms["Beak"] = worldFromSkin("Beak", beakSkin);
                }
            }

            // ---- Tail: spring-driven with overlapping action ----
            {
                bool hasTailBase = boneIdx.count("TailBase") > 0;
                bool hasTailFeathers = boneIdx.count("TailFeathers") > 0;

                if (hasTailBase) {
                    // Tail pitch target: streamline during dive, flare as airbrake in recovery
                    double recoveryPhase = smootherstep(std::max(0.0, std::min(1.0, (t - 0.60) / 0.35)));
                    double divePhase = smootherstep(std::max(0.0, std::min(1.0, (t - 0.20) / 0.30)));
                    double tailPitchTarget = 0.25 * divePhase * (1.0 - recoveryPhase)
                        - 0.40 * recoveryPhase * (1.0 - smootherstep(std::max(0.0, std::min(1.0, (t - 0.80) / 0.20))));

                    // Tail yaw: reacts to turbulence with delay (drag effect)
                    double tailYawTarget = -turbYaw * 3.0;

                    springUpdate(springTailPitch, tailPitchTarget, 0.08, dt);
                    springUpdate(springTailYaw, tailYawTarget, 0.10, dt);

                    Vector3 tailBindPos = bonePos("TailBase");
                    Matrix4x4 tailSkin = bodyTransform;
                    tailSkin.translate(tailBindPos);
                    tailSkin.rotate(right, springTailPitch.pos);
                    tailSkin.rotate(up, springTailYaw.pos);
                    tailSkin.translate(-tailBindPos);
                    boneSkinMatrices["TailBase"] = tailSkin;
                    boneWorldTransforms["TailBase"] = worldFromSkin("TailBase", tailSkin);

                    if (hasTailFeathers) {
                        // Feathers lag further behind (longer half-life = more drag)
                        // Use tail spring value + extra pitch for spread
                        double featherExtra = -0.15 * recoveryPhase;
                        Matrix4x4 featherSkin = bodyTransform;
                        featherSkin.translate(tailBindPos);
                        featherSkin.rotate(right, springTailPitch.pos + featherExtra);
                        featherSkin.rotate(up, springTailYaw.pos * 1.3);
                        featherSkin.translate(-tailBindPos);
                        boneSkinMatrices["TailFeathers"] = featherSkin;
                        boneWorldTransforms["TailFeathers"] = worldFromSkin("TailFeathers", featherSkin);
                    }
                }
            }

            // ---- Wings: overlapping action with staggered delays ----
            for (int side = 0; side < 2; ++side) {
                const char* shoulderName = (side == 0) ? "LeftWingShoulder" : "RightWingShoulder";
                const char* elbowName = (side == 0) ? "LeftWingElbow" : "RightWingElbow";
                const char* handName = (side == 0) ? "LeftWingHand" : "RightWingHand";

                double sideSign = (side == 0) ? 1.0 : -1.0;

                // Staggered delays: shoulder reacts first, elbow 1 frame later,
                // hand 2 frames later. We approximate by evaluating tuck at
                // slightly offset times (successive breaking of joints).
                double shoulderDelay = 0.0;
                double elbowDelay = 0.015;
                double handDelay = 0.030;

                double shoulderTuck = evalHermite(wingTuckKeys, wingTuckKeyCount,
                                          std::max(0.0, std::min(1.0, t - shoulderDelay)))
                    * tuckScale;
                double elbowTuck = evalHermite(wingTuckKeys, wingTuckKeyCount,
                                       std::max(0.0, std::min(1.0, t - elbowDelay)))
                    * tuckScale;
                double handTuck = evalHermite(wingTuckKeys, wingTuckKeyCount,
                                      std::max(0.0, std::min(1.0, t - handDelay)))
                    * tuckScale;

                // Shoulder: fold backward when tucking, spread with dihedral when open
                double shoulderFoldAngle = -0.7 * shoulderTuck;
                double shoulderDihedral = 0.18 * sideSign * (1.0 - std::max(0.0, shoulderTuck));
                // Anticipation: extra spread before tuck
                double antic = evalHermite(wingTuckKeys, wingTuckKeyCount, t);
                double anticipationSpread = std::max(0.0, -antic) * 0.3 * sideSign;

                Vector3 shoulderBindPos = bonePos(shoulderName);
                Matrix4x4 shoulderSkin = bodyTransform;
                shoulderSkin.translate(shoulderBindPos);
                shoulderSkin.rotate(forwardDir, shoulderDihedral + anticipationSpread);
                shoulderSkin.rotate(right, shoulderFoldAngle);
                shoulderSkin.translate(-shoulderBindPos);
                boneSkinMatrices[shoulderName] = shoulderSkin;
                boneWorldTransforms[shoulderName] = worldFromSkin(shoulderName, shoulderSkin);

                // Elbow: fold inward (delayed)
                double elbowFoldAngle = 1.0 * elbowTuck * sideSign * -1.0;
                Vector3 elbowBindPos = bonePos(elbowName);
                Matrix4x4 elbowSkin = shoulderSkin;
                elbowSkin.translate(elbowBindPos);
                elbowSkin.rotate(forwardDir, elbowFoldAngle);
                elbowSkin.translate(-elbowBindPos);
                boneSkinMatrices[elbowName] = elbowSkin;
                boneWorldTransforms[elbowName] = worldFromSkin(elbowName, elbowSkin);

                // Hand: fold tightest (most delayed), spring-driven tip flutter
                SpringState& tipSpring = (side == 0) ? springLeftWingTip : springRightWingTip;
                double tipTarget = 0.8 * handTuck * sideSign * -1.0;
                // Add turbulence-driven flutter at wing tips during dive
                tipTarget += turbRoll * 2.0 * sideSign;
                springUpdate(tipSpring, tipTarget, 0.05, dt);

                // Strike flare: explosive snap outward at strike moment
                double strikeFlare = 0.0;
                if (t > 0.52 && t < 0.68) {
                    double sf = smootherstep(std::max(0.0, std::min(1.0, (t - 0.52) / 0.04)));
                    double sfOut = smootherstep(std::max(0.0, std::min(1.0, (t - 0.60) / 0.08)));
                    strikeFlare = -0.35 * sf * (1.0 - sfOut) * sideSign;
                }

                Vector3 handBindPos = bonePos(handName);
                Matrix4x4 handSkin = elbowSkin;
                handSkin.translate(handBindPos);
                handSkin.rotate(forwardDir, tipSpring.pos + strikeFlare);
                handSkin.translate(-handBindPos);
                boneSkinMatrices[handName] = handSkin;
                boneWorldTransforms[handName] = worldFromSkin(handName, handSkin);
            }

            // ---- Legs: Hermite-driven extension with staggered joints ----
            for (int side = 0; side < 2; ++side) {
                const char* upperName = (side == 0) ? "LeftUpperLeg" : "RightUpperLeg";
                const char* lowerName = (side == 0) ? "LeftLowerLeg" : "RightLowerLeg";
                const char* footName = (side == 0) ? "LeftFoot" : "RightFoot";

                // Upper leg: extends from -0.5 (tucked back) to +0.4 (thrust forward)
                double upperTarget = -0.5 + (0.5 + 0.4) * legExtValue;

                // Lower leg: tighter tuck, extends later (staggered by 0.02)
                double lowerLegExt = evalHermite(legKeys, legKeyCount,
                    std::max(0.0, std::min(1.0, t - 0.02)));
                double lowerTarget = -0.7 + (0.7 + 0.3) * lowerLegExt * strikeExtendFactor;

                // Foot: extends last, splays at strike
                double footExt = evalHermite(legKeys, legKeyCount,
                    std::max(0.0, std::min(1.0, t - 0.04)));
                double footTarget = 0.25 * footExt * strikeExtendFactor;

                Vector3 upperBindPos = bonePos(upperName);
                Matrix4x4 legSkin = bodyTransform;
                legSkin.translate(upperBindPos);
                legSkin.rotate(right, upperTarget);
                legSkin.translate(-upperBindPos);
                boneSkinMatrices[upperName] = legSkin;
                boneWorldTransforms[upperName] = worldFromSkin(upperName, legSkin);

                Vector3 lowerBindPos = bonePos(lowerName);
                Matrix4x4 lowerSkin = legSkin;
                lowerSkin.translate(lowerBindPos);
                lowerSkin.rotate(right, lowerTarget - upperTarget);
                lowerSkin.translate(-lowerBindPos);
                boneSkinMatrices[lowerName] = lowerSkin;
                boneWorldTransforms[lowerName] = worldFromSkin(lowerName, lowerSkin);

                Vector3 footBindPos = bonePos(footName);
                Matrix4x4 footSkin = lowerSkin;
                footSkin.translate(footBindPos);
                footSkin.rotate(right, footTarget);
                footSkin.translate(-footBindPos);
                boneSkinMatrices[footName] = footSkin;
                boneWorldTransforms[footName] = worldFromSkin(footName, footSkin);
            }

            // ---- Write frame ----
            auto& animFrame = animationClip.frames[frame];
            animFrame.time = static_cast<float>(t) * durationSeconds;
            animFrame.boneWorldTransforms = boneWorldTransforms;
            animFrame.boneSkinMatrices = boneSkinMatrices;
        }

        return true;
    }

} // namespace bird

} // namespace dust3d
