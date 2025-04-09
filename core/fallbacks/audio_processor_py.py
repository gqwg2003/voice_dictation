#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Python-реализация AudioProcessor для работы в чистом Python режиме.
"""

import math
import array
import logging
from typing import List, Union, Optional
import struct

logger = logging.getLogger(__name__)

class AudioProcessor:
    """
    Python-версия класса для обработки аудио.
    Реализует тот же интерфейс, что и C++ версия для обеспечения совместимости.
    """
    
    @staticmethod
    def normalize_audio(audio_data: List[float]) -> List[float]:
        """Нормализация аудио данных в диапазон [-1, 1]."""
        if not audio_data:
            return audio_data
            
        # Находим максимальное значение
        max_val = max(abs(sample) for sample in audio_data)
        
        if max_val == 0.0:
            return audio_data
            
        # Нормализуем в диапазон [-1, 1]
        normalized = [sample / max_val for sample in audio_data]
        return normalized
    
    @staticmethod
    def calculate_level(audio_data: Union[bytes, List[int], List[float]]) -> float:
        """
        Расчет уровня аудио с динамической чувствительностью.
        Поддерживает несколько форматов ввода для совместимости с C++ версией.
        """
        if isinstance(audio_data, bytes):
            # Преобразуем байты в int16
            int16_max = 32768.0
            samples = []
            for i in range(0, len(audio_data), 2):
                if i + 1 < len(audio_data):
                    sample = struct.unpack('<h', audio_data[i:i+2])[0]
                    samples.append(float(sample) / int16_max)
            return AudioProcessor._calculate_level_impl(samples)
            
        elif isinstance(audio_data, list):
            if not audio_data:
                return 0.0
                
            if isinstance(audio_data[0], int):
                # Если входные данные - список int
                int16_max = 32768.0
                float_data = [float(sample) / int16_max for sample in audio_data]
                return AudioProcessor._calculate_level_impl(float_data)
            elif isinstance(audio_data[0], float):
                # Если входные данные уже в формате float
                return AudioProcessor._calculate_level_impl(audio_data)
                
        return 0.0
    
    @staticmethod
    def _calculate_level_impl(audio_data: List[float]) -> float:
        """Реализация расчета уровня аудио."""
        if not audio_data:
            return 0.0
            
        # RMS (Root Mean Square) для расчета энергии
        sum_squares = sum(sample * sample for sample in audio_data)
        rms = math.sqrt(sum_squares / len(audio_data))
        
        # Применяем нелинейную кривую для лучшего визуального отображения
        reference_level = 0.01  # Уровень тишины
        
        if rms < reference_level:
            return 0.0
            
        # Логарифмический масштаб и нормализация в диапазон [0, 1]
        try:
            db = 20.0 * math.log10(rms / reference_level)
            max_db = 60.0  # Максимальный уровень в дБ
            return min(max(db / max_db, 0.0), 1.0)
        except (ValueError, ZeroDivisionError):
            return 0.0
    
    @staticmethod
    def calculate_spectrum(raw_data: bytes, band_count: int = 16) -> List[float]:
        """
        Расчет спектра для визуализации.
        Упрощенная версия без FFT для совместимости с C++ версией.
        """
        if not raw_data:
            return [0.0] * band_count
            
        # Преобразуем байты в float
        int16_max = 32768.0
        float_data = []
        for i in range(0, len(raw_data), 2):
            if i + 1 < len(raw_data):
                sample = struct.unpack('<h', raw_data[i:i+2])[0]
                float_data.append(float(sample) / int16_max)
                
        return AudioProcessor._calculate_spectrum_impl(float_data, band_count)
    
    @staticmethod
    def _calculate_spectrum_impl(audio_data: List[float], band_count: int) -> List[float]:
        """Реализация расчета спектра (простая версия без FFT)."""
        if not audio_data or band_count == 0:
            return [0.0] * band_count
            
        bands = [0.0] * band_count
        
        # Размер фрагмента для одной полосы
        samples_per_band = len(audio_data) // band_count
        if samples_per_band == 0:
            samples_per_band = 1
            
        # Для каждой полосы вычисляем энергию
        for band in range(band_count):
            start = band * samples_per_band
            end = min((band + 1) * samples_per_band, len(audio_data))
            
            band_energy = 0.0
            for i in range(start, end):
                band_energy += audio_data[i] * audio_data[i]
                
            if end > start:
                band_energy /= (end - start)
                bands[band] = math.sqrt(band_energy)
                
        return bands
    
    @staticmethod
    def detect_speech(raw_data: bytes, threshold: float = 0.02) -> bool:
        """Обнаружение речи в аудио потоке."""
        level = AudioProcessor.calculate_level(raw_data)
        return level > threshold
    
    @staticmethod
    def estimate_signal_quality(raw_data: bytes) -> float:
        """Оценка качества сигнала (SNR - отношение сигнал/шум)."""
        if not raw_data:
            return 0.0
            
        # Преобразуем байты в float
        int16_max = 32768.0
        float_data = []
        for i in range(0, len(raw_data), 2):
            if i + 1 < len(raw_data):
                sample = struct.unpack('<h', raw_data[i:i+2])[0]
                float_data.append(float(sample) / int16_max)
                
        # Получаем спектр
        spectrum = AudioProcessor._calculate_spectrum_impl(float_data, 32)
        
        # Разделяем на низкие, средние и высокие частоты
        low_freq_energy = sum(spectrum[:len(spectrum)//3])
        mid_freq_energy = sum(spectrum[len(spectrum)//3:2*len(spectrum)//3])
        high_freq_energy = sum(spectrum[2*len(spectrum)//3:])
        
        # Человеческая речь в основном сосредоточена в средних частотах
        voice_energy = mid_freq_energy
        noise_energy = low_freq_energy + high_freq_energy
        
        if noise_energy < 1e-6:
            return 0.0
            
        # Возвращаем отношение сигнал/шум, ограничивая максимум
        return min(voice_energy / noise_energy, 1.0) 