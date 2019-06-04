#ifndef SPECTRUMVIEW_H
#define SPECTRUMVIEW_H

#include <QtCharts>
#include <random>
#include <stack>
#include <utility>
#include <vector>

#include "callout.h"

QT_CHARTS_USE_NAMESPACE

class spectrumView : public QChartView {
    Q_OBJECT

  public:
    spectrumView(QWidget *parent = nullptr);

    void scaleY(qreal factor);

    void addRoi(std::pair<uint64_t, uint64_t> r);
    // return true when success
    bool removeRoi(uint64_t channel);
    bool removeRoi(std::pair<uint64_t, uint64_t> r);
    void redrawRoi();
    void clearRoi();
    std::pair<uint64_t, uint64_t> *findRoi(QPointF point);

    template <class T>
    QPointF pointToSeriesData(T point);
    qreal channelToPointX(uint64_t chn);

    QValueAxis *getAxisX() const;
    QValueAxis *getAxisY() const;
    QLineSeries *getSeries() const;
    QChart *getChart() const;

    uint64_t getCursor() const;

    QPointF getMax() const;
    void setMax(const QPointF &value);

    uint64_t getSum() const;
    void setSum(const uint64_t &value);

  private Q_SLOTS:
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
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

  protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

  private:
    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();
    QLineSeries *series = new QLineSeries();
    QChart *chart = new QChart();

    // max and min values
    QPointF max;
    uint64_t sum = 0;

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
    Callout *tooltip = new Callout(this->chart);
    QList<Callout *> callouts;

    // cursor
    uint64_t cursor = 0;

    // timer and rng for simulation
    QTimer *simTimer = new QTimer(this);
    uint64_t simStartTime;
    double simInteval = 0;
    std::default_random_engine gen;
//    std::normal_distribution<float> *dist;
    std::binomial_distribution<> *dist;
};

#endif // SPECTRUMVIEW_H
