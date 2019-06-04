#include "spectrumview.h"
#include <iostream>

QT_CHARTS_USE_NAMESPACE

spectrumView::spectrumView(QWidget *parent) : QChartView(new QChart(), parent) {
    this->chart->legend()->hide();
    this->chart->addSeries(this->series);
    this->chart->addAxis(this->axisX, Qt::AlignBottom);
    this->chart->addAxis(this->axisY, Qt::AlignLeft);

    // TODO: chart theme

    this->series->attachAxis(this->axisX);
    this->series->attachAxis(this->axisY);
//    this->series->setUseOpenGL(true);

    this->setChart(this->chart);
    this->setRenderHint(QPainter::Antialiasing);
}

void spectrumView::scaleY(qreal factor) {
    //    qreal center = (this->axisY->max() + this->axisY->min()) * 0.5;
    //    this->axisY->setMax((this->axisY->max() - center) * factor);
    //    this->axisY->setMin((center - this->axisY->min()) * factor);
    this->axisY->setMax(std::ceil(this->axisY->max() * factor));
}

void spectrumView::wheelEvent(QWheelEvent *event) {
    qreal factor = event->angleDelta().y() > 0 ? 0.8 : 1.2;
    this->scaleY(factor);
    event->accept();
    QChartView::wheelEvent(event);
}

void spectrumView::keyPressEvent(QKeyEvent *event) {
    switch(event->key()) {
    case Qt::Key_Up: {
        qreal shift = (this->axisY->max() - this->axisY->min()) * 0.1;
        this->axisY->setRange(this->axisY->min() + shift, this->axisY->max() + shift);
        break;
    }
    case Qt::Key_Down: {
        qreal shift = (this->axisY->max() - this->axisY->min()) * 0.1;
        if(shift > this->axisY->min()) shift = this->axisY->min();
        this->axisY->setRange(this->axisY->min() - shift, this->axisY->max() - shift);
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
        if(this->cursor >= 1) this->cursor--;
        this->update();
        emit spectrumUpdated();
        break;
    }
    case Qt::Key_Right: {
        if(this->cursor < this->series->count() - 1) this->cursor++;
        this->update();
        emit spectrumUpdated();
        break;
    }
    }

    event->accept();
    QChartView::keyPressEvent(event);
}

template <class T>
QPointF spectrumView::pointToSeriesData(T point) {
    auto scenePos = mapToScene(QPoint(static_cast<int>(point.x()), static_cast<int>(point.y())));
    auto chartItemPos = this->chart->mapFromScene(scenePos);
    auto valueGivenSeries = this->chart->mapToValue(chartItemPos);

    if(valueGivenSeries.x() < this->axisX->min()) valueGivenSeries.setX(this->axisX->min());
    if(valueGivenSeries.x() > this->axisX->max()) valueGivenSeries.setX(this->axisX->max());

    return valueGivenSeries;
}

qreal spectrumView::channelToPointX(uint64_t chn) {
    if(this->series->count() == 0) return 0;

    qreal chn_width = this->chart->plotArea().width() / this->series->count();
    return this->chart->plotArea().left() + chn_width * chn;
}

