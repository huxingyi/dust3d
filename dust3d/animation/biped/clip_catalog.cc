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

#include <dust3d/animation/biped/bow.h>
#include <dust3d/animation/biped/bow_aim.h>
#include <dust3d/animation/biped/bow_draw.h>
#include <dust3d/animation/biped/bow_shot.h>
#include <dust3d/animation/biped/cast_recover.h>
#include <dust3d/animation/biped/cast_release.h>
#include <dust3d/animation/biped/cast_start.h>
#include <dust3d/animation/biped/channel_enter.h>
#include <dust3d/animation/biped/channel_exit.h>
#include <dust3d/animation/biped/channel_interrupt.h>
#include <dust3d/animation/biped/cheer.h>
#include <dust3d/animation/biped/chop.h>
#include <dust3d/animation/biped/clap.h>
#include <dust3d/animation/biped/clip_catalog.h>
#include <dust3d/animation/biped/crouch_enter.h>
#include <dust3d/animation/biped/crouch_exit.h>
#include <dust3d/animation/biped/crouch_idle.h>
#include <dust3d/animation/biped/dance.h>
#include <dust3d/animation/biped/dodge_roll.h>
#include <dust3d/animation/biped/drink.h>
#include <dust3d/animation/biped/eat.h>
#include <dust3d/animation/biped/gather.h>
#include <dust3d/animation/biped/get_up.h>
#include <dust3d/animation/biped/interact.h>
#include <dust3d/animation/biped/knockdown.h>
#include <dust3d/animation/biped/mine.h>
#include <dust3d/animation/biped/mounted_idle.h>
#include <dust3d/animation/biped/mounted_ride.h>
#include <dust3d/animation/biped/one_hand_slash.h>
#include <dust3d/animation/biped/parry.h>
#include <dust3d/animation/biped/pick_up.h>
#include <dust3d/animation/biped/point.h>
#include <dust3d/animation/biped/sit_down.h>
#include <dust3d/animation/biped/sit_idle.h>
#include <dust3d/animation/biped/sleep_idle.h>
#include <dust3d/animation/biped/sleep_lie_down.h>
#include <dust3d/animation/biped/sneak.h>
#include <dust3d/animation/biped/sprint.h>
#include <dust3d/animation/biped/stand_up.h>
#include <dust3d/animation/biped/stunned.h>
#include <dust3d/animation/biped/swim_backward.h>
#include <dust3d/animation/biped/swim_forward.h>
#include <dust3d/animation/biped/swim_idle.h>
#include <dust3d/animation/biped/swim_left.h>
#include <dust3d/animation/biped/swim_right.h>
#include <dust3d/animation/biped/talk.h>
#include <dust3d/animation/biped/two_hand_swing.h>
#include <dust3d/animation/biped/wake_up.h>
#include <dust3d/animation/biped/wave.h>
namespace dust3d {
namespace biped {
    const std::vector<ClipDefinition>& additionalClips()
    {
        static const std::vector<ClipDefinition> clips = {
            { "BipedSprint", 0.65, 32, true, "cycle", "cycle", sprint },
            { "BipedSneak", 1.4, 44, true, "crouched", "crouched", sneak },
            { "BipedCrouchEnter", 0.45, 24, false, "relaxed", "crouched", crouchEnter },
            { "BipedCrouchIdle", 2, 60, true, "crouched", "crouched", crouchIdle },
            { "BipedCrouchExit", 0.45, 24, false, "crouched", "relaxed", crouchExit },
            { "BipedMountedIdle", 2, 60, true, "mounted", "mounted", mountedIdle },
            { "BipedMountedRide", 1, 40, true, "mounted", "mounted", mountedRide },
            { "BipedStunned", 1.8, 60, true, "stunned", "stunned", stunned },
            { "BipedKnockdown", 0.9, 48, false, "relaxed", "knockedDown", knockdown },
            { "BipedGetUp", 2.4, 80, false, "knockedDown", "relaxed", getUp },
            { "BipedDodgeRoll", 0.8, 48, false, "relaxed", "relaxed", dodgeRoll },
            { "BipedSwimIdle", 2, 48, true, "swimming", "swimming", swimIdle },
            { "BipedSwimForward", 1.4, 48, true, "swimming", "swimming", swimForward },
            { "BipedSwimBackward", 1.4, 48, true, "swimming", "swimming", swimBackward },
            { "BipedSwimLeft", 1.4, 48, true, "swimming", "swimming", swimLeft },
            { "BipedSwimRight", 1.4, 48, true, "swimming", "swimming", swimRight },
            { "BipedOneHandSlash", 0.8, 40, false, "relaxed", "relaxed", oneHandSlash },
            { "BipedTwoHandSwing", 1.1, 48, false, "relaxed", "relaxed", twoHandSwing },
            { "BipedBowDraw", 0.8, 40, false, "relaxed", "bowAim", bowDraw },
            { "BipedBowAim", 2, 60, true, "bowAim", "bowAim", bowAim },
            { "BipedBowShot", 0.7, 36, false, "bowAim", "relaxed", bowShot },
            { "BipedParry", 0.6, 32, false, "relaxed", "relaxed", parry },
            { "BipedCastStart", 0.25, 20, false, "relaxed", "castPrepared", castStart },
            { "BipedCastRelease", 0.3, 24, false, "castPrepared", "castReleased", castRelease },
            { "BipedCastRecover", 0.45, 28, false, "castReleased", "relaxed", castRecover },
            { "BipedChannelEnter", 0.5, 28, false, "relaxed", "channel", channelEnter },
            { "BipedChannelExit", 0.5, 28, false, "channel", "relaxed", channelExit },
            { "BipedChannelInterrupt", 0.3, 24, false, "channel", "relaxed", channelInterrupt },
            { "BipedGather", 1.8, 60, false, "relaxed", "relaxed", gather },
            { "BipedPickUp", 1.2, 48, false, "relaxed", "relaxed", pickUp },
            { "BipedInteract", 1, 36, false, "relaxed", "relaxed", interact },
            { "BipedMine", 1.2, 48, true, "work", "work", mine },
            { "BipedChop", 1.2, 48, true, "work", "work", chop },
            { "BipedDrink", 2.2, 64, false, "relaxed", "relaxed", drink },
            { "BipedEat", 2.4, 72, false, "relaxed", "relaxed", eat },
            { "BipedSitDown", 1, 48, false, "relaxed", "seated", sitDown },
            { "BipedSitIdle", 3, 72, true, "seated", "seated", sitIdle },
            { "BipedStandUp", 1, 48, false, "seated", "relaxed", standUp },
            { "BipedSleepLieDown", 3, 90, false, "relaxed", "sleeping", sleepLieDown },
            { "BipedSleepIdle", 4, 90, true, "sleeping", "sleeping", sleepIdle },
            { "BipedWakeUp", 3, 90, false, "sleeping", "relaxed", wakeUp },
            { "BipedWave", 2, 64, false, "relaxed", "relaxed", wave },
            { "BipedCheer", 2, 64, false, "relaxed", "relaxed", cheer },
            { "BipedBow", 1.8, 60, false, "relaxed", "relaxed", bow },
            { "BipedPoint", 1.5, 48, false, "relaxed", "relaxed", point },
            { "BipedClap", 2, 64, false, "relaxed", "relaxed", clap },
            { "BipedDance", 2.4, 80, true, "cycle", "cycle", dance },
            { "BipedTalk", 3, 90, true, "cycle", "cycle", talk },
        };
        return clips;
    }
    const ClipDefinition* additionalClip(const std::string& type)
    {
        for (const auto& clip : additionalClips())
            if (type == clip.type)
                return &clip;
        return nullptr;
    }
} // namespace biped
} // namespace dust3d
