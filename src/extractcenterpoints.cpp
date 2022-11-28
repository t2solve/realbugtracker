// cpp c
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <stdio.h>  /* printf, scanf, puts, NULL */
#include <stdlib.h> /* srand, rand */
#include <time.h>   /* time */
#include <ctime>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

// cpp 11X
#include <regex>

// opencv
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

// bgslibrary
#include <algorithms/algorithms.h>

// my class
#include "ttoolbox.h"

#define PROCESS_CENTER_VERSION_MAJOR 2
#define PROCESS_CENTER_VERSION_MINOR 0
#define PROCESS_CENTER_VERSION_MINOR_FIXES 1

//optional debug 
//#define MC_SHOW_STEP_ANALYSE

using namespace cv;
using namespace std;

//! function to copy from the archvie to a opencv buffer
vector<char> copyDataInBuffer(struct archive *aw);

//! we convert our actual jpg file number to framenumber
int toFrameNumber(string filename);

//! test of the file end with prefix
bool stringEndWith(string value, string ending);

int main(int argc, char *argv[])
{

    cout << "using extractcenterpoints version " << PROCESS_CENTER_VERSION_MAJOR << "." << PROCESS_CENTER_VERSION_MINOR << "." << PROCESS_CENTER_VERSION_MINOR_FIXES << endl;
    cout << "using OpenCV version " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << endl;

    CommandLineParser parser(argc, argv,
                                 "{ i input             |           | input tar file file }"
                                 "{ a amount  |        | amount of files in tar, should be biggest number of frame }"
                                 "{ c centerfile  |        | path to center config file  }"
                                 "{ o output             |           | out put file name  }"
                                 "{ p cameraparameter       |           | camera calibration parameter  }"

    );
    //parse input parameter
    String inputFile = parser.get<String>("i");
    int amountFiles = parser.get<int>("a");
    String centerFileString = parser.get<String>("c");
    String outputDir = parser.get<String>("o");
    String cameraParameterFile = parser.get<String>("p");
    cout << "./extractcenterpoints -i=" << inputFile << " -a=" << amountFiles << " -c=" << centerFileString << "-o=" << outputDir << "-p=" << cameraParameterFile << endl;

    // load libarchive structure for processing tar 
    struct archive *archive;
    struct archive *ext;
    struct archive_entry *entry;
    int r;
    int flags = ARCHIVE_EXTRACT_TIME; // see https://linux.die.net/man/3/archive_write_disk for more flags
    const char *filename = inputFile.data();
    archive = archive_read_new();
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_read_support_format_tar(archive);

    //we get the filename
    if (filename != NULL && strcmp(filename, "-") == 0)
        filename = NULL;
    // and open the handler
    if ((r = archive_read_open_filename(archive, filename, 10240)))
    {
        cerr << "archive_read_open_filename: error: " << archive_error_string(archive) << " will abort" << endl;
        exit(1);
    }

    //background subtraction methods */
    IBGS *bgs;
    bgs = new PixelBasedAdaptiveSegmenter;

    // see paper https://dl.acm.org/citation.cfm?id=2321600
    // https://ieeexplore.ieee.org/document/4527178/
    // was in benchmark on top https://www.researchgate.net/publication/259340906_A_comprehensive_review_of_background_subtraction_algorithms_evaluated_with_synthetic_and_real_videos
    // my own adapter to use the model

    // init the random number generator
    srand(time(NULL));
    clock_t beginAll = clock();

    // we read the center config file for cut out the ROI
    // TODO merge this config 
    // circle param
    int circleCenterX = 880;
    int circleCenterY = 750;
    int circleRadius = 700;
    String configFileNameCenter(centerFileString);
    // read the config
    cout << "parameter of centerConfigFile.xml" << endl;
    FileStorage fsCen;
    fsCen.open(configFileNameCenter, FileStorage::READ);
    if (!fsCen.isOpened())
    {
        cout << "error during open " << centerFileString << " will abort\n ";
        return EXIT_FAILURE;
    }
    circleCenterX = (int)fsCen["circleCenterX"];
    cout << "circleCenterX: " << circleCenterX << endl;
    circleCenterY = (int)fsCen["circleCenterY"];
    cout << "circleCenterY: " << circleCenterY << endl;
    circleRadius = (int)fsCen["circleRadius"];
    cout << "circleRadius: " << circleRadius << endl;
    fsCen.release();

    // we open the camera config file
    String camParam(cameraParameterFile);
    Mat intrinsicsCameraMatrix, distortionCoeff;
    {
        cout << "read cam parameter of: " << cameraParameterFile;
        FileStorage fs;
        fs.open(camParam, FileStorage::READ);
        if (!fs.isOpened())
        {
            cerr << "error during open " << cameraParameterFile << " will abort\n ";
            return -1;
        }
        fs["camera_matrix"] >> intrinsicsCameraMatrix;
        cout << "camera_matrix = " << intrinsicsCameraMatrix << endl;
        fs["distortion_coefficients"] >> distortionCoeff;
        cout << "distortion_coefficients" << distortionCoeff << endl;
    }

    int counterWriteDebugPicuteEveryNPics = 20; // 40 * 5;
    // string fileName;
    int frameCounter = 0;
    int myFileTarWriterHeaderCounter = 0;
    int frameCounterOld = -1;
    //if errors occure we save the file name
    vector<string> myFileErrorList;                 // list for pure io error
    vector<string> myFileListNoContour;             // list for no contours found;
    vector<string> myFileListAfterContourSelection; // list for no contours found after selection;
    // here we loop over all picture of the tar archive, independing how many this are
    for (;;)
    {

        // measure time consumption
        clock_t begin = clock(); // for every single file
        vector<char> vec;
        // we read the next header
        r = archive_read_next_header(archive, &entry);
        if (r == ARCHIVE_EOF)
            break;
        if (r != ARCHIVE_OK)
        {
            cerr << "archive_read_next_header: error: " << archive_error_string(archive) << " will abort" << endl;
            exit(1);
        }

        // we get the filename
        const char *fileNamePtr = archive_entry_pathname(entry);
        string filename(fileNamePtr, strlen(fileNamePtr));
        // we skip all tt files
        if (!stringEndWith(filename, "jpg"))
        {
            cout << "warning file " << filename << " NOT jpg in tar container found, will ignore this" << endl;
            continue;
        }
        cout << "process file: " << filename << endl;
        // we convert it to a framenumber
        frameCounter = toFrameNumber(filename);
        if (frameCounterOld > frameCounter)
        {
            cerr << " error during read in the file number, we do have non montonic order, will abort";
            exit(1);
        }
        frameCounterOld = frameCounter;

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK)
        {
            cerr << "archive_write_header() error: " << archive_error_string(ext) << endl;
            myFileTarWriterHeaderCounter++;
            continue; // we overjump all in our for loop
        }
     
        // we copy all data to the buffer vector
        vec = copyDataInBuffer(archive);
        if (vec.empty())
            cerr << "error during load data into buffer";

        r = archive_write_finish_entry(ext);
        if (r != ARCHIVE_OK)
        {
            cerr << "archive_write_finish_entry: error: " << archive_error_string(ext) << " will abort" << endl;
            exit(1);
        }
 

        // we read the image buffer as a jpg picture and decode it
        Mat img_input = imdecode(Mat(vec), 1);

        // we define the type better to prevent error
        filename = string(inputFile + TToolBox::getFileName(frameCounter));
        String fileNameCV(filename);
        cout << "\t" << frameCounter << "\tof \t" << amountFiles << "  load file :" << filename << endl;
        
        // of data is present
        if (img_input.data) // checkk for non null
        {

            //! step 0 we take care about the lense distortion
            //! correct lense distortion
            Mat imgLenseCorrection = img_input.clone();
            // see https://docs.opencv.org/2.4/modules/imgproc/doc/geometric_transformations.html#undistort
            undistort(img_input, imgLenseCorrection, intrinsicsCameraMatrix, distortionCoeff);

            //! step 1)
            //! we cut out a smaller ROI,
            img_input = TToolBox::cropImageCircle(imgLenseCorrection, circleCenterX, circleCenterY, circleRadius);

            //! step 2)
            //!  normal bgs processing
            Mat img_mask;
            Mat img_bkgmodel;
            bgs->process(img_input, img_mask, img_bkgmodel); // by default, it shows automatically the foreground mask image

            //! step 3) we make  we apply a edge detection
            // TODO make this in a function, how many times is this listed ??
            // TODO read all parameter from config applyCannyEdgeAndCalcCountours also !!
            double threshholdMin = 150;
            double threshholdMax = 200;
            int apertureSize = 3;
            vector<vector<Point>> contours = TToolBox::applyCannyEdgeAndCalcCountours(img_mask, threshholdMin, threshholdMax, apertureSize);

            // define what we will write down
            vector<vector<Point>> contourSelection;
            vector<Point2f> massCenters;
            vector<Point> conHull;
            Point2f muConvexHullMassCenter(0.0, 0.0);

            if (!contours.empty())
            {

                //! step 4) we make a selection out of all counters with area sizes
                // we calc all min rotated  rectangles for all contour from candy egde detect
                // we exlcude very small one and very big ones
                vector<RotatedRect> minRect(contours.size());

                // calc boxes around the contours
                for (size_t i = 0; i < contours.size(); i++)
                    minRect[i] = minAreaRect(Mat(contours[i])); // may use boundingRect ?? to use fix non rotated rectangles ?

                vector<Point2f> recCenterPoints;
                float areaMinThreshold = 150;
                float areaMaxThreshold = 300000; // old max threshold was to small
                // TODO apply moving filter ??, an more adaptive approach

                // iterate all rectangles
                for (size_t i = 0; i < minRect.size(); i++)
                {
                    Point2f rect_points[4];
                    // get all points of the retange
                    minRect[i].points(rect_points);

                    // construct contour based on rectangle points because contour != rectangle with points
                    vector<Point> contourRect;
                    for (int j = 0; j < 4; j++)
                        contourRect.push_back(rect_points[j]);

                    // calc the area of the contour
                    double area0 = contourArea(contourRect);
                    // over stepp all small areas
                    if (area0 < areaMinThreshold || area0 > areaMaxThreshold)
                    {
                        // cout<<i<<":area0:"<<area0<<" dismissed "<<endl;
                        // skip if the area is too small
                        continue;
                    }
                    else
                    {
                        // cout<<i<<":area0:"<<area0<<" choose "<<endl;
                        // get the center of this rectangle
                        Point2f center = minRect[i].center;
                        recCenterPoints.push_back(center);

                        // we also add the this contour to a selection
                        contourSelection.push_back(contours[i]);
                    }
                } // end iterate all min rectangle

                if (!contourSelection.empty())
                {
                    //! step 5) we calc the moments from the contour selection
                    vector<Moments> mu(contourSelection.size());
                    for (size_t i = 0; i < contourSelection.size(); i++)
                    {
                        mu[i] = moments(contourSelection[i], false);
                    }

                    //  Get the mass centers:
                    massCenters = vector<Point2f>(contourSelection.size());
                    for (size_t i = 0; i < contourSelection.size(); i++)
                    {
                        massCenters[i] = Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
                    }

                    //! step 6) calc convex hull of all points of the contour selection *********
                    // TODO produce center of convex hull of all polygones
                    // TODO double check if ne contour sharing a center point ? nearby ??

                    // we merge all points
                    vector<Point> allContourPoints;
                    for (size_t cC = 0; cC < contourSelection.size(); ++cC)
                        for (size_t cP = 0; cP < contourSelection[cC].size(); cP++)
                        {
                            Point currentContourPixel = contourSelection[cC][cP];
                            allContourPoints.push_back(currentContourPixel);
                        }

                    conHull = vector<Point>(allContourPoints.size());
                    convexHull(Mat(allContourPoints), conHull, false);
                    //optional step: calc the min circle around
                    //Point roiCenter;
                    //float roiRadius;
                    //minEnclosingCircle(conHull,roiCenter,roiRadius);

                    //! step 7) calc the hull ******************
                    /// we calc the mass center of the convex hull
                    Moments muConvexHull;
                    if (!conHull.empty())
                    {
                        muConvexHull = moments(conHull, true);
                        muConvexHullMassCenter = Point2f(muConvexHull.m10 / muConvexHull.m00, muConvexHull.m01 / muConvexHull.m00);
                    }
                }
                else // end after selection is empty
                {
                    cerr << "error, no contour found after selection in file: " << filename << endl;
                    myFileListAfterContourSelection.push_back(filename);
                }
            } // end if contours are empty, to canny edge found nothing
            else
            {
                cerr << "error, no contour found at all in file: " << filename << endl;
                myFileListNoContour.push_back(filename);
            }
#ifdef MC_SHOW_STEP_ANALYSE

            if (!conHull.empty()) // if we any elements, we process further
            {
                Mat imgConvexHull = Mat::zeros(img_input.size(), CV_8UC3);
                imgConvexHull = Scalar(255, 255, 255); // fille the picture

                Scalar colorB(0, 0, 255, 255); // red
                polylines(imgConvexHull, hullComplete, true, colorB, 1, 8);

                // draw circle around
                // circle( imgConvexHull, roiCenter, (int) roiRadius, colorB, 2, 8, 0 );

                // we draw it
                String outpath2 = outputDir;
                ostringstream convert2;
                convert2 << outpath2 << TToolBox::mNzero(frameCounter) << "_convex_hull.jpg";
                imwrite(convert2.str().c_str(), imgConvexHull);
            }
            else
                cout << "convex hull has no points will skip file: " << i << endl;

#endif

            //! step 8: we write down all our results in yml file
            string nameOutPutFileData = outputDir + TToolBox::mNzero(frameCounter) + ".yml";

            FileStorage fs(nameOutPutFileData.c_str(), FileStorage::WRITE);
            fs << "masscenters" << massCenters;
            fs << "polygonselection" << contourSelection;
            fs << "convexhull" << conHull;
            fs << "masscenterconvexhull" << muConvexHullMassCenter;
            fs.release();

            //! we write from time to time a dbg picture
            if (frameCounter % counterWriteDebugPicuteEveryNPics == 0)
            {
                Scalar colorRed(0, 0, 255, 255); // red
                RNG rng(4344234);
                Mat imgDebugPaint2 = Mat::zeros(img_input.size(), CV_8UC3);
                imgDebugPaint2 = Scalar(255, 255, 255); // fill the picture white

                // we write all polyies of the selection and the mass centers with a random color
                for (size_t i = 0; i < contourSelection.size(); i++)
                {
                    // random color
                    Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
                    // contour
                    drawContours(imgDebugPaint2, contourSelection, i, color, 1, LINE_AA);
                    // draw the center
                    circle(imgDebugPaint2, massCenters[i], 4, color, -1, 8, 0);
                }

                // we write the convex hull
                if (!conHull.empty())
                {
                    // the poly
                    polylines(imgDebugPaint2, conHull, true, colorRed, 1, 8);
                    // the center
                    circle(imgDebugPaint2, muConvexHullMassCenter, 4, colorRed, -1, 8, 0);
                }

                // we make a copy
                Mat imgOverlay2 = img_input.clone();
                // we add a overlay of our paitings
                addWeighted(imgDebugPaint2, 0.7, imgOverlay2, 0.3, 0.0, imgOverlay2);
                // we write the file down
                string nameOutPutFileDBGpic = outputDir + TToolBox::mNzero(frameCounter) + string(".jpg");

                imwrite(nameOutPutFileDBGpic.c_str(), imgOverlay2);

            } // end if we write a dbg picture

        } // end if the loaded picture has data
        else
        {
            cerr << "error loading file: " << filename << ", will skip this file" << endl;
            myFileErrorList.push_back(filename);
        }

        // we calc the time which we used for a picture
        clock_t end = clock();
        double elapsedSecs = double(end - begin) / CLOCKS_PER_SEC;

        // we calc the time which was used for all picture
        clock_t endAll = clock();
        double elapsedSecTotal = double(endAll - beginAll) / CLOCKS_PER_SEC;
        cout << "process single pic:\t" << elapsedSecs << " s  - \t\t" << (int)(elapsedSecTotal / 60) << " min -\t" << (int)(elapsedSecTotal / 60 / 60) << " h" << endl;
    } // end for ever loop

    // finishing time
    clock_t endAll = clock();
    double elapsed = double(endAll - beginAll) / CLOCKS_PER_SEC;
    cout << "process : " << amountFiles << " files took:\t" << (int)(elapsed / 60) << " min -\t" << (int)(elapsed / 60 / 60) << " h \n in total" << endl;

    // TODO we should merge the file
    cout << "amount of libarchive read header errors :" << myFileTarWriterHeaderCounter;

    // we write the random file list to a file
    string nameOutErrorList = outputDir + "fileErrorList.yml";
    cout << "amount of file errors: " << myFileErrorList.size() << endl;
    FileStorage fs2(nameOutErrorList.c_str(), FileStorage::WRITE);
    fs2 << "fileErrorIOs" << myFileErrorList;

    // we write error list no contours found in file
    cout << "amount contour errors with canny edge: " << myFileListNoContour.size() << endl;
    fs2 << "fileErrorNoContours" << myFileListNoContour;

    // we write error list no contours found in file
    cout << "amount contour errors after selection: " << myFileListAfterContourSelection.size() << endl;
    fs2 << "fileErrorNoContoursAfterSelections" << myFileListAfterContourSelection;

    // the bgs related objects
    delete bgs;

    // opencv related objects
    fs2.release();
  
    // close lib archive related objects
    archive_read_close(archive);
    archive_read_free(archive);

    archive_write_close(ext);
    archive_write_free(ext);

    return 0;
}

