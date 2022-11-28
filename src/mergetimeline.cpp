//std
#include <math.h>
#include <iostream>
#include <iterator>
#include <fstream>
#include <string>
#include <stdio.h>
#include <map>

//opencv
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

using namespace cv;
using namespace std;

static void help()
{
    cout << "usage:\n./mergetimeline -d=datafolder -r=resourcefolder -a=amountFile -o=outputfile"<< endl;
}

//data classe for position
class DataTuple
{
public:
    unsigned int t_sec;
    unsigned int t_msec;
    float pos_x;
    float pos_y;
    int pic_ID;
};

//template for iterate file
template<char delimiter>
class WordDelimitedBy : public std::string
{};

//function for zero padding a number
std::string mNzero(int i);

// a function which return the read in map
std::map<int,DataTuple> readFileList(String file);

int main( int argc, char** argv)
{
    cout <<"make time table version "<< 3 << "."<<2<<endl;

    // Define and process cmdline arguments.
    cv::CommandLineParser parser(argc, argv,
                                 "{ t timetable            |           | timetable file }"
                                 "{ r results  |        | path to recults from bgs  }"
                                 "{ a amount  |        | amount of single files }"
                                 "{ o output             |           | out put file name  }"
                                 );

    String fileTimeTable = parser.get<String>("t");
    String pathResults = parser.get<String>("r");
    int amountFiles = parser.get<int>("a");
    String outPutFileName = parser.get<String>("o");

    cout <<"./mergetimeline -t="<<fileTimeTable <<" -r=" << pathResults <<" -a="<<amountFiles << "-o="<<outPutFileName<<endl;

    if (!parser.check())
    {
        parser.printErrors();
        return 0;
    }

    int i=0;
    int counterFrameIgnoredFrameNoPoly=0;
    int counterTwoOrMorePolyies = 0;
    int counterExactOnePoly = 0;
    vector<DataTuple> mDataVec;

    //here we read a the tuple  framenumber,  to data
    std::map<int,DataTuple> mapTimeMap =  readFileList(fileTimeTable);
    DataTuple oldData;
    //we search for the key zero
    if (mapTimeMap.find(0) != mapTimeMap.end())
        oldData=mapTimeMap[0];
    else
        cout <<"error during search for the first key "<< endl;


    int counterFrameIgnoreYMLerror = 0;
    int counterFrameIgnoreTTerror = 0;
    //we produce all files
    for(i=1;i<amountFiles;i++)
    {
        if(i%1000==0)
            cout <<"processing step:" << i<<" of "<<amountFiles<<" steps"<<endl;

        //we get then frame time table entry **************

        //we check if the key NOT exits
        if (mapTimeMap.find(i) == mapTimeMap.end()) // the same as:  if(mapTimeMap.count(i) <= 0)
        {
            cout<<"warning #0815 problem  could not found the frames number   "<< i<<" in map"<<endl;
            counterFrameIgnoreTTerror++; //we count the error
            continue; //we overjump the rest
        }
        DataTuple dat =mapTimeMap[i];

        //we get the result out of our yml file **************
        vector<Point2f> massCenters; //all mass center of the polygones
        vector<vector<Point> > contourSelection; //a collection of all contours, with minRect area > 150 &&  area<15000 pixel x pixel
        vector<Point> conHull; //convex  hull over all polygone points
        Point2f muConvexHullMassCenter;//mass center @see muConvexHull.m10/muConvexHull.m00 , muConvexHull.m01/muConvexHull.m00
        //we load two files
        cv::String resultsFile = pathResults + String(mNzero(i)) +String(".yml");

        //we read in
        FileStorage fs;
        fs.open(resultsFile, FileStorage::READ);
        if (!fs.isOpened())
        {
            cout<<"error during open file:" << resultsFile<<" will ignore this file"<< endl;
            //return  EXIT_FAILURE;
            counterFrameIgnoreYMLerror++;
            continue; //we jump to the next frame
        }
        else
        {
            fs["masscenters"] >> massCenters;
            fs["polygonselection"] >> contourSelection;
            fs["convexhull" ]>>conHull;
            fs["masscenterconvexhull"]>>muConvexHullMassCenter;
        }
        fs.release();

        //now we prodcue the tuple
        //if we only the convex hull centroid, if there is at least 1 mass center
        if(massCenters.size()>=1)
        {
            //TODO: implement more intelligent selection for case masscenter.size >= 2
               //e.g. single polygon distance is more than thresold e.g. 100 pixel ??
               //we ignore

            //DataTuple dat;
            //we here grab all the time the convex polygon center
            dat.pos_x = muConvexHullMassCenter.x;
            dat.pos_y = muConvexHullMassCenter.y;

            mDataVec.push_back(dat);
            //we remember the old
            oldData = dat;
        }
        else
        {
            //cout<<"warning will ignore frame: "<< i<< endl;
            counterFrameIgnoredFrameNoPoly++;

            //TODO here we copy the old position with a new time
            //we update the time
            dat.pos_x = oldData.pos_x;
            dat.pos_y = oldData.pos_y;

            mDataVec.push_back(dat);

            //TODO implement intelligent selection
        }
        //we count if their is more than one
        if(massCenters.size()>1)
            counterTwoOrMorePolyies++;

        if(massCenters.size()==1)
            counterExactOnePoly ++;

    }//end for loop

    //now we calc the mean of the radius

    cout<< "#amount frames used: "<<i<<endl;
    cout<< "#amount ignored frames (no poly at all exclude): "<<counterFrameIgnoredFrameNoPoly<<endl;
    cout<< "#amount ignored frames (yml file not found): "<<counterFrameIgnoreYMLerror<<endl;
    cout<< "#amount ignored frames (tt file not found): "<<counterFrameIgnoreTTerror<<endl;
    cout<< "#amount counting frames (more than 1 polygones): "<<counterTwoOrMorePolyies<<endl;
    cout<< "#amount counting frames (exact 1 polygone): "<<counterExactOnePoly<<endl;
    
    //we write all down
    ofstream myfile2 (outPutFileName);
    std::string sep(";");

    if (myfile2.is_open())
    {
        //we write the header
        myfile2<< "#time table of record\n";
        myfile2<< "#amount frames used: "<<i<<endl;
        myfile2<< "#amount ignored frames (no poly at all): "<<counterFrameIgnoredFrameNoPoly<<endl;
        myfile2<< "#amount ignored frames (yml file not found): "<<counterFrameIgnoreYMLerror<<endl;
        myfile2<< "#amount ignored frames (tt file not found): "<<counterFrameIgnoreTTerror<<endl;
        myfile2<< "#amount counting frames (more than 1 polygones in frame): "<<counterTwoOrMorePolyies<<endl;
        myfile2<< "#amount counting frames (exact 1 polygone in frame): "<<counterExactOnePoly<<endl;

        myfile2<<"#header"<<endl;
        myfile2<<"#sec"<<sep<<"msec"<<sep<<"posX"<<sep<<"posY"<<sep<<"frameNumber\n";

        for(DataTuple const& value: mDataVec)
        {
            myfile2 << value.t_sec<<sep<<value.t_msec<<sep<<value.pos_x<<sep<<value.pos_y<<sep<<value.pic_ID<<"\n";
        }
    }
    else cout << "Unable to open file" <<outPutFileName ;
    myfile2.close();


    return 0;
}

