use std::sync::Mutex;
use tauri::State;

#[derive(Clone, Copy, PartialEq)]
pub enum CharAction {
    Skip,
    Diacritize,
    Delete,
}

impl Default for CharAction {
    fn default() -> Self {
        CharAction::Diacritize
    }
}

impl CharAction {
    pub fn from_str(s: &str) -> Self {
        match s {
            "skip" => CharAction::Skip,
            "delete" => CharAction::Delete,
            _ => CharAction::Diacritize,
        }
    }
}

pub enum CharCat {
    Arabic,
    English,
    ArabicDigit,
    HindiDigit,
    Emoji,
    Other,
}

fn get_char_category(c: char) -> CharCat {
    let u = c as u32;
    if (u >= 0x0660 && u <= 0x0669) || (u >= 0x06F0 && u <= 0x06F9) {
        return CharCat::HindiDigit;
    }
    
    let is_arabic_block = (u >= 0x0600 && u <= 0x06FF) 
        || (u >= 0x0750 && u <= 0x077F) 
        || (u >= 0x08A0 && u <= 0x08FF) 
        || (u >= 0xFB50 && u <= 0xFDFF) 
        || (u >= 0xFE70 && u <= 0xFEFF);
        
    if is_arabic_block && c.is_alphabetic() {
        return CharCat::Arabic;
    }

    if c.is_ascii_alphabetic() {
        return CharCat::English;
    }
    
    if c.is_ascii_digit() {
        return CharCat::ArabicDigit;
    }

    if (u >= 0x1F300 && u <= 0x1FAFF) || (u >= 0x2600 && u <= 0x27BF) || (u >= 0x1F600 && u <= 0x1F64F) {
        return CharCat::Emoji;
    }

    if c.is_whitespace() {
        return CharCat::Other;
    }

    CharCat::Other
}

fn get_action_for_char(c: char, state: &AppState) -> CharAction {
    match get_char_category(c) {
        CharCat::Arabic => CharAction::Diacritize,
        CharCat::English => state.action_english,
        CharCat::ArabicDigit => state.action_arabic_num,
        CharCat::HindiDigit => state.action_hindi_num,
        CharCat::Emoji => state.action_emoji,
        CharCat::Other => state.action_other,
    }
}

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
    
    action_english: CharAction,
    action_arabic_num: CharAction,
    action_hindi_num: CharAction,
    action_emoji: CharAction,
    action_other: CharAction,
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

fn auto_advance(s: &mut AppState) {
    loop {
        if s.current_word_idx >= s.words.len() {
            break;
        }

        let word_chars: Vec<char> = s.words[s.current_word_idx].chars().collect();
        if s.current_char_idx >= word_chars.len() {
            break;
        }

        let c = word_chars[s.current_char_idx];
        let action = get_action_for_char(c, s);

        let mut auto_diac = None;

        if s.keep_diacritics {
            let pre = &s.pre_diacritics[s.current_word_idx][s.current_char_idx];
            if !pre.is_empty() {
                auto_diac = Some(pre.clone());
            }
        }

        if action == CharAction::Skip {
            s.current_word_processed.push(c);
            if let Some(d) = auto_diac {
                s.current_word_processed.push_str(&d);
            }
            
            s.current_char_idx += 1;
            if s.current_char_idx >= word_chars.len() {
                s.processed_words.push(s.current_word_processed.clone());
                s.current_word_idx += 1;
                s.current_char_idx = 0;
                s.current_word_processed.clear();
            }
            continue;
        }

        if let Some(d) = auto_diac {
            s.current_word_processed.push(c);
            s.current_word_processed.push_str(&d);
            
            s.current_char_idx += 1;
            if s.current_char_idx >= word_chars.len() {
                s.processed_words.push(s.current_word_processed.clone());
                s.current_word_idx += 1;
                s.current_char_idx = 0;
                s.current_word_processed.clear();
            }
            continue;
        }

        break;
    }
}

#[tauri::command]
fn start_tashkeel(
    text: String,
    keep_diacritics: bool,
    skip_shadda: bool,
    mark_shadda: bool,
    action_english: String,
    action_arabic_num: String,
    action_hindi_num: String,
    action_emoji: String,
    action_other: String,
    state: State<'_, Mutex<AppState>>,
) -> RenderState {
    let mut s = state.lock().unwrap();

    s.keep_diacritics = keep_diacritics;
    s.action_english = CharAction::from_str(&action_english);
    s.action_arabic_num = CharAction::from_str(&action_arabic_num);
    s.action_hindi_num = CharAction::from_str(&action_hindi_num);
    s.action_emoji = CharAction::from_str(&action_emoji);
    s.action_other = CharAction::from_str(&action_other);
    s.pre_diacritics.clear();

    // Filter out "Delete" characters
    let filtered_text: String = text.chars().filter(|&c| {
        if is_arabic_diacritic(c) || is_shadda(c) || c.is_whitespace() {
            return true;
        }
        get_action_for_char(c, &s) != CharAction::Delete
    }).collect();

    if keep_diacritics {
        s.words.clear();
        for raw_word in filtered_text.split_whitespace() {
            let (clean, diac_map) = parse_word_with_diacritics(raw_word);
            s.words.push(clean);
            s.pre_diacritics.push(diac_map);
        }
    } else {
        s.words = filtered_text
            .split_whitespace()
            .map(|w| filter_diacritics(w, skip_shadda, mark_shadda))
            .collect();
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

    auto_advance(&mut s);

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
        // تقدم تلقائي
        auto_advance(&mut s);
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
    // تقدم تلقائي
    auto_advance(&mut s);

    build_render_state(&s)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_store::Builder::default().build()) // السطر المُضاف
        .manage(Mutex::new(AppState::default()))
        .invoke_handler(tauri::generate_handler![
            start_tashkeel,
            apply_mark,
            advance_char
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}