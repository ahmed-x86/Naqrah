use std::sync::Mutex;
use tauri::State;

#[derive(Default)]
struct AppState {
    /// الكلمات النظيفة (حروف أساسية فقط بدون تشكيل)
    words: Vec<String>,
    /// التشكيل الموجود مسبقاً لكل حرف: pre_diacritics[word][char] = "َ" أو ""
    pre_diacritics: Vec<Vec<String>>,
    /// هل وضع "أخذه بتشكيل" مفعّل؟
    keep_diacritics: bool,
    processed_words: Vec<String>,
    current_word_idx: usize,
    current_char_idx: usize,
    current_word_processed: String,
    is_waiting_for_mark_after_shadda: bool,
}

#[derive(serde::Serialize, Clone)]
struct RenderState {
    current_word: String,
    current_char_idx: usize,
    waiting_after_shadda: bool,
    processed_text: String,
    is_finished: bool,
}

fn build_render_state(state: &AppState) -> RenderState {
    RenderState {
        current_word: if state.current_word_idx < state.words.len() {
            state.words[state.current_word_idx].clone()
        } else {
            String::new()
        },
        current_char_idx: state.current_char_idx,
        waiting_after_shadda: state.is_waiting_for_mark_after_shadda,
        processed_text: state.processed_words.join(" "),
        is_finished: state.current_word_idx >= state.words.len(),
    }
}

/// هل الحرف علامة تشكيل عربية؟
fn is_arabic_diacritic(c: char) -> bool {
    matches!(c,
        '\u{0610}'..='\u{061A}' |
        '\u{064B}'..='\u{065F}' |
        '\u{0670}' |
        '\u{06D6}'..='\u{06DC}' |
        '\u{06DF}'..='\u{06E4}' |
        '\u{06E7}'..='\u{06E8}' |
        '\u{06EA}'..='\u{06ED}'
    )
}

/// هل الحرف شدة؟
fn is_shadda(c: char) -> bool {
    c == '\u{0651}'
}

/// تحليل كلمة إلى حروف أساسية + تشكيل لكل حرف
fn parse_word_with_diacritics(word: &str) -> (String, Vec<String>) {
    let chars: Vec<char> = word.chars().collect();
    let mut clean_word = String::new();
    let mut diacritics_map: Vec<String> = Vec::new();

    let mut i = 0;
    while i < chars.len() {
        if is_arabic_diacritic(chars[i]) {
            // علامة تشكيل يتيمة في البداية — نتخطاها
            i += 1;
            continue;
        }

        clean_word.push(chars[i]);
        let mut diac = String::new();

        // جمع كل علامات التشكيل اللاحقة
        let mut j = i + 1;
        while j < chars.len() && is_arabic_diacritic(chars[j]) {
            diac.push(chars[j]);
            j += 1;
        }

        diacritics_map.push(diac);
        i = j;
    }

    (clean_word, diacritics_map)
}

/// إزالة التشكيل من كلمة مع خيارات التحكم بالشدة
fn filter_diacritics(word: &str, skip_shadda: bool, mark_shadda: bool) -> String {
    let mut result = String::new();
    for c in word.chars() {
        if is_shadda(c) {
            if !skip_shadda || mark_shadda {
                result.push(c);
            }
        } else if is_arabic_diacritic(c) {
            // تخطي جميع علامات التشكيل الأخرى
        } else {
            result.push(c);
        }
    }
    result
}

fn advance_char_logic(s: &mut AppState) {
    let chars: Vec<char> = s.words[s.current_word_idx].chars().collect();
    s.current_char_idx += 1;
    s.is_waiting_for_mark_after_shadda = false;

    if s.current_char_idx >= chars.len() {
        s.processed_words.push(s.current_word_processed.clone());
        s.current_word_idx += 1;
        s.current_char_idx = 0;
        s.current_word_processed.clear();
    }
}

