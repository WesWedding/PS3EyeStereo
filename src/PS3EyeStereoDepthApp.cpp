#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "PS3EyePair.h"
#include "PS3EyeCalibration.h"

using namespace ci;
using namespace ci::app;
using namespace std;

static void yuv422_to_rgba(const uint8_t *yuv_src, const int stride, uint8_t *dst, const int width, const int height);

class PS3EyeStereoDepthApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
    void shutdown();
    
  protected:
    void drawCamera(int index);
    void drawCamera1(void);
    void drawCamera2(void);
    void eyeUpdateThreadFn();
    
    bool stopThreads = FALSE;
    std::vector<Surface> cameraSurfaces;
    std::thread devicesUpdateThread;
    std::vector<gl::Texture> displayTextures;
    std::vector<WindowRef> windowRefs;
    PS3EyePair camPair;
   private:
    void calibrateCameras(void);
    void updateCameraRef(ps3eye::PS3EYECam::PS3EYERef ref, int index);
};


void PS3EyeStereoDepthApp::setup()
{
    int windowWidth = 640, windowHeight = 400;
    
    // We'll be pushing a new window into the WindowRefs vector for each camera.
    // Since this window already exists, we're always going to push it immediately
    // and then assign camera one to it.
    windowRefs.push_back(getWindowIndex(0));
        
    // Get the found PS3Eyes and try to initialize.
    std::vector<ps3eye::PS3EYECam::PS3EYERef> devices(ps3eye::PS3EYECam::getDevices());
    if (camPair.size() >= 1) {
        
        /* @todo: It would be cool to make this more dynamic somehow but I couldn't figure out how.  So each window has a hardcoded draw function and window
            creation. */
        getWindowIndex(0)->connectDraw(&PS3EyeStereoDepthApp::drawCamera1, this);
        WindowRef camWindow2 = createWindow( Window::Format().size(windowWidth, windowHeight));
        camWindow2->getSignalDraw().connect(
                                           [this] () {
                                               gl::clear(ColorA::gray(0.5f));
                                           });
        camWindow2->setPos(getWindowIndex(0)->getPos() - ci::Vec2i(windowWidth, 0));
        camWindow2->connectDraw(&PS3EyeStereoDepthApp::drawCamera2, this);
        windowRefs.push_back(camWindow2);
        
        camPair.init(windowWidth, windowHeight);
        cameraSurfaces.assign(camPair.size(), Surface{});
        displayTextures.assign(camPair.size(), gl::Texture{});
        vector<ps3eye::PS3EYECam::PS3EYERef> camRefs = camPair.getRefs();
        ps3eye::PS3EYECam::PS3EYERef eyeRef;
        for (int i = 0; i < camRefs.size(); ++i) {
            eyeRef = camRefs[i];
            uint8_t *frame = camPair.framePtrAt(i);
            cameraSurfaces[i] = Surface(frame, eyeRef->getWidth(), eyeRef->getHeight(), eyeRef->getWidth() * 4, SurfaceChannelOrder::BGRA);
        }
        
        camPair.start();
    }
}

void PS3EyeStereoDepthApp::mouseDown( MouseEvent event )
{
}

void PS3EyeStereoDepthApp::update()
{
    bool newFrame = FALSE;
    if (camPair.size() > 0) {
        vector<ps3eye::PS3EYECam::PS3EYERef> refs = camPair.getRefs();
        for (int i = 0; i < camPair.size(); ++i) {
            ps3eye::PS3EYECam::PS3EYERef eyeRef = refs[i];
            newFrame = newFrame || eyeRef->isNewFrame();
            if (newFrame) {
                updateCameraRef(eyeRef, i);
            }            
        }
    }
}

void PS3EyeStereoDepthApp::drawCamera1() {
    drawCamera(0);
}

void PS3EyeStereoDepthApp::drawCamera2() {
    drawCamera(1);
}

void PS3EyeStereoDepthApp::drawCamera(int index) {
	// clear out the window with black
    gl::clear( Color( 0, 0, index * 100 ) );
    gl::disableDepthRead();
    gl::disableDepthWrite();
    gl::enableAlphaBlending();
    gl::Texture texture;
    
    gl::setMatricesWindow(getWindowWidth(), getWindowHeight());
    try {
      texture = displayTextures.at(index);
    }
    catch (...) {
        console() << "Missing texture" << endl;
    }
    if (texture) {
        glPushMatrix();
        gl::draw(texture);
        glPopMatrix();
    }
}


void PS3EyeStereoDepthApp::calibrateCameras(void) {
    PS3EyeCalibration calibrator{camPair};
    
}

void PS3EyeStereoDepthApp::updateCameraRef(ps3eye::PS3EYECam::PS3EYERef ref, int index)
{
    auto frame = cameraSurfaces[index];
    uint8 *frameMemPtr = camPair.framePtrAt(index);
    yuv422_to_rgba(ref->getLastFramePointer(), ref->getRowBytes(), frameMemPtr, frame.getWidth(), frame.getHeight());
    displayTextures[index] = gl::Texture(frame);
}

void PS3EyeStereoDepthApp::shutdown() {
    camPair.stop();
}

static const int ITUR_BT_601_CY = 1220542;
static const int ITUR_BT_601_CUB = 2116026;
static const int ITUR_BT_601_CUG = -409993;
static const int ITUR_BT_601_CVG = -852492;
static const int ITUR_BT_601_CVR = 1673527;
static const int ITUR_BT_601_SHIFT = 20;

static void yuv422_to_rgba(const uint8_t *yuv_src, const int stride, uint8_t *dst, const int width, const int height) {
    const int bIdx = 0;
    const int uIdx = 0;
    const int yIdx = 0;
    
    const int uidx = 1 - yIdx + uIdx * 2;
    const int vidx = (2 + uidx) % 4;
    int j, i;
    
#define _max(a, b) (((a) > (b)) ? (a) : (b))
#define _saturate(v) static_cast<uint8_t>(static_cast<uint32_t>(v) <= 0xff ? v : v > 0 ? 0xff : 0)
    
    for (j = 0; j < height; j++, yuv_src += stride)
    {
        uint8_t* row = dst + (width * 4) * j; // 4 channels
        
        for (i = 0; i < 2 * width; i += 4, row += 8)
        {
            int u = static_cast<int>(yuv_src[i + uidx]) - 128;
            int v = static_cast<int>(yuv_src[i + vidx]) - 128;
            
            int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
            int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
            int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;
            
            int y00 = _max(0, static_cast<int>(yuv_src[i + yIdx]) - 16) * ITUR_BT_601_CY;
            row[2-bIdx] = _saturate((y00 + ruv) >> ITUR_BT_601_SHIFT);
            row[1]      = _saturate((y00 + guv) >> ITUR_BT_601_SHIFT);
            row[bIdx]   = _saturate((y00 + buv) >> ITUR_BT_601_SHIFT);
            row[3]      = (0xff);
            
            int y01 = _max(0, static_cast<int>(yuv_src[i + yIdx + 2]) - 16) * ITUR_BT_601_CY;
            row[6-bIdx] = _saturate((y01 + ruv) >> ITUR_BT_601_SHIFT);
            row[5]      = _saturate((y01 + guv) >> ITUR_BT_601_SHIFT);
            row[4+bIdx] = _saturate((y01 + buv) >> ITUR_BT_601_SHIFT);
            row[7]      = (0xff);
        }
    }
}

CINDER_APP_NATIVE( PS3EyeStereoDepthApp, RendererGl )
