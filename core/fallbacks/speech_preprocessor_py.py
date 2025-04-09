#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Fallback Python implementation of SpeechPreprocessor for systems where C++ extensions are not available.
This module provides the same functionality as the C++ version but with potentially lower performance.
"""

import numpy as np
import struct
import logging
from typing import List, Union, Tuple

# Setup logging
logger = logging.getLogger(__name__)

class SpeechPreprocessor:
    """Python fallback implementation for speech preprocessing operations"""
    
    @staticmethod
    def noise_reduction(audio_data: List[float], noise_level: float = 0.01) -> List[float]:
        """
        Apply noise reduction using spectral subtraction.
        
        Args:
            audio_data: List of float audio samples
            noise_level: Estimated noise level (0.0-1.0)
            
        Returns:
            List of processed audio samples
        """
        if not audio_data:
            return audio_data
            
        result = []
        
        # Simple spectral subtraction (simplified algorithm)
        for value in audio_data:
            sign = 1.0 if value >= 0.0 else -1.0
            abs_value = abs(value)
            
            # Apply threshold-based noise suppression
            if abs_value < noise_level:
                result.append(0.0)
            else:
                # Subtract estimated noise level
                result.append(sign * (abs_value - noise_level))
        
        return result
    
    @staticmethod
    def enhance_speech(raw_data: bytes, noise_level: float = 0.01) -> bytes:
        """
        Enhance speech quality for better recognition.
        
        Args:
            raw_data: Raw audio data bytes (16-bit PCM format)
            noise_level: Noise level for reduction
            
        Returns:
            Enhanced audio data bytes
        """
        if not raw_data:
            return raw_data
            
        # Convert bytes to int16 samples
        samples = []
        for i in range(0, len(raw_data), 2):
            if i + 1 < len(raw_data):
                sample = struct.unpack('<h', raw_data[i:i+2])[0]
                samples.append(sample)
        
        if not samples:
            return raw_data
            
        # Convert to float for processing
        float_data = []
        int16_max = 32768.0
        
        for sample in samples:
            float_data.append(float(sample) / int16_max)
        
        # Apply noise reduction
        processed = SpeechPreprocessor.noise_reduction(float_data, noise_level)
        
        # Normalize result
        max_val = max([abs(value) for value in processed]) if processed else 0.0
        
        if max_val > 0.0:
            gain = 0.95 / max_val  # Leave a small margin
            processed = [value * gain for value in processed]
        
        # Convert back to int16
        output_samples = []
        for value in processed:
            output_samples.append(int(value * int16_max))
        
        # Convert to bytes
        output_bytes = bytearray()
        for sample in output_samples:
            output_bytes.extend(struct.pack('<h', sample))
        
        return bytes(output_bytes)
    
    @staticmethod
    def trim_silence(raw_data: bytes, threshold: float = 0.02) -> bytes:
        """
        Trim silence from the beginning and end of an audio recording.
        
        Args:
            raw_data: Raw audio data bytes (16-bit PCM format)
            threshold: Silence threshold (0.0-1.0)
            
        Returns:
            Trimmed audio data bytes
        """
        if not raw_data:
            return raw_data
            
        # Convert bytes to int16 samples
        samples = []
        for i in range(0, len(raw_data), 2):
            if i + 1 < len(raw_data):
                sample = struct.unpack('<h', raw_data[i:i+2])[0]
                samples.append(sample)
        
        if not samples:
            return raw_data
        
        # Find start and end of speech
        start = 0
        end = len(samples)
        int16_max = 32768.0
        abs_threshold = threshold * int16_max
        
        # Find start of sound
        for i, sample in enumerate(samples):
            if abs(float(sample)) > abs_threshold:
                start = max(0, i - 1000)  # Leave a small margin at the start
                break
        
        # Find end of sound
        for i in range(len(samples) - 1, -1, -1):
            if abs(float(samples[i])) > abs_threshold:
                end = min(i + 1000, len(samples))  # Leave a small margin at the end
                break
        
        # If no significant sound or all sound is significant
        if start >= end or (start == 0 and end == len(samples)):
            return raw_data
        
        # Extract only the significant sound
        trimmed_samples = samples[start:end]
        
        # Convert to bytes
        output_bytes = bytearray()
        for sample in trimmed_samples:
            output_bytes.extend(struct.pack('<h', sample))
        
        return bytes(output_bytes)
    
    @staticmethod
    def optimize_for_recognition(raw_data: bytes, noise_level: float = 0.01, silence_threshold: float = 0.02) -> bytes:
        """
        Apply comprehensive optimization for speech recognition.
        
        Args:
            raw_data: Raw audio data bytes
            noise_level: Noise level for reduction
            silence_threshold: Threshold for silence detection
            
        Returns:
            Optimized audio data bytes
        """
        try:
            # First enhance speech quality
            enhanced = SpeechPreprocessor.enhance_speech(raw_data, noise_level)
            
            # Then trim silence
            trimmed = SpeechPreprocessor.trim_silence(enhanced, silence_threshold)
            
            return trimmed
        except Exception as e:
            logger.error(f"Error in optimize_for_recognition: {e}")
            return raw_data 