#include "spectrumview.h"
#include <iostream>

QT_CHARTS_USE_NAMESPACE

SpectrumView::SpectrumView(QWidget *parent) : BasicSpectrumView(new BasicSpectrumView(parent)) {
}

void SpectrumView::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_Up: {
        qreal shift = (this->getAxisY()->max() - this->getAxisY()->min()) * 0.1;
        this->getAxisY()->setRange(this->getAxisY()->min() + shift, this->getAxisY()->max() + shift);
        break;
    }
    case Qt::Key_Down: {
        qreal shift = (this->getAxisY()->max() - this->getAxisY()->min()) * 0.1;
        if(shift > this->getAxisY()->min()) shift = this->getAxisY()->min();
        this->getAxisY()->setRange(this->getAxisY()->min() - shift, this->getAxisY()->max() - shift);
        break;
    }
    case Qt::Key_Plus: {
        this->scaleY(1.2);
        break;
    }
    case Qt::Key_Minus: {
        this->scaleY(0.8);
        break;
    }
    case Qt::Key_Left: {
        if(this->getCursor() >= 1) this->setCursor(this->getCursor() - 1);
        emit cursorUpdated(this->getCursor());
        break;
    }
    case Qt::Key_Right: {
        if(this->getCursor() < this->getSeries()->count() - 1) this->setCursor(this->getCursor() + 1);
        emit cursorUpdated(this->getCursor());
        break;
    }
    }

    event->accept();
    QChartView::keyPressEvent(event);
}

void SpectrumView::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        this->dragStartPosition = event->pos();
        if(!this->simTimer->isActive()) {
            this->rubberBandOrigin = event->pos();
            this->rubberBand->setGeometry(QRect(this->rubberBandOrigin, QSize()));
        }
    } else if(event->button() == Qt::RightButton) {
        this->removeRoi(this->pointToSeriesData(event->pos()).x());
    }

    event->accept();
}

void SpectrumView::mouseMoveEvent(QMouseEvent *event) {
    if((event->buttons() & Qt::LeftButton) && !this->simQueueSyncTimer->isActive()) {
        if((event->pos() - this->dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
            // TODO: click logic
        } else {
            QRect rect = this->getChart()->plotArea().toRect();
            int width = event->pos().x() - this->rubberBandOrigin.x();
            int height = event->pos().y() - this->rubberBandOrigin.y();

            this->rubberBandOrigin.setY(rect.top());
            height = rect.height();

            this->rubberBand->setGeometry(QRect(this->rubberBandOrigin.x(), this->rubberBandOrigin.y(), width, height).normalized());
            this->rubberBand->show();
        }
    } else if(event->buttons() == Qt::NoButton) { // hover
        if(this->getChart()->series().size() != 0) {
            auto point = this->pointToSeriesData(event->pos());
            auto r = this->findRoi(point);
            if(r != nullptr) {
                uint64_t sum = 0;
                for(auto i = r->first; i < r->second; i++) sum += this->getSeries()->at(i).y();
                this->tooltip->setText(QString("ROI %1\n To %2\n Sum %3").arg(r->first).arg(r->second).arg(sum));
                this->tooltip->setAnchor(point);
                this->tooltip->setZValue(10);
                this->tooltip->updateGeometry();
                this->tooltip->show();
            } else {
                this->tooltip->hide();
            }
        }
    }

    event->accept();
}

void SpectrumView::mouseReleaseEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        if((event->pos() - this->dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
            auto data = this->pointToSeriesData(event->pos());
            auto roi = this->findRoi(data);
            if(roi != nullptr) {
                PointsVector roiData;
                for(size_t i = roi->first; i < roi->second; i++) {
                    roiData.push_back(this->getSeries()->pointsVector().at(i));
                }
                emit roiUpdated(roiData);
            }

            this->setCursor(data.x());
            emit cursorUpdated(this->getCursor());
        } else if(!this->simQueueSyncTimer->isActive()) {
            this->rubberBand->hide();

            QRectF rect = this->rubberBand->geometry();
            rect.setY(this->getChart()->plotArea().y());
            rect.setHeight(this->getChart()->plotArea().height());

            std::pair<uint64_t, uint64_t> region;

            region.first = this->pointToSeriesData(rect.bottomLeft()).x();
            region.second = this->pointToSeriesData(rect.bottomRight()).x();

            this->addRoi(region);
        }
    }
}

void SpectrumView::startSimAcq(double inteval) {
    for(size_t i = 0; i < 1024; i++) {
        this->getSeries()->append(i, 0);
    }
    this->simQueue.resize(1024);
    this->getAxisX()->setRange(0, 1024);

    this->dist = new std::binomial_distribution(this->getSeries()->count(), 0.5);
    this->getAxisY()->setRange(0, 512);

    connect(this->simTimer, SIGNAL(timeout()), this, SLOT(simEnqueue()));
    //    this->simTimer->setTimerType(Qt::PreciseTimer);
    this->simTimer->start(inteval);
    this->simInteval = inteval;

    connect(this->simQueueSyncTimer, SIGNAL(timeout()), this, SLOT(simQueueSync()));
    this->simQueueSyncTimer->start(500);
    this->simStartTime = QDateTime::currentMSecsSinceEpoch();
}

