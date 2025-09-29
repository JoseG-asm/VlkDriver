#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace Log {

    enum Level {
        INFO,
        ERROR
    };

    enum LevelType {
        CONTEXT,
        INSTANCE,
        INSTANCE_DISPATCH_TABLE,
        PHYSICAL_DEVICE,
        DEVICE
    };

    template<typename Level, typename LevelType, typename... Args>
    void VLK_LOG(Level level, LevelType type, Args &&...args) {
        std::ostringstream oss;
        std::string typeStr;

        switch (type) {
            case LevelType::CONTEXT:
                typeStr = "Context";
                break;
            case LevelType::INSTANCE_DISPATCH_TABLE:
                typeStr = "InstanceDispatchTable";
                break;
            case LevelType::INSTANCE:
                typeStr = "Instance";
                break;
            case LevelType::PHYSICAL_DEVICE:
                typeStr = "PhysicalDevice";
                break;
            case LevelType::DEVICE:
                typeStr = "Device";
                break;
        }

        oss << std::string("[" + typeStr + "]") << " ";
        ((oss << std::forward<Args>(args)), ...);

        printf("%s \n", oss.str().c_str());

        switch (level) {
            case Level::ERROR:
                throw std::runtime_error(oss.str());
                break;
        }
    }
}