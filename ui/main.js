const { invoke } = window.__TAURI__.core;
const { load } = window.__TAURI__.store;

const inputText = document.getElementById('inputText');
const btnStartTashkeel = document.getElementById('btnStartTashkeel');
const lblCurrentWord = document.getElementById('currentWordLabel');
const btnNextChar = document.getElementById('btnNextChar');
const btnCopy = document.getElementById('btnCopy');
const outputText = document.getElementById('outputText');
const markButtons = document.querySelectorAll('.btnMark');

// ===== عناصر التنقل =====
const navItems = document.querySelectorAll('.nav-item');
const pages = document.querySelectorAll('.page');

// ===== عناصر الإعدادات =====
const btnThemeLight = document.getElementById('btnThemeLight');
const btnThemeDark = document.getElementById('btnThemeDark');
const toggleStripDiacritics = document.getElementById('toggleStripDiacritics');
const toggleKeepDiacritics = document.getElementById('toggleKeepDiacritics');
const toggleSkipShadda = document.getElementById('toggleSkipShadda');
const toggleMarkShadda = document.getElementById('toggleMarkShadda');
const stripDiacriticsSubOptions = document.getElementById('stripDiacriticsSubOptions');

// ===== إعداد Store =====
let settingsStore = null;

async function initStore() {
    // ملف الإعدادات سيحفظ تلقائياً في مسار التطبيق في النظام
    settingsStore = await load('settings.json', { autoSave: true });
    await loadSettings();
}

// ===== تحميل الإعدادات من Store =====
async function loadSettings() {
    // الثيم
    const theme = (await settingsStore.get('naqrah-theme')) ?? 'dark';
    applyTheme(theme);

    // خيارات التشكيل — الافتراضي: أخذه بدون تشكيل
    const savedMode = await settingsStore.get('naqrah-diacritics-mode');
    const diacriticsMode = (savedMode === 'strip' || savedMode === 'keep') ? savedMode : 'strip';
    applyDiacriticsMode(diacriticsMode);

    // خيارات الشدة — الافتراضي: تجاوز الشدة
    const savedShadda = await settingsStore.get('naqrah-shadda-mode');
    const shaddaMode = (savedShadda === 'skip' || savedShadda === 'mark') ? savedShadda : 'skip';
    applyShaddaMode(shaddaMode);
}

// ===== الثيم =====
async function applyTheme(theme) {
    if (theme === 'light') {
        document.body.classList.add('light');
        btnThemeLight.classList.add('active');
        btnThemeDark.classList.remove('active');
    } else {
        document.body.classList.remove('light');
        btnThemeDark.classList.add('active');
        btnThemeLight.classList.remove('active');
    }
    if (settingsStore) await settingsStore.set('naqrah-theme', theme);
}

btnThemeLight.addEventListener('click', () => applyTheme('light'));
btnThemeDark.addEventListener('click', () => applyTheme('dark'));

// ===== التنقل بين الصفحات =====
navItems.forEach(item => {
    item.addEventListener('click', () => {
        const targetPage = item.getAttribute('data-page');

        // تحديث الأزرار النشطة
        navItems.forEach(n => n.classList.remove('active'));
        item.classList.add('active');

        // تبديل الصفحات
        pages.forEach(p => p.classList.remove('active'));
        document.getElementById(targetPage).classList.add('active');
    });
});

// ===== خيارات التشكيل (Radio: دائماً واحد مفعّل) =====
async function applyDiacriticsMode(mode) {
    if (mode === 'strip') {
        toggleStripDiacritics.checked = true;
        toggleKeepDiacritics.checked = false;
        stripDiacriticsSubOptions.classList.add('visible');
    } else {
        toggleStripDiacritics.checked = false;
        toggleKeepDiacritics.checked = true;
        stripDiacriticsSubOptions.classList.remove('visible');
    }
    if (settingsStore) await settingsStore.set('naqrah-diacritics-mode', mode);
}

toggleStripDiacritics.addEventListener('change', () => applyDiacriticsMode('strip'));
toggleKeepDiacritics.addEventListener('change', () => applyDiacriticsMode('keep'));

// ===== خيارات الشدة (Radio: دائماً واحد مفعّل) =====
async function applyShaddaMode(mode) {
    if (mode === 'skip') {
        toggleSkipShadda.checked = true;
        toggleMarkShadda.checked = false;
    } else {
        toggleSkipShadda.checked = false;
        toggleMarkShadda.checked = true;
    }
    if (settingsStore) await settingsStore.set('naqrah-shadda-mode', mode);
}

toggleSkipShadda.addEventListener('change', () => applyShaddaMode('skip'));
toggleMarkShadda.addEventListener('change', () => applyShaddaMode('mark'));

// ===== منطق التشكيل الأصلي =====
btnStartTashkeel.addEventListener('click', async () => {
    let text = inputText.value.trim();
    if (!text) return;

    const isKeepMode = toggleKeepDiacritics.checked;
    const skipShadda = toggleSkipShadda.checked;
    const markShadda = toggleMarkShadda.checked;

    const state = await invoke('start_tashkeel', {
        text,
        keepDiacritics: isKeepMode,
        skipShadda,
        markShadda
    });
    renderState(state);
});

btnNextChar.addEventListener('click', async () => {
    const state = await invoke('advance_char');
    renderState(state);
});

markButtons.forEach(btn => {
    btn.addEventListener('click', async () => {
        const mark = btn.getAttribute('data-mark');
        const state = await invoke('apply_mark', { mark });
        renderState(state);
    });
});

// إزالة الإشعار المزعج وتغيير نص الزر مؤقتاً
btnCopy.addEventListener('click', async () => {
    const text = outputText.value;
    if (!text) return; // لا تفعل شيئاً إذا كان المربع فارغاً

    try {
        await navigator.clipboard.writeText(text);
        
        // حفظ النص الأصلي وتغييره
        const originalText = btnCopy.textContent;
        btnCopy.textContent = 'تم النسخ ✔';
        btnCopy.style.borderColor = 'var(--primary)'; // حركة جمالية بسيطة
        btnCopy.style.color = 'var(--primary)';

        // إعادته بعد ثانيتين
        setTimeout(() => {
            btnCopy.textContent = originalText;
            btnCopy.style.borderColor = 'var(--border)';
            btnCopy.style.color = 'var(--text)';
        }, 2000);

    } catch (err) {
        console.error('Failed to copy text: ', err);
    }
});

function renderState(state) {
    outputText.value = state.processed_text;
    if (state.is_finished) {
        lblCurrentWord.innerHTML = 'انتهى النص!';
        btnNextChar.textContent = 'إنهاء';
        return;
    }
    const currentWord = state.current_word;
    const chars = Array.from(currentWord);
    const charIdx = state.current_char_idx;
    if (charIdx >= chars.length) return;
    let html = '';
    for (let i = 0; i < charIdx; i++) {
        html += chars[i];
    }
    html += `<span class="highlight-char">${chars[charIdx]}</span>`;
    if (state.waiting_after_shadda) {
        html += 'ّ';
    }
    for (let i = charIdx + 1; i < chars.length; i++) {
        html += chars[i];
    }
    lblCurrentWord.innerHTML = html;
    if (charIdx === chars.length - 1) {
        btnNextChar.textContent = 'الكلمة التالية';
    } else {
        btnNextChar.textContent = 'الحرف التالي';
    }
}

// ===== بدء التشغيل وتهيئة Store =====
initStore();