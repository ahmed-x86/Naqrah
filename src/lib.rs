use std::sync::Mutex;
use tauri::State;

#[derive(Default)]
struct AppState {
    words: Vec<String>,
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

#[tauri::command]
fn start_tashkeel(text: String, state: State<'_, Mutex<AppState>>) -> RenderState {
    let mut s = state.lock().unwrap();
    s.words = text.split_whitespace().map(String::from).collect();
    s.processed_words.clear();
    s.current_word_idx = 0;
    s.current_char_idx = 0;
    s.current_word_processed = String::new();
    s.is_waiting_for_mark_after_shadda = false;

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
