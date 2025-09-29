# Vlk Driver

the driver is in early stage of development the goal is to achieve the vulkan 1.1 specification in which most android devices are obviously supporting the following xcb/xlib and wayland protocol support display platforms

# Structure

- the driver will expose the vulkan 1.1 for now because of the complexity of the functions 1.1+ that took longer to be implemented, this is enough to cover some versions of dxvk of course that depending on the driver of the device may be missing some things but I can implement them later as well

- icd_interface.cpp contains the logical structure to communicate to the driver and loader vulkan
- context.cpp contains the main logical to create a simple vulkan context and interface to provide support for multiples drivers and load the mainly dispatch tables in which are instance, physical device and device functions
- instance.cpp contains all logical to create a driver instance and intercept some functions we need
- physical_device.cpp contains the logical to create a driver physical device e also intercept and implement some functions
- device.cpp most complicated part implement de logical driver device creation and implement his functions

# Important 

in couple weeks we have a functional driver with all features i want and maybe a 1.3 vulkan spec for some devices and better than wrapper because i use the most power of android i can with no zero copy of image top the present in xcb buffer

# Note

device functions are returned from the 'vlk_trampoline_call_GetDeviceProcAddr'
hook if they are not implemented the driver will try to recover them from host
pointers but as the driver structure is different from the internal structure
where we use real and non-opaque handles end up getting SIGBUS when some function
is not implemented for now I am implementing until it reaches vulkan spec 1.1 so
if more functions are required I will have to implement them later so if you get
SIGBUS please opens a request to implement the corresponding x function, so that's all!!

# Thanks to

- provide the logical device <https://github.com/xMeM/mesa/tree/wrapper>
- general idea and wsi engine zero copy <https://github.com/newDINO/xvk_droid>