#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private:
    Ui::MainWindow *ui;

    QImage inputImage;
};

#endif // MAINWINDOW_H
