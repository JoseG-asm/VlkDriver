#pragma once


/**
 * vulkan core includes
 */
#include <vulkan/vulkan.h>

/**
 * utils
 */
#include <unordered_map>
#include <string>

/**
 * forward declaration
 */
namespace VulkanContext {
    class Context;
}

namespace VulkanDispatcher::Instance {
    template<typename K, typename V>
    using DispatchMap_T = std::unordered_map<K, V>;

    class InstanceDispatchTable {
    public:
        explicit InstanceDispatchTable(VulkanContext::Context* context);

        bool factory();


        /**
         * These trampolines are the pattern of vulkan expects in his functions of the driver dispatcher
         */

        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_CreateInstance(
                const VkInstanceCreateInfo *pCreateInfo,
                const VkAllocationCallbacks *pAllocator,
                VkInstance *pInstance);

        static VKAPI_ATTR void VKAPI_CALL
        vlk_trampoline_call_DestroyInstance(
                VkInstance instance,
                const VkAllocationCallbacks *pAllocator);


        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_EnumerateInstanceVersion(
                uint32_t *pApiVersion);

        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_EnumerateInstanceExtensionProperties(
                const char *pLayerName,
                uint32_t *pPropertyCount,
                VkExtensionProperties *pProperties
        );

        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_EnumerateInstanceLayerProperties(
                uint32_t *pPropertyCount,
                VkLayerProperties *pProperties);

        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
        vlk_trampoline_call_GetInstanceProcAddr(
                VkInstance instance,
                const char *pName);


        // xcb surface support for surface creation
        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_CreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);

    private:
        /**
         * temporary objects in which contains the main objects will be used in this class
         */
        static VulkanContext::Context* vk_context;

        /**
         * hash map of instance functions
         */
        static DispatchMap_T<std::string, PFN_vkVoidFunction> instance_disp_table;

        /**
         *
         * @tparam T type of function that will be cast
         * @param func the reference of function
         * @return generic pointer for function
         */
        template<typename T>
        inline PFN_vkVoidFunction to(T func) {
            return reinterpret_cast<PFN_vkVoidFunction>(func);
        }

        /**
         *
         * @tparam T type of pointer receiver function
         * @param pName name of function that will be registered
         * @param func the lvalue for a function
         * @return true for sucessfully registration otherwise false
         */
        template<typename T>
        bool registerTrampoline(const std::string& pName, T func) {
            auto state = false;

            instance_disp_table[pName] = to(func);

            const auto &iterator = instance_disp_table.find(pName);

            if (iterator != instance_disp_table.end()) {
                state = true;
            }

            return state;
        }
    };
}