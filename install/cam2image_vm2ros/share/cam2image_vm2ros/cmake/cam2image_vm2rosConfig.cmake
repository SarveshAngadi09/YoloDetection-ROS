# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_cam2image_vm2ros_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED cam2image_vm2ros_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(cam2image_vm2ros_FOUND FALSE)
  elseif(NOT cam2image_vm2ros_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(cam2image_vm2ros_FOUND FALSE)
  endif()
  return()
endif()
set(_cam2image_vm2ros_CONFIG_INCLUDED TRUE)

# output package information
if(NOT cam2image_vm2ros_FIND_QUIETLY)
  message(STATUS "Found cam2image_vm2ros: 0.21.0 (${cam2image_vm2ros_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'cam2image_vm2ros' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT cam2image_vm2ros_DEPRECATED_QUIET)
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(cam2image_vm2ros_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "")
foreach(_extra ${_extras})
  include("${cam2image_vm2ros_DIR}/${_extra}")
endforeach()
