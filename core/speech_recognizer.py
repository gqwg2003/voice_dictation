#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Speech recognition module for the voice dictation application.
Provides functionality to convert speech to text using Google's recognition service.
"""

# ==== IMPORTS ====
# System imports
import threading
import os
import json
import sys
import logging
from contextlib import contextmanager
from typing import Dict, List, Optional, Union

# Third-party imports
import speech_recognition as sr
import numpy as np
from PyQt6.QtCore import QThread, pyqtSignal

# Add C++ extensions directory to Python path
current_dir = os.path.dirname(os.path.abspath(__file__))
cpp_extensions_dir = os.path.join(current_dir, 'cpp_extensions', 'build', 'Release')
if cpp_extensions_dir not in sys.path:
    sys.path.append(cpp_extensions_dir)

# C++ extensions
try:
    from audio_processor import AudioProcessor
    from text_processor import TextProcessor
except ImportError as e:
    logging.error(f"Failed to import C++ extensions: {e}")
    AudioProcessor = None
    TextProcessor = None

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class SpeechRecognizer(QThread):
    """
    Module for speech recognition that runs in a separate thread to prevent 
    UI freezing during recognition.
    """
    # ==== SIGNALS ====
    recognized_text = pyqtSignal(str)
    error_occurred = pyqtSignal(str)
    listening_started = pyqtSignal()
    listening_ended = pyqtSignal()
    processing_started = pyqtSignal()
    processing_ended = pyqtSignal(bool)  # Success flag
    audio_level = pyqtSignal(float)  # Level from 0.0 to 1.0
    
    def __init__(self, language: str = 'ru-RU'):
        """
        Initialize the speech recognizer.
        
        Args:
            language (str): Language code for recognition (default: 'ru-RU')
        """
        super().__init__()
        try:
            self.recognizer = sr.Recognizer()
            self.language = language
            self.is_listening = False
            self._audio_monitor_thread = None
            self._stop_monitor = threading.Event()
            self._lock = threading.Lock()
            
            # Initialize data structures
            self.custom_terms: Dict[str, List[str]] = {}
            self.common_phrases: Dict[str, List[str]] = {
                'ru-RU': [],
                'en-US': []
            }
            
            # Load initial data
            self._load_custom_vocabulary()
            self._load_common_phrases()
            
            # Recognition settings with defaults
            self.settings = {
                'energy_threshold': 300,
                'dynamic_energy_threshold': True,
                'energy_adjustment_ratio': 1.5,
                'pause_threshold': 0.8,
                'non_speaking_duration': 0.5,
                'ambient_noise_duration': 1.0,
                'apply_audio_normalization': True,
                'phrase_similarity_threshold': 0.65,
            }
            
            logger.info(f"SpeechRecognizer initialized with language: {language}")
        except Exception as e:
            logger.error(f"Failed to initialize SpeechRecognizer: {e}")
            raise
    
    def __del__(self):
        """Cleanup resources when object is destroyed."""
        try:
            self.stop_listening()
            if self.isRunning():
                self.wait(1000)  # Wait up to 1 second for thread to finish
        except Exception as e:
            logger.error(f"Error during cleanup: {e}")
    
    # ==== PUBLIC METHODS ====
    def start(self) -> None:
        """Start the recognition thread if not already running."""
        try:
            if not self.isRunning():
                with self._lock:
                    self.is_listening = True
                super().start()
                logger.info("Recognition thread started")
        except Exception as e:
            logger.error(f"Failed to start recognition thread: {e}")
            self.error_occurred.emit(str(e))
    
    def stop_listening(self) -> None:
        """Stop the current listening session."""
        try:
            with self._lock:
                if self.is_listening:
                    self.is_listening = False
                    self._stop_audio_monitor()
                    logger.info("Listening stopped")
        except Exception as e:
            logger.error(f"Error stopping listening: {e}")
    
    def set_language(self, language_code: str) -> None:
        """
        Set the language for recognition.
        
        Args:
            language_code (str): Language code (e.g., 'ru-RU', 'en-US')
        """
        try:
            if language_code not in self.common_phrases:
                logger.warning(f"Unsupported language code: {language_code}")
                return
                
            self.language = language_code
            self._load_custom_vocabulary()
            self._load_common_phrases()
            logger.info(f"Language set to: {language_code}")
        except Exception as e:
            logger.error(f"Error setting language: {e}")
            self.error_occurred.emit(str(e))
    
    def add_custom_term(self, term: str, language_code: Optional[str] = None) -> None:
        """
        Add a custom term to the vocabulary.
        
        Args:
            term (str): The term to add
            language_code (str, optional): Language code. If None, uses current language
        """
        try:
            if not term or not isinstance(term, str):
                logger.warning("Invalid term provided")
                return
                
            if language_code is None:
                language_code = self.language
                
            if language_code not in self.custom_terms:
                self.custom_terms[language_code] = []
                
            if term not in self.custom_terms[language_code]:
                with self._lock:
                    self.custom_terms[language_code].append(term)
                    self._save_custom_vocabulary()
                logger.info(f"Added custom term: {term} for language: {language_code}")
        except Exception as e:
            logger.error(f"Error adding custom term: {e}")
            self.error_occurred.emit(str(e))
    
    def remove_custom_term(self, term: str, language_code: Optional[str] = None) -> None:
        """
        Remove a custom term from the vocabulary.
        
        Args:
            term (str): The term to remove
            language_code (str, optional): Language code. If None, uses current language
        """
        try:
            if language_code is None:
                language_code = self.language
                
            if language_code in self.custom_terms and term in self.custom_terms[language_code]:
                with self._lock:
                    self.custom_terms[language_code].remove(term)
                    self._save_custom_vocabulary()
                logger.info(f"Removed custom term: {term} for language: {language_code}")
        except Exception as e:
            logger.error(f"Error removing custom term: {e}")
            self.error_occurred.emit(str(e))
    
    def get_custom_terms(self, language_code: Optional[str] = None) -> List[str]:
        """
        Get all custom terms for a language.
        
        Args:
            language_code (str, optional): Language code. If None, uses current language
            
        Returns:
            list: List of custom terms
        """
        try:
            if language_code is None:
                language_code = self.language
                
            return self.custom_terms.get(language_code, [])
        except Exception as e:
            logger.error(f"Error getting custom terms: {e}")
            return []
    
    # ==== THREAD IMPLEMENTATION ====
    def run(self) -> None:
        """Main thread function that listens for speech and emits signals."""
        mic = None
        try:
            mic = sr.Microphone()
            with mic as source:
                self._configure_recognizer(source)
                self._start_recognition(source)
                
        except Exception as e:
            error_msg = f"Error in recognition thread: {str(e)}"
            logger.error(error_msg)
            self.error_occurred.emit(error_msg)
            self.processing_ended.emit(False)
        finally:
            if mic is not None:
                try:
                    mic.__exit__(None, None, None)
                except Exception as e:
                    logger.error(f"Error closing microphone: {e}")
    
    def _configure_recognizer(self, source: sr.Microphone) -> None:
        """Configure the recognizer with current settings."""
        try:
            self.recognizer.energy_threshold = self.settings['energy_threshold']
            self.recognizer.dynamic_energy_threshold = self.settings['dynamic_energy_threshold']
            self.recognizer.pause_threshold = self.settings['pause_threshold']
            self.recognizer.non_speaking_duration = self.settings['non_speaking_duration']
            
            self.recognizer.adjust_for_ambient_noise(
                source, 
                duration=max(self.settings['ambient_noise_duration'], 1.5)
            )
            
            if self.settings['energy_adjustment_ratio'] != 1.0:
                original_threshold = self.recognizer.energy_threshold
                adjusted_threshold = original_threshold / self.settings['energy_adjustment_ratio']
                min_threshold = 150
                self.recognizer.energy_threshold = max(adjusted_threshold, min_threshold)
                logger.info(f"Adjusted energy threshold: {original_threshold} -> {self.recognizer.energy_threshold}")
                
        except Exception as e:
            logger.error(f"Error configuring recognizer: {str(e)}")
            raise
    
    def _start_recognition(self, source: sr.Microphone) -> None:
        """Start the recognition process."""
        try:
            self.listening_started.emit()
            with self._lock:
                self.is_listening = True
            self._start_audio_monitor(source)
            
            audio = self.recognizer.listen(
                source, 
                timeout=10, 
                phrase_time_limit=8
            )
            
            self._stop_audio_monitor()
            self.listening_ended.emit()
            self.processing_started.emit()
            
            # Process the audio
            try:
                text = self.recognizer.recognize_google(
                    audio,
                    language=self.language,
                    show_all=False
                )
                
                # Apply post-processing if C++ extensions are available
                if TextProcessor is not None:
                    try:
                        text = TextProcessor.process(text, self.custom_terms.get(self.language, []))
                    except Exception as e:
                        logger.warning(f"Text processing failed: {e}")
                
                self.recognized_text.emit(text)
                self.processing_ended.emit(True)
                
            except sr.UnknownValueError:
                logger.warning("Speech recognition could not understand audio")
                self.error_occurred.emit("Не удалось распознать речь")
                self.processing_ended.emit(False)
            except sr.RequestError as e:
                logger.error(f"Could not request results from Google Speech Recognition service: {e}")
                self.error_occurred.emit("Ошибка сервиса распознавания речи")
                self.processing_ended.emit(False)
            except Exception as e:
                logger.error(f"Error during speech recognition: {e}")
                self.error_occurred.emit("Ошибка при распознавании речи")
                self.processing_ended.emit(False)
                
        except Exception as e:
            logger.error(f"Error in recognition process: {e}")
            self.error_occurred.emit(str(e))
            self.processing_ended.emit(False)
    
    def _start_audio_monitor(self, source: sr.Microphone) -> None:
        """Start monitoring audio levels."""
        try:
            self._stop_monitor.clear()
            self._audio_monitor_thread = threading.Thread(
                target=self._monitor_audio_level,
                args=(source,)
            )
            self._audio_monitor_thread.daemon = True
            self._audio_monitor_thread.start()
        except Exception as e:
            logger.error(f"Error starting audio monitor: {e}")
    
    def _stop_audio_monitor(self) -> None:
        """Stop monitoring audio levels."""
        try:
            self._stop_monitor.set()
            if self._audio_monitor_thread and self._audio_monitor_thread.is_alive():
                self._audio_monitor_thread.join(timeout=1.0)
        except Exception as e:
            logger.error(f"Error stopping audio monitor: {e}")
    
    def _monitor_audio_level(self, source: sr.Microphone) -> None:
        """Monitor audio levels and emit signals using C++ extension."""
        try:
            # Check if C++ extension is available
            if AudioProcessor is None:
                logger.error("AudioProcessor C++ extension is not available")
                return
                
            while not self._stop_monitor.is_set():
                try:
                    with source as s:
                        audio = self.recognizer.listen(s, timeout=0.1, phrase_time_limit=0.1)
                        if audio:
                            # Use C++ extension to calculate audio level
                            level = AudioProcessor.calculate_level(audio.get_raw_data())
                            self.audio_level.emit(level)
                except sr.WaitTimeoutError:
                    # Expected when no audio is detected
                    pass
                except Exception as e:
                    logger.error(f"Error in audio monitoring: {e}")
                    break
        except Exception as e:
            logger.error(f"Error in audio monitor thread: {e}")
    
    def _load_custom_vocabulary(self) -> None:
        """Load custom vocabulary from file."""
        try:
            vocab_file = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 
                                    'resources', 'custom_vocabulary.json')
            if os.path.exists(vocab_file):
                with open(vocab_file, 'r', encoding='utf-8') as f:
                    self.custom_terms = json.load(f)
        except Exception as e:
            logger.error(f"Error loading custom vocabulary: {e}")
    
    def _save_custom_vocabulary(self) -> None:
        """Save custom vocabulary to file."""
        try:
            vocab_file = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 
                                    'resources', 'custom_vocabulary.json')
            with open(vocab_file, 'w', encoding='utf-8') as f:
                json.dump(self.custom_terms, f, ensure_ascii=False, indent=2)
        except Exception as e:
            logger.error(f"Error saving custom vocabulary: {e}")
    
    def _load_common_phrases(self) -> None:
        """Load common phrases from file."""
        try:
            phrases_file = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 
                                      'resources', 'common_phrases.json')
            if os.path.exists(phrases_file):
                with open(phrases_file, 'r', encoding='utf-8') as f:
                    self.common_phrases = json.load(f)
        except Exception as e:
            logger.error(f"Error loading common phrases: {e}")
    
    def update_recognition_settings(self, settings_dict: Dict[str, Union[int, float, bool]]) -> None:
        """
        Update recognition settings.
        
        Args:
            settings_dict (dict): Dictionary of settings to update
        """
        try:
            with self._lock:
                self.settings.update(settings_dict)
            logger.info("Recognition settings updated")
        except Exception as e:
            logger.error(f"Error updating settings: {str(e)}")
            raise 