#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "manager.h"

#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>

void addTip(QPushButton* infoButton, QString text){
    QPushButton::connect(infoButton, &QPushButton::clicked, [text]{
        QMessageBox msgBox;
        msgBox.setText(text);
        msgBox.exec();
    });
}


void saveSettings(Ui::MainWindow* ui){
    QSettings settings;
    settings.setValue("periods_num", ui->spinBox->value());
    settings.setValue("period_type", ui->PeriodComboBox->currentIndex());
    settings.setValue("mean", ui->meanEdit->value());
    settings.setValue("sigma", ui->sigmaEdit->value());
    settings.setValue("purchasePrice", ui->doubleSpinBox_3->value());
    settings.setValue("profitOfOnePurchase", ui->doubleSpinBox_4->value());
    settings.setValue("storageCosts", ui->doubleSpinBox_5->value());
    settings.setValue("deficitCoefficient", ui->doubleSpinBox_6->value());
    settings.setValue("inflation", ui->doubleSpinBox_7->value());
    settings.setValue("cur_stock", ui->doubleSpinBox_8->value());
}



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);



    QCoreApplication::setOrganizationName("SPbU");
    QCoreApplication::setOrganizationDomain("spbu.ru");
    QCoreApplication::setApplicationName("Purchase_Forecast");

    QSettings settings;
    ui->spinBox->setValue(settings.value("periods_num", 1).toInt());
    periodsNum = settings.value("periods_num", 1).toInt();
    ui->PeriodComboBox->setCurrentIndex(settings.value("period_type", 0).toInt());
    int indx = settings.value("period_type", 0).toInt();
    if(indx == 0){
        period = Period::day;
    } else if(indx == 1){
        period = Period::week;
    } else if(indx == 2){
        period = Period::month;
    }
    ui->meanEdit->setValue(settings.value("mean", 0.0).toDouble());
    gaussDestr.mean = settings.value("mean", 0.0).toDouble();
    ui->sigmaEdit->setValue(settings.value("sigma", 0.0).toDouble());
    gaussDestr.sigma = settings.value("sigma", 0.0).toDouble();

    ui->doubleSpinBox_3->setValue(settings.value("purchasePrice", 0.0).toDouble());
    params.purchasePrice = settings.value("purchasePrice", 0.0).toDouble();
    ui->doubleSpinBox_4->setValue(settings.value("profitOfOnePurchase", 0.0).toDouble());
    params.profitOfOnePurchase = settings.value("profitOfOnePurchase", 0.0).toDouble();
    ui->doubleSpinBox_5->setValue(settings.value("storageCosts", 0.0).toDouble());
    params.storageCosts = settings.value("storageCosts", 0.0).toDouble();
    ui->doubleSpinBox_6->setValue(settings.value("deficitCoefficient", 0.0).toDouble());
    params.deficitCoefficient = settings.value("deficitCoefficient", 0.0).toDouble();

    ui->doubleSpinBox_7->setValue(settings.value("inflation", 0.0).toDouble());
    params.inflation = 1.0 - settings.value("inflation", 0.0).toDouble() / 100.0;
    ui->doubleSpinBox_8->setValue(settings.value("cur_stock", 0.0).toDouble());
    cur_x = settings.value("cur_stock", 0.0).toDouble();

    ui->widget_10->setEnabled(false);
    ui->resultWidget->setVisible(false);
    ui->resultWidget_2->setVisible(false);
    ui->resultWidget_3->setVisible(false);
    addTip(ui->info_1, QString("Выберете, пожалуйста, для какой периода времени необходимо произвести расчет."));
    addTip(ui->info_2, QString("Напечатайте количество периодов для которых нужно произвести расчет."));
    addTip(ui->info_3, QString("Напечатайте стоимость покупки или производства единицы товара."));
    addTip(ui->info_4, QString("Напечатайте доход с реализации одной единицы рассматриваемого товара. Он обязан быть больше чем затраты на покупку."));
    addTip(ui->info_5, QString("Напечатайте стоимость хранения единицы товара за один рассматриваемый период"));
    addTip(ui->info_6, QString("Напечатайте стоимость издержек за единицу товара при его отсутствии."));
    addTip(ui->info_7, QString("Напечатайте значение инфляции за рассматриваемый период."));
    addTip(ui->info_8, QString("Напечатайте количество единиц имеющего запаса на начало первого рассматриваемого периода."));

    addTip(ui->info_9, QString("Это значение пропорционально степени двойки. Используется при построении сетки. Чем больше - тем точнее рассчеты."));

    connect(ui->meanEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double res){
        gaussDestr.mean = res;
    });
    connect(ui->sigmaEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double res){
        gaussDestr.sigma = res;
    });
    connect(ui->doubleSpinBox_3, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double res){
        params.purchasePrice = res;
    });
    connect(ui->doubleSpinBox_4, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double res){
        params.profitOfOnePurchase = res;
    });
    connect(ui->doubleSpinBox_5, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double res){
        params.storageCosts = res;
    });
    connect(ui->doubleSpinBox_6, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double res){
        params.deficitCoefficient = res;
    });
    connect(ui->doubleSpinBox_7, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double res){
        params.inflation = 1.0 - res / 100.0;
    });
    connect(ui->doubleSpinBox_8, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double res){
        cur_x = res;
    });
    connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int res){
        periodsNum = res;
    });
    connect(ui->horizontalSlider, QOverload<int>::of(&QSlider::valueChanged), this, [this](int res){
        dotsNum = 1 << res;
    });
    connect(ui->PeriodComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        if(index == 0){
            period = Period::day;
        } else if(index == 1){
            period = Period::week;
        } else if(index == 2){
            period = Period::month;
        }
    });
    connect(ui->clearPushButton, &QPushButton::clicked, [this]{
        ui->meanEdit->setValue(0.0);
        ui->sigmaEdit->setValue(0.0);
        ui->PeriodComboBox->setCurrentIndex(0);
        ui->doubleSpinBox_3->setValue(0.0);
        ui->doubleSpinBox_4->setValue(0.0);
        ui->doubleSpinBox_5->setValue(0.0);
        ui->doubleSpinBox_6->setValue(0.0);
        ui->doubleSpinBox_7->setValue(0.0);
        ui->doubleSpinBox_8->setValue(0.0);
        params = {};
        period = Period::day;
        periodsNum = 1;
        gaussDestr = {};
        cur_x = 0.0;
        dotsNum = 1024;
        y_max = {};
        profit_max = {};
    });

    connect(ui->startPushButton, &QPushButton::clicked, this, [this]{
        auto gauss_for_init = gaussDestr;
        if(!gaussVec.empty() && ui->noStatRadioButton->isChecked()){
            for(const auto& it : gaussVec){
                gauss_for_init.mean = std::max(gauss_for_init.mean, it.mean);
                gauss_for_init.sigma = std::max(gauss_for_init.sigma, it.sigma);
            }
        }
        TaskCalculator calculator(params, gauss_for_init, periodsNum, dotsNum);
        int n = 1;
        if(!gaussVec.empty()  && ui->noStatRadioButton->isChecked()){
            calculator.setGaussVector(gaussVec);
        }
        while(calculator.calcPeriod()){
            ui->statusbar->showMessage(QLatin1String("Period ") + QString::number(n));
            n++;
        }
        ui->statusbar->clearMessage();
        y_max = calculator.getMaxY();
        profit_max = calculator.getMaxProfit();
        std::optional<Answer> ans = calculator.getAnswer(cur_x);
        double yRes = ((ans->y - cur_x) < 0.01) ? 0.0 : (ans->y - cur_x);
        ui->doubleSpinBox_9->setValue(yRes);
        ui->doubleSpinBox_10->setValue(ans->thisPeriodProfit);
        ui->doubleSpinBox_11->setValue(ans->MaxProfit);
        ui->resultWidget->setVisible(true);
        ui->resultWidget_2->setVisible(true);
        ui->resultWidget_3->setVisible(true);

    });

    connect(ui->showResPushButton, &QPushButton::clicked, [this]{
        std::ostringstream out;
        for(int i = 0; i<y_max.size() ;++i){
            out<< "Period: "<< i+2<< "\t purchase -> "<< y_max[i] <<"\t profit -> " << profit_max[i]<<"\n";
        }
        QString res = QString::fromStdString(out.str());
        if(!res.isEmpty()){
            QMessageBox msgBox;
            msgBox.setText(res);
            msgBox.exec();
        }
    });

    connect(ui->savePushButton, &QPushButton::clicked, [this]{
        saveSettings(ui);
    });

    connect(ui->chooseFileButton, &QPushButton::clicked, [this]{
        auto fileName = QFileDialog::getOpenFileName(this, "Open purchcase file", {}, "Text files (*.txt)");
        auto rawData = readFile(fileName.toStdString());
        if(!rawData.empty()){
            gaussVec = parsePeriods(rawData, period);
            ui->chooseFileText->setText(QString::number(gaussVec.size()).append(" periods were found"));
        } else{
            ui->chooseFileText->setText(QString::fromStdString("File wasn't correct"));
        }
    });


}

MainWindow::~MainWindow()
{
    delete ui;
}

