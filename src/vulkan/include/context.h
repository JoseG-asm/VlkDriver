
#pragma once

#include "xcb/xcb.h"

/**
 * vulkan api headers
 */
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xcb.h>
#include <vulkan/vk_icd.h>
#include <vulkan/vulkan.h>

/**
 * std headers
 */
#include <unordered_map>
#include <functional>
#include <bit>

/**
 * library headers
 */
#include <dlfcn.h>

/**
 * provide use for ahb buffers
 */
#include <android/hardware_buffer.h>

/**
 * definitions and macros
 */
#define VULKAN_PATH "/system/lib64/libvulkan.so"

/**
 * project headers
 */
#include "log.h"
#include "instance.h"
#include "physical_device.h"
#include "device.h"
#include "surface.h"

namespace VulkanContext {

    /**
     * This Callbacks is used to open and close the lib
     */
    struct Cb {
        /**
         * Handle is used to store a pointer of the library that will be opened
         */
        struct Handle {
            void *so{nullptr};
        };

        std::function<void(void *)> close;
        std::function<bool(const std::string &)> open;
    };

    /**
     * every single class bellow this represents a Object in vulkan api
     */

    /**
     * base class
     */
    template<typename VulkanApiObject>
    class BaseObj {
    public:
        using OBJ_TYPE = VulkanApiObject;


        void* CreateDispObjHandle() {
            auto data = new VK_LOADER_DATA;
            set_loader_magic_value(data);

            return data;
        }

        static void DestroyDispObjHandle(void *handle) {
            delete reinterpret_cast<VK_LOADER_DATA *>(handle);
        }

    protected:
        explicit BaseObj() {
            obj = static_cast<OBJ_TYPE>(CreateDispObjHandle());
        }

        ~BaseObj() {
            DestroyDispObjHandle((void *) obj);
        }

        OBJ_TYPE obj;
    };

    /**
      * VkPhysicalDevice Obj impl
      */
    struct VkPhysicalDeviceObject : public BaseObj<VkPhysicalDevice> {
        VkPhysicalDeviceObject() : BaseObj<VkPhysicalDevice>() {}

        ~VkPhysicalDeviceObject() = default;

        /**
         * store the alloc obj
         */
        OBJ_TYPE dispatch_handle{obj};
    };


    /**
     * starts with VkInstance Obj impl
     */
    struct VkInstanceObject : public BaseObj<VkInstance> {
        VkInstanceObject() : BaseObj<VkInstance>() {}

        ~VkInstanceObject() = default;

        /**
         * instance magic
         */
        static uint64_t instance_magic;

        /**
         * store the alloc obj
         */
        OBJ_TYPE dispatch_handle{obj};

        /**
         * enabled extensions ->
         */
        std::vector<VkExtensionProperties> enabledProps;

    };


    /**
     * VkDevice Obj impl
     */
    struct VkDeviceObject : public BaseObj<VkDevice> {
        VkDeviceObject() : BaseObj<VkDevice>() {}

        ~VkDeviceObject() = default;

        /**
         * device magic
         */
        static uint64_t device_magic;

        /**
         * store the alloc obj
         */
        OBJ_TYPE dispatch_handle{obj};

        /**
         * sub device objects
         */

        /**
         * Queues
         */
        class VkQueueObject {
        public:
            VkQueue dispatch_handle{nullptr};

            VkDevice deviceObject;
        };

        struct QueueKey {
            uint32_t family;
            uint32_t index;

            bool operator==(const QueueKey &other) const {
                return family == other.family && index == other.index;
            }

            struct Hash {
                std::size_t operator()(const QueueKey &k) const {
                    return std::hash<uint32_t>()(k.family) ^ (std::hash<uint32_t>()(k.index) << 1);
                }
            };
        };

        std::unordered_map<QueueKey, VkQueueObject, QueueKey::Hash> queues;

        std::unordered_map<VkQueue, VkQueueObject> queue_by_handles;

        /**
         * Memory Allocation
         */
        class VkDeviceMemoryObject {
        public:
            /**
             * android stuff
             */
            AHardwareBuffer *buffer{};

            /**
             * size of allocation for example 1024 bytes for allocation on host
             */
            size_t size{};

            /**
             * type of memory for example HOST_VISIBLE etc
             */
            uint32_t memoryTypeIndex{};

            /**
             * if this memory is mapped for example
             * VkDeviceMemory of size 1024
             */
            bool is_mapped = false;

            void *mapped_ptr{};
        };

        std::unordered_map<VkDeviceMemory, std::shared_ptr<VkDeviceMemoryObject>> memory_map;
        uint64_t memory_handler_id{1};

        /**
         * device buffers
         */
        class VkBufferObject {
        public:
            VkDeviceSize size = 0;
            VkBufferUsageFlags usage = 0;
            VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            /*
             * host visible
             */
            void* mapped_ptr;

            VulkanContext::VkDeviceObject::VkDeviceMemoryObject* bound_memory = nullptr;
            VkDeviceSize bound_offset = 0;
        };

        std::unordered_map<VkBuffer, std::unique_ptr<VkBufferObject>> buffers;
        uint64_t buffer_handler_id{1};

        /*
         * buffer views
         */
        class VkBufferViewObject {
        public:
            VkBuffer handle;

            /**
             * details
             */
            VkFormat format;
            VkDeviceSize offset;
            VkDeviceSize range_buffer;
        };

        std::unordered_map<VkBufferView, VkBufferViewObject> buffer_views;
        uint64_t buffer_views_handler_id{1};

        /**
         * device images
         */
        class VkImageObject {
        public:
            VkExtent3D extent;
            VkFormat format;
            VkImageUsageFlags usage;
            uint32_t mipLevels;
            uint32_t arrayLayers;

