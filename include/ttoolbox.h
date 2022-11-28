#ifndef TTOOLBOX_H
#define TTOOLBOX_H

//cpp,c
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <ctime>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

//open cv
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

class TToolBox
{
public:

    //! return the number as string with 10 digits and '0' leading ones
    static std::string mNzero(int i);

    //! return string plus data path and jpg suffix
    static std::string getFileName(int i);

    //! returns a image, which is resulting circle
    //! return roi of image which is cloned
    //! without resize the image
    //! will fill rest outer bound black
    static cv::Mat cropImageCircle(cv::Mat image, int x, int y, int r);

    //! canny edge detect
    static std::vector<std::vector<cv::Point>> applyCannyEdgeAndCalcCountours(cv::Mat imgSource, double threshholdMin, double threshholdMax, int apertureSize);

    //! will test if file is not corrupted
    //! @return 1 if corrupted
    //! @return 0 if not
    static int checkFileCorrupted(std::string filename);

    //! will try to execute the cmd on the bash
    static  std::string execCMD(const char* cmd);

};

#endif // TTOOLBOX_H
