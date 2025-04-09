#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Audio processing module for the voice dictation application.
Provides functionality for audio capture and processing.
"""

import numpy as np
import pyaudio
import wave
import logging
from typing import Optional, Tuple
from PyQt6.QtCore import QObject, pyqtSignal

# ==== LOGGER SETUP ====
logger = logging.getLogger('voice_dictation.audio_processor')

# ==== CONSTANTS ====
CHUNK_SIZE = 1024
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 16000
SILENCE_THRESHOLD = 500
SILENCE_DURATION = 1.0

# ==== AUDIO PROCESSOR CLASS ====
class AudioProcessor(QObject):
    """
    Handles audio capture and processing.
    """
    audio_level = pyqtSignal(float)  # Level from 0.0 to 1.0
    audio_data = pyqtSignal(bytes)  # Raw audio data
    error_occurred = pyqtSignal(str)
    
    def __init__(self):
        """Initialize the audio processor."""
        super().__init__()
        self.audio = pyaudio.PyAudio()
        self.stream = None
        self.is_recording = False
        self.silence_counter = 0
        self.silence_threshold = SILENCE_THRESHOLD
        self.silence_duration = SILENCE_DURATION
        
    def __del__(self):
        """Cleanup resources."""
        self.stop_recording()
        self.audio.terminate()
        
    def start_recording(self) -> bool:
        """
        Start audio recording.
        
        Returns:
            bool: Success status
        """
        try:
            if self.stream is not None:
                self.stop_recording()
                
            self.stream = self.audio.open(
                format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                input=True,
                frames_per_buffer=CHUNK_SIZE,
                stream_callback=self._audio_callback
            )
            
            self.is_recording = True
            self.silence_counter = 0
            logger.info("Audio recording started")
            return True
            
        except Exception as e:
            error_msg = f"Error starting audio recording: {e}"
            logger.error(error_msg)
            self.error_occurred.emit(error_msg)
            return False
            
    def stop_recording(self) -> bool:
        """
        Stop audio recording.
        
        Returns:
            bool: Success status
        """
        try:
            if self.stream is not None:
                self.stream.stop_stream()
                self.stream.close()
                self.stream = None
                
            self.is_recording = False
            logger.info("Audio recording stopped")
            return True
            
        except Exception as e:
            error_msg = f"Error stopping audio recording: {e}"
            logger.error(error_msg)
            self.error_occurred.emit(error_msg)
            return False
            
    def _audio_callback(self, in_data, frame_count, time_info, status):
        """
        Callback for audio stream processing.
        
        Args:
            in_data: Input audio data
            frame_count: Number of frames
            time_info: Time information
            status: Status flags
            
        Returns:
            Tuple: (None, pyaudio.paContinue)
        """
        try:
            if self.is_recording:
                # Calculate audio level
                audio_data = np.frombuffer(in_data, dtype=np.int16)
                level = np.abs(audio_data).mean() / 32768.0
                self.audio_level.emit(level)
                
                # Check for silence
                if level < self.silence_threshold / 32768.0:
                    self.silence_counter += 1
                    if self.silence_counter >= int(self.silence_duration * RATE / CHUNK_SIZE):
                        self.stop_recording()
                else:
                    self.silence_counter = 0
                    
                # Emit audio data
                self.audio_data.emit(in_data)
                
        except Exception as e:
            logger.error(f"Error in audio callback: {e}")
            
        return (None, pyaudio.paContinue)
        
    def save_audio(self, data: bytes, file_path: str) -> bool:
        """
        Save audio data to a WAV file.
        
        Args:
            data (bytes): Audio data
            file_path (str): Output file path
            
        Returns:
            bool: Success status
        """
        try:
            with wave.open(file_path, 'wb') as wf:
                wf.setnchannels(CHANNELS)
                wf.setsampwidth(self.audio.get_sample_size(FORMAT))
                wf.setframerate(RATE)
                wf.writeframes(data)
            return True
        except Exception as e:
            error_msg = f"Error saving audio file: {e}"
            logger.error(error_msg)
            self.error_occurred.emit(error_msg)
            return False
            
    def load_audio(self, file_path: str) -> Optional[bytes]:
        """
        Load audio data from a WAV file.
        
        Args:
            file_path (str): Input file path
            
        Returns:
            Optional[bytes]: Audio data or None on error
        """
        try:
            with wave.open(file_path, 'rb') as wf:
                data = wf.readframes(wf.getnframes())
            return data
        except Exception as e:
            error_msg = f"Error loading audio file: {e}"
            logger.error(error_msg)
            self.error_occurred.emit(error_msg)
            return None
            
    def set_silence_threshold(self, threshold: int) -> None:
        """
        Set silence detection threshold.
        
        Args:
            threshold (int): New threshold value
        """
        self.silence_threshold = threshold
        
    def set_silence_duration(self, duration: float) -> None:
        """
        Set silence duration threshold.
        
        Args:
            duration (float): New duration in seconds
        """
        self.silence_duration = duration 