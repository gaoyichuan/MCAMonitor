#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QtCharts>
#include <QtWidgets>

#include <vector>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  public Q_SLOTS:
    void on_menu_startAcqAction();
    void on_menu_stopAcqAction();
    void on_menu_clearAcqDataAction();
    void on_menu_openFileAction();
    void on_menu_saveFileAction();
    void on_spectrum_simUpdated(uint64_t startTime);
    void on_spectrum_dataUpdated();
    void on_spectrum_cursorUpdated(uint64_t cur);

private:
    Ui::MainWindow *ui;
    QLabel statusBar_filenameLabel;
};

#endif // MAINWINDOW_H
