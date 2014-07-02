//
//  PS3EyeCalibration.h
//  PS3EyeStereoDepth
//
//  Created by Weston Wedding on 6/30/14.
//
//

#ifndef __PS3EyeCalibration_h
#define __PS3EyeCalibration_h

#include "ps3eye.h"
#include "PS3EyePair.h"
#include "opencv.hpp"
#include <string>
#include <exception>

using namespace std;
using namespace cv;

typedef PS3EyePair PS3EyeCams;

class PS3EyeCalibration {
public:
    PS3EyeCalibration(const PS3EyeCams &cameras);
    
    bool generateCalibrationPointsFrame(int camIndex, int height, int width, uint8_t *frameMemPtr, int rowCornerCount, int columnCornerCount, int bufferByteSize, bool draw = FALSE);
    bool finishedCalibrating(void);
    bool readyToCalibrate(void);
    bool calibrate(int camIdx, int rowCount, int columnCount, int squareSize, int imgWidth, int imgHeight);
    void stereoDepthMap(Mat left, Mat right);
    void loadSettingsFile(string string = "default.xml");
protected:
    const PS3EyeCams &cameras;
    bool calibrated = FALSE;
    bool finished = FALSE;
    
    std::map<int, vector<vector<Point2f>>> calibrationPoints;
    std::map<int, vector<Mat>> cameraMatrixes;
    std::map<int, vector<Mat>> distortionCoeffs;
    std::map<int, vector<Mat>> rotationVecs;
    std::map<int, vector<Mat>> translationVecs;

private:
    const int MAX_CALIBRATION_POINT_VECTOR_COUNT = 10;
};



class PS3EyeCalibrationException : public exception {
   
private:
    string explanation = "";
    
public:
    PS3EyeCalibrationException(string explain) {
        explanation = explain;
    }
    virtual const char* what() const throw() {
        return explanation.c_str();
    }
};


#endif
