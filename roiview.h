#ifndef ROIVIEW_H
#define ROIVIEW_H

#include "basicspectrumview.h"
#include <QWidget>
#include <utility>

class RoiView : public BasicSpectrumView {
    Q_OBJECT

  public:
    RoiView(BasicSpectrumView *parent = nullptr);
    RoiView(QWidget *parent = nullptr);

  public Q_SLOTS:
    void updateRoi(PointsVector data);
    void clearRoi();
    void updateCursor(uint64_t cur);
};

#endif // ROIVIEW_H
