#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <complex>

namespace py = pybind11;

class AudioProcessor {
public:
    // Нормализация аудио данных в диапазон [-1, 1]
    static std::vector<float> normalize_audio(const std::vector<float>& audio_data) {
        if (audio_data.empty()) return audio_data;
        
        // Находим максимальное значение
        float max_val = 0.0f;
        for (float sample : audio_data) {
            max_val = std::max(max_val, std::abs(sample));
        }
        
        if (max_val == 0.0f) return audio_data;
        
        // Нормализуем в диапазон [-1, 1]
        std::vector<float> normalized(audio_data.size());
        float scale = 1.0f / max_val;
        for (size_t i = 0; i < audio_data.size(); ++i) {
            normalized[i] = audio_data[i] * scale;
        }
        
        return normalized;
    }
    
    // Расчет уровня аудио с динамической чувствительностью
    static float calculate_level(const std::vector<int16_t>& audio_data) {
        if (audio_data.empty()) return 0.0f;
        
        // Преобразование int16_t в float и нормализация
        std::vector<float> float_data(audio_data.size());
        constexpr float int16_max = 32768.0f;
        
        for (size_t i = 0; i < audio_data.size(); ++i) {
            float_data[i] = static_cast<float>(audio_data[i]) / int16_max;
        }
        
        return calculate_level_impl(float_data);
    }
    
    // Перегрузка для байтового массива (для Python)
    static float calculate_level(const py::bytes& raw_data) {
        std::string str = static_cast<std::string>(raw_data);
        
        if (str.empty())
            return 0.0f;
            
        // Предполагаем формат int16_t little-endian
        const int16_t* samples = reinterpret_cast<const int16_t*>(str.data());
        size_t sample_count = str.size() / sizeof(int16_t);
        
        std::vector<int16_t> audio_data(samples, samples + sample_count);
        return calculate_level(audio_data);
    }
    
    // Перегрузка для расчета уровня из float-массива
    static float calculate_level(const std::vector<float>& audio_data) {
        return calculate_level_impl(audio_data);
    }
    
    // Расчет спектра для визуализации
    static std::vector<float> calculate_spectrum(const py::bytes& raw_data, size_t band_count = 16) {
        std::string str = static_cast<std::string>(raw_data);
        
        if (str.empty())
            return std::vector<float>(band_count, 0.0f);
            
        // Предполагаем формат int16_t little-endian
        const int16_t* samples = reinterpret_cast<const int16_t*>(str.data());
        size_t sample_count = str.size() / sizeof(int16_t);
        
        // Преобразуем в float для обработки
        std::vector<float> float_data(sample_count);
        constexpr float int16_max = 32768.0f;
        
        for (size_t i = 0; i < sample_count; ++i) {
            float_data[i] = static_cast<float>(samples[i]) / int16_max;
        }
        
        return calculate_spectrum_impl(float_data, band_count);
    }
    
    // Обнаружение речи в аудио потоке
    static bool detect_speech(const py::bytes& raw_data, float threshold = 0.02f) {
        float level = calculate_level(raw_data);
        return level > threshold;
    }
    
