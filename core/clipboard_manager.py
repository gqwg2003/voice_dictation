#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Clipboard manager module for the voice dictation application.
Provides secure clipboard operations with input validation and error handling.
"""

# ==== IMPORTS ====
import logging
import pyperclip
import re
from PyQt6.QtCore import QObject, pyqtSignal

# ==== LOGGER SETUP ====
logger = logging.getLogger('voice_dictation.clipboard_manager')

# ==== CONSTANTS ====
MAX_CLIPBOARD_SIZE = 1024 * 1024  # 1MB maximum size for clipboard data
MAX_LINE_LENGTH = 1000  # Maximum length for a single line
MAX_LINES = 1000  # Maximum number of lines
INVALID_CHARACTERS = re.compile(r'[\x00-\x08\x0B\x0C\x0E-\x1F\x7F-\x9F]')  # Control characters

# ==== CLIPBOARD MANAGER CLASS ====
class ClipboardManager(QObject):
    """
    Module for secure clipboard operations with input validation.
    """
    text_copied = pyqtSignal(str)
    error_occurred = pyqtSignal(str)
    
    def __init__(self):
        """Initialize the clipboard manager."""
        super().__init__()
        logger.debug("Clipboard manager initialized")
    
    def copy_to_clipboard(self, text):
        """
        Copy text to system clipboard with input validation.
        
        Args:
            text (str): Text to copy to clipboard
            
        Returns:
            bool: Success status
        """
        try:
            # Input validation
            if not isinstance(text, str):
                error_msg = f"Invalid clipboard data type: {type(text)}"
                logger.error(error_msg)
                self.error_occurred.emit(error_msg)
                return False
                
            # Check for empty text
            if not text:
                error_msg = "Attempt to copy empty text to clipboard"
                logger.warning(error_msg)
                self.error_occurred.emit(error_msg)
                return False
                
            # Check size limit to prevent memory issues
            if len(text) > MAX_CLIPBOARD_SIZE:
                error_msg = f"Clipboard text too large: {len(text)} bytes (max: {MAX_CLIPBOARD_SIZE})"
                logger.error(error_msg)
                self.error_occurred.emit(error_msg)
                return False
            
            # Check line length and count
            lines = text.splitlines()
            if len(lines) > MAX_LINES:
                error_msg = f"Too many lines: {len(lines)} (max: {MAX_LINES})"
                logger.error(error_msg)
                self.error_occurred.emit(error_msg)
                return False
                
            for i, line in enumerate(lines, 1):
                if len(line) > MAX_LINE_LENGTH:
                    error_msg = f"Line {i} too long: {len(line)} characters (max: {MAX_LINE_LENGTH})"
                    logger.error(error_msg)
                    self.error_occurred.emit(error_msg)
                    return False
            
            # Sanitize text (remove control characters)
            sanitized_text = INVALID_CHARACTERS.sub('', text)
            if sanitized_text != text:
                logger.warning("Text contained control characters that were removed")
                text = sanitized_text
            
            # Copy to clipboard
            try:
                pyperclip.copy(text)
                self.text_copied.emit(text)
                logger.debug(f"Copied {len(text)} characters to clipboard")
                return True
            except Exception as e:
                error_msg = f"Error copying to clipboard: {e}"
                logger.error(error_msg)
                self.error_occurred.emit(error_msg)
                return False
                
        except Exception as e:
            error_msg = f"Unexpected error in copy_to_clipboard: {e}"
            logger.error(error_msg)
            self.error_occurred.emit(error_msg)
            return False
    
    def get_clipboard_content(self):
        """
        Get current text from system clipboard with error handling.
        
        Returns:
            str: Current clipboard text or empty string on error
        """
        try:
            content = pyperclip.paste()
            
            # Input validation
            if not isinstance(content, str):
                logger.error(f"Invalid clipboard content type: {type(content)}")
                return ""
            
            # Size validation for retrieved content
            if len(content) > MAX_CLIPBOARD_SIZE:
                logger.warning(f"Retrieved clipboard content exceeds size limit: {len(content)} bytes")
                # Truncate for safety
                content = content[:MAX_CLIPBOARD_SIZE]
            
            # Sanitize content
            content = INVALID_CHARACTERS.sub('', content)
            
            return content
            
        except Exception as e:
            logger.error(f"Error getting clipboard content: {e}")
            return ""
    
    def clear_clipboard(self):
        """
        Clear the system clipboard.
        
        Returns:
            bool: Success status
        """
        try:
            pyperclip.copy('')
            logger.debug("Clipboard cleared")
            return True
        except Exception as e:
            error_msg = f"Error clearing clipboard: {e}"
            logger.error(error_msg)
            self.error_occurred.emit(error_msg)
            return False 