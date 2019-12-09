#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <tuple>
#include <iostream>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <mpi.h>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

void convolve(array<array<int, 3>, 3> mask, uchar *image, int length);
tuple<array<array<int, 3>, 3>, array<array<int, 3>, 3>> createMasks();

int main(int argc, char **argv)
{
    int size, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    String imageName(fs::current_path().u8string() + "/688699.jpg");
    int threshold = 127;
    Mat image;
    try
    {
        image = imread(samples::findFile(imageName));
    }
    catch (int e)
    {
        cout << "Could not open image" << endl;
        return -1;
    }
    GaussianBlur(image, image, Size(5, 5), 3, 3);
    cvtColor(image, image, COLOR_BGR2GRAY);
    auto [mask_one, mask_two] = createMasks();
    copyMakeBorder(image, image, 1, 1, 1, 1, BORDER_REFLECT101);
    uchar *global_data[image.rows];
    uchar *local_data;
    if (rank == 0)
    {
        for (int i = 0; i < image.rows; i++)
        {
            global_data[i] = image.ptr<uchar>(i);
        }
    }
    // Area that can be decomposed in threads
    // End area that can be decomposed in threads
    MPI_Scatter(&global_data, image.rows * image.cols / size, MPI_UNSIGNED_CHAR, &local_data, image.rows * image.cols / size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    convolve(mask_one, local_data, image.cols);
    MPI_Gather(&global_data, image.rows * image.cols / size, MPI_UNSIGNED_CHAR, &local_data, image.rows * image.cols / size, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    /*

        Apply threshold and add images

    */

    /*
    namedWindow("Display window", WINDOW_AUTOSIZE);
    imshow("Display window", image);
    namedWindow("Result", WINDOW_AUTOSIZE);
    imshow("Result", convolvedImg);
    waitKey(0);
    */
    if (rank == 0)
    {
        imwrite("test.jpg", image);
    }

    MPI_Finalize();
    return 0;
}

tuple<array<array<int, 3>, 3>, array<array<int, 3>, 3>> createMasks()
{
    array<array<int, 3>, 3> smask_one = {{{-1, 0, 1},
                                          {-2, 0, 2},
                                          {-1, 0, 1}}};
    array<array<int, 3>, 3> smask_two = {{{1, 2, 1},
                                          {0, 0, 0},
                                          {-1, -2, -1}}};
    tuple<array<array<int, 3>, 3>, array<array<int, 3>, 3>> returnValue;
    get<0>(returnValue) = smask_one;
    get<1>(returnValue) = smask_two;

    return returnValue;
}

void convolve(array<array<int, 3>, 3> mask, uchar *image, int length)
{
    for (int i = 0; i < length; i++)
    {
        cout << (int)image[i] << endl;

        image[i] = 255;
    }
}
/*for (int row = 1; row < image.rows; row++)
{
    uchar *img1 = resultImg1.ptr<uchar>(row);
    uchar *img2 = resultImg2.ptr<uchar>(row);
    uchar *convolved = convolvedImg.ptr<uchar>(row);
    for (int col = 1; col < image.cols; col++)
    {
        float floatTemp = sqrt(pow((int)img1[col], 2) +
                               pow((int)img2[col], 2));
        int temp = static_cast<int>(floatTemp);
        if (temp > 250)
        {
            convolved[col] = 100;
        }
        else
        {
            convolved[col] = 0;
        }
    }
}*/