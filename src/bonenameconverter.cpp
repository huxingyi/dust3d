#include <QDebug>
#include "bonenameconverter.h"
#include "poser.h"

void BoneNameConverter::addBoneName(const QString &boneName)
{
    m_originalNames.push_back(boneName);
}

bool BoneNameConverter::convertToReadable()
{
    m_convertedMap.clear();
    std::map<QString, std::vector<QString>> chains;
    Poser::fetchChains(m_originalNames, chains);
    if (chains.find("LeftLimb3") != chains.end())
        return false;
    if (chains.find("RightLimb3") != chains.end())
        return false;
    auto findLeftLimb1 = chains.find("LeftLimb1");
    if (findLeftLimb1 != chains.end()) {
        for (const auto &it: findLeftLimb1->second) {
            if ("LeftLimb1_Joint1" == it) {
                m_convertedMap["LeftLimb1_Joint1"] = "LeftUpperLeg";
            } else if ("LeftLimb1_Joint2" == it) {
                m_convertedMap["LeftLimb1_Joint2"] = "LeftLowerLeg";
            } else if ("LeftLimb1_Joint3" == it) {
                m_convertedMap["LeftLimb1_Joint3"] = "LeftFoot";
            } else {
                return false;
            }
        }
    }
    auto findLeftLimb2 = chains.find("LeftLimb2");
    if (findLeftLimb2 != chains.end()) {
        for (const auto &it: findLeftLimb2->second) {
            if ("LeftLimb2_Joint1" == it) {
                m_convertedMap["LeftLimb2_Joint1"] = "LeftUpperArm";
            } else if ("LeftLimb2_Joint2" == it) {
                m_convertedMap["LeftLimb2_Joint2"] = "LeftLowerArm";
            } else if ("LeftLimb2_Joint3" == it) {
                m_convertedMap["LeftLimb2_Joint3"] = "LeftHand";
            } else {
                return false;
            }
        }
    }
    auto findRightLimb1 = chains.find("RightLimb1");
    if (findRightLimb1 != chains.end()) {
        for (const auto &it: findRightLimb1->second) {
            if ("RightLimb1_Joint1" == it) {
                m_convertedMap["RightLimb1_Joint1"] = "RightUpperLeg";
            } else if ("RightLimb1_Joint2" == it) {
                m_convertedMap["RightLimb1_Joint2"] = "RightLowerLeg";
            } else if ("RightLimb1_Joint3" == it) {
                m_convertedMap["RightLimb1_Joint3"] = "RightFoot";
            } else {
                return false;
            }
        }
    }
    auto findRightLimb2 = chains.find("RightLimb2");
    if (findRightLimb2 != chains.end()) {
        for (const auto &it: findRightLimb2->second) {
            if ("RightLimb2_Joint1" == it) {
                m_convertedMap["RightLimb2_Joint1"] = "RightUpperArm";
            } else if ("RightLimb2_Joint2" == it) {
                m_convertedMap["RightLimb2_Joint2"] = "RightLowerArm";
            } else if ("RightLimb2_Joint3" == it) {
                m_convertedMap["RightLimb2_Joint3"] = "RightHand";
            } else {
                return false;
            }
        }
    }
    auto findTail = chains.find("Tail");
    if (findTail != chains.end()) {
        auto findSpine = chains.find("Spine");
        if (findSpine != chains.end()) {
            for (const auto &it: findSpine->second) {
                if ("Spine3" == it) {
                    m_convertedMap["Spine3"] = "Spine";
                } else if ("Spine4" == it) {
                    m_convertedMap["Spine4"] = "Chest";
                }
            }
        }
    } else {
        auto findSpine = chains.find("Spine");
        if (findSpine != chains.end()) {
            for (const auto &it: findSpine->second) {
                if ("Spine1" == it) {
                    m_convertedMap["Spine1"] = "Spine";
                } else if ("Spine2" == it) {
                    m_convertedMap["Spine2"] = "Chest";
                }
            }
        }
    }
    return true;
}

void BoneNameConverter::convertFromReadable()
{
    bool hasTail = false;
    for (const auto &it: m_originalNames) {
        if (it.startsWith("Tail")) {
            hasTail = true;
        }
    }
    
    for (const auto &it: m_originalNames) {
        if ("LeftUpperLeg" == it) {
            m_convertedMap["LeftUpperLeg"] = "LeftLimb1_Joint1";
        } else if ("LeftLowerLeg" == it) {
            m_convertedMap["LeftLowerLeg"] = "LeftLimb1_Joint2";
        } else if ("LeftFoot" == it) {
            m_convertedMap["LeftFoot"] = "LeftLimb1_Joint3";
        } else if ("LeftUpperArm" == it) {
            m_convertedMap["LeftUpperArm"] = "LeftLimb2_Joint1";
        } else if ("LeftLowerArm" == it) {
            m_convertedMap["LeftLowerArm"] = "LeftLimb2_Joint2";
        } else if ("LeftHand" == it) {
            m_convertedMap["LeftHand"] = "LeftLimb2_Joint3";
        } else if ("RightUpperLeg" == it) {
            m_convertedMap["RightUpperLeg"] = "RightLimb1_Joint1";
        } else if ("RightLowerLeg" == it) {
            m_convertedMap["RightLowerLeg"] = "RightLimb1_Joint2";
        } else if ("RightFoot" == it) {
            m_convertedMap["RightFoot"] = "RightLimb1_Joint3";
        } else if ("RightUpperArm" == it) {
            m_convertedMap["RightUpperArm"] = "RightLimb2_Joint1";
        } else if ("RightLowerArm" == it) {
            m_convertedMap["RightLowerArm"] = "RightLimb2_Joint2";
        } else if ("RightHand" == it) {
            m_convertedMap["RightHand"] = "RightLimb2_Joint3";
        } else if ("Spine" == it) {
            m_convertedMap["Spine"] = hasTail ? "Spine3" : "Spine1";
        } else if ("Chest" == it) {
            m_convertedMap["Chest"] = hasTail ? "Spine4" : "Spine2";
        }
    }
}

const std::map<QString, QString> &BoneNameConverter::converted()
{
    return m_convertedMap;
}