void spectrumView::mousePressEvent(QMouseEvent *event) {
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

void spectrumView::mouseMoveEvent(QMouseEvent *event) {
    if((event->buttons() & Qt::LeftButton) && !this->simQueueSyncTimer->isActive()) {
        if((event->pos() - this->dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
            // TODO: click logic
        } else {
            QRect rect = this->chart->plotArea().toRect();
            int width = event->pos().x() - this->rubberBandOrigin.x();
            int height = event->pos().y() - this->rubberBandOrigin.y();

            this->rubberBandOrigin.setY(rect.top());
            height = rect.height();

            this->rubberBand->setGeometry(QRect(this->rubberBandOrigin.x(), this->rubberBandOrigin.y(), width, height).normalized());
            this->rubberBand->show();
        }
    } else if(event->buttons() == Qt::NoButton) { // hover
        if(this->chart->series().size() != 0) {
            auto point = this->pointToSeriesData(event->pos());
            auto r = this->findRoi(point);
            if(r != nullptr) {
                this->tooltip->setText(QString("ROI %1\n To %2").arg(r->first).arg(r->second));
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

void spectrumView::mouseReleaseEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        if((event->pos() - this->dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
            //            auto data = this->pointToSeriesData(event->pos());
            //            qDebug() << "click on" << data.x() << "," << data.y();
            //            if(data.x() >= 0 && data.x() < this->series->count()) {
            //                this->cursor = data.x();
            //            }
        } else if(!this->simQueueSyncTimer->isActive()) {
            this->rubberBand->hide();

            QRectF rect = this->rubberBand->geometry();
            rect.setY(this->chart->plotArea().y());
            rect.setHeight(this->chart->plotArea().height());

            std::pair<uint64_t, uint64_t> region;

            region.first = this->pointToSeriesData(rect.bottomLeft()).x();
            region.second = this->pointToSeriesData(rect.bottomRight()).x();

            this->addRoi(region);
        }
    }
}

void spectrumView::startSimAcq(double inteval) {
    this->clearAcqData();
    for(size_t i = 0; i < 1024; i++) {
        this->series->append(i, 0);
    }
    this->simQueue.resize(1024);
    this->axisX->setRange(0, 1024);

    this->dist = new std::binomial_distribution(this->series->count(), 0.5);
    this->axisY->setRange(0, 512);

    connect(this->simTimer, SIGNAL(timeout()), this, SLOT(simEnqueue()));
//    this->simTimer->setTimerType(Qt::PreciseTimer);
    this->simTimer->start(inteval);
    this->simInteval = inteval;

    connect(this->simQueueSyncTimer, SIGNAL(timeout()), this, SLOT(simQueueSync()));
    this->simQueueSyncTimer->start(500);
    this->simStartTime = QDateTime::currentMSecsSinceEpoch();
}

void spectrumView::stopSimAcq() {
    if(this->simQueueSyncTimer->isActive()) {
        this->simTimer->stop();
        this->simQueueSyncTimer->stop();
    }
}

void spectrumView::clearAcqData() {
    this->clearRoi();
    this->series->clear();
    this->max = QPointF(0, 0);
    this->sum = 0;
    this->simQueue.clear();
}

void spectrumView::simEnqueue() {
    uint32_t x;

//    do {
        x = this->dist->operator()(this->gen);
//    } while(x >= this->series->count());

    this->simQueue[x] += 2;
}

void spectrumView::simQueueSync() {
//    this->simTimer->stop();

    for(uint64_t x = 0; x < this->series->count(); x++) {
        auto point = this->series->at(x);
        this->series->replace(x, x, point.y() + this->simQueue[x]);
        this->sum += this->simQueue[x];

        if(point.y() + this->simQueue[x] > this->max.y()) {
            this->max.setX(x);
            this->max.setY(point.y() + this->simQueue[x]);
        }
    }

    this->simQueue.clear();
    this->simQueue.resize(1024);

//    this->simTimer->start(this->simInteval);
    if(this->axisY->max() <= this->max.y() * 1.2) this->scaleY(1.2);
    emit simUpdated(this->simStartTime);
}

void spectrumView::paintEvent(QPaintEvent *event) {
    QChartView::paintEvent(event);

    if(this->series->count() > 0) {
        qreal curX = this->channelToPointX(this->cursor);
        QPainter painter(this->viewport());
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QColor(0, 0, 0));
        painter.setBrush(painter.pen().color());
        painter.drawLine(curX, this->chart->plotArea().bottom(), curX, this->chart->plotArea().top());
    }

    event->accept();
}

uint64_t spectrumView::getSum() const {
    return sum;
}

void spectrumView::setSum(const uint64_t &value) {
    sum = value;
}

bool compareRoi(std::pair<uint64_t, uint64_t> l, std::pair<uint64_t, uint64_t> r) {
    return (l.first < r.first);
}

void spectrumView::redrawRoi() {
    // TODO: wait for better way to do that
    // removeAllSeries() will actually DELETE the series, so copy one first
    QLineSeries *newSeries = new QLineSeries();
    for(auto point : this->series->pointsVector()) newSeries->append(point);

    // clean everything and add back our spectrum series
    this->chart->removeAllSeries();
    this->series = newSeries;
    this->chart->addSeries(this->series);

    this->series->attachAxis(this->axisX);
    this->series->attachAxis(this->axisY);

    // use QAreaSeries to fill roi
    QPen pen = this->series->pen();
    QBrush brush(Qt::yellow); // TODO: config

    for(size_t i = 0; i < this->roi.size(); i++) {
        QLineSeries *top_line = new QLineSeries();
        QLineSeries *bottom_line = new QLineSeries();
        for(uint64_t k = this->roi[i].first; k < this->roi[i].second; k++) {
            top_line->append(this->series->pointsVector().at(k));
            bottom_line->append(this->series->pointsVector().at(k).x(), 0);
        }
        QAreaSeries *area = new QAreaSeries(top_line, bottom_line);
        area->setPen(pen);
        area->setBrush(brush);
        this->chart->addSeries(area);
        area->attachAxis(this->axisX);
        area->attachAxis(this->axisY);
    }
}

std::pair<uint64_t, uint64_t> *spectrumView::findRoi(QPointF point) {
    for(auto it = this->roi.begin(); it != this->roi.end(); it++) {
        if((point.x() >= (*it).first) && (point.x() < (*it).second)) {
            return &(*it);
        }
    }
    return nullptr;
}

void spectrumView::addRoi(std::pair<uint64_t, uint64_t> r) {
    this->roi.push_back(r);
    std::sort(this->roi.begin(), this->roi.end(), compareRoi);
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

    qDebug() << "Add ROI:" << r.first << "," << r.second;
    qDebug() << "Now we have" << this->roi.size() << "ROIs";
    this->redrawRoi();
}

bool spectrumView::removeRoi(uint64_t channel) {
    for(auto it = this->roi.begin(); it != this->roi.end(); ++it) {
        if(channel >= (*it).first && channel < (*it).second) {
            this->roi.erase(it);
            this->redrawRoi();
            return true;
        }
    }
    return false;
}

bool spectrumView::removeRoi(std::pair<uint64_t, uint64_t> r) {
    for(auto it = this->roi.begin(); it != this->roi.end(); it++) {
        if((*it) == r) {
            this->roi.erase(it);
            this->redrawRoi();
            return true;
        }
    }

    return false;
}

void spectrumView::clearRoi() {
    this->roi.clear();
    this->redrawRoi();
}

QChart *spectrumView::getChart() const {
    return chart;
}

QLineSeries *spectrumView::getSeries() const {
    return series;
}

QValueAxis *spectrumView::getAxisY() const {
    return axisY;
}

QValueAxis *spectrumView::getAxisX() const {
    return axisX;
}

QPointF spectrumView::getMax() const {
    return max;
}

void spectrumView::setMax(const QPointF &value) {
    max = value;
}
uint64_t spectrumView::getCursor() const {
    return cursor;
}
