#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Text processing module for the voice dictation application.
Provides functionality for text processing and formatting.
"""

import re
import logging
from typing import List, Optional, Dict
from PyQt6.QtCore import QObject, pyqtSignal

# ==== LOGGER SETUP ====
logger = logging.getLogger('voice_dictation.text_processor')

# ==== CONSTANTS ====
MAX_LINE_LENGTH = 100
MAX_PARAGRAPH_LENGTH = 500
SENTENCE_ENDINGS = ['.', '!', '?', '...']
PARAGRAPH_SEPARATORS = ['\n\n', '\r\n\r\n']

# ==== TEXT PROCESSOR CLASS ====
class TextProcessor(QObject):
    """
    Handles text processing and formatting.
    """
    text_processed = pyqtSignal(str)
    error_occurred = pyqtSignal(str)
    
    def __init__(self):
        """Initialize the text processor."""
        super().__init__()
        self.custom_terms: Dict[str, List[str]] = {}
        self.common_phrases: Dict[str, List[str]] = {}
        
    def process_text(self, text: str, language: str = 'ru-RU') -> str:
        """
        Process and format text.
        
        Args:
            text (str): Input text
            language (str): Language code
            
        Returns:
            str: Processed text
        """
        try:
            # Basic text cleaning
            text = self._clean_text(text)
            
            # Apply custom terms
            text = self._apply_custom_terms(text, language)
            
            # Apply common phrases
            text = self._apply_common_phrases(text, language)
            
            # Format paragraphs
            text = self._format_paragraphs(text)
            
            # Format sentences
            text = self._format_sentences(text)
            
            # Final cleanup
            text = self._final_cleanup(text)
            
            self.text_processed.emit(text)
            return text
            
        except Exception as e:
            error_msg = f"Error processing text: {e}"
            logger.error(error_msg)
            self.error_occurred.emit(error_msg)
            return text
            
    def _clean_text(self, text: str) -> str:
        """
        Clean input text.
        
        Args:
            text (str): Input text
            
        Returns:
            str: Cleaned text
        """
        # Remove extra whitespace
        text = re.sub(r'\s+', ' ', text)
        
        # Remove control characters
        text = re.sub(r'[\x00-\x1F\x7F-\x9F]', '', text)
        
        # Remove leading/trailing whitespace
        text = text.strip()
        
        return text
        
    def _apply_custom_terms(self, text: str, language: str) -> str:
        """
        Apply custom terms to text.
        
        Args:
            text (str): Input text
            language (str): Language code
            
        Returns:
            str: Text with custom terms applied
        """
        if language in self.custom_terms:
            for term in self.custom_terms[language]:
                # Replace term with proper capitalization
                pattern = re.compile(re.escape(term), re.IGNORECASE)
                text = pattern.sub(term, text)
        return text
        
    def _apply_common_phrases(self, text: str, language: str) -> str:
        """
        Apply common phrases to text.
        
        Args:
            text (str): Input text
            language (str): Language code
            
        Returns:
            str: Text with common phrases applied
        """
        if language in self.common_phrases:
            for phrase in self.common_phrases[language]:
                # Replace phrase with proper capitalization
                pattern = re.compile(re.escape(phrase), re.IGNORECASE)
                text = pattern.sub(phrase, text)
        return text
        
    def _format_paragraphs(self, text: str) -> str:
        """
        Format paragraphs in text.
        
        Args:
            text (str): Input text
            
        Returns:
            str: Formatted text
        """
        # Split into paragraphs
        paragraphs = re.split('|'.join(PARAGRAPH_SEPARATORS), text)
        
        # Process each paragraph
        formatted_paragraphs = []
        for paragraph in paragraphs:
            # Clean paragraph
            paragraph = paragraph.strip()
            if not paragraph:
                continue
                
            # Split into lines if too long
            if len(paragraph) > MAX_PARAGRAPH_LENGTH:
                lines = self._split_into_lines(paragraph)
                formatted_paragraphs.extend(lines)
            else:
                formatted_paragraphs.append(paragraph)
                
        # Join paragraphs
        return '\n\n'.join(formatted_paragraphs)
        
    def _format_sentences(self, text: str) -> str:
        """
        Format sentences in text.
        
        Args:
            text (str): Input text
            
        Returns:
            str: Formatted text
        """
        # Split into sentences
        sentences = re.split(r'(?<=[.!?])\s+', text)
        
        # Process each sentence
        formatted_sentences = []
        for sentence in sentences:
            # Capitalize first letter
            if sentence:
                sentence = sentence[0].upper() + sentence[1:]
            formatted_sentences.append(sentence)
            
        # Join sentences
        return ' '.join(formatted_sentences)
        
    def _split_into_lines(self, text: str) -> List[str]:
        """
        Split text into lines of appropriate length.
        
        Args:
            text (str): Input text
            
        Returns:
            List[str]: List of lines
        """
        words = text.split()
        lines = []
        current_line = []
        current_length = 0
        
        for word in words:
            word_length = len(word)
            if current_length + word_length + len(current_line) > MAX_LINE_LENGTH:
                lines.append(' '.join(current_line))
                current_line = [word]
                current_length = word_length
            else:
                current_line.append(word)
                current_length += word_length
                
        if current_line:
            lines.append(' '.join(current_line))
            
        return lines
        
    def _final_cleanup(self, text: str) -> str:
        """
        Perform final text cleanup.
        
        Args:
            text (str): Input text
            
        Returns:
            str: Cleaned text
        """
        # Remove extra newlines
        text = re.sub(r'\n{3,}', '\n\n', text)
        
        # Remove extra spaces
        text = re.sub(r' +', ' ', text)
        
        # Remove spaces before punctuation
        text = re.sub(r'\s+([.,!?])', r'\1', text)
        
        # Add space after punctuation
        text = re.sub(r'([.,!?])([^\s])', r'\1 \2', text)
        
        return text.strip()
        
    def add_custom_term(self, term: str, language: str) -> None:
        """
        Add a custom term.
        
        Args:
            term (str): Term to add
            language (str): Language code
        """
        if language not in self.custom_terms:
            self.custom_terms[language] = []
        if term not in self.custom_terms[language]:
            self.custom_terms[language].append(term)
            
    def remove_custom_term(self, term: str, language: str) -> None:
        """
        Remove a custom term.
        
        Args:
            term (str): Term to remove
            language (str): Language code
        """
        if language in self.custom_terms and term in self.custom_terms[language]:
            self.custom_terms[language].remove(term)
            
    def add_common_phrase(self, phrase: str, language: str) -> None:
        """
        Add a common phrase.
        
        Args:
            phrase (str): Phrase to add
            language (str): Language code
        """
        if language not in self.common_phrases:
            self.common_phrases[language] = []
        if phrase not in self.common_phrases[language]:
            self.common_phrases[language].append(phrase)
            
    def remove_common_phrase(self, phrase: str, language: str) -> None:
        """
        Remove a common phrase.
        
        Args:
            phrase (str): Phrase to remove
            language (str): Language code
        """
        if language in self.common_phrases and phrase in self.common_phrases[language]:
            self.common_phrases[language].remove(phrase) 