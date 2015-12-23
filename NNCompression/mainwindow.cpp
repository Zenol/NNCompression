#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>

#include <vector>
#include <algorithm>

#include "Network.hpp"

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

template<typename T>
boost::numeric::ublas::vector<T>
patchify(const boost::numeric::ublas::vector<T> &input,
         unsigned int i, unsigned int j,
         unsigned int width, unsigned int patch_size)
{
    using namespace boost::numeric::ublas;

    vector<T> patch(patch_size * patch_size * 3);
    patch.clear();

    for (unsigned int x = 0; x < patch_size; x++)
        for (unsigned int y = 0; y < patch_size; y++)
        {
            const unsigned int x2 = x + i * patch_size;
            const unsigned int y2 = y + j * patch_size;
            const unsigned int opos = 3 * (x + y * patch_size);
            const unsigned int ipos = 3 * (x2 + y2 * width);
            patch[opos] = input[ipos];
            patch[opos + 1] = input[ipos + 1];
            patch[opos + 2] = input[ipos + 2];
        }

    return patch;
}
//TODO : create an object "patch" with this methods

template<typename T>
void unpatchify(boost::numeric::ublas::vector<T> &output,
                const boost::numeric::ublas::vector<T> &patch,
                unsigned int i, unsigned int j,
                unsigned int width, unsigned int patch_size)
{
    for (int x = 0; x < patch_size; x++)
        for (int y = 0; y < patch_size; y++)
        {
            const unsigned int x2 = x + i * patch_size;
            const unsigned int y2 = y + j * patch_size;
            const unsigned int ipos = 3 * (x + y * patch_size);
            const unsigned int opos = 3 * (x2 + y2 * width);
            output[opos] = patch[ipos];
            output[opos + 1] = patch[ipos + 1];
            output[opos + 2] = patch[ipos + 2];
        }
}

// Heart of the algorithm : compress the picture by training a network
template<typename T>
boost::numeric::ublas::vector<T>
compress_picture(ffnn::Network<T> &net, const boost::numeric::ublas::vector<T> &input,
                 T h,
                 unsigned int width, unsigned int height,
                 unsigned int patch_size,
                 QProgressBar *pgBar)
{
    using namespace boost::numeric::ublas;

    // TODO handle side blocks
    int rows = (width - 1) / patch_size;
    int cols = (height - 1)/ patch_size;

    // TODO : Create and store a vector of patchs
    // Encoding
    std::vector< vector<double> > patch_list;
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            patch_list.push_back(patchify(input, i, j, width, patch_size));

    const int nb_cp = 1;
    const int nb_iter = nb_cp * rows * cols;
    int count = 0;
    for (int z = 0; z < nb_cp; z++)
        for (auto patch : patch_list)
        {
            // Learn to be an autoencoder
            net.train(1, patch, patch);
            count++;
            if (count % 10)
                pgBar->setValue(5 + 90 * count / nb_iter);
        }

    std::cout << net;

    // Decoding
    vector<T> output(input.size());
    for (unsigned int i = 0; i < rows; i++)
        for (unsigned int j = 0; j < cols; j++)
        {
            auto patch = patchify(input, i, j, width, patch_size);
            patch = net.eval(patch);

            unpatchify(output, patch, i, j, width, patch_size);
        }
    pgBar->setValue(99);

    return output;
}

void MainWindow::on_pushButton_clicked()
{
    // Unactivate button
    ui->pushButton->setEnabled(false);

    // TODO : Do the compression in an other thread

    /////////////////////////////////////
    // Build network used for compression
    //
    // A bottleneck network trained as autoencoder.
    //
    // *           *
    // * \   * /   *
    // * - > * - > *
    // * /   * \   *
    // *           *
    //
    // We divide by 256 because 1 is a value that cannot be output, although 0 is obtained by converting to int a small value.

    const int patch_size = ui->inputSize->value();
    const int input_size = patch_size * patch_size * 3;
    const int width = inputImage.width();
    const int height = inputImage.height();
    const int hidden_size = ui->hiddenSize->value();

    ffnn::Network<double> net;
    // Patchs are made of patch_size_patch_size square, where each pixel
    // has 3 colors.
    ffnn::Layer<double> l1(input_size, hidden_size, ffnn::sigmoid, ffnn::sigmoid_prime);
    l1.randomize();
    ffnn::Layer<double> l2(hidden_size, input_size, ffnn::sigmoid, ffnn::sigmoid_prime);
    l2.randomize();
    net.connect_layer(l1);
    net.connect_layer(l2);

    // Prepare matrix for compression
    using namespace boost::numeric::ublas;

    vector<double> input(width * height * 3);
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++)
        {
            const int pos = 3 * (x + y * width);
            const QRgb pix = inputImage.pixel(x, y);

            // Add an offset
            input[pos] = (double)qRed(pix) / 256.;
            input[pos + 1] = (double)qGreen(pix) / 256.;
            input[pos + 2] = (double)qBlue(pix) / 256.;
        }
    ui->progressBar->setValue(5);

    //Compression algorithm
    vector<double> output = compress_picture(net,
                                             input, 1.0,
                                             width, height,
                                             patch_size,
                                             ui->progressBar);

    //////////////////////////
    // Build compressed QImage

    //TODO FOUND IT :D
    QImage outputImage(width, height, QImage::Format_RGB32);
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++)
        {
            const int pos = 3 * (x + y * width);
            // Remove offset
            const QRgb pix = qRgb(std::max(255, output[pos] * 256),
                                  std::max(255, output[pos + 1] * 256),
                                  std::max(255, output[pos + 2] * 256));
            outputImage.setPixel(x, y, pix);
        }

    ui->cmpLabel->setPixmap(QPixmap::fromImage(outputImage));

    //Disable compression
    ui->progressBar->setValue(100);
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
