//
//  PS3EyePair.h
//  PS3EyeStereoDepth
//
//  Created by Weston Wedding on 6/27/14.
//
//

#ifndef __PS3EyePair_h
#define __PS3EyePair_h

#include <iostream>
#include <vector>
#include "ps3eye.h"

class PS3EyePair {
    
public:
    PS3EyePair() :
        camRefs(ps3eye::PS3EYECam::getDevices())
        {};
    ~PS3EyePair();
    
    void init(int width, int height, int index = -1);
    void init(int index = -1);
    void stop(int index = -1);
    void start(int index = -1);
    int size(void) const;
    uint8_t *framePtrAt(int index);
    std::vector<ps3eye::PS3EYECam::PS3EYERef> getRefs(void);
    
protected:
    void updateCameraRef(ps3eye::PS3EYECam::PS3EYERef eyeRef, int index);
    bool stopThread = FALSE;

private:
    void initOnRef(int index, int width, int height);
    void stopOnRef(int index, int width, int height);
    void startOnRef(int index, int width, int height);
    void startUpdateThread();
    void stopUpdateThread();
    void updateThreadFn(void);
    
    int camWidth;
    int camHeight;
    std::vector<ps3eye::PS3EYECam::PS3EYERef> camRefs;
    std::vector<uint8_t *> frameMemPtrs;
    std::thread devicesUpdateThread;
    
};

#endif /* defined(__PS3EyeStereoDepth__PS3EyePair__) */
