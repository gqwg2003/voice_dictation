#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <regex>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class TextProcessor {
private:
    static double levenshtein_distance(const std::string& s1, const std::string& s2) {
        const size_t len1 = s1.size();
        const size_t len2 = s2.size();
        
        std::vector<std::vector<size_t>> d(len1 + 1, std::vector<size_t>(len2 + 1));
        
        for (size_t i = 0; i <= len1; ++i) d[i][0] = i;
        for (size_t j = 0; j <= len2; ++j) d[0][j] = j;
        
        for (size_t i = 1; i <= len1; ++i) {
            for (size_t j = 1; j <= len2; ++j) {
                d[i][j] = std::min({
                    d[i-1][j] + 1,
                    d[i][j-1] + 1,
                    d[i-1][j-1] + (s1[i-1] == s2[j-1] ? 0 : 1)
                });
            }
        }
        
        return 1.0 - static_cast<double>(d[len1][len2]) / std::max(len1, len2);
    }

    // Преобразовать строку в нижний регистр
    static std::string to_lower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), 
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    // Проверка, содержит ли строка кириллические символы
    static bool contains_cyrillic(const std::string& text) {
        for (unsigned char c : text) {
            // Примерный диапазон символов кириллицы в UTF-8
            if ((c >= 0xD0 && c <= 0xD1) || (c >= 0x90 && c <= 0xBF))
                return true;
        }
        return false;
    }

    // Разделение текста на слова
    static std::vector<std::string> split_words(const std::string& text) {
        std::vector<std::string> words;
        std::string current_word;
        
        for (char c : text) {
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '-' || c == '\'') {
                current_word += c;
            } else if (!current_word.empty()) {
                words.push_back(current_word);
                current_word.clear();
            }
        }
        
        if (!current_word.empty()) {
            words.push_back(current_word);
        }
        
        return words;
    }

