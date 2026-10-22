#pragma once

#include "Parameters.h"

#include <string>
#include <vector>

namespace cerberus
{

struct PresetValue
{
    const char* id;
    float value;
};

struct FactoryPreset
{
    const char* name;
    std::vector<PresetValue> overrides;
};

inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets
    {
        {
            "Funk Sweep",
            {}
        },
        {
            "Rubber Bass",
            {
                { params::wahSensitivity, 0.68f }, { params::wahStart, 120.0f },
                { params::wahRange, 1.6f }, { params::wahResonance, 7.5f },
                { params::wahAttack, 12.0f }, { params::wahRelease, 240.0f },
                { params::compThreshold, -28.0f }, { params::compRatio, 6.0f },
                { params::compAttack, 12.0f }, { params::compRelease, 180.0f },
                { params::compMakeup, 7.0f }, { params::compMix, 0.90f },
                { params::distDrive, 10.0f }, { params::distTone, 0.35f },
                { params::distMix, 0.35f }, { params::distOutput, -2.0f },
                { params::distMode, 1.0f }
            }
        },
        {
            "Crushed Drums",
            {
                { params::wahOn, 0.0f },
                { params::compThreshold, -32.0f }, { params::compRatio, 8.0f },
                { params::compAttack, 4.0f }, { params::compRelease, 70.0f },
                { params::compMakeup, 8.0f },
                { params::distDrive, 22.0f }, { params::distTone, 0.55f },
                { params::distMix, 0.90f }, { params::distOutput, -4.0f },
                { params::distMode, 1.0f }
            }
        },
        {
            "Vocal Glue",
            {
                { params::wahOn, 0.0f },
                { params::compThreshold, -22.0f }, { params::compRatio, 3.0f },
                { params::compAttack, 20.0f }, { params::compRelease, 160.0f },
                { params::compMakeup, 2.5f }, { params::compMix, 0.90f },
                { params::distDrive, 4.0f }, { params::distTone, 0.80f },
                { params::distMix, 0.22f },
                { params::distMode, 0.0f }
            }
        },
        {
            "Lo-Fi Lead",
            {
                { params::wahSensitivity, 0.45f }, { params::wahStart, 500.0f },
                { params::wahRange, 2.3f }, { params::wahResonance, 3.5f },
                { params::wahAttack, 4.0f }, { params::wahRelease, 110.0f },
                { params::compThreshold, -26.0f }, { params::compRatio, 4.0f },
                { params::compAttack, 6.0f }, { params::compRelease, 100.0f },
                { params::compMakeup, 5.0f },
                { params::distDrive, 26.0f }, { params::distTone, 0.45f },
                { params::distMix, 1.0f }, { params::distOutput, -4.0f },
                { params::distMode, 2.0f }
            }
        },
        {
            "Gentle Bus",
            {
                { params::wahOn, 0.0f },
                { params::compThreshold, -16.0f }, { params::compRatio, 2.0f },
                { params::compAttack, 30.0f }, { params::compRelease, 300.0f },
                { params::compMakeup, 1.0f }, { params::compMix, 0.85f },
                { params::distDrive, 2.0f }, { params::distTone, 0.70f },
                { params::distMix, 0.12f }
            }
        },
        {
            "Broken Radio",
            {
                { params::inputGain, -3.0f }, { params::outputGain, -2.0f },
                { params::wahOn, 0.0f },
                { params::compThreshold, -38.0f }, { params::compRatio, 14.0f },
                { params::compAttack, 2.0f }, { params::compRelease, 80.0f },
                { params::compMakeup, 10.0f },
                { params::distDrive, 32.0f }, { params::distTone, 0.22f },
                { params::distMix, 1.0f }, { params::distOutput, -8.0f },
                { params::distMode, 2.0f }
            }
        },
        {
            "Clean Slate",
            {
                { params::wahOn, 0.0f },
                { params::compOn, 0.0f },
                { params::distOn, 0.0f },
                { params::inputGain, 0.0f }, { params::outputGain, 0.0f }
            }
        }
    };

    return presets;
}

inline int getNumFactoryPresets()
{
    return static_cast<int> (getFactoryPresets().size());
}

inline std::string getFactoryPresetName (int index)
{
    const auto& presets = getFactoryPresets();

    if (index >= 0 && index < static_cast<int> (presets.size()))
        return presets[static_cast<size_t> (index)].name;

    return {};
}

} // namespace cerberus
