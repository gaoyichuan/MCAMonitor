#include "mainwindow.h"
#include "spectrumdata.h"
#include "ui_mainwindow.h"

#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->showMaximized();

    this->statusBar_filenameLabel.setText("No file opened");
    this->statusBar()->addWidget(&statusBar_filenameLabel);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_menu_openFileAction() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Ortec Files(*.chn *.Chn)"));
    dialog.setNameFilterDetailsVisible(true);
    dialog.setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    if(dialog.exec())
        fileNames = dialog.selectedFiles();

    if(fileNames.length() == 0) return;

    SpectrumData *spec = new SpectrumData();

    try {
        spec->readFromFile(fileNames[0]);
    } catch(std::exception &e) {
        QString status;
        status.append("Error in reading file: ");
        status.append(e.what());
        this->statusBar_filenameLabel.setText(status);
        return;
    } catch(const char *e) {
        this->statusBar_filenameLabel.setText(e);
        return;
    }

    this->statusBar_filenameLabel.setText("Opened file: " + fileNames[0]);
    this->on_menu_clearAcqDataAction();

    QPointF maxp(0, 0);
    uint64_t sum = 0;

    for(uint64_t i = 0; i < spec->data.size(); i++) {
        this->ui->spectrumArea->getSeries()->append(i, spec->data[i]);
        sum += spec->data[i];
        if(spec->data[i] > maxp.y()) {
            maxp.setX(i);
            maxp.setY(spec->data[i]);
        }
    }

    this->ui->spectrumArea->getAxisX()->setRange(0, spec->data.size());
    this->ui->spectrumArea->getAxisY()->setRange(0, maxp.y() * 1.2);
    this->ui->spectrumArea->setMax(maxp);
    this->ui->spectrumArea->setSum(sum);
    this->on_spectrum_dataUpdated();
}

void MainWindow::on_menu_saveFileAction() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Ortec Files(*.chn *.Chn)"));
    dialog.setNameFilterDetailsVisible(true);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    QStringList fileNames;
    if(dialog.exec())
        fileNames = dialog.selectedFiles();

    if(fileNames.length() == 0) return;

    SpectrumData *spec = new SpectrumData();

    spec->filename = fileNames[0];
    spec->acqStart = QDateTime::fromMSecsSinceEpoch(this->ui->spectrumArea->getSimStartTime()).toTime_t();
    spec->acqStop = QDateTime::currentMSecsSinceEpoch();
    spec->dev_name = "MCA";

    for(auto p : this->ui->spectrumArea->getSeries()->pointsVector()) {
        spec->data.push_back(p.y());
    }

    try {
        spec->saveToFile(spec->filename);
    } catch(std::exception &e) {
        QString status;
        status.append("Error in saving file: ");
        status.append(e.what());
        this->statusBar_filenameLabel.setText(status);
        return;
    } catch(const char *e) {
        this->statusBar_filenameLabel.setText(e);
        return;
    }

    this->statusBar_filenameLabel.setText("Saved file: " + fileNames[0]);
}

// call from spectrumArea
void MainWindow::on_spectrum_simUpdated(uint64_t startTime) {
    double secElasped = (QDateTime::currentMSecsSinceEpoch() - startTime) / 1000.0;
    this->ui->simTimeElaspedLabel->setText(QString::number(secElasped, 10, 2));

    if(this->ui->simMethodInput->currentIndex() == 0) {     // given time
        double setTime = this->ui->simTimeInput->value();
        auto setTimeUnit = this->ui->simTimeUnit->currentIndex();
        switch(setTimeUnit) {
        case 1: // time in min
            setTime *= 60.0;
            break;
        case 2: // time in hour
            setTime *= 3600.0;
            break;
        }

        if(secElasped >= setTime) {
            this->on_menu_stopAcqAction();
        }
    } else {
        auto sum = this->ui->simCountInput->value();
        if(this->ui->spectrumArea->getSum() >= sum) {
            this->on_menu_stopAcqAction();
        }
    }

    this->on_spectrum_dataUpdated();
}

void MainWindow::on_spectrum_dataUpdated() {
    auto vec = this->ui->spectrumArea->getSeries()->pointsVector();
    this->ui->startChnAddr->setNum((int)vec.begin()->x());
    this->ui->startChnCnt->setNum((int)vec.begin()->y());
    this->ui->stopChnAddr->setNum((int)((vec.end() - 1)->x()));
    this->ui->stopChnCnt->setNum((int)((vec.end() - 1)->y()));
    this->ui->curChnAddr->setNum((int)this->ui->spectrumArea->getCursor());
    this->ui->curChnCnt->setNum((int)this->ui->spectrumArea->getSeries()->at(this->ui->spectrumArea->getCursor()).y());
    this->ui->totalCnt->setNum((int)this->ui->spectrumArea->getSum());
}

void MainWindow::on_spectrum_cursorUpdated(uint64_t cur) {
    this->ui->curChnAddr->setNum((int)cur);
    this->ui->curChnCnt->setNum((int)this->ui->spectrumArea->getSeries()->at(cur).y());
}

// call by menu buttons
void MainWindow::on_menu_startAcqAction() {
    this->on_menu_clearAcqDataAction();
    double setInteval = this->ui->simIntevalInput->value();
    this->ui->spectrumArea->startSimAcq(setInteval);
}

void MainWindow::on_menu_clearAcqDataAction() {
    this->ui->spectrumArea->clearAcqData();
    this->ui->roiScaleArea->clearRoi();
}

void MainWindow::on_menu_stopAcqAction() {
    this->ui->spectrumArea->stopSimAcq();
}
