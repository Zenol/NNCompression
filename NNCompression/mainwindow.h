#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

typedef unsigned int uint;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_actionLoad_a_picture_triggered();

    void on_inputSize_valueChanged(int arg1);

    void on_hiddenSize_valueChanged(int arg1);

    void updateValues(bool allowCompress);

    void setUIstate(bool st);

    void on_loop_valueChanged(int arg1);

    void on_actionSave_output_picture_triggered();

private:
    Ui::MainWindow *ui;

    QImage inputImage;
    QImage outputImage;
};

#endif // MAINWINDOW_H
