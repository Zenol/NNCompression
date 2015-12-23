#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>

#include <vector>
#include <algorithm>

#include "Network.hpp"

#define __SQ(x) ((x) * (x))

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
         uint i, uint j,
         uint width, uint patch_size)
{
    using namespace boost::numeric::ublas;

    vector<T> patch(patch_size * patch_size);
    patch.clear();

    for (uint x = 0; x < patch_size; x++)
        for (uint y = 0; y < patch_size; y++)
        {
            const uint x2 = x + i * patch_size;
            const uint y2 = y + j * patch_size;
            const uint opos = x + y * patch_size;
            const uint ipos = x2 + y2 * width;
            patch[opos] = input[ipos];
        }

    return patch;
}
//TODO : create an object "patch" with this methods

template<typename T>
void unpatchify(boost::numeric::ublas::vector<T> &output,
                const boost::numeric::ublas::vector<T> &patch,
                uint i, uint j,
                uint width, uint patch_size)
{
    for (int x = 0; x < patch_size; x++)
        for (int y = 0; y < patch_size; y++)
        {
            const uint x2 = x + i * patch_size;
            const uint y2 = y + j * patch_size;
            const uint ipos = x + y * patch_size;
            const uint opos = x2 + y2 * width;
            output[opos] = patch[ipos];
        }
}

// Heart of the algorithm : compress the picture by training a network
template<typename T>
std::vector<boost::numeric::ublas::vector<T>>
compress_picture(ffnn::Network<T> &net,
                 std::vector<boost::numeric::ublas::vector<T>> &inputs,
                 T h, uint loops,
                 uint width, uint height,
                 uint patch_size,
                 QProgressBar *pgBar)
{
    using namespace boost::numeric::ublas;

    // TODO handle side blocks
    int rows = width / patch_size;
    int cols = height / patch_size;

    // TODO : Create and store a vector of patchs
    // Encoding
    std::vector< vector<float> > patch_list;
    for (auto input : inputs)
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++)
                patch_list.push_back(patchify(input, i, j, width, patch_size));

    const int nb_iter = loops * patch_list.size();
    int count = 0;
    for (int z = 0; z < loops; z++)
        for (auto patch : patch_list)
        {
            // Learn to be an autoencoder
            net.train(h, patch, patch);
            count++;
            if (count % 10)
                pgBar->setValue(5 + 90 * count / nb_iter);
        }

    net.save_file("network.json");

    // Decoding
    std::vector<vector<T>> outputs;
    for (auto input : inputs)
    {
        vector<T> output(input.size());
        output.clear(); // Put 0 on uncovored bands
        for (uint i = 0; i < rows; i++)
            for (uint j = 0; j < cols; j++)
            {
                auto patch = patchify(input, i, j, width, patch_size);
                patch = net.eval(patch);
                unpatchify(output, patch, i, j, width, patch_size);
            }
        outputs.push_back(std::move(output));
    }
    pgBar->setValue(99);

    return outputs;
}

void MainWindow::on_pushButton_clicked()
{
    // Unactivate button
    setUIstate(false);

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

    const uint patch_size = ui->inputSize->value();
    const uint input_size = __SQ(patch_size);
    const uint width = inputImage.width();
    const uint height = inputImage.height();
    const uint hidden_size = ui->hiddenSize->value();
    const float h = ui->step->value();
    const uint z = ui->loop->value();

    ffnn::Network<float> net;
    // Patchs are made of patch_size_patch_size square, where each pixel
    // has 3 colors.
    ffnn::Layer<float> l1(input_size, hidden_size, ffnn::sigmoid, ffnn::sigmoid_prime);
    ffnn::Layer<float> l2(hidden_size, input_size, ffnn::sigmoid, ffnn::sigmoid_prime);
    l1.randomize();
    l2.randomize();
    net.connect_layer(l1);
    net.connect_layer(l2);

    // Prepare matrix for compression
    using namespace boost::numeric::ublas;

    vector<float> r(width * height);
    vector<float> g(width * height);
    vector<float> b(width * height);
    for (uint x = 0; x < width; x++)
        for (uint y = 0; y < height; y++)
        {
            const int pos = x + y * width;
            const QRgb pix = inputImage.pixel(x, y);

            // Add an offset
            r[pos] = (float)qRed(pix) / 256.;
            g[pos] = (float)qGreen(pix) / 256.;
            b[pos] = (float)qBlue(pix) / 256.;
        }
    ui->progressBar->setValue(5);

    std::vector<vector<float>> images;
    images.push_back(r);
    images.push_back(g);
    images.push_back(b);

    //Compression algorithm
    std::vector<vector<float>> outputs =
        compress_picture(net,
                         images,
                         h, z,
                         width, height,
                         patch_size,
                         ui->progressBar);

    //////////////////////////
    // Build compressed QImage

    //TODO FOUND IT :D
    outputImage = QImage(width, height, QImage::Format_RGB32);
    for (uint x = 0; x < width; x++)
        for (uint y = 0; y < height; y++)
        {
            const int pos = x + y * width;
            // Remove offset
            const QRgb pix = qRgb(std::min(255, (int)(outputs[0][pos] * 256)),
                                  std::min(255, (int)(outputs[1][pos] * 256)),
                                  std::min(255, (int)(outputs[2][pos] * 256)));
            outputImage.setPixel(x, y, pix);
        }

    ui->cmpLabel->setPixmap(QPixmap::fromImage(outputImage));

    ui->progressBar->setValue(100);
    setUIstate(true);
    updateValues(true);
    ui->actionSave_output_picture->setEnabled(true);
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
    setUIstate(true);
    ui->progressBar->setValue(0);
    updateValues(true);
    ui->cmpLabel->setPixmap(QPixmap());
}

void MainWindow::setUIstate(bool st)
{
    ui->inputSize->setEnabled(st);
    ui->hiddenSize->setEnabled(st);
    ui->progressBar->setEnabled(st);
    ui->pushButton->setEnabled(st);
    ui->loop->setEnabled(st);
    ui->step->setEnabled(st);
}

void MainWindow::on_inputSize_valueChanged(int)
{
    updateValues(true);
}

void MainWindow::on_hiddenSize_valueChanged(int)
{
    updateValues(true);
}

void MainWindow::updateValues(bool allowCompress)
{
    const int hidden_size = ui->hiddenSize->value();
    const int input_size = ui->inputSize->value();
    const int rows = inputImage.width() / input_size;
    const int cols = inputImage.height() / input_size;
    const int isize = inputImage.width() * inputImage.height() * 3 / 1000;
    const int networksize = hidden_size + input_size + hidden_size * input_size;
    const int osize = ((networksize + rows * cols * hidden_size * 3) / 1000);
    const int tsize = ui->loop->value() * rows * cols * 3;

    ui->isize->setText(QString::number(isize) + "k");
    ui->osize->setText(QString::number(osize) + "k");
    ui->ratio->setText(QString::number((float)isize/(float)osize));
    ui->training_size->setText(QString::number(tsize));
    ui->pushButton->setEnabled(allowCompress);
}

void MainWindow::on_loop_valueChanged(int)
{
    updateValues(true);
}

void MainWindow::on_actionSave_output_picture_triggered()
{

    //Load picture
    QString fileName = QFileDialog::getSaveFileName(this, "Open Image", "", "Image Files (*.png *.jpg *.jpeg *.gif *bmp)");

    if(!outputImage.save(fileName))
    {
        QMessageBox::information(this, "NNCompress", "Can't save file '" + fileName + "' !");
        return;
    }

}
