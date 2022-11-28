#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/opencv.hpp"
#include <math.h>
#include <iostream>
#include <sstream>      
#include <fstream>
#include <string>
#include <stdio.h>
#include <queue>
#include <vector>
#include <numeric>

using namespace cv;
using namespace std;

static void help()
{
    cout << "usage:\n./produceTrajectory -c=camparamfile.xml -t=timetable.txt -o=outdir -r=recid  "<< endl;
}

//! a a small data class which contains the parsed data
class DataTuple
{
public:
    unsigned int t_sec;
    unsigned int t_msec;
    float pos_x;
    float pos_y;
    int pic_ID;
};

//***** see example in folder evalProjection
//! the matrix for the projection
Mat g_inverseHomographyMatrix;

//! we do project the mat a from pixel space into the real world unit space mm
Mat project(Mat a);

//! we convert a Point2d to a matrix with x,y,0 --> z dimension is always zero, so we assume a flat surface
Mat convertPointToMat(Point2d a);

//! we calc the distance of Mat with cv norm
//!  uses the norm 2 function https://docs.opencv.org/3.4/d2/de8/group__core__array.html#gad12cefbcb5291cf958a85b4b67b6149f
double calcEuclidianDistanc(Mat a, Mat b);

//! we load the inverse homography matrix in file s
Mat loadFileAndCalcInverseHomographyMatrix(String filename);

//! convert a string to int
//! @return in case of error return = -1
int stringToIntWithRemove(std::string s);

//! convert a string to float
//! @return in case of error return = -1
float stringToFloat(std::string s);

//! get pixel distance
double getEuklidDistance(Point2f pA, Point2f pB);

//! we calc the euclidian distance with projection !
//! @return mm distance
double getEuklidDistanceInMM(DataTuple a,DataTuple b);

//! @return the abs diff of the times
double getTimeDiff(DataTuple a,DataTuple b);

//! do zero padding string, with 10 leading zeros
std::string mNzero(int i);

//! we calc a frequency of a queue
double calcFrequency(std::queue<double> queue );

