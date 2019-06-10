#ifndef DUST3D_BONE_NAME_CONVERTER_H
#define DUST3D_BONE_NAME_CONVERTER_H
#include <QString>
#include <map>
#include <vector>

class BoneNameConverter
{
public:
    void addBoneName(const QString &boneName);
    bool convertToReadable();
    void convertFromReadable();
    const std::map<QString, QString> &converted();
private:
    std::vector<QString> m_originalNames;
    std::map<QString, QString> m_convertedMap;
};

#endif
