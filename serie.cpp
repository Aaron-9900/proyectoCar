#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <tuple>
#include <iostream>
#include <cmath>
#include <chrono>
#include <filesystem>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

Mat convolve(array<array<int, 3>, 3> mask, Mat image, Mat resultImage);
tuple<array<array<int, 3>, 3>, array<array<int, 3>, 3>> createMasks();

int main(int argc, char **argv)
{
    String imageName(fs::current_path().u8string() + "/688699.jpg");
    int threshold = 127;
    if (argc > 1)
    {
        imageName = argv[1];
    }
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

    Mat resultImg1 = image.clone();
    Mat resultImg2 = image.clone();
    Mat convolvedImg = image.clone();
    auto start = chrono::high_resolution_clock::now();
    // Area that can be decomposed in threads
    resultImg1 = convolve(mask_one, image, resultImg1);
    resultImg2 = convolve(mask_two, image, resultImg2);
    // End area that can be decomposed in threads
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << duration.count() << endl;
    /*

        Apply threshold and add images

    */
    for (int row = 1; row < image.rows; row++)
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
    }
    namedWindow("Display window", WINDOW_AUTOSIZE);
    imshow("Display window", image);
    namedWindow("Result", WINDOW_AUTOSIZE);
    imshow("Result", convolvedImg);
    waitKey(0);
    // imwrite("test.jpg", convolvedImg);
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

Mat convolve(array<array<int, 3>, 3> mask, Mat image, Mat resultImage)
{
    for (int row = 1; row < image.rows; row++)
    {
        uchar *ptr = resultImage.ptr<uchar>(row);
        uchar *ptrImg = image.ptr<uchar>(row);
        uchar *ptrImgPrev = image.ptr<uchar>(row - 1);
        uchar *ptrImgNext = image.ptr<uchar>(row + 1);

        for (int col = 1; col < image.cols; col++)
        {
            ptr[col] = ((int)ptrImgPrev[col - 1] * mask[0][0] +
                        (int)ptrImgPrev[col] * mask[0][1] +
                        (int)ptrImgPrev[col + 1] * mask[0][2] +
                        (int)ptrImg[col - 1] * mask[1][0] +
                        (int)ptrImg[col] * mask[1][1] +
                        (int)ptrImg[col + 1] * mask[1][2] +
                        (int)ptrImgNext[col - 1] * mask[2][0] +
                        (int)ptrImgNext[col] * mask[2][1] +
                        (int)ptrImgNext[col + 1] * mask[2][2]) /
                       30;
        }
    }
    return resultImage;
}