std::string mNzero(int i)
{
    std::ostringstream convert;
    convert << i ;
    std::string numberString = convert.str();
    std::string newNumberString = std::string(10 - numberString.length(), '0') + numberString;
    return newNumberString;
}

std::map<int,DataTuple> readFileList(String file)
{
    //TODO open time table
    int sec = -1;
    int msec = -1;
    int picID = -1;
    int t_secHistory = 0;
    std::map<int,DataTuple> map;


    //since opencv 4.X cv::String == std::string
    //see https://stackoverflow.com/a/52964322/14105642
    std::string ttFileName = file; //.operator std::string(); //ttFileName = pathData + mNzero(i) + std::string(".tt");
    string line;
    ifstream myfile (ttFileName);
    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
            //we check if line contains a comment so we skip it
            if (line.find(std::string("#")) != std::string::npos) {
                cout <<"comment found on line: "<< line << "\nwill skip it"<<endl;
                continue;
            }

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
            if(results.size()!=3)
            {
                cout<<"error 42 during parse file:" << ttFileName<<" at line:"<<line<<"with size"<<results.size()<< " will abort"<< endl;
                exit(1);
            }
            else
            {
                sec =  std::stoi(results[0]);
                msec =  std::stoi(results[1]);
                picID = std::stoi(results[2]);

                // we double check if monontic time values 
                if (sec - t_secHistory < 0)
                {
                    cout << "error parse file no monotonic time value at frame:" << picID << " of file:" << ttFileName << "at line:" << line << " will abort"
                         << " :msec-t_secHistory" << (int)(msec - t_secHistory) << endl;
                    exit(1);
                }

                t_secHistory = sec;
                //cout << "timetable read : fr:"<< picID << " ;  "<< sec << "s ;  " << msec<< " ms"<< endl;

                DataTuple d;

                d.t_sec = sec;
                d.t_msec = msec;
                d.pic_ID = picID;

                if (map.find(picID) != map.end())
                {
                    cerr<<" error during insert into map: key exists"<< picID<<" problem with doubled frames number !!! \n"<<endl;
                    //exit(1);

                }
                else
                {
                    //we insert this in our map
                    //we save our reference
                    map[picID] = d;
                }
            }
        }
        myfile.close();
    }
    else
    {
        cerr<<"error during open file:" << ttFileName<<" will abort here"<< endl;
        exit(1);
    }

    return map;
}
