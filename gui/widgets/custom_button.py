#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Custom button widget for the voice dictation application.
Provides enhanced button functionality with custom styling and animations.
"""

from PyQt6.QtWidgets import QPushButton
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QIcon, QPainter, QColor

class CustomButton(QPushButton):
    """
    Enhanced button widget with custom styling and animations.
    """
    def __init__(self, text="", icon=None, parent=None):
        super().__init__(text, parent)
        self.setCursor(Qt.CursorShape.PointingHandCursor)
        self.setIcon(icon)
        self.setIconSize(QSize(24, 24))
        self.setMinimumHeight(40)
        self.setMinimumWidth(100)
        
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Draw background
        if self.isDown():
            painter.fillRect(self.rect(), QColor(200, 200, 200))
        elif self.underMouse():
            painter.fillRect(self.rect(), QColor(230, 230, 230))
        else:
            painter.fillRect(self.rect(), QColor(240, 240, 240))
            
        # Draw border
        painter.setPen(QColor(180, 180, 180))
        painter.drawRect(self.rect().adjusted(0, 0, -1, -1))
        
        # Draw text and icon
        super().paintEvent(event) 