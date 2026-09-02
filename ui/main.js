const { invoke } = window.__TAURI__.core;
const inputText = document.getElementById('inputText');
const btnStartTashkeel = document.getElementById('btnStartTashkeel');
const lblCurrentWord = document.getElementById('currentWordLabel');
const btnNextChar = document.getElementById('btnNextChar');
const btnCopy = document.getElementById('btnCopy');
const outputText = document.getElementById('outputText');
const markButtons = document.querySelectorAll('.btnMark');

btnStartTashkeel.addEventListener('click', async () => {
    const text = inputText.value.trim();
    if (!text) return;
    const state = await invoke('start_tashkeel', { text });
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

// هنا تم التعديل: إزالة الإشعار المزعج وتغيير نص الزر مؤقتاً
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