/// تقدّم تلقائي للحروف التي لها تشكيل مسبق (وضع "أخذه بتشكيل")
fn auto_advance_pre_diacritized(s: &mut AppState) {
    if !s.keep_diacritics {
        return;
    }

    loop {
        if s.current_word_idx >= s.words.len() {
            break;
        }

        let word_chars: Vec<char> = s.words[s.current_word_idx].chars().collect();
        if s.current_char_idx >= word_chars.len() {
            break;
        }

        let pre = &s.pre_diacritics[s.current_word_idx][s.current_char_idx];
        if pre.is_empty() {
            // لا يوجد تشكيل مسبق — نتوقف وننتظر المستخدم
            break;
        }

        // تطبيق التشكيل المسبق تلقائياً
        s.current_word_processed.push(word_chars[s.current_char_idx]);
        s.current_word_processed.push_str(pre);

        // التقدم للحرف التالي
        s.current_char_idx += 1;
        if s.current_char_idx >= word_chars.len() {
            s.processed_words.push(s.current_word_processed.clone());
            s.current_word_idx += 1;
            s.current_char_idx = 0;
            s.current_word_processed.clear();
            // نكمل للكلمة التالية
        }
    }
}

#[tauri::command]
fn start_tashkeel(
    text: String,
    keep_diacritics: bool,
    skip_shadda: bool,
    mark_shadda: bool,
    state: State<'_, Mutex<AppState>>,
) -> RenderState {
    let mut s = state.lock().unwrap();

    s.keep_diacritics = keep_diacritics;
    s.pre_diacritics.clear();

    if keep_diacritics {
        // وضع "أخذه بتشكيل" — نفصل الحروف عن التشكيل
        s.words.clear();
        for raw_word in text.split_whitespace() {
            let (clean, diac_map) = parse_word_with_diacritics(raw_word);
            s.words.push(clean);
            s.pre_diacritics.push(diac_map);
        }
    } else {
        // وضع "أخذه بدون تشكيل" — نزيل التشكيل
        s.words = text
            .split_whitespace()
            .map(|w| filter_diacritics(w, skip_shadda, mark_shadda))
            .collect();
        // تشكيل مسبق فارغ لكل حرف
        let counts: Vec<usize> = s.words.iter().map(|w| w.chars().count()).collect();
        for count in counts {
            s.pre_diacritics.push(vec![String::new(); count]);
        }
    }

    s.processed_words.clear();
    s.current_word_idx = 0;
    s.current_char_idx = 0;
    s.current_word_processed = String::new();
    s.is_waiting_for_mark_after_shadda = false;

    // تقدم تلقائي للحروف المُشكّلة مسبقاً
    auto_advance_pre_diacritized(&mut s);

    build_render_state(&s)
}

#[tauri::command]
fn apply_mark(mark: String, state: State<'_, Mutex<AppState>>) -> RenderState {
    let mut s = state.lock().unwrap();
    if s.current_word_idx >= s.words.len() {
        return build_render_state(&s);
    }

    let chars: Vec<char> = s.words[s.current_word_idx].chars().collect();
    if s.current_char_idx >= chars.len() {
        return build_render_state(&s);
    }

    // استخراج الحرف أولاً لتجنب مشكلة الـ Borrow Checker
    let current_char = chars[s.current_char_idx];

    if mark == "ّ" {
        s.current_word_processed.push(current_char);
        s.current_word_processed.push_str(&mark);
        s.is_waiting_for_mark_after_shadda = true;
    } else {
        if s.is_waiting_for_mark_after_shadda {
            s.current_word_processed.push_str(&mark);
        } else {
            s.current_word_processed.push(current_char);
            s.current_word_processed.push_str(&mark);
        }
        advance_char_logic(&mut s);
        // تقدم تلقائي للحروف المُشكّلة مسبقاً
        auto_advance_pre_diacritized(&mut s);
    }

    build_render_state(&s)
}

#[tauri::command]
fn advance_char(state: State<'_, Mutex<AppState>>) -> RenderState {
    let mut s = state.lock().unwrap();
    if s.current_word_idx >= s.words.len() {
        return build_render_state(&s);
    }

    let chars: Vec<char> = s.words[s.current_word_idx].chars().collect();
    if s.current_char_idx >= chars.len() {
        return build_render_state(&s);
    }

    // استخراج الحرف أولاً
    let current_char = chars[s.current_char_idx];

    if !s.is_waiting_for_mark_after_shadda {
        s.current_word_processed.push(current_char);
    }
    advance_char_logic(&mut s);
    // تقدم تلقائي للحروف المُشكّلة مسبقاً
    auto_advance_pre_diacritized(&mut s);

    build_render_state(&s)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .manage(Mutex::new(AppState::default()))
        .invoke_handler(tauri::generate_handler![
            start_tashkeel,
            apply_mark,
            advance_char
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
