#!/bin/bash

# تحديد الألوان للمخرجات
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}>>> بدء عملية بناء مشروع نقرة...${NC}"

# إنشاء مجلد البناء إذا لم يكن موجوداً
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
    echo -e "${GREEN}>>> تم إنشاء مجلد ${BUILD_DIR}${NC}"
fi

# الدخول للمجلد وتشغيل CMake
cd "$BUILD_DIR" || exit
echo -e "${BLUE}>>> جاري تكوين ملفات المشروع عبر CMake...${NC}"
cmake ..

# التحقق من نجاح CMake
if [ $? -eq 0 ]; then
    echo -e "${GREEN}>>> تم التكوين بنجاح. جاري التجميع (Compilation)...${NC}"
    # البناء باستخدام كافة الأنوية المتاحة
    make -j$(nproc)
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}>>> تم البناء بنجاح!${NC}"
        echo -e "${BLUE}>>> لتشغيل البرنامج:${NC} ./${BUILD_DIR}/Naqrah"
    else
        echo -e "${RED}>>> حدث خطأ أثناء التجميع (make).${NC}"
    fi
else
    echo -e "${RED}>>> حدث خطأ أثناء تكوين CMake.${NC}"
fi