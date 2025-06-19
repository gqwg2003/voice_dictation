import speech_recognition as sr
import json
import os
import sys
from langdetect import detect, DetectorFactory
import requests
import base64
import tempfile
import uuid
from pydub import AudioSegment
import io
import numpy as np
import sounddevice as sd
import re
import time

DetectorFactory.seed = 0

class SpeechRecognizer:
    def __init__(self, api_key=None, proxy=None, region="westus"):
        """
        Initialize speech recognizer with optional API key and proxy settings.
        
        Args:
            api_key (str, optional): API key for speech recognition service
            proxy (dict, optional): Proxy configuration in the format {"http": "http://...", "https": "https://..."}
            region (str, optional): Microsoft Azure region for speech services
        """
        self.recognizer = sr.Recognizer()
        self.api_key = api_key
        self.proxy = proxy
        self.region = region
        self.session = requests.Session()
        self.language = "ru-RU"  # Default language
        
        if proxy:
            self.session.proxies.update(proxy)
    
    def set_language(self, language):
        """
        Set the recognition language.
        
        Args:
            language (str): Language code (e.g., 'ru-RU', 'en-US')
            
        Returns:
            bool: True if language was set successfully
        """
        if not language:
            return False
        self.language = language
        print(f"Language set to: {language}")
        return True
    
    def update_config(self, apiKey=None, proxy=None, region=None):
        """
        Update recognizer configuration.
        
        Args:
            apiKey (str, optional): API key for speech recognition service
            proxy (dict, optional): Proxy configuration
            region (str, optional): Region for Microsoft services
            
        Returns:
            bool: True if configuration was updated successfully
        """
        if apiKey is not None:
            self.api_key = apiKey
        if proxy is not None:
            self.proxy = proxy
            if proxy:
                self.session.proxies.update(proxy)
            else:
                self.session.proxies.clear()
        if region is not None:
            self.region = region
        
        print(f"Configuration updated. API key: {'Set' if self.api_key else 'Not set'}, "
              f"Proxy: {'Set' if self.proxy else 'Not set'}, "
              f"Region: {self.region}")
        return True
    
    def recognize_from_microphone(self, language=None, duration=5):
        """
        Record audio from microphone and recognize speech.
        
        Args:
            language (str, optional): Language code ('ru-RU', 'en-US'), or None for auto-detection
            duration (int, optional): Duration to record in seconds
            
        Returns:
            dict: Recognition result with text, language, and confidence
        """
        if language is None:
            language = self.language
        
        with sr.Microphone() as source:
            self.recognizer.adjust_for_ambient_noise(source)
            print(f"Recording for {duration} seconds...")
            audio = self.recognizer.record(source, duration=duration)
        
        return self._process_audio(audio, language)
    
    def recognize_from_file(self, file_path, language=None):
        """
        Recognize speech from audio file.
        
        Args:
            file_path (str): Path to audio file
            language (str, optional): Language code ('ru-RU', 'en-US'), or None for auto-detection
            
        Returns:
            dict: Recognition result with text, language, and confidence
        """
        if language is None:
            language = self.language
        
        with sr.AudioFile(file_path) as source:
            audio = self.recognizer.record(source)
        
        return self._process_audio(audio, language)
    
    def _process_audio(self, audio, language=None):
        """
        Process audio data and perform speech recognition.
        
        Args:
            audio: Audio data from recognizer
            language (str, optional): Language code or None for auto-detection
            
        Returns:
            dict: Recognition result with text, language, and confidence
        """
        wav_data = audio.get_wav_data()
        audio_segment = AudioSegment.from_wav(io.BytesIO(wav_data))
        
        if not language:
            detected_lang = self._detect_language_from_audio(audio_segment)
            language = "ru-RU" if detected_lang == "ru" else "en-US"
        
        try:
            if self.api_key:
                result = self._recognize_with_api(audio, language)
            else:
                text = self.recognizer.recognize_google(audio, language=language)
                result = {"text": text, "language": language, "confidence": 0.8}
            
            result["text"] = self._process_mixed_language_text(result["text"], language)
            return result
        except sr.UnknownValueError:
            return {"error": "Could not understand audio", "language": language, "confidence": 0.0}
        except sr.RequestError as e:
            return {"error": f"Recognition service error: {str(e)}", "language": language, "confidence": 0.0}
        except Exception as e:
            return {"error": f"Error: {str(e)}", "language": language, "confidence": 0.0}
    
    def _detect_language_from_audio(self, audio_segment):
        """
        Detect language from audio sample.
        
        Args:
            audio_segment: Audio segment from pydub
            
        Returns:
            str: Detected language code ('ru' or 'en')
        """
        sample_duration = min(2000, len(audio_segment))
        sample = audio_segment[:sample_duration]
        
        with tempfile.NamedTemporaryFile(suffix='.wav', delete=False) as temp_file:
            sample_path = temp_file.name
            sample.export(sample_path, format="wav")
        
        try:
            with sr.AudioFile(sample_path) as source:
                sample_audio = self.recognizer.record(source)
            
            try:
                sample_text = self.recognizer.recognize_google(sample_audio)
                detected_lang = detect(sample_text)
                return detected_lang
            except:
                return "en"
        finally:
            if os.path.exists(sample_path):
                os.unlink(sample_path)
    
    def _recognize_with_api(self, audio, language):
        """
        Recognize speech using the configured API key.
        
        Args:
            audio: Audio data
            language (str): Language code
            
        Returns:
            dict: Recognition result
        """
        if self.api_key and self.api_key.startswith("microsoft:"):
            api_key = self.api_key.split(":", 1)[1]
            return self._recognize_with_microsoft(audio, language, api_key)
        
        # Default to Google's API
        text = self.recognizer.recognize_google(audio, language=language)
        return {"text": text, "language": language, "confidence": 0.8}
    
    def _recognize_with_microsoft(self, audio, language, api_key):
        """
        Recognize speech using Microsoft Cognitive Services.
        
        Args:
            audio: Audio data
            language (str): Language code
            api_key (str): Microsoft API key
            
        Returns:
            dict: Recognition result
        """
        endpoint = f"https://{self.region}.api.cognitive.microsoft.com/sts/v1.0/issueToken"
        
        headers = {
            'Ocp-Apim-Subscription-Key': api_key,
            'Content-type': 'application/x-www-form-urlencoded',
            'Content-Length': '0'
        }
        
        try:
            response = self.session.post(endpoint, headers=headers)
            response.raise_for_status()
            
            access_token = response.text
            wav_data = audio.get_wav_data()
            
            speech_endpoint = f"https://{self.region}.stt.speech.microsoft.com/speech/recognition/conversation/cognitiveservices/v1"
            
            speech_headers = {
                'Authorization': f'Bearer {access_token}',
                'Content-type': 'audio/wav; codec=audio/pcm; samplerate=16000',
                'Accept': 'application/json'
            }
            
            params = {
                'language': language,
                'format': 'detailed'
            }
            
            speech_response = self.session.post(speech_endpoint, headers=speech_headers, 
                                               params=params, data=wav_data)
            speech_response.raise_for_status()
            
            result = speech_response.json()
            
            if result.get('RecognitionStatus') == 'Success':
                text = result.get('DisplayText', '')
                confidence = 0.8
                
                # Try to extract confidence from NBest results if available
                if 'NBest' in result and len(result['NBest']) > 0:
                    confidence = result['NBest'][0].get('Confidence', 0.8)
                
                return {
                    "text": text,
                    "language": language,
                    "confidence": confidence
                }
            else:
                return {"error": f"Recognition failed: {result.get('RecognitionStatus', 'Unknown')}", 
                        "language": language, "confidence": 0.0}
                
        except Exception as e:
            return {"error": f"Microsoft API error: {str(e)}", "language": language, "confidence": 0.0}
    
    def _process_mixed_language_text(self, text, primary_language):
        """
        Process text that might contain mixed language content.
        
        Args:
            text (str): Text to process
            primary_language (str): Primary language of the text
            
        Returns:
            str: Processed text
        """
        if not text:
            return text
            
        # Define character sets for common scripts
        cyrillic = set("абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ")
        latin = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
        
        # Check if we have mixed scripts
        has_cyrillic = any(c in cyrillic for c in text)
        has_latin = any(c in latin for c in text)
        
        if not (has_cyrillic and has_latin):
            return text
            
        words = text.split()
        processed_words = []
        
        for word in words:
            if not word:
                processed_words.append(word)
                continue
                
            # Count characters from each script
            cyrillic_count = sum(1 for c in word if c in cyrillic)
            latin_count = sum(1 for c in word if c in latin)
            
            # Determine predominant script in this word
            is_cyrillic = cyrillic_count > latin_count
            is_latin = latin_count > cyrillic_count
            
            # Process based on primary language
            if primary_language.startswith(('ru', 'ru-')):
                # For Russian as primary language
                if is_latin and not self._is_likely_technical_term(word):
                    # Consider transliteration for non-technical terms
                    # Note: transliteration not fully implemented in this version
                    processed_words.append(word)
                else:
                    processed_words.append(word)
            else:
                # For English as primary language
                processed_words.append(word)
                
        return ' '.join(processed_words)
    
    def _is_likely_technical_term(self, word):
        """
        Determine if a word is likely a technical term that should be preserved.
        
        Args:
            word (str): Word to check
            
        Returns:
            bool: True if the word is likely a technical term
        """
        if not word or len(word) < 2:
            return False
            
        # Common technical markers/abbreviations
        tech_markers = [
            'api', 'sdk', 'http', 'json', 'xml', 'url', 'sql', 'db', 
            'app', 'dev', 'git', 'npm', 'css', 'html', 'js', 'py',
            'ui', 'cli', 'id', 'ip', 'yaml', 'toml', 'ssl', 'ssh',
            'ftp', 'rest', 'oauth', 'jwt', 'spa', 'cdn', 'cors'
        ]
        
        word_lower = word.lower()
        
        # Check for technical markers
        for marker in tech_markers:
            if marker in word_lower:
                return True
                
        # Check for camelCase or PascalCase (common in programming)
        has_lower = any(c.islower() for c in word)
        has_upper = any(c.isupper() for c in word)
        has_digit = any(c.isdigit() for c in word)
        
        # CamelCase check - lowercase followed by uppercase
        if has_lower and has_upper:
            for i in range(1, len(word)):
                if word[i-1].islower() and word[i].isupper():
                    return True
                    
        # Check for words with digits (often technical)
        if has_digit and (has_lower or has_upper):
            return True
            
        # Check for words with underscores or hyphens (common in programming)
        if '_' in word or '-' in word:
            return True
            
        return False
    
    def _transliterate_latin_to_cyrillic(self, text):
        """
        Transliterate Latin characters to Cyrillic.
        
        Args:
            text (str): Text to transliterate
            
        Returns:
            str: Transliterated text
        """
        # Mapping from Latin to Cyrillic
        translit_map = {
            'a': 'а', 'b': 'б', 'v': 'в', 'g': 'г', 'd': 'д', 'e': 'е',
            'yo': 'ё', 'zh': 'ж', 'z': 'з', 'i': 'и', 'j': 'й', 'k': 'к',
            'l': 'л', 'm': 'м', 'n': 'н', 'o': 'о', 'p': 'п', 'r': 'р',
            's': 'с', 't': 'т', 'u': 'у', 'f': 'ф', 'h': 'х', 'ts': 'ц',
            'ch': 'ч', 'sh': 'ш', 'sch': 'щ', 'y': 'ы', 'e': 'э', 'yu': 'ю',
            'ya': 'я',
            # Capital letters
            'A': 'А', 'B': 'Б', 'V': 'В', 'G': 'Г', 'D': 'Д', 'E': 'Е',
            'Yo': 'Ё', 'Zh': 'Ж', 'Z': 'З', 'I': 'И', 'J': 'Й', 'K': 'К',
            'L': 'Л', 'M': 'М', 'N': 'Н', 'O': 'О', 'P': 'П', 'R': 'Р',
            'S': 'С', 'T': 'Т', 'U': 'У', 'F': 'Ф', 'H': 'Х', 'Ts': 'Ц',
            'Ch': 'Ч', 'Sh': 'Ш', 'Sch': 'Щ', 'Y': 'Ы', 'E': 'Э', 'Yu': 'Ю',
            'Ya': 'Я'
        }
        
        # Simple implementation - would need more sophisticated logic for real use
        result = text
        for lat, cyr in sorted(translit_map.items(), key=lambda x: len(x[0]), reverse=True):
            result = result.replace(lat, cyr)
            
        return result
    
    def record_audio(self, duration=5, sample_rate=16000):
        """
        Record audio from microphone.
        
        Args:
            duration (int): Duration to record in seconds
            sample_rate (int): Sample rate in Hz
            
        Returns:
            np.ndarray: Recorded audio data
        """
        print(f"Recording {duration} seconds of audio...")
        audio_data = sd.rec(int(duration * sample_rate), 
                           samplerate=sample_rate, channels=1, dtype='int16')
        sd.wait()
        return audio_data
    
    def get_available_languages(self):
        """
        Get available language options.
        
        Returns:
            list: List of available language codes and names
        """
        return [
            {"code": "ru-RU", "name": "Russian"},
            {"code": "en-US", "name": "English (US)"},
            {"code": "en-GB", "name": "English (UK)"},
            {"code": "fr-FR", "name": "French"},
            {"code": "de-DE", "name": "German"},
            {"code": "es-ES", "name": "Spanish"}
        ]

# Command-line interface for testing
if __name__ == "__main__":
    recognizer = SpeechRecognizer()
    recognizer.set_language("en-US")
    print(f"Current language: {recognizer.language}")
    
    result = recognizer.recognize_from_microphone(duration=5)
    print(json.dumps(result, ensure_ascii=False, indent=2)) 