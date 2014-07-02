//
//  PS3EyePair.cpp
//  PS3EyeStereoDepth
//
//  Created by Weston Wedding on 6/27/14.
//
//

#include "PS3EyePair.h"

PS3EyePair::~PS3EyePair() {
    stop();
    for_each(frameMemPtrs.begin(), frameMemPtrs.end(),
             [](uint8_t *ptr) {
                 delete[] ptr;
             });
}


int PS3EyePair::size(void) const {
    return camRefs.size();
}


void PS3EyePair::init(int width, int height, int index) {
    frameMemPtrs.assign(camRefs.size(), nullptr);
    if (index >= 0) {
        return initOnRef(index, width, height);
    }
    for (int i = 0; i < camRefs.size(); ++i) {
        initOnRef(i, width, height);
    }
}

void PS3EyePair::initOnRef(int index, int width, int height) {
    // Initialize the camera refs with window dimensions,
    // and create frame memory bitmap for each, then  start
    // the threads for each one.
    ps3eye::PS3EYECam::PS3EYERef eyeRef = camRefs[index];
    if (eyeRef->init(width, height, 60)) {
        // Pad the data to 4 bytes per pixel for fast 4-byte indexing.
        frameMemPtrs[index] = new uint8_t[eyeRef->getWidth() * eyeRef->getHeight() * 4];
    }
}


void PS3EyePair::start(int index) {
    if (index >= 0) {
        return camRefs[index]->start();
    }
    
    for (int i = 0; i < camRefs.size(); ++i) {
        camRefs[i]->start();
    }
    startUpdateThread();
}



void PS3EyePair::stop(int index) {
    if (index >= 0) {
        return camRefs[index]->stop();
    }
    else {
        stopUpdateThread();
    }
    
    for (int i = 0; i < camRefs.size(); ++i) {
        camRefs[i]->stop();
    }
}



uint8_t *PS3EyePair::framePtrAt(int index) {
    return frameMemPtrs.at(index);
}

std::vector<ps3eye::PS3EYECam::PS3EYERef> PS3EyePair::getRefs(void) {
    return camRefs;
}

void PS3EyePair::startUpdateThread(void) {
    stopUpdateThread();
    stopThread = FALSE;
    devicesUpdateThread = std::thread(std::bind(&PS3EyePair::updateThreadFn, this));
}

void PS3EyePair::stopUpdateThread(void) {
    stopThread = TRUE;
    if (devicesUpdateThread.joinable()) {
        devicesUpdateThread.join();
    }
}

void PS3EyePair::updateThreadFn(void) {
    int updateCount = 0;
    while (!stopThread) {
        ++updateCount;
        bool res = ps3eye::PS3EYECam::updateDevices();
        if (!res) {
            break;
        }
    }
}