    // Оценка качества сигнала (SNR - отношение сигнал/шум)
    static float estimate_signal_quality(const py::bytes& raw_data) {
        std::string str = static_cast<std::string>(raw_data);
        
        if (str.empty())
            return 0.0f;
            
        const int16_t* samples = reinterpret_cast<const int16_t*>(str.data());
        size_t sample_count = str.size() / sizeof(int16_t);
        
        // Преобразуем в float для обработки
        std::vector<float> float_data(sample_count);
        constexpr float int16_max = 32768.0f;
        
        for (size_t i = 0; i < sample_count; ++i) {
            float_data[i] = static_cast<float>(samples[i]) / int16_max;
        }
        
        // Получаем спектр
        std::vector<float> spectrum = calculate_spectrum_impl(float_data, 32);
        
        // Разделяем на низкие, средние и высокие частоты
        float low_freq_energy = 0.0f;
        float mid_freq_energy = 0.0f;
        float high_freq_energy = 0.0f;
        
        for (size_t i = 0; i < spectrum.size(); ++i) {
            if (i < spectrum.size() / 3) {
                low_freq_energy += spectrum[i];
            } else if (i < 2 * spectrum.size() / 3) {
                mid_freq_energy += spectrum[i];
            } else {
                high_freq_energy += spectrum[i];
            }
        }
        
        // Человеческая речь в основном сосредоточена в средних частотах
        float voice_energy = mid_freq_energy;
        float noise_energy = low_freq_energy + high_freq_energy;
        
        if (noise_energy < 1e-6f)
            return 0.0f;
            
        // Возвращаем отношение сигнал/шум, ограничивая максимум
        return std::min(voice_energy / noise_energy, 1.0f);
    }

private:
    // Реализация расчета уровня аудио
    static float calculate_level_impl(const std::vector<float>& audio_data) {
        if (audio_data.empty()) return 0.0f;
        
        // RMS (Root Mean Square) для расчета энергии
        float sum = 0.0f;
        for (float sample : audio_data) {
            sum += sample * sample;
        }
        
        float rms = std::sqrt(sum / audio_data.size());
        
        // Применяем нелинейную кривую для лучшего визуального отображения
        // (логарифмическая шкала для более естественного восприятия громкости)
        constexpr float reference_level = 0.01f;  // Уровень тишины
        
        if (rms < reference_level)
            return 0.0f;
            
        // Логарифмический масштаб и нормализация в диапазон [0, 1]
        float db = 20.0f * std::log10(rms / reference_level);
        constexpr float max_db = 60.0f;  // Максимальный уровень в дБ
        
        return std::min(std::max(db / max_db, 0.0f), 1.0f);
    }
    
    // Реализация расчета спектра (простая версия без FFT)
    static std::vector<float> calculate_spectrum_impl(const std::vector<float>& audio_data, size_t band_count) {
        if (audio_data.empty() || band_count == 0)
            return std::vector<float>(band_count, 0.0f);
            
        std::vector<float> bands(band_count, 0.0f);
        
        // Размер фрагмента для одной полосы
        size_t samples_per_band = audio_data.size() / band_count;
        if (samples_per_band == 0)
            samples_per_band = 1;
            
        // Для каждой полосы вычисляем энергию
        for (size_t band = 0; band < band_count; ++band) {
            size_t start = band * samples_per_band;
            size_t end = std::min((band + 1) * samples_per_band, audio_data.size());
            
            float band_energy = 0.0f;
            for (size_t i = start; i < end; ++i) {
                band_energy += audio_data[i] * audio_data[i];
            }
            
            if (end > start) {
                band_energy /= (end - start);
                bands[band] = std::sqrt(band_energy);
            }
        }
        
        // Нормализуем полосы, чтобы самая громкая была 1.0
        float max_band = *std::max_element(bands.begin(), bands.end());
        if (max_band > 1e-6f) {
            for (float& band : bands) {
                band /= max_band;
            }
        }
        
        return bands;
    }
};

PYBIND11_MODULE(audio_processor, m) {
    m.doc() = "Расширенный модуль обработки аудио с визуализацией";
    
    py::class_<AudioProcessor>(m, "AudioProcessor")
        .def_static("normalize_audio", &AudioProcessor::normalize_audio)
        .def_static("calculate_level", static_cast<float (*)(const py::bytes&)>(&AudioProcessor::calculate_level),
            "Рассчитать уровень аудио (0.0-1.0) из необработанных аудиоданных")
        .def_static("calculate_spectrum", &AudioProcessor::calculate_spectrum, 
            "Рассчитать спектр для визуализации",
            py::arg("raw_data"), py::arg("band_count") = 16)
        .def_static("detect_speech", &AudioProcessor::detect_speech,
            "Обнаружить речь в аудио данных",
            py::arg("raw_data"), py::arg("threshold") = 0.02f)
        .def_static("estimate_signal_quality", &AudioProcessor::estimate_signal_quality,
            "Оценить качество сигнала (отношение сигнал/шум)");
} 