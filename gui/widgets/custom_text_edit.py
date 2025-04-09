#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Custom text edit widget for the voice dictation application.
Provides enhanced text editing functionality with custom styling and features.
"""

from PyQt6.QtWidgets import QTextEdit
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QFont, QTextCursor

class CustomTextEdit(QTextEdit):
    """
    Enhanced text edit widget with custom styling and features.
    """
    text_changed = pyqtSignal(str)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFont(QFont("Segoe UI", 12))
        self.setMinimumHeight(200)
        self.setMinimumWidth(400)
        self.setLineWrapMode(QTextEdit.LineWrapMode.WidgetWidth)
        self.setAcceptRichText(False)
        self.textChanged.connect(self._on_text_changed)
        
    def _on_text_changed(self):
        """Emit signal when text changes."""
        self.text_changed.emit(self.toPlainText())
        
    def insert_text(self, text):
        """Insert text at cursor position."""
        cursor = self.textCursor()
        cursor.insertText(text)
        self.setTextCursor(cursor)
        
    def clear_text(self):
        """Clear all text."""
        self.clear()
        
    def get_text(self):
        """Get current text."""
        return self.toPlainText()
        
    def set_text(self, text):
        """Set text content."""
        self.setPlainText(text)
        
    def append_text(self, text):
        """Append text to the end."""
        self.moveCursor(QTextCursor.MoveOperation.End)
        self.insert_text(text)
        
    def get_selected_text(self):
        """Get currently selected text."""
        return self.textCursor().selectedText()
        
    def set_read_only(self, read_only):
        """Set read-only mode."""
        self.setReadOnly(read_only)
        
    def set_placeholder(self, text):
        """Set placeholder text."""
        self.setPlaceholderText(text) 