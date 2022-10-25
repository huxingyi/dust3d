#ifndef DUST3D_APPLICATION_UV_MAP_GENERATOR_H_
#define DUST3D_APPLICATION_UV_MAP_GENERATOR_H_

#include <QObject>
#include <dust3d/base/object.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/uv/uv_map_packer.h>
#include <memory>

class UvMapGenerator : public QObject {
    Q_OBJECT
public:
    UvMapGenerator(std::unique_ptr<dust3d::Object> object, std::unique_ptr<dust3d::Snapshot> snapshot);
    void generate();
    std::unique_ptr<dust3d::Object> takeObject();
signals:
    void finished();
public slots:
    void process();

private:
    std::unique_ptr<dust3d::Object> m_object;
    std::unique_ptr<dust3d::Snapshot> m_snapshot;
    std::unique_ptr<dust3d::UvMapPacker> m_mapPacker;
};

#endif
