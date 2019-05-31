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
        spec->readFromFile(fileNames[0].toUtf8().constData());
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

    this->statusBar_filenameLabel.setText(fileNames[0]);

    this->ui->spectrumArea->getSeries()->clear();

    for(uint64_t i = 0; i < spec->data.size(); i++) {
        this->ui->spectrumArea->getSeries()->append(i, spec->data[i]);
    }

    this->ui->spectrumArea->getAxisX()->setRange(0, spec->data.size());
    this->ui->spectrumArea->getAxisY()->setRange(0, *std::max_element(spec->data.begin(), spec->data.end()));
    this->on_spectrum_dataUpdated();
}

// call from spectrumArea
void MainWindow::on_spectrum_simUpdated(uint64_t startTime) {
    double secElasped = (QDateTime::currentMSecsSinceEpoch() - startTime) / 1000.0;
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

    this->ui->simTimeElaspedLabel->setText(QString::number(secElasped, 10, 2));
    if(secElasped >= setTime) {
        this->ui->spectrumArea->stopSimAcq();
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
    this->ui->curChnCnt->setNum((int)vec.at(this->ui->spectrumArea->getCursor()).y());

    uint64_t totalCnt = 0;
    for(auto v : vec) totalCnt += v.y();
    this->ui->totalCnt->setNum((int)totalCnt);
}

// call by menu button
void MainWindow::on_menu_startAcqAction() {
    double setInteval = this->ui->simIntevalInput->value();
    this->ui->spectrumArea->startSimAcq(setInteval);
}
