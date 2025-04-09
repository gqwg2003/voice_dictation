#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Hybrid Mode Manager для Voice Dictation
Обеспечивает интеграцию между Python и C++ компонентами и управляет режимами работы приложения.
"""

import os
import sys
import importlib.util
import logging
import platform
from typing import Dict, Any, Optional, List, Tuple

# Настройка логгера
logger = logging.getLogger(__name__)

class HybridManager:
    """
    Класс для управления гибридным режимом работы приложения.
    Обеспечивает прозрачное использование C++ расширений или чистого Python в зависимости от доступности.
    """
    
    def __init__(self):
        self.system_info = self._get_system_info()
        self.extensions_available = {
            'audio_processor': False,
            'text_processor': False,
            'speech_preprocessor': False
        }
        self.modules = {}
        self.fallback_modules = {}
        
        # Проверяем и загружаем доступные расширения
        self._init_extensions()
        
    def _get_system_info(self) -> Dict[str, Any]:
        """Получение информации о системе для адаптации компонентов."""
        return {
            'os': platform.system(),
            'architecture': platform.architecture()[0],
            'python_version': platform.python_version(),
            'processor': platform.processor(),
            'is_64bit': sys.maxsize > 2**32,
            'is_mobile': platform.system() in ['Android', 'iOS']
        }
    
    def _init_extensions(self) -> None:
        """Инициализация и проверка доступности C++ расширений."""
        # Проверяем директорию с расширениями
        extensions_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'core')
        release_dir = os.path.join(extensions_dir, 'Release')
        
        # Список расширений для проверки
        extensions = ['audio_processor', 'text_processor', 'speech_preprocessor']
        
        for ext in extensions:
            # Пытаемся загрузить C++ расширение
            try:
                if os.path.exists(os.path.join(release_dir, f"{ext}.pyd")):
                    # Для Windows
                    ext_path = os.path.join(release_dir, f"{ext}.pyd")
                elif os.path.exists(os.path.join(extensions_dir, f"{ext}.so")):
                    # Для Linux/Mac
                    ext_path = os.path.join(extensions_dir, f"{ext}.so")
                else:
                    # Расширение не найдено, будем использовать Python-реализацию
                    logger.info(f"C++ extension {ext} not found, will use Python fallback")
                    self._load_fallback_module(ext)
                    continue
                
                # Загружаем расширение
                spec = importlib.util.spec_from_file_location(ext, ext_path)
                if spec and spec.loader:
                    module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(module)
                    self.modules[ext] = module
                    self.extensions_available[ext] = True
                    logger.info(f"Loaded C++ extension: {ext}")
                else:
                    logger.warning(f"Failed to load extension: {ext}")
                    self._load_fallback_module(ext)
            except Exception as e:
                logger.warning(f"Error loading C++ extension {ext}: {e}")
                self._load_fallback_module(ext)
    
    def _load_fallback_module(self, module_name: str) -> None:
        """Загрузка Python-версии модуля в случае недоступности C++ расширения."""
        try:
            if module_name == 'audio_processor':
                from core.fallbacks.audio_processor_py import AudioProcessor
                self.fallback_modules[module_name] = AudioProcessor
            elif module_name == 'text_processor':
                from core.fallbacks.text_processor_py import TextProcessor
                self.fallback_modules[module_name] = TextProcessor
            elif module_name == 'speech_preprocessor':
                from core.fallbacks.speech_preprocessor_py import SpeechPreprocessor
                self.fallback_modules[module_name] = SpeechPreprocessor
            logger.info(f"Loaded Python fallback for {module_name}")
        except ImportError as e:
            logger.error(f"Failed to load fallback for {module_name}: {e}")
    
    def get_audio_processor(self):
        """Получение экземпляра AudioProcessor (C++ или Python)."""
        if self.extensions_available['audio_processor']:
            return self.modules['audio_processor'].AudioProcessor
        return self.fallback_modules.get('audio_processor')
    
    def get_text_processor(self):
        """Получение экземпляра TextProcessor (C++ или Python)."""
        if self.extensions_available['text_processor']:
            return self.modules['text_processor'].TextProcessor
        return self.fallback_modules.get('text_processor')
        
    def get_speech_preprocessor(self):
        """Получение экземпляра SpeechPreprocessor (C++ или Python)."""
        if self.extensions_available['speech_preprocessor']:
            return self.modules['speech_preprocessor'].SpeechPreprocessor
        return self.fallback_modules.get('speech_preprocessor')
    
    def is_hybrid_mode_available(self) -> bool:
        """Проверка доступности гибридного режима."""
        return any(self.extensions_available.values())
    
    def get_optimal_mode(self) -> str:
        """Определение оптимального режима работы на основе системы."""
        if self.system_info['is_mobile']:
            return 'light'
        
        if all(self.extensions_available.values()):
            return 'hybrid_full'
        
        if any(self.extensions_available.values()):
            return 'hybrid_partial'
            
        return 'pure_python'
    
    def get_mode_info(self) -> Dict[str, Any]:
        """Получение информации о текущем режиме работы."""
        mode = self.get_optimal_mode()
        return {
            'mode': mode,
            'extensions_available': self.extensions_available.copy(),
            'system_info': self.system_info.copy(),
            'performance_level': self._get_performance_level(mode)
        }
    
    def _get_performance_level(self, mode: str) -> int:
        """Оценка уровня производительности режима (1-5)."""
        if mode == 'hybrid_full':
            return 5
        elif mode == 'hybrid_partial':
            return 4
        elif mode == 'pure_python' and self.system_info['is_64bit']:
            return 3
        elif mode == 'pure_python':
            return 2
        elif mode == 'light':
            return 1
        return 0

# Создаем глобальный экземпляр менеджера
hybrid_manager = HybridManager()

# Функции для удобного доступа к функционалу
def get_audio_processor():
    return hybrid_manager.get_audio_processor()

def get_text_processor():
    return hybrid_manager.get_text_processor()

def get_speech_preprocessor():
    return hybrid_manager.get_speech_preprocessor()

def get_mode_info():
    return hybrid_manager.get_mode_info()

def is_hybrid_available():
    return hybrid_manager.is_hybrid_mode_available() 