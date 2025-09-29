#include "jni.h"
#include "android/native_window_jni.h"
#include "android/log.h"
#include <string>
#include <istream>
#include <sstream>

struct Buffer {
    int x, y;

    [[nodiscard]] int size() const {
        return x * y;
    }


    std::vector<uint32_t> pixels;

};

namespace Log {

    /**
     *
     * @tparam Args variadic template type
     * @param label label of log
     * @param args pack parameter of variadic template
     */
    template<typename ...Args>
    void LOG(android_LogPriority&& prio, std::string&& label, Args&&... args) {
        std::stringstream oss;
        ((oss << args << " "), ...);

        __android_log_print(prio, std::string("[" + label + "]").c_str(), "%s", oss.str().c_str());
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mcdev_aurora_NativeCode_surfaceCreated(
        JNIEnv* env,
        jobject /* this */,
        jobject surface,
        jint width,
        jint height,
        jint format) {
    auto& sfc = surface;

    Log::LOG(ANDROID_LOG_INFO,"SurfaceView", "SURFACE VIEW INFO: width: ", width, " height: ", height);


    ANativeWindow* window = ANativeWindow_fromSurface(env, sfc);

    if (ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888) < 0) {
        ANativeWindow_release(window);
        return;
    }

    ANativeWindow_Buffer buffer;

    if (ANativeWindow_lock(window, &buffer, nullptr) < 0) {
        ANativeWindow_release(window);
        return;
    }



    uint32_t* pixels = static_cast<uint32_t*>(buffer.bits);
    int stride = buffer.stride;
    auto BLUE_bgra = 0xFFFFFFFF;

    Log::LOG(ANDROID_LOG_INFO, "SurfaceView", "Stride size: ", buffer.stride, "size in bytes: ", buffer.stride * buffer.height * 4);

    for (int h = 0; h < buffer.height; h++) {
        for(int w = 0; w < buffer.width; w++) {
            pixels[h * stride + w] = BLUE_bgra;
        }
    }

    int squareSize = 250;

    int centerX, centerY;

    centerX = (buffer.width / 2);
    centerY = (buffer.height / 2);

    int startX = centerX - (squareSize / 2);
    int startY = centerY - (squareSize / 2);

    int lastX = centerX + (squareSize / 2);
    int lastY = centerY + (squareSize / 2);

    Log::LOG(ANDROID_LOG_INFO, "SurfaceView", "starts in x, y size: ", startX, startY);


    for (int y = startY; y < lastY; y++) {
        for (int x = startX; x < lastX; x++) {
            pixels[y * stride + x] = 0x000000FF;
        }
    }


    /** buffer copy 4x3
     *
     */
     Buffer buff{
         .x = 4,
         .y = 3
     };

     int area = buff.size();

    buff.pixels = {
            /**
             * 3 x 4
             */
            0xFF000000, 0xFF000000, 0xFF000000,
            0xFF000000,0xFF000000, 0xFF000000,
            0xFF000000, /* pixel (1, 2) */ 0xFFFFFFFF,0xFF000000,
            0xFF000000, 0xFF000000, 0xFF000000,
    };

    /**
     * acessar pixel(1, 2) formula = target_y * width if without stride + target_x
     */
     auto index_px_position = 2 * 3 + 1;


    ANativeWindow_unlockAndPost(window);

    ANativeWindow_release(window);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_mcdev_aurora_NativeCode_surfaceDestroyed(
        JNIEnv* env,
        jobject /* this */) {
}
