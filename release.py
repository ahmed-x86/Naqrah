#!/usr/bin/env python3
"""
سكربت بسيط بديل عن كتابة رقم التاج يدويًا كل مرة.

اللي بيعمله بالظبط (بديل عن أوامرك اليدوية):
  git add .
  git commit -m "..."
  git push
  git tag vX.Y   <- بيحسب الرقم التالي لوحده من آخر تاج موجود
  git push --tags

الاستخدام:
  python3 release.py                  # commit برسالة افتراضية "."
  python3 release.py "رسالة الكوميت"   # commit برسالة مخصصة
"""
import re
import subprocess
import sys


def run(cmd, check=True):
    print(f"$ {' '.join(cmd)}")
    result = subprocess.run(cmd, text=True, capture_output=True)
    if result.stdout.strip():
        print(result.stdout.strip())
    if check and result.returncode != 0:
        print(result.stderr.strip())
        sys.exit(result.returncode)
    return result.stdout.strip()


def get_all_version_tags():
    """يرجع كل التاجات اللي شكلها vX.Y كـ (major, minor, النص الأصلي)."""
    raw = run(["git", "tag", "-l", "v*"], check=False)
    tags = []
    for line in raw.splitlines():
        line = line.strip()
        m = re.fullmatch(r"v(\d+)\.(\d+)", line)
        if m:
            tags.append((int(m.group(1)), int(m.group(2)), line))
    return tags


def next_tag():
    tags = get_all_version_tags()
    if not tags:
        return "v1.0"
    tags.sort()
    major, minor, _ = tags[-1]
    return f"v{major}.{minor + 1}"


def main():
    commit_msg = sys.argv[1] if len(sys.argv) > 1 else "."

    # لو فيه تعديلات جديدة، اعملها add + commit. لو مفيش، كمل عادي.
    status = run(["git", "status", "--porcelain"], check=False)
    if status:
        run(["git", "add", "."])
        run(["git", "commit", "-m", commit_msg])
    else:
        print("مفيش تعديلات جديدة على الملفات، هنكمل من آخر commit موجود.")

    run(["git", "push"])

    new_tag = next_tag()
    print(f"\n➡️  التاج الجديد: {new_tag}\n")

    run(["git", "tag", new_tag])
    run(["git", "push", "--tags"])

    print(f"\n✅ تم! push و tag {new_tag} خلصوا. البناء بدأ على GitHub Actions.")


if __name__ == "__main__":
    main()