int main( int argc, char** argv)
{
    typedef std::numeric_limits< double > dbl;
    cout.precision(dbl::max_digits10);
    //@see https://stackoverflow.com/questions/554063/how-do-i-print-a-double-value-with-full-precision-using-cout


    // Define and process cmdline arguments.
    cv::CommandLineParser parser(argc, argv,
                                 "{ c camparam            |           | camera parameter from calibration }"
                                 "{ t timetable  |        | table of position and time  }"
                                 "{ o outdir  |        | directory of output  }"
                                 );

    String pathDataCamParamFile = parser.get<String>("c");
    String pathDataTimeTableFile = parser.get<String>("t");
    String pathDataOut = parser.get<String>("o");

    cout <<"./produceTrajecotry v.0.1 -c="<<pathDataCamParamFile <<" -t=" << pathDataTimeTableFile<<" -o="<<pathDataOut<<endl;

    if (!parser.check())
    {
        parser.printErrors();
        return 0;
    }

    //TODO check all parameter value

    //we load our global file
    g_inverseHomographyMatrix = loadFileAndCalcInverseHomographyMatrix(pathDataCamParamFile);
    cout<< "load camera param done, inverseHomographyMatrix : "<< g_inverseHomographyMatrix << endl;

    //we read in all the data tuple
    vector<DataTuple> mDataVec;
    int amountOfFrames = 0;
    int amountOfNonDectedFrames = 0;

    ifstream myfile (pathDataTimeTableFile);
    string line;
    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
            //we check for comment lines
            if (line.find(string("#")) != std::string::npos)
            {
                //we check for the first line
                if (line.find(string("amount frames used:")) != std::string::npos)
                {
                    //we have the
                    //cout <<"\n ---- s 1 found" << line <<endl;
                    //TODO this is the second option for string split
                    std::string delimiter = ": ";
                    std::string sAmountFrames = line.substr(line.find(delimiter),line.length());
                    amountOfFrames = stringToIntWithRemove(sAmountFrames);
                    cout <<"amountFrames :"<<amountOfFrames <<endl;

                }//end s1

                //we check for the second line
                if (line.find(string("amount ignored frames (no poly at all):")) != std::string::npos)
                {
                    //we have the
                    //cout <<"s 2 found" << line <<endl;

                    std::string delimiter = ": ";
                    std::string sAmountNonDect = line.substr(line.find(delimiter),line.length());
                    amountOfNonDectedFrames = stringToIntWithRemove(sAmountNonDect);
                    cout <<"amountFrames not dected :"<<amountOfNonDectedFrames <<endl;
                }

                continue;
            }
	    //cout <<"will parse line:"<<line<<endl; 
            //we make a
            std::vector<std::string> results;

            char *str = const_cast<char*> ( line.c_str() );

            //we iterate all we use a old c split version
            //@see http://www.cplusplus.com/reference/cstring/strtok/
            char * pch;
            pch = strtok (str,";");

            while (pch != NULL)
            {
                results.push_back(std::string(pch));
                pch = strtok (NULL, ";");
            }

            //we should cast it
            if(results.size()!=5)
            {
                cout<<"error 42 during parse file:" << pathDataTimeTableFile<<" at line:\n"<<line<<"\nwith size"<<results.size()<< " will abort"<< endl;
                return  EXIT_FAILURE;
            }
            else
            {
          	DataTuple d;
		try {
                	//#sec;msec;posX;posY;frameNumber
                	d.t_sec =  std::stoi(results[0]);
                	d.t_msec =  std::stoi(results[1]);
                	d.pos_x = std::stof(results[2]);
                	d.pos_y = std::stof(results[3]);
                	d.pic_ID = std::stoi(results[4]);
		}
    		catch (exception &ex) {
        		cout << ex.what() << " error during cast data tuple from timetable file "<< endl ;
                        d.pos_x = 0.0; //std::stof(results[2]);
                        d.pos_y = 0.0; //std::stof(results[3]);

    		}

                mDataVec.push_back(d);

            }
        }
        myfile.close();
	//cout <<"read in done, next step project\n";
    }
    else
    {
        cerr<<"error during open file:" << pathDataTimeTableFile<<" will abort"<< endl;
        return  EXIT_FAILURE;
    }


    if(mDataVec.empty()||mDataVec.size()<2)
    {
        cerr<<"error no data parsed will skip here";
        return  EXIT_FAILURE;
    }

    //calc distance, maybe we should use:
    //https://github.com/DIPlib/diplib
    //maybe we shoud use a fixed relation for
    //maybe with the focal length

    //we calc the maximum time in ms
    //double maxTimeInMs = 3600000;
    DataTuple dStart = mDataVec[0];


    //the really exact fit method
    //we calc time diff from the sampling method
    //we try to out to new file
    ofstream myTrajectFile;
    std::string outFileNameTraj = pathDataOut + "/" + "trajectory.csv";
    myTrajectFile.open (outFileNameTraj);
    myTrajectFile << "#timeSinceStartInMM;positionX;positionY;framenumber"<<endl;
    myTrajectFile.precision(dbl::max_digits10);

    for(size_t i=1;i<mDataVec.size();i++)
    {
        //we get our data point
        DataTuple d = mDataVec[i];

        //get the diff time
        double timeSinceStartInMS= getTimeDiff(d,dStart);

        //we do the projection stuff

        //we do create 2d points
        Point2d p2d(d.pos_x,d.pos_y);

        //we covert our 2dpoints to matrix, with z dimension zero
        Mat pPixel = convertPointToMat(p2d);

        //we project the points into real world space
        Mat pReal = project(pPixel);

        // again a point
        //cout <<"x:\t"<<pReal.at<double>(0,0) <<" y:\t"<<pReal.at<double>(1,0)<<" z:\t"<<pReal.at<double>(2,0)<<endl;
        //myTrajectFile << "#timeSinceStartInMM;positionX;positionY;framenumber"<<endl;
        myTrajectFile << timeSinceStartInMS << ";"<< pReal.at<double>(0,0)<<";"<<pReal.at<double>(1,0)<<";"<< d.pic_ID<<endl;

    }
    myTrajectFile.close();

    return EXIT_SUCCESS;
}

double getEuklidDistance(Point2f pA, Point2f pB)
{
    return cv::norm(cv::Mat(pA),cv::Mat(pB));
}

Mat project(Mat a)
{
    Mat point3Dw = g_inverseHomographyMatrix*a;
    //cout << "Point 3D-W : " << point3Dw << endl << endl;

    double w = point3Dw.at<double>(2, 0);//get W
    //cout << "W: " << w << endl << endl;

    //we need to normalise this point
    divide(point3Dw,w,point3Dw);

    return point3Dw;
}

Mat convertPointToMat(Point2d a)
{
    return (Mat_<double>(3, 1) << a.x, a.y, 1.0);
}

double calcEuclidianDistanc(Mat a, Mat b)
{
    return cv::norm(a,b,cv::NORM_L2);
}

