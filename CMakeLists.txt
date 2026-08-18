cmake_minimum_required(VERSION 3.16)
project(Naqrah VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(SOURCES
    main.cpp
    theme.cpp
    theme.h
)

if(ANDROID)
    # على أندرويد، Qt محتاج تبني المشروع كمكتبة مشتركة مش تنفيذي عادي،
    # وبعدين androiddeployqt هو اللي بيغلفها جوا APK.
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(WIN32)
    # يشيل نافذة الـconsole السودة اللي بتظهر خلف الواجهة في ريليز ويندوز
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(ANDROID)
    # مطلوب علشان androiddeployqt يعرف يلاقي مكتبات Qt الصح لكل ABI
    set_property(TARGET ${PROJECT_NAME} PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()
