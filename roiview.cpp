#include "roiview.h"

QT_CHARTS_USE_NAMESPACE

RoiView::RoiView(BasicSpectrumView *parent) : BasicSpectrumView(parent) {
}

RoiView::RoiView(QWidget *parent) : BasicSpectrumView(new BasicSpectrumView(parent)) {
}

void RoiView::updateRoi(PointsVector data) {
    this->clearRoi();
    for(auto p : data) {
        if(p.y() > this->getMax().y()) this->setMax(p);
        this->getSeries()->append(p);
    }

    this->getAxisX()->setRange(data.begin()->x(), (data.end() - 1)->x());
    this->getAxisY()->setRange(0, this->getMax().y());
}

void RoiView::clearRoi() {
    this->getSeries()->clear();
    this->setMax(QPointF(0, 0));
}

void RoiView::updateCursor(uint64_t cur)
{
    if(cur >= this->getAxisX()->min() && cur < this->getAxisX()->max()) {
        qDebug() << "update cursor:" << cur;
        this->setCursor(cur);
    }
    this->repaint();
}
