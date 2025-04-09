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
            'speech_preprocessor': False,
            'speech_recognizer': False,
            'hotkey_manager': False,
            'clipboard_manager': False
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
        core_dir = os.path.dirname(__file__)
        root_dir = os.path.dirname(core_dir)
        
        # Пути к возможным местам расположения расширений
        extension_paths = [
            os.path.join(core_dir, 'cpp_extensions', 'build', 'Release'),  # Стандартный путь сборки
            os.path.join(core_dir, 'cpp_extensions', 'Release'),           # Альтернативный путь
            os.path.join(core_dir, 'Release'),                             # Для совместимости
        ]
        
        # Список расширений для проверки
        extensions = [
            'audio_processor', 
            'text_processor', 
            'speech_preprocessor', 
            'speech_recognizer',
            'hotkey_manager',
            'clipboard_manager'
        ]
        
        for ext in extensions:
            # Пытаемся найти и загрузить C++ расширение
            ext_path = None
            
            # Проверяем все возможные пути
            for path in extension_paths:
                pyd_path = os.path.join(path, f"{ext}.pyd")
                so_path = os.path.join(path, f"{ext}.so")
                
                if os.path.exists(pyd_path):
                    ext_path = pyd_path
                    break
                elif os.path.exists(so_path):
                    ext_path = so_path
                    break
            
            if not ext_path:
                # Расширение не найдено, будем использовать Python-реализацию
                logger.info(f"C++ extension {ext} not found, will use Python fallback")
                self._load_fallback_module(ext)
                continue
                
            # Загружаем расширение
            try:
                spec = importlib.util.spec_from_file_location(ext, ext_path)
                if spec and spec.loader:
                    module = importlib.util.module_from_spec(spec)
                    spec.loader.exec_module(module)
                    self.modules[ext] = module
                    self.extensions_available[ext] = True
                    logger.info(f"Loaded C++ extension: {ext} from {ext_path}")
                else:
                    logger.warning(f"Failed to load extension: {ext}")
                    self._load_fallback_module(ext)
            except Exception as e:
                logger.warning(f"Error loading C++ extension {ext}: {e}")
                self._load_fallback_module(ext)
    
    def _load_fallback_module(self, module_name: str) -> None:
        """Загрузка Python-версии модуля в случае недоступности C++ расширения."""
        try:
            # Избегаем циклических зависимостей, импортируя только при необходимости
            if module_name == 'audio_processor':
                # Проверяем наличие модуля в fallbacks
                if os.path.exists(os.path.join(os.path.dirname(__file__), 'fallbacks', 'audio_processor_py.py')):
                    from core.fallbacks.audio_processor_py import AudioProcessor
                    self.fallback_modules[module_name] = AudioProcessor
                else:
                    from core.audio_processor import AudioProcessor
                    self.fallback_modules[module_name] = AudioProcessor
                    
            elif module_name == 'text_processor':
                if os.path.exists(os.path.join(os.path.dirname(__file__), 'fallbacks', 'text_processor_py.py')):
                    from core.fallbacks.text_processor_py import TextProcessor
                    self.fallback_modules[module_name] = TextProcessor
                else:
                    from core.text_processor import TextProcessor
                    self.fallback_modules[module_name] = TextProcessor
                    
            elif module_name == 'speech_preprocessor':
                if os.path.exists(os.path.join(os.path.dirname(__file__), 'fallbacks', 'speech_preprocessor_py.py')):
                    from core.fallbacks.speech_preprocessor_py import SpeechPreprocessor
                    self.fallback_modules[module_name] = SpeechPreprocessor
                else:
                    # Возможно этот модуль недоступен в Python версии
                    logger.warning(f"No Python fallback for {module_name}")
                    
            elif module_name == 'speech_recognizer':
                # Используем относительный импорт для предотвращения циклических зависимостей
                try:
                    # Сначала проверяем fallbacks директорию
                    if os.path.exists(os.path.join(os.path.dirname(__file__), 'fallbacks', 'speech_recognizer_py.py')):
                        from core.fallbacks.speech_recognizer_py import SpeechRecognizer
                        self.fallback_modules[module_name] = SpeechRecognizer
                    else:
                        # Если нет в fallbacks, используем основной модуль
                        from core.speech_recognizer import SpeechRecognizer
                        self.fallback_modules[module_name] = SpeechRecognizer
                except ImportError as e:
                    logger.warning(f"Could not import speech_recognizer: {e}")
                    
            elif module_name == 'hotkey_manager':
                if os.path.exists(os.path.join(os.path.dirname(__file__), 'fallbacks', 'hotkey_manager_py.py')):
                    from core.fallbacks.hotkey_manager_py import HotkeyManager
                    self.fallback_modules[module_name] = HotkeyManager
                else:
                    from core.hotkey_manager import HotkeyManager
                    self.fallback_modules[module_name] = HotkeyManager
                    
            elif module_name == 'clipboard_manager':
                if os.path.exists(os.path.join(os.path.dirname(__file__), 'fallbacks', 'clipboard_manager_py.py')):
                    from core.fallbacks.clipboard_manager_py import ClipboardManager
                    self.fallback_modules[module_name] = ClipboardManager
                else:
                    from core.clipboard_manager import ClipboardManager
                    self.fallback_modules[module_name] = ClipboardManager
                    
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
    
    def get_speech_recognizer(self):
        """Получение экземпляра SpeechRecognizer (C++ или Python)."""
        if self.extensions_available['speech_recognizer']:
            return self.modules['speech_recognizer'].SpeechRecognizer
        return self.fallback_modules.get('speech_recognizer')
    
    def get_hotkey_manager(self):
        """Получение экземпляра HotkeyManager (C++ или Python)."""
        if self.extensions_available['hotkey_manager']:
            return self.modules['hotkey_manager'].HotkeyManager
        return self.fallback_modules.get('hotkey_manager')
    
    def get_clipboard_manager(self):
        """Получение экземпляра ClipboardManager (C++ или Python)."""
        if self.extensions_available['clipboard_manager']:
            return self.modules['clipboard_manager'].ClipboardManager
        return self.fallback_modules.get('clipboard_manager')
    
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

def get_speech_recognizer():
    return hybrid_manager.get_speech_recognizer()

def get_hotkey_manager():
    return hybrid_manager.get_hotkey_manager()

def get_clipboard_manager():
    return hybrid_manager.get_clipboard_manager()

def get_mode_info():
    return hybrid_manager.get_mode_info()

def is_hybrid_available():
    return hybrid_manager.is_hybrid_mode_available() 