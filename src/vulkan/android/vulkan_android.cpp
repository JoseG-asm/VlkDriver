//
// Created by Joseghds on 5/30/2025.
//

#include "vulkan_android.h"

bool vulkron::vulkan::android::VulkanAndroidSurface::create(JNIEnv* env, jobject* surface, int width, int height, int format) {
    ANativeWindow* _window = nullptr;
    if (surface != nullptr) {
        window = ANativeWindow_fromSurface(env, *surface);
    }

    if (window != nullptr) {
       __android_log_print(ANDROID_LOG_DEBUG, "VulkanAndroidSurface", "Successfully created VulkanAndroidSurface");
       window = _window;
       return true;
    }

    return false;
}