void SpectrumView::stopSimAcq() {
    if(this->simQueueSyncTimer->isActive()) {
        this->simTimer->stop();
        this->simQueueSyncTimer->stop();
    }
}

void SpectrumView::clearAcqData() {
    this->clearRoi();
    this->getSeries()->clear();
    this->setMax(QPointF(0, 0));
    this->setSum(0);
    this->simQueue.clear();
}

void SpectrumView::simEnqueue() {
    this->simQueue[this->dist->operator()(this->gen)] += 2;
}

void SpectrumView::simQueueSync() {
    for(uint64_t x = 0; x < this->getSeries()->count(); x++) {
        auto point = this->getSeries()->at(x);
        this->getSeries()->replace(x, x, point.y() + this->simQueue[x]);
        this->setSum(this->getSum() + this->simQueue[x]);

        if(point.y() + this->simQueue[x] > this->getMax().y()) {
            this->getMax().setX(x);
            this->getMax().setY(point.y() + this->simQueue[x]);
        }
    }

    this->simQueue.clear();
    this->simQueue.resize(1024);

    if(this->getAxisY()->max() <= this->getMax().y() * 1.2) this->scaleY(1.2);
    emit simUpdated(this->simStartTime);
}

uint64_t SpectrumView::getSimStartTime() const {
    return simStartTime;
}

bool SpectrumView::compareRoi(std::pair<uint64_t, uint64_t> l, std::pair<uint64_t, uint64_t> r) {
    return (l.first < r.first);
}

void SpectrumView::redrawRoi() {
    // TODO: wait for better way to do that
    // removeAllSeries() will actually DELETE the series, so copy one first
    QLineSeries *newSeries = new QLineSeries();
    for(auto point : this->getSeries()->pointsVector()) newSeries->append(point);

    // clean everything and add back our spectrum series
    this->getChart()->removeAllSeries();
    this->setSeries(newSeries);
    this->getChart()->addSeries(this->getSeries());

    this->getSeries()->attachAxis(this->getAxisX());
    this->getSeries()->attachAxis(this->getAxisY());

    // use QAreaSeries to fill roi
    QPen pen = this->getSeries()->pen();
    QBrush brush(Qt::yellow); // TODO: config

    for(size_t i = 0; i < this->roi.size(); i++) {
        QLineSeries *top_line = new QLineSeries();
        QLineSeries *bottom_line = new QLineSeries();
        for(uint64_t k = this->roi[i].first; k < this->roi[i].second; k++) {
            top_line->append(this->getSeries()->pointsVector().at(k));
            bottom_line->append(this->getSeries()->pointsVector().at(k).x(), 0);
        }
        QAreaSeries *area = new QAreaSeries(top_line, bottom_line);
        area->setPen(pen);
        area->setBrush(brush);
        this->getChart()->addSeries(area);
        area->attachAxis(this->getAxisX());
        area->attachAxis(this->getAxisY());
    }
}

std::pair<uint64_t, uint64_t> *SpectrumView::findRoi(QPointF point) {
    for(auto it = this->roi.begin(); it != this->roi.end(); it++) {
        if((point.x() >= (*it).first) && (point.x() < (*it).second)) {
            return &(*it);
        }
    }
    return nullptr;
}

void SpectrumView::addRoi(std::pair<uint64_t, uint64_t> r) {
    this->roi.push_back(r);
    std::sort(this->roi.begin(), this->roi.end(), this->compareRoi);
    std::stack<std::pair<uint64_t, uint64_t> > s;
    s.push(this->roi[0]);

    // TODO: config for merging
    // Start from the next interval and merge if necessary
    for(size_t i = 1; i < this->roi.size(); i++) {
        // get interval from stack top
        auto top = s.top();

        // if current interval is not overlapping with stack top,
        // push it to the stack
        if(top.second < this->roi[i].first)
            s.push(this->roi[i]);

        // Otherwise update the ending time of top if ending of current
        // interval is more
        else if(top.second < this->roi[i].second) {
            top.second = this->roi[i].second;
            s.pop();
            s.push(top);
        }
    }

    this->roi.clear();
    while(!s.empty()) {
        this->roi.push_back(s.top());
        s.pop();
    }

    this->redrawRoi();
}

bool SpectrumView::removeRoi(uint64_t channel) {
    for(auto it = this->roi.begin(); it != this->roi.end(); ++it) {
        if(channel >= (*it).first && channel < (*it).second) {
            this->roi.erase(it);
            this->redrawRoi();
            return true;
        }
    }
    return false;
}

bool SpectrumView::removeRoi(std::pair<uint64_t, uint64_t> r) {
    for(auto it = this->roi.begin(); it != this->roi.end(); it++) {
        if((*it) == r) {
            this->roi.erase(it);
            this->redrawRoi();
            return true;
        }
    }

    return false;
}

void SpectrumView::clearRoi() {
    this->roi.clear();
    this->redrawRoi();
}
