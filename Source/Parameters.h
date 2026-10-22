#pragma once

#include <array>

namespace cerberus::params
{

inline constexpr auto inputGain = "inputGain";
inline constexpr auto outputGain = "outputGain";

inline constexpr auto wahOn = "wahOn";
inline constexpr auto wahSensitivity = "wahSensitivity";
inline constexpr auto wahStart = "wahStart";
inline constexpr auto wahRange = "wahRange";
inline constexpr auto wahResonance = "wahResonance";
inline constexpr auto wahAttack = "wahAttack";
inline constexpr auto wahRelease = "wahRelease";

inline constexpr auto compOn = "compOn";
inline constexpr auto compThreshold = "compThreshold";
inline constexpr auto compRatio = "compRatio";
inline constexpr auto compAttack = "compAttack";
inline constexpr auto compRelease = "compRelease";
inline constexpr auto compMakeup = "compMakeup";
inline constexpr auto compMix = "compMix";

inline constexpr auto distOn = "distOn";
inline constexpr auto distDrive = "distDrive";
inline constexpr auto distTone = "distTone";
inline constexpr auto distMix = "distMix";
inline constexpr auto distOutput = "distOutput";
inline constexpr auto distMode = "distMode";

inline constexpr std::array<const char*, 22> allParameterIDs
{
    inputGain, outputGain,
    wahOn, wahSensitivity, wahStart, wahRange, wahResonance, wahAttack, wahRelease,
    compOn, compThreshold, compRatio, compAttack, compRelease, compMakeup, compMix,
    distOn, distDrive, distTone, distMix, distOutput, distMode
};

} // namespace cerberus::params
