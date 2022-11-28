// opencv
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

int main(int argc, char *argv[])
{
    //cv::Mat img_input = cv::imread("/home/tb55xemi/work/idiv/realbugtracker/examples/rec97371911/data/0000001000.jpg", cv::IMREAD_ANYCOLOR);

    // of data is present
    //if (img_input.data) // checkk for non null
    {

        // cv::imshow("foo",img_input);
        // cv::waitKey(0);
   
    // std::cout <<"size" <<img_input.size() <<std::endl; 
    // std::cout <<"type" <<img_input.type() <<std::endl; 
    // Draw the mask: white circle on black background
    cv::Mat mask(1936,1456,CV_8UC3,cv::Scalar(0,0,0)); // Create your mask)
    mask.setTo(0);
    //cv::Mat1b mask(img_input.size(), uchar(0));
    //cv::Mat mask(img_input.size(), CV_8UC1, uchar(0));
    
    //std::cout << mask <<std::endl; 
    std::cout <<  cv::FILLED;
    cv::circle(mask, cv::Point(10, 10), 10, 255, cv::FILLED);
    //circle(mask, cv::Point(10, 10), 10, 2, 255, cv::FILLED);

    }
}