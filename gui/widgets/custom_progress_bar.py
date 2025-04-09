#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Custom progress bar widget for the voice dictation application.
Provides enhanced progress visualization with custom styling and animations.
"""

from PyQt6.QtWidgets import QProgressBar
from PyQt6.QtCore import Qt, QTimer, pyqtSignal
from PyQt6.QtGui import QPainter, QColor, QLinearGradient

class CustomProgressBar(QProgressBar):
    """
    Enhanced progress bar widget with custom styling and animations.
    """
    progress_changed = pyqtSignal(int)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimum(0)
        self.setMaximum(100)
        self.setValue(0)
        self.setMinimumHeight(20)
        self.setMinimumWidth(200)
        self.setTextVisible(True)
        self.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        # Animation timer
        self._animation_timer = QTimer(self)
        self._animation_timer.timeout.connect(self._update_animation)
        self._animation_value = 0
        self._is_animating = False
        
    def start_animation(self):
        """Start progress animation."""
        if not self._is_animating:
            self._is_animating = True
            self._animation_timer.start(50)  # 20 FPS
            
    def stop_animation(self):
        """Stop progress animation."""
        if self._is_animating:
            self._is_animating = False
            self._animation_timer.stop()
            
    def _update_animation(self):
        """Update animation value."""
        self._animation_value = (self._animation_value + 1) % 100
        self.update()
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Draw background
        painter.fillRect(self.rect(), QColor(240, 240, 240))
        
        # Calculate progress rect
        progress_width = int(self.width() * self.value() / 100)
        progress_rect = self.rect().adjusted(0, 0, -self.width() + progress_width, 0)
        
        # Create gradient
        gradient = QLinearGradient(progress_rect.topLeft(), progress_rect.topRight())
        gradient.setColorAt(0, QColor(100, 150, 255))
        gradient.setColorAt(1, QColor(50, 100, 200))
        
        # Draw progress
        painter.fillRect(progress_rect, gradient)
        
        # Draw animation overlay if animating
        if self._is_animating:
            animation_width = int(self.width() * 0.1)  # 10% of width
            animation_pos = int(self.width() * self._animation_value / 100)
            animation_rect = self.rect().adjusted(
                animation_pos - animation_width, 0,
                animation_pos, 0
            )
            painter.fillRect(animation_rect, QColor(255, 255, 255, 100))
            
        # Draw border
        painter.setPen(QColor(180, 180, 180))
        painter.drawRect(self.rect().adjusted(0, 0, -1, -1))
        
        # Draw text
        painter.setPen(QColor(0, 0, 0))
        painter.drawText(self.rect(), Qt.AlignmentFlag.AlignCenter, f"{self.value()}%")
        
    def set_value(self, value):
        """Set progress value."""
        self.setValue(value)
        self.progress_changed.emit(value)
        
    def reset(self):
        """Reset progress to 0."""
        self.setValue(0)
        self.progress_changed.emit(0) 