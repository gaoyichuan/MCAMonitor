#ifndef SPECTRUMVIEW_H
#define SPECTRUMVIEW_H

#include "basicspectrumview.h"

#include <QtCharts>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "callout.h"

QT_CHARTS_USE_NAMESPACE

class SpectrumView : public BasicSpectrumView {
    Q_OBJECT

  public:
    SpectrumView(BasicSpectrumView *parent = nullptr);
    SpectrumView(QWidget *parent = nullptr);

    static bool compareRoi(std::pair<uint64_t, uint64_t> l, std::pair<uint64_t, uint64_t> r);
    void addRoi(std::pair<uint64_t, uint64_t> r);
    // return true when success
    bool removeRoi(uint64_t channel);
    bool removeRoi(std::pair<uint64_t, uint64_t> r);
    void redrawRoi();
    void clearRoi();
    std::pair<uint64_t, uint64_t> *findRoi(QPointF point);

    uint64_t getSimStartTime() const;

  private Q_SLOTS:
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void simEnqueue();
    void simQueueSync();

  public Q_SLOTS:
    void startSimAcq(double inteval);
    void stopSimAcq();
    void clearAcqData();

  Q_SIGNALS:
    void simUpdated(uint64_t startTime);
    void spectrumUpdated();
    void roiUpdated(PointsVector data);
    void cursorUpdated(uint64_t cur);

  private:
    // simulation update queue
    std::vector<uint64_t> simQueue;
    QTimer *simQueueSyncTimer = new QTimer(this);

    // rubber band for selection
    QRubberBand *rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    QPoint rubberBandOrigin;
    QPoint dragStartPosition;

    // roi storage
    std::vector<std::pair<uint64_t, uint64_t> > roi;

    // callout storage
    Callout *tooltip = new Callout(this->getChart());
    QList<Callout *> callouts;

    // timer and rng for simulation
    QTimer *simTimer = new QTimer(this);
    uint64_t simStartTime;
    double simInteval = 0;
    std::default_random_engine gen;
    //    std::normal_distribution<float> *dist;
    std::binomial_distribution<> *dist;
};

#endif // SPECTRUMVIEW_H