            /**
             * full info
             */
            VkImageCreateInfo info;

            /**
             * backend of memory
             */
            bool is_binded = false;

            /** real handle */
            VkImage dispatch_handle{VK_NULL_HANDLE};

            VulkanContext::VkDeviceObject::VkDeviceMemoryObject* bound_memory = VK_NULL_HANDLE;
            VkDeviceSize memory_offset = 0;
            void* mapped_ptr = nullptr;
        };

        std::unordered_map<VkImage, std::unique_ptr<VkImageObject>> chain_images;
        std::unordered_map<VkImage, VkImageView> chain_image_views;
        uint64_t image_handler_id{1};

        /**
         * device fences
         */
        class VkFenceObject {
        public:
            bool signaled;
            VkFence dispatch_handle;
        };

        std::unordered_map<VkFence, std::unique_ptr<VkFenceObject>> fences;
        uint64_t fence_handler_id{1};

        /**
         * device semaphores
         */
        struct VkSemaphoreObject {
            bool signaled = false;
            VkSemaphore dispatch_handle;
        };

        std::unordered_map<VkSemaphore, std::unique_ptr<VkSemaphoreObject>> semaphores;
        uint64_t semaphore_handler_id{1};

        /**
         * device events
         */
        struct VkEventObject {
            std::atomic<VkBool32> signaled;

            VkEventObject() : signaled(VK_FALSE) {}
        };

        std::unordered_map<VkEvent, std::unique_ptr<VkEventObject>> events;
        uint64_t event_handler_id{1};


        /**
         * query pools
         */
        class VkQueryPoolObject {
        public:

            /**
             * type of query
             */
            VkQueryType type;

            /**
             * count of queries
             */
            uint32_t count;

            /**
             * stats
             */
            VkQueryPipelineStatisticFlags pipelineStats;

            /**
             * results
             */
            struct VkQueryResultObject {
                uint64_t value;

                bool available;
            };

            std::vector<VkQueryResultObject> results;
        };

        std::unordered_map<VkQueryPool, std::unique_ptr<VkQueryPoolObject>> queries;
        uint64_t queries_handler_id{1};


        struct VkDeviceSubmitInfoObject {

            // objects per submit of application
            std::vector<VkSemaphore> wait_semaphores;
            std::vector<VkSemaphore> signal_semaphores;
            std::vector<VkCommandBuffer> cmd_buffers;
            std::vector<VkPipelineStageFlags> wait_dst_stage_mask;


            // the submit driver version of the app side
            // the submit driver version of the app side
            VkSubmitInfo submit_info;
        };
    };

    /**
    * VkSurfaceKHR Obj impl
    */
    template<typename SurfaceType>
    struct VkSurfaceObject {
        VkSurfaceObject() {}

        ~VkSurfaceObject() {
            delete surface;
        };

        /**
         * store the alloc obj
         */
        VkSurfaceKHR dispatch_handle = VK_NULL_HANDLE;

        SurfaceType *surface = new SurfaceType();

        bool make_surface(std::function<bool(SurfaceType *)> allocSurface) {
            return allocSurface(surface);
        };

    };


    class ContextBase {
    public:
        ContextBase() = default;

        ~ContextBase() = default;

        /**
         * entrypoints for global functions vulkan api
         */
        PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
        PFN_vkCreateInstance CreateInstance;
        PFN_vkEnumerateInstanceExtensionProperties EnumerateInstanceExtensionProperties;
        PFN_vkEnumerateInstanceLayerProperties EnumerateInstanceLayerProperties;
        PFN_vkEnumerateInstanceVersion EnumerateInstanceVersion;

        std::unordered_map<std::string, PFN_vkVoidFunction> instance_hooks;
        std::unordered_map<std::string, PFN_vkVoidFunction> device_hooks;


        virtual bool initialize() = 0;

        /**
         * This function is only utilized for get the pointer of vulkan functions
         */
        template<typename T>
        T GetFuncAddrFromHandle(Cb::Handle &handle, const char *name) {
            if (!handle.so) {
                Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::CONTEXT,
                             "GetFuncAddrFromHandle has received a invalid handle");
                throw std::runtime_error(dlerror());
            }

            const auto func = reinterpret_cast<T>(dlsym(handle.so, name));

            func ? Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                                "GetFuncAddrFromHandle get vulkan global func: ", name)
                 : Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::CONTEXT,
                                "GetFuncAddrFromHandle cannot get vulkan global func: ", name);

            return func;
        }


        /**
         * objects typedef to difer it to a common vector of objects
         */
        template<typename K, typename V>
        using VulkanApiObjects = std::unordered_map<K, V>;

        /**
         * All objects that will be created in the driver lifecycle
         */
        VulkanApiObjects<uint64_t, std::unique_ptr<VkInstanceObject>> instances;
        VulkanApiObjects<uint64_t, std::unique_ptr<VkPhysicalDeviceObject>> physicalDevices;
        VulkanApiObjects<uint64_t, std::unique_ptr<VkDeviceObject>> devices;

        VulkanApiObjects<uint64_t, std::unique_ptr<VkSurfaceObject<VkIcdSurfaceXcb>>> surfaces;
        x11::Surface m_surface_object_creator;
        uint64_t m_surface_handler_id{1};


        /**
         * instance dispatcher
         */
        std::unique_ptr<VulkanDispatcher::Instance::InstanceDispatchTable> instance_disp;

        /**
         * physical device dispatcher
         */
        std::unique_ptr<VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable> physical_device_disp;

        /**
         * device dispatcher
         */
        std::unique_ptr<VulkanDispatcher::Device::DeviceDispatchTable> device_disp;
    };

    class Context : public ContextBase {
    public:
        Context() : ContextBase() {};

        bool initialize() override;

        bool ready = false;
    };
}