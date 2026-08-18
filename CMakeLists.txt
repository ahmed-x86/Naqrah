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

# في Qt6 نستخدم qt_add_executable دائماً لجميع المنصات.
# على أندرويد، Qt ستقوم تلقائياً بتحويله إلى Shared Library وربطه بـ Gradle
qt_add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(WIN32)
    # إخفاء نافذة الـ console في ويندوز
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(ANDROID)
    # تحديد مسار ملفات الأندرويد المخصصة (مثل AndroidManifest.xml)
    set_property(TARGET ${PROJECT_NAME} PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()
