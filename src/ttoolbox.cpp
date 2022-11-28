#include "ttoolbox.h"

std::string TToolBox::mNzero(int i)
{
    std::ostringstream convert;
    convert << i ;
    std::string numberString = convert.str();
    std::string newNumberString = std::string(10 - numberString.length(), '0') + numberString;
    return newNumberString;
}
std::string TToolBox::getFileName(int i)
{
    std::string fileName =std::string ( std::string ("/data/")+  mNzero(i) + std::string (".jpg"));
    return fileName;
}

std::vector<std::vector<cv::Point>> TToolBox::applyCannyEdgeAndCalcCountours(cv::Mat imgSource, double threshholdMin, double threshholdMax, int apertureSize)
{
    cv::Mat imgCannyEdge;
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;

    //! 2 make a copy
    cv::Mat imgBinary = imgSource.clone();
    //imgMarkBinary = Scalar(255,255,255); //fill white

    //! 3 apply binary filter not reduce noise
    //method to threshold important changes
    int binaryThreshold = 80; //TODO as parameter
    imgBinary = imgSource > binaryThreshold;


    //! 4 smooth with gaussian filter to suppress no connected lines
    //add a gaussian filter //see http://docs.opencv.org/2.4/doc/tutorials/imgproc/gausian_median_blur_bilateral_filter/gausian_median_blur_bilateral_filter.html
    cv::Mat imgSmoothed = imgBinary.clone();
    int exclusPara3 = 3; //TODO as parameter
    cv::GaussianBlur(imgBinary,imgSmoothed,cv::Size(exclusPara3,exclusPara3),0);  //0 = BORDER_CONSTANT

    //! 5 we make the canny edge detect
    // Detect edges using canny
    cv::Canny( imgSmoothed, imgCannyEdge, threshholdMin, threshholdMax, apertureSize );

    // Find contours, use RETR_EXTERNAL ignore inner child structures, TREE more usefull ?
    cv::findContours( imgCannyEdge, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

#define MC_SHOW_STEP_ANALYSE
#ifdef  MC_SHOW_STEP_ANALYSE
    // Draw contours on extra mat
    cv::Mat imgContour = cv::Mat::zeros( imgCannyEdge.size(), CV_8UC3 );
    imgContour =  cv::Scalar(255,255,255); //fille the picture
    cv::RNG rng(232323);
    for( size_t i = 0; i< contours.size(); i++ )
    {
        //random color
        cv::Scalar color = cv::Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
        //contour
        cv::drawContours( imgContour, contours, i, color, 1, cv::LINE_AA, hierarchy, 0, cv::Point() );
    }

    cv::String outpath=  "./";
    std::ostringstream convert;
    convert << outpath << "test_countour_canny_edge.jpg";
    cv::imwrite(convert.str().c_str(), imgContour);

#endif

    //    return minRect;
    return contours;
}


cv::Mat TToolBox::cropImageCircle(cv::Mat image, int x, int y, int r)
{

 
    // draw the mask: white circle on black background
    cv::Mat1b mask(image.size(), uchar(0));
    cv::circle(mask, cv::Point(x, y), r, cv::Scalar(255), cv::FILLED);
    // create a black image
    cv::Mat res = cv::Mat::zeros( image.size(), CV_8UC3 );
    res = cv::Scalar(0,0,0); //fill with black color
    // copy only the image under the white circle to black image
    image.copyTo(res, mask);

    return res;
}

int TToolBox::checkFileCorrupted(std::string filename)
{
    int retVal = 1;//is as long Corrupted as we dont find it in another way
    char command[] =  "identify ";
    const char *fileNameArr = filename.c_str();
    char command2[] = " > /dev/null 2>&1 ; echo $?";
    char *cmd = new char[std::strlen(command)+std::strlen(fileNameArr) + std::strlen(command2) +1];
    //we put all peaces together
    std::strcpy(cmd,command);
    std::strcat(cmd,fileNameArr);
    std::strcat(cmd,command2);

    //std:: cout <<"cmd is: "<< cmd<< std::endl;
    //execute the command
    std::string result = TToolBox::execCMD((const char* ) cmd);
    //std:: cout <<"results is: "<< result<< std::endl;
    if(!result.empty())
    {
        try {
            retVal = std::stoi(result);
        }
        catch(std::invalid_argument& e)
        {
            std::cerr <<"wrong argument for stoi"<<std::endl;
            // if no conversion could be performed
        }
        catch (const std::exception& e)
        {
            std::cerr <<"error during use stoi"<<std::endl;
        }

    }//end if
    return retVal;
}


std::string TToolBox::execCMD(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
        //std::cout<<result<<std::endl;

    }
    //std::cout<<"call:       " <<result<<std::endl;
    return result;
}