vector<char> copyDataInBuffer(struct archive *aw)
{
    int r;
    const void *buff;
    vector<char> vec;
    size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
    int64_t offset;
#else
    off_t offset;
#endif
    // forever till we get a specific return
    for (;;)
    {
        r = archive_read_data_block(aw, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
        {
            return vec; // everything fine, were are just at the end
        }

        if (r != ARCHIVE_OK)
        {
            cerr << "error during read archive " << endl;
            return vec;
        }

        // a good discussion :  https://stackoverflow.com/questions/259297/how-do-you-copy-the-contents-of-an-array-to-a-stdvector-in-c-without-looping
        //  Method 4: vector::insert
        {
            // we rename our pointer to avoid a weird compiler warning
            char *foo = (char *)buff;
            vec.insert(vec.end(), &foo[0], &foo[size]);
        }
    }

    return vec;
}

int toFrameNumber(string filename)
{
    int retVal = -1;

    // we cut out all non needed substrings
    filename = regex_replace(filename, regex("/"), "");
    filename = regex_replace(filename, regex("data-frames"), "");
    filename = regex_replace(filename, regex("jpg"), "");
    filename = regex_replace(filename, regex("\\."), "");

    retVal = stoi(filename);

    cout << "filename out:" << filename << " number: " << retVal << endl;

    return retVal;
}

bool stringEndWith(string value, string ending)
{
    if (ending.size() > value.size())
        return false;
    return equal(ending.rbegin(), ending.rend(), value.rbegin());
}
