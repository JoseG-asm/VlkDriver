//
// Created by Joseghds on 5/30/2025.
//
#pragma once

#include <iostream>

#include "android/log.h"
#include "android/native_window.h"
#include "android/native_window_jni.h"

namespace vulkron {
    namespace vulkan {
        namespace android {
            struct VulkanAndroidSurface {
                ~VulkanAndroidSurface() {
                    ANativeWindow_release(window);
                };

                bool create(JNIEnv* env, jobject* surface, int width, int height, int format);

            private:
                ANativeWindow* window;
            };

            class VulkanAndroid {
            public:

                VulkanAndroidSurface* surface = (VulkanAndroidSurface*) calloc(1, sizeof(VulkanAndroidSurface));

                ~VulkanAndroid() {
                    free(surface);
                }
            };
        }
    }
}

