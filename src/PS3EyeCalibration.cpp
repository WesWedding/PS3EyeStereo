//
//  PS3EyeCalibration.cpp
//  PS3EyeStereoDepth
//
//  Created by Weston Wedding on 6/30/14.
//
//

#include "PS3EyeCalibration.h"

PS3EyeCalibration::PS3EyeCalibration(const PS3EyeCams &cams) : cameras(cams)
{
    for (int i = 0; i < cameras.size(); ++i) {
      calibrationPoints.insert(make_pair(0, vector<vector<Point2f>>{}));
    }
}

void PS3EyeCalibration::loadSettingsFile(string fName) {
    FileStorage fs(fName, FileStorage::READ);
    if (!fs.isOpened()) {
        throw new PS3EyeCalibrationException("Couldn't open settings file (name: " + fName + ").");
    }
}

bool PS3EyeCalibration::calibrate(int camIdx, int rowCount, int columnCount, int squareSize, int imgWidth, int imgHeight) {
    bool ok = FALSE;
    map<int, vector<vector<Point3f>>> objectPoints;
    cv::Size boardSize(rowCount, columnCount), imageSize(imgHeight, imgHeight);
    // Do the following steps for every camera in the index:
    // 1.  Create a vector of vectors of Point3f that represent the object's 3D
    //     coordinates.  We're just pretending it was sitting at Z = 0 because
    //     we're using a planar pattern.
    // 2.  Duplicate that first calculated set of points so that we have one for
    //     each capture.
    // 3.  Use calibrateCamera() to get the intrinsic and extrinsic parameters of
    //     the camera.
    // @todo: Move the logic inside the for into an easer-to-read code block of its own.
    Mat distortionCoeffs = Mat::zeros(8, 1, CV_64F);
    vector<Mat> rvecs;
    vector<Mat> tvecs;
    vector<float> reprojectionErrors;
    double totalAvgError;
    Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
    
    
    // Create an empty objectPoints vector for this particular camera index.
    objectPoints.insert(make_pair(camIdx, vector<vector<Point3f>>(1)));
    for( int i = 0; i < boardSize.height; ++i )
        for( int j = 0; j < boardSize.width; ++j ) {
            objectPoints[camIdx][0].push_back(Point3f(float( j*squareSize ), float( i*squareSize ), 0));
        }
    // Now that we have one, resize the array so we have a set of object points
    // for every single calibrationPoints set for this camera.
    objectPoints[camIdx].resize(calibrationPoints[camIdx].size(), objectPoints[camIdx][0]);
    double rms = calibrateCamera(objectPoints[camIdx], calibrationPoints[camIdx], imageSize, cameraMatrix, distortionCoeffs, rvecs, tvecs);
    ok = checkRange(cameraMatrix) && checkRange(distortionCoeffs);
    calibrated = ok;
    finished = TRUE;
    return ok;
}

bool PS3EyeCalibration::finishedCalibrating(void) {
    return finished;
}

bool PS3EyeCalibration::generateCalibrationPointsFrame(int camIndex, int height, int width, uint8_t *frameMemPtr, int rowCornerCount, int columnCornerCount, int bufferByteSize, bool draw) {
    if (calibrationPoints[camIndex].size() >= MAX_CALIBRATION_POINT_VECTOR_COUNT) {
        return FALSE;
    }
    
    bool found = FALSE;
    vector<Point2f> corners;
    cv::Size patternSize(rowCornerCount, columnCornerCount);
    
    Mat image(cv::Size(width, height), CV_8UC4, frameMemPtr);
    // Converting to grey might help down the road.
    //Mat grey_image;
    //cvtColor(image, grey_image, CV_RGBA2GRAY);
    //    cout << image << endl;
    try {
        found = findChessboardCorners(image, cv::Size(rowCornerCount, columnCornerCount), corners, CV_CALIB_CB_ADAPTIVE_THRESH + CV_CALIB_CB_NORMALIZE_IMAGE + CV_CALIB_CB_FAST_CHECK);
    }
    catch(...) {
        found = FALSE;
    }
    
    if (found) {
        calibrationPoints[camIndex].push_back(corners);
    }
    
    return found;
}

bool PS3EyeCalibration::readyToCalibrate(void) {
    for (int i = 0; i < cameras.size(); ++i) {
        if (calibrationPoints[i].size() < MAX_CALIBRATION_POINT_VECTOR_COUNT) {
            return FALSE;
        }
    }
    return TRUE;
}

void PS3EyeCalibration::stereoDepthMap(Mat left, Mat right) {
    //http://stackoverflow.com/questions/23643813/opencv-stereo-vision-depth-map-code-does-not-work
    Mat disp;
    StereoBM sbm;
    sbm.state->SADWindowSize = 9;
    sbm.state->numberOfDisparities = 112;
    sbm.state->preFilterSize = 5;
    sbm.state->preFilterCap = 61;
    sbm.state->minDisparity = -39;
    sbm.state->textureThreshold = 507;
    sbm.state->uniquenessRatio = 0;
    sbm.state->speckleWindowSize = 0;
    sbm.state->speckleRange = 8;
    sbm.state->disp12MaxDiff = 1;
    
    sbm(left, right, disp);
}