Mat loadFileAndCalcInverseHomographyMatrix(String filename)
{
    Mat cameraMatrix, distCoeffs, rotationVector, rotationMatrix,
            translationVector,extrinsicMatrix, projectionMatrix, homographyMatrix,
            inverseHomographyMatrix,combindedBigMat;

    FileStorage fs(filename, FileStorage::READ);

    fs["camera_matrix"] >> cameraMatrix;
    //cout << "Camera Matrix: " << cameraMatrix << endl << endl;

    fs["distortion_coefficients"] >> distCoeffs;
    //cout << "Distortion Coefficients: " << distCoeffs << endl << endl;

    //we also grep all rvec and tvec
    fs["extrinsic_parameters"] >>combindedBigMat;

    //we read the first translation and rotationVector vector
    int i=0;//for the first picture //@see camera_calbration function  saveCameraParams for read in
    Mat r = combindedBigMat(Range(int(i), int(i+1)), Range(0,3));
    Mat t = combindedBigMat(Range(int(i), int(i+1)), Range(3,6));

    //cout <<"rvec:"<<r<<endl << endl;
    //cout <<"tvec:"<<t<<endl << endl;

    rotationVector = r.t(); //we use the one from the camer calibration
    translationVector = t.t(); //we use the one from the camer calibration

    //cout << "Rotation Vector: update " << endl << rotationVector << endl << endl;
    //cout << "Translation Vector: update " << endl << translationVector << endl << endl;

    Rodrigues(rotationVector, rotationMatrix);
    //cout << "Rotation Matrix: " << endl << rotationMatrix << endl << endl;

    hconcat(rotationMatrix, translationVector, extrinsicMatrix);
    //cout << "Extrinsic Matrix: " << endl << extrinsicMatrix << endl << endl;

    projectionMatrix = cameraMatrix * extrinsicMatrix;
    //cout << "Projection Matrix: " << endl << projectionMatrix << endl << endl;

    //we get single elements
    double p11 = projectionMatrix.at<double>(0, 0),
            p12 = projectionMatrix.at<double>(0, 1),
            p14 = projectionMatrix.at<double>(0, 3),
            p21 = projectionMatrix.at<double>(1, 0),
            p22 = projectionMatrix.at<double>(1, 1),
            p24 = projectionMatrix.at<double>(1, 3),
            p31 = projectionMatrix.at<double>(2, 0),
            p32 = projectionMatrix.at<double>(2, 1),
            p34 = projectionMatrix.at<double>(2, 3);


    homographyMatrix = (Mat_<double>(3, 3) << p11, p12, p14, p21, p22, p24, p31, p32, p34);
    //cout << "Homography Matrix: " << endl << homographyMatrix << endl << endl;

    inverseHomographyMatrix = homographyMatrix.inv();


    return inverseHomographyMatrix;
}

int stringToIntWithRemove(std::string s)
{
    //cout <<"s: "<<s;
    //we remove all non meaning full chars
    //see https://stackoverflow.com/questions/816001/removing-non-integers-from-a-string-in-c
    //for discussion
    s.erase(
                remove_if(s.begin(), s.end(),
                          not1(ptr_fun(static_cast<int(*)(int)>(isdigit)))
                          ),
                s.end()
                );

    int i = -1;
    try {
        i  = std::stoi(s.c_str());
    }
    catch (const std::invalid_argument& ia) {
        std::cerr << "Invalid argument: " << ia.what() << '\n';
    }

    return i;
}



float stringToFloat(std::string s)
{
    std::string::size_type sz;
    float i = -1;
    try {
        i  = std::stof(s,&sz);
    }
    catch (const std::invalid_argument& ia) {
        std::cerr << "Invalid argument: " << ia.what() << '\n';
    }

    return i;
}


double getEuklidDistanceInMM(DataTuple a,DataTuple b)
{
    //we do create 2d points
    Point2d aIp(a.pos_x,a.pos_y);
    Point2d bIp(b.pos_x,b.pos_y);

    //we covert our 2dpoints to matrix
    Mat aI = convertPointToMat(aIp);
    Mat bI = convertPointToMat(bIp);

    //we project the points into real world space
    Mat aR = project(aI);
    Mat bR = project(bI);

    //we calc the distance
    return calcEuclidianDistanc(aR,bR);
}

// @return the abs diff of the times
double getTimeDiff(DataTuple a,DataTuple b)
{

    //the better option:
    double ta= (a.t_sec * 1000) + a.t_msec;
    double tb= (b.t_sec * 1000) + b.t_msec;

    double timeTotalInMs = abs(ta-tb); //time in ms

    return timeTotalInMs;
}


std::string mNzero(int i)
{
    std::ostringstream convert;
    convert << i ;
    std::string numberString = convert.str();
    std::string newNumberString = std::string(10 - numberString.length(), '0') + numberString;
    return newNumberString;
}

double calcFrequency(std::queue<double> queue )
{
    double f= 0;
    //we calc the average T
    //so the average distance between two times
    std::vector<double> ts;
    double told = queue.front(); // we get the first element
    queue.pop(); //we remove it

    while (!queue.empty())
    {
        double t = queue.front();
        double diff =t-told;
        //cout <<"diff "<< diff << " in ms"<< endl;
        ts.push_back(diff); //we add the difference to our vector
        queue.pop();
        //we update
        told = t;
    }

    //now calc the mean PERIOD Time
    double meanPeriodTime = std::accumulate( ts.begin(), ts.end(), 0.0)/ts.size();
    //we convert from ms to seconds
    meanPeriodTime= meanPeriodTime / 1000;
    f= 1 / meanPeriodTime;

    return f;
}
