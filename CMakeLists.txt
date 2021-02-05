cmake_minimum_required(VERSION 3.1)

#set project name and language
project(vf-veinmanager LANGUAGES CXX)


#Automoc and RCC ON. Needed in QT projects
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#C++ 11 Flag# spawn out some info on configuration
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Set project Version
set(VfVeinmanager_VERSION_MAJOR "0")
set(VfVeinmanager_VERSION_MINOR "0")
set(VfVeinmanager_VERSION_PATCH "1")

set(PROJECT_VERSION "${VfVeinmanager_VERSION_MAJOR}.${VfVeinmanager_VERSION_MINOR}.${VfVeinmanager_VERSION_PATCH}")


#Adding cmake functions like feature_summary()
include(FeatureSummary)
include(GNUInstallDirs)

#Find dependecies
find_package(Qt5 COMPONENTS Core Gui CONFIG REQUIRED)
find_package(VfComponent REQUIRED)
find_package(VfHelpers REQUIRED)
find_package(VfEvent REQUIRED)


#sum up project Files 
file(GLOB SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    )

file(GLOB HEADER
    ${PROJECT_SOURCE_DIR}/src/*.h
    )

file(GLOB RESOURCES 
    ${PROJECT_SOURCE_DIR}/*.qrc
    )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
#create library 
add_library(VfVeinmanager SHARED
    ${SOURCES}
    ${HEADER}
    ${RESOURCES}
    )




#link dependencies
target_link_libraries(VfVeinmanager 
    PRIVATE
    Qt5::Core
    Qt5::Gui
    VeinMeta::VfHelpers
    VeinMeta::VfEvent
    VeinMeta::VfComponent
    )


#set target Version
set_target_properties(VfVeinmanager PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(VfVeinmanager PROPERTIES SOVERSION ${VfVeinmanager_VERSION_MAJOR})

#generate export header
include(GenerateExportHeader)
generate_export_header(VfVeinmanager)


#install export header
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vfveinmanager_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VfVeinmanager
    )

# announce headers - target perspective
target_include_directories(VfVeinmanager
    PUBLIC
    $<INSTALL_INTERFACE:include/VfVeinmanager>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

# install library
install(TARGETS VfVeinmanager
    EXPORT VfVeinmanagerExport
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

# configure *.cmake File
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    VfVeinmanagerConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMinorVersion
    )

# configure *.cmake File 
configure_file(VfVeinmanagerConfig.cmake.in VfVeinmanagerConfig.cmake @ONLY)


# install *(Version).cmake
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/VfVeinmanagerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VfVeinmanagerConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VfVeinmanager
    )

# install targets cmake-files
install(EXPORT VfVeinmanagerExport
    FILE VfVeinmanagerTargets.cmake
    NAMESPACE VeinMeta::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VfVeinmanager
    )

# install public headers
install(
    FILES ${HEADER}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VfVeinmanager
    )

# spawn out some info on configuration
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)


