#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include "models.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    TaskParameters params{};
    Period period = Period::day;
    int periodsNum = 1;
    GaussDestrParameters gaussDestr{};
    double cur_x = 0.0;
    int dotsNum = 1024;
    std::vector<double> y_max{};
    std::vector<double> profit_max{};
};
#endif // MAINWINDOW_H
