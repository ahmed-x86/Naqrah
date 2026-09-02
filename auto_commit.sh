#!/bin/bash

# 1. تحديث .gitignore لتجنب رفع الملف التنفيذي
if ! grep -q "^naqrah$" .gitignore; then
    echo "naqrah" >> .gitignore
    git add .gitignore
    git commit -m "chore: update .gitignore to exclude linux binary"
fi

# 2. قراءة كل ملف يحتوي على تغيير
git status --porcelain | while read -r line; do
    # استخراج اسم الملف
    file=$(echo "$line" | cut -c 4-)
    status=$(echo "$line" | cut -c 1-2)

    # تجاهل الملف التنفيذي إذا ظهر
    if [[ "$file" == "naqrah" ]]; then
        continue
    fi

    # إضافة الملف
    git add "$file"

    # تحديد رسالة الكوميت حسب الحالة (حذف، إضافة، أو تعديل)
    if [[ "$status" == *"D"* ]]; then
        git commit -m "refactor: remove legacy Qt/C++ file -> $file"
    elif [[ "$status" == "??"* ]]; then
        git commit -m "feat: add new Rust/Tauri file -> $file"
    else
        git commit -m "chore: update and configure -> $file"
    fi
done

# 3. رفع جميع الكوميتس مرة واحدة
echo "🚀 جاري رفع الكوميتس إلى المستودع..."
git push origin main

# 4. إنشاء التاج v1.6 ورفعه
echo "🏷️ جاري إنشاء التاج v1.6..."
git tag v1.6
git push origin v1.6

echo "✅ تمت العملية بنجاح! مبروك عليك المربعات الخضراء 🟩🟩🟩"
