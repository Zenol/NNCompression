#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->isize->setText("NA");
    ui->osize->setText("NA");
    ui->cmpLabel->setText("");
    ui->imgLabel->setText("");

    ui->imgLabel->setBackgroundRole(QPalette::Dark);
    ui->imgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->imgLabel->setScaledContents(true);
    ui->cmpLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    ui->cmpLabel->setScaledContents(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    ui->inputSize->value();
    ui->hiddenSize->value();
    ui->cmpLabel->setPixmap(QPixmap::fromImage(inputImage));

    //TODO : Call compression algorithm

    ui->progressBar->setValue(100);

    ui->pushButton->setEnabled(false);
}

void MainWindow::on_actionLoad_a_picture_triggered()
{
    //Load picture
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Image Files (*.png *.jpg *.jpeg *.gif *bmp)");

    if (fileName == "")
        return;
    if(!inputImage.load(fileName))
    {
        QMessageBox::information(this, "NNCompress", "Can't open file '" + fileName + "' !");
        return;
    }

    ui->imgLabel->setPixmap(QPixmap::fromImage(inputImage));

    // Enable controls
    ui->inputSize->setEnabled(true);
    ui->hiddenSize->setEnabled(true);
    ui->progressBar->setEnabled(true);
    ui->pushButton->setEnabled(true);
    ui->progressBar->setValue(0);
    ui->cmpLabel->setPixmap(QPixmap());
}

void MainWindow::on_inputSize_valueChanged(int arg1)
{
    ui->pushButton->setEnabled(true);
}

void MainWindow::on_hiddenSize_valueChanged(int arg1)
{
    ui->pushButton->setEnabled(true);
}
