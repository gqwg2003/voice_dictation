#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

class SpeechPreprocessor {
public:
    // Подавление шума с использованием спектрального вычитания
    static std::vector<float> noise_reduction(const std::vector<float>& audio_data, float noise_level = 0.01f) {
        if (audio_data.empty()) 
            return audio_data;
        
        std::vector<float> result(audio_data.size());
        
        // Простое спектральное вычитание (упрощенный алгоритм)
        for (size_t i = 0; i < audio_data.size(); ++i) {
            float value = audio_data[i];
            float sign = (value >= 0.0f) ? 1.0f : -1.0f;
            float abs_value = std::abs(value);
            
            // Применяем пороговое шумоподавление
            if (abs_value < noise_level) {
                result[i] = 0.0f;
            } else {
                // Вычитаем оцененный уровень шума
                result[i] = sign * (abs_value - noise_level);
            }
        }
        
        return result;
    }
    
    // Обработка звука для улучшения распознавания речи
    static py::bytes enhance_speech(const py::bytes& raw_data, float noise_level = 0.01f) {
        std::string str = static_cast<std::string>(raw_data);
        
        if (str.empty())
            return raw_data;
            
        // Преобразуем байты в аудио-данные (предполагаем int16)
        const int16_t* samples = reinterpret_cast<const int16_t*>(str.data());
        size_t sample_count = str.size() / sizeof(int16_t);
        
        // Преобразуем в float для обработки
        std::vector<float> float_data(sample_count);
        constexpr float int16_max = 32768.0f;
        
        for (size_t i = 0; i < sample_count; ++i) {
            float_data[i] = static_cast<float>(samples[i]) / int16_max;
        }
        
        // Применяем шумоподавление
        std::vector<float> processed = noise_reduction(float_data, noise_level);
        
        // Нормализуем результат
        float max_val = 0.0f;
        for (float value : processed) {
            max_val = std::max(max_val, std::abs(value));
        }
        
        if (max_val > 0.0f) {
            float gain = 0.95f / max_val;  // Оставляем небольшой запас
            for (float& value : processed) {
                value *= gain;
            }
        }
        
        // Преобразуем обратно в int16_t
        std::vector<int16_t> output_samples(sample_count);
        for (size_t i = 0; i < sample_count; ++i) {
            output_samples[i] = static_cast<int16_t>(processed[i] * int16_max);
        }
        
        // Возвращаем как bytes
        return py::bytes(reinterpret_cast<char*>(output_samples.data()), output_samples.size() * sizeof(int16_t));
    }
    
    // Функция для отсечения тишины в начале и конце записи
    static py::bytes trim_silence(const py::bytes& raw_data, float threshold = 0.02f) {
        std::string str = static_cast<std::string>(raw_data);
        
        if (str.empty())
            return raw_data;
            
        const int16_t* samples = reinterpret_cast<const int16_t*>(str.data());
        size_t sample_count = str.size() / sizeof(int16_t);
        
        // Находим начало и конец речи
        size_t start = 0;
        size_t end = sample_count;
        constexpr float int16_max = 32768.0f;
        const float abs_threshold = threshold * int16_max;
        
        // Поиск начала звука
        for (size_t i = 0; i < sample_count; ++i) {
            if (std::abs(static_cast<float>(samples[i])) > abs_threshold) {
                start = (i > 1000) ? (i - 1000) : 0;  // Оставляем небольшой запас в начале
                break;
            }
        }
        
        // Поиск конца звука
        for (size_t i = sample_count; i-- > 0;) {
            if (std::abs(static_cast<float>(samples[i])) > abs_threshold) {
                end = std::min(i + 1000, sample_count);  // Оставляем небольшой запас в конце
                break;
            }
        }
        
        // Если нет значимого звука или весь звук значимый
        if (start >= end || start == 0 && end == sample_count) {
            return raw_data;
        }
        
        // Вырезаем только значимый звук
        size_t output_size = end - start;
        return py::bytes(reinterpret_cast<const char*>(samples + start), output_size * sizeof(int16_t));
    }
    
    // Ускорение обработки звука для распознавания
    static py::bytes optimize_for_recognition(const py::bytes& raw_data, float noise_level = 0.01f, float silence_threshold = 0.02f) {
        // Сначала улучшаем качество речи
        py::bytes enhanced = enhance_speech(raw_data, noise_level);
        
        // Затем вырезаем тишину
        py::bytes trimmed = trim_silence(enhanced, silence_threshold);
        
        return trimmed;
    }
};

PYBIND11_MODULE(speech_preprocessor, m) {
    m.doc() = "Модуль предварительной обработки речи для улучшения распознавания";
    
    py::class_<SpeechPreprocessor>(m, "SpeechPreprocessor")
        .def_static("noise_reduction", &SpeechPreprocessor::noise_reduction,
            "Применить шумоподавление к аудио данным",
            py::arg("audio_data"), py::arg("noise_level") = 0.01f)
        .def_static("enhance_speech", &SpeechPreprocessor::enhance_speech,
            "Улучшить качество речи для распознавания",
            py::arg("raw_data"), py::arg("noise_level") = 0.01f)
        .def_static("trim_silence", &SpeechPreprocessor::trim_silence,
            "Отсечь тишину в начале и конце записи",
            py::arg("raw_data"), py::arg("threshold") = 0.02f)
        .def_static("optimize_for_recognition", &SpeechPreprocessor::optimize_for_recognition,
            "Комплексная оптимизация для распознавания речи",
            py::arg("raw_data"), py::arg("noise_level") = 0.01f, py::arg("silence_threshold") = 0.02f);
} 