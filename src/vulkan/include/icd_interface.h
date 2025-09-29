#pragma once

#include "context.h"

/**
  * version of loader icd used by this driver
  */
static const uint32_t SUPPORTED_LOADER_ICD_INTERFACE_VERSION = 5;

/**
 * version negotiated by the Vulkan Loader and store call of function
 */
static uint32_t loader_interface_version = 0;
static bool negotiate_loader_icd_interface_called = false;