public:
    // Исправление регистра первых букв предложений
    static std::string fix_capitalization(const std::string& text) {
        if (text.empty()) return text;
        
        std::string result;
        bool start_sentence = true;
        
        for (char c : text) {
            if (start_sentence && std::isalpha(static_cast<unsigned char>(c))) {
                result += std::toupper(static_cast<unsigned char>(c));
                start_sentence = false;
            } else {
                result += c;
            }
            
            if (c == '.' || c == '!' || c == '?') {
                start_sentence = true;
            }
        }
        
        return result;
    }

    // Исправление пунктуации
    static std::string fix_punctuation(const std::string& text) {
        if (text.empty()) return text;
        
        std::string result;
        bool last_was_space = false;
        
        for (size_t i = 0; i < text.length(); ++i) {
            char c = text[i];
            
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!last_was_space) {
                    result += ' ';
                    last_was_space = true;
                }
            } else if (c == '.' || c == ',' || c == '!' || c == '?' || c == ':' || c == ';') {
                // Убираем пробел перед знаком препинания, если он есть
                if (!result.empty() && result.back() == ' ') {
                    result.pop_back();
                }
                result += c;
                last_was_space = false;
            } else {
                result += c;
                last_was_space = false;
            }
        }
        
        return result;
    }

    // Основной метод обработки текста
    static std::string process(
        const std::string& text,
        const std::vector<std::string>& custom_terms = {},
        double similarity_threshold = 0.65
    ) {
        if (text.empty()) return text;
        
        // Создаем копию текста для обработки
        std::string result = text;
        
        // Применяем пользовательские термины
        result = apply_custom_terms(result, custom_terms, similarity_threshold);
        
        // Исправляем пунктуацию
        result = fix_punctuation(result);
        
        // Исправляем регистр
        result = fix_capitalization(result);
        
        return result;
    }

    // Улучшенная версия для работы с пользовательскими терминами
    static std::string apply_custom_terms(
        const std::string& text,
        const std::vector<std::string>& custom_terms,
        double similarity_threshold = 0.65
    ) {
        if (text.empty() || custom_terms.empty()) {
            return text;
        }
        
        std::string result = text;
        std::vector<std::string> words = split_words(text);
        
        for (const auto& term : custom_terms) {
            // Для точного совпадения
            std::string pattern = "\\b" + term + "\\b";
            try {
                std::regex word_regex(pattern, std::regex_constants::icase);
                result = std::regex_replace(result, word_regex, term);
            } catch (const std::regex_error&) {
                // Игнорируем ошибки регулярных выражений
                continue;
            }
            
            // Для нечеткого совпадения
            for (const auto& word : words) {
                if (word.length() > 2 && term.length() > 2) {
                    double similarity = levenshtein_distance(to_lower(word), to_lower(term));
                    if (similarity >= similarity_threshold && similarity < 1.0) {
                        try {
                            std::regex word_regex("\\b" + word + "\\b", 
                                                std::regex_constants::icase);
                            result = std::regex_replace(result, word_regex, term);
                        } catch (const std::regex_error&) {
                            // Игнорируем ошибки регулярных выражений
                            continue;
                        }
                    }
                }
            }
        }
        
        return result;
    }

    // Поддержка обратной совместимости с существующим кодом
    static std::string post_process_text(
        const std::string& text,
        const std::unordered_map<std::string, std::string>& custom_terms,
        const std::vector<std::string>& common_phrases,
        double similarity_threshold = 0.65
    ) {
        std::string result = text;
        
        // Применяем пользовательские термины
        for (const auto& [term, replacement] : custom_terms) {
            size_t pos = 0;
            while ((pos = result.find(term, pos)) != std::string::npos) {
                result.replace(pos, term.length(), replacement);
                pos += replacement.length();
            }
        }
        
        // Применяем фразы по умолчанию
        for (const auto& phrase : common_phrases) {
            size_t pos = 0;
            while ((pos = result.find(phrase, pos)) != std::string::npos) {
                // Проверяем, является ли фраза полным словом
                bool is_word_boundary_start = (pos == 0 || !std::isalnum(static_cast<unsigned char>(result[pos-1])));
                bool is_word_boundary_end = (pos + phrase.length() == result.length() || 
                                           !std::isalnum(static_cast<unsigned char>(result[pos + phrase.length()])));
                
                if (is_word_boundary_start && is_word_boundary_end) {
                    // Уже правильная фраза
                    pos += phrase.length();
                } else {
                    // Ищем наилучшее совпадение
                    double best_similarity = 0.0;
                    size_t best_pos = pos;
                    size_t best_length = phrase.length();
                    
                    for (size_t i = 0; i < result.length() - phrase.length() + 1; ++i) {
                        std::string substr = result.substr(i, phrase.length());
                        double similarity = levenshtein_distance(substr, phrase);
                        if (similarity > best_similarity && similarity >= similarity_threshold) {
                            best_similarity = similarity;
                            best_pos = i;
                            best_length = phrase.length();
                        }
                    }
                    
                    if (best_similarity >= similarity_threshold) {
                        result.replace(best_pos, best_length, phrase);
                        pos = best_pos + phrase.length();
                    } else {
                        pos += phrase.length();
                    }
                }
            }
        }
        
        // Исправляем пунктуацию и регистр
        result = fix_punctuation(result);
        result = fix_capitalization(result);
        
        return result;
    }

    // Определение языка текста
    static std::string detect_language(const std::string& text) {
        if (text.empty()) {
            return "unknown";
        }
        
        int cyrillic_count = 0;
        int latin_count = 0;
        
        for (unsigned char c : text) {
            // Проверяем кириллические символы
            if ((c >= 0xD0 && c <= 0xD1) || (c >= 0x90 && c <= 0xBF)) {
                cyrillic_count++;
            }
            // Проверяем латинские символы
            else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                latin_count++;
            }
        }
        
        if (cyrillic_count > latin_count) {
            return "ru-RU";
        } else if (latin_count > 0) {
            return "en-US";
        } else {
            return "unknown";
        }
    }
};

PYBIND11_MODULE(text_processor, m) {
    m.doc() = "Улучшенный модуль обработки текста";
    
    py::class_<TextProcessor>(m, "TextProcessor")
        .def_static("process", &TextProcessor::process,
            "Основная функция для обработки текста",
            py::arg("text"), 
            py::arg("custom_terms") = std::vector<std::string>(),
            py::arg("similarity_threshold") = 0.65)
        .def_static("post_process_text", &TextProcessor::post_process_text,
            "Обработка текста с заменой пользовательских терминов (устаревший метод)",
            py::arg("text"), 
            py::arg("custom_terms"), 
            py::arg("common_phrases"), 
            py::arg("similarity_threshold") = 0.65)
        .def_static("fix_punctuation", &TextProcessor::fix_punctuation,
            "Исправление пунктуации в тексте")
        .def_static("fix_capitalization", &TextProcessor::fix_capitalization,
            "Исправление регистра в тексте")
        .def_static("detect_language", &TextProcessor::detect_language,
            "Определение языка текста (ru-RU, en-US или unknown)");
} 