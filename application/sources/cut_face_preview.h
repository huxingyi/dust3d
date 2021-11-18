#ifndef DUST3D_APPLICATION_CUT_FACE_PREVIEW_H_
#define DUST3D_APPLICATION_CUT_FACE_PREVIEW_H_

#include <QImage>
#include <dust3d/base/cut_face.h>

QImage *buildCutFaceTemplatePreviewImage(const std::vector<dust3d::Vector2> &cutTemplate);
const QImage *cutFacePreviewImage(dust3d::CutFace cutFace);

#endif
