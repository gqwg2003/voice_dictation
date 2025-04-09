#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Audio visualizer for real-time display of voice levels.
"""

import os
import math
import random
import numpy as np
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QObject, QSize, QPropertyAnimation, QEasingCurve, QRectF
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QProgressBar, 
    QLabel, QSizePolicy, QSpacerItem, QFrame
)
from PyQt6.QtGui import QColor, QPalette, QLinearGradient, QBrush, QFont

class AudioBar(QFrame):
    """Custom visualizer bar with gradient and rounded corners"""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedWidth(10)
        self.setValue(0)
        self.setFrameShape(QFrame.Shape.NoFrame)
        self.setMinimumHeight(80)
        self.max_height = 100
        self.value = 0
        self.color = QColor("#00AA00")
        
    def setValue(self, value):
        """Set the bar value (0-100)"""
        self.value = max(0, min(100, value))
        self.update()
        
    def setColor(self, color):
        """Set the bar color"""
        self.color = color
        self.update()
        
    def paintEvent(self, event):
        """Custom paint event for the bar"""
        from PyQt6.QtGui import QPainter, QPainterPath
        
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        
        # Calculate height based on value
        height = int(self.height() * self.value / 100)
        
        # Create gradient
        gradient = QLinearGradient(0, self.height() - height, 0, self.height())
        
        # Dynamic color based on level
        base_color = self.color
        
        # Add gradient stops
        gradient.setColorAt(0, base_color.lighter(150))
        gradient.setColorAt(0.5, base_color)
        gradient.setColorAt(1, base_color.darker(150))
        
        # Draw rounded rectangle
        path = QPainterPath()
        rect = QRectF(self.rect())
        rect.setHeight(height)
        rect.moveTop(self.height() - height)
        
        # Make the corners more rounded for lower values to create a bubble effect
        corner_radius = min(5, max(3, 5 * (1 - self.value/100)))
        path.addRoundedRect(rect, corner_radius, corner_radius)
        
        painter.fillPath(path, QBrush(gradient))
        
        # Add a shine effect at the top
        if height > 5:
            shine_path = QPainterPath()
            shine_rect = QRectF(rect)
            shine_rect.setHeight(min(5, height/3))
            shine_path.addRoundedRect(shine_rect, corner_radius, corner_radius)
            
            shine_gradient = QLinearGradient(0, rect.top(), 0, rect.top() + shine_rect.height())
            shine_gradient.setColorAt(0, QColor(255, 255, 255, 130))
            shine_gradient.setColorAt(1, QColor(255, 255, 255, 0))
            
            painter.fillPath(shine_path, QBrush(shine_gradient))

class AudioVisualizer(QWidget):
    """
    Widget that displays audio levels during recording.
    """
    
    def __init__(self, parent=None):
        super().__init__(parent)
        
        self.setObjectName("audioVisualizer")
        
        # Audio level
        self.audio_level = 0.0
        self.target_audio_level = 0.0
        self.is_active = False
        self.bars_count = 16  # Number of visualizer bars
        
        # Animation settings
        self.animation_speed = 0.15  # Speed of level changes
        self.bar_heights = [0.0] * self.bars_count
        self.target_heights = [0.0] * self.bars_count
        
        # Ensure we're visible
        self.setMinimumHeight(120)
        self.setVisible(True)
        
        self.setup_ui()
        
        # Animation timer
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_visualization)
        
        # Status
        self.status = "idle"  # idle, listening, processing, success, error
        
        # Colors
        self.colors = {
            "idle": QColor("#888888"),
            "listening": QColor("#1E88E5"),  # Blue
            "processing": QColor("#FF9800"),  # Orange
            "success": QColor("#00C853"),     # Green
            "error": QColor("#E53935")        # Red
        }
        
    def setup_ui(self):
        """Set up the UI components."""
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)
        
        # Status indicator
        self.status_indicator = QLabel("Готов к работе")
        self.status_indicator.setObjectName("statusIndicator")
        self.status_indicator.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.status_indicator.setSizePolicy(
            QSizePolicy.Policy.Expanding, 
            QSizePolicy.Policy.Fixed
        )
        self.status_indicator.setFont(QFont("Segoe UI", 10, QFont.Weight.Bold))
        self.status_indicator.setStyleSheet("""
            border-radius: 15px;
            padding: 5px;
        """)
        
        main_layout.addWidget(self.status_indicator)
        
        # Audio level meter
        self.audio_level_container = QWidget()
        self.audio_level_container.setFixedHeight(20)
        self.audio_level_container.setStyleSheet("""
            background-color: rgba(0, 0, 0, 0.2);
            border-radius: 10px;
        """)
        
        level_layout = QHBoxLayout(self.audio_level_container)
        level_layout.setContentsMargins(2, 2, 2, 2)
        
        self.audio_level_bar = QProgressBar()
        self.audio_level_bar.setObjectName("audioLevelBar")
        self.audio_level_bar.setMinimum(0)
        self.audio_level_bar.setMaximum(100)
        self.audio_level_bar.setValue(0)
        self.audio_level_bar.setTextVisible(False)
        self.audio_level_bar.setFixedHeight(16)
        self.audio_level_bar.setOrientation(Qt.Orientation.Horizontal)
        level_layout.addWidget(self.audio_level_bar)
        
        main_layout.addWidget(self.audio_level_container)
        
        # Visualizer container with background
        self.visualizer_container = QWidget()
        self.visualizer_container.setObjectName("visualizerContainer")
        self.visualizer_container.setStyleSheet("""
            background-color: rgba(0, 0, 0, 0.2);
            border-radius: 10px;
            padding: 5px;
        """)
        
        visualizer_layout = QHBoxLayout(self.visualizer_container)
        visualizer_layout.setContentsMargins(10, 5, 10, 5)
        visualizer_layout.setSpacing(4)
        
        # Visualizer bars
        self.visualizer_bars = []
        for i in range(self.bars_count):
            bar = AudioBar()
            visualizer_layout.addWidget(bar)
            self.visualizer_bars.append(bar)
        
        main_layout.addWidget(self.visualizer_container)
        
        # Apply theme
        self.setStyleSheet("""
            QWidget#audioVisualizer {
                background-color: transparent;
            }
        """)
    
    def set_status(self, status, message=None):
        """
        Set the current status of the visualizer.
        
        Args:
            status: String status (idle, listening, processing, success, error)
            message: Optional message to display
        """
        self.status = status
        color = self.colors[status]
        
        # Always ensure we're visible
        self.setVisible(True)
        
        # Update status indicator
        if status == "idle":
            self.status_indicator.setStyleSheet(f"""
                border-radius: 15px;
                padding: 5px;
                color: {color.name()};
                background-color: transparent;
            """)
            self.status_indicator.setText(message or "Готов к работе")
            self.is_active = False
            # Don't hide - just show inactive state
            
        elif status == "listening":
            self.status_indicator.setStyleSheet(f"""
                border-radius: 15px;
                padding: 5px;
                color: white;
                background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                                               stop:0 {color.darker(120).name()}, 
                                               stop:1 {color.lighter(120).name()});
            """)
            self.status_indicator.setText(message or "Слушаю...")
            self.is_active = True
            
        elif status == "processing":
            self.status_indicator.setStyleSheet(f"""
                border-radius: 15px;
                padding: 5px;
                color: white;
                background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                                               stop:0 {color.darker(120).name()}, 
                                               stop:1 {color.lighter(120).name()});
            """)
            self.status_indicator.setText(message or "Обработка...")
            self.is_active = False
            
        elif status == "success":
            self.status_indicator.setStyleSheet(f"""
                border-radius: 15px;
                padding: 5px;
                color: white;
                background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                                               stop:0 {color.darker(120).name()}, 
                                               stop:1 {color.lighter(120).name()});
            """)
            self.status_indicator.setText(message or "Успешно!")
            self.is_active = False
            
        elif status == "error":
            self.status_indicator.setStyleSheet(f"""
                border-radius: 15px;
                padding: 5px;
                color: white;
                background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                                               stop:0 {color.darker(120).name()}, 
                                               stop:1 {color.lighter(120).name()});
            """)
            self.status_indicator.setText(message or "Ошибка")
            self.is_active = False
            
        # Update audio level bar style
        self.audio_level_bar.setStyleSheet(f"""
            QProgressBar {{
                border: none;
                border-radius: 8px;
                background-color: rgba(0, 0, 0, 0.2);
                text-align: center;
            }}
            QProgressBar::chunk {{
                border-radius: 8px;
                background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                                               stop:0 {color.darker(120).name()}, 
                                               stop:1 {color.lighter(120).name()});
            }}
        """)
        
        # Update bars color
        for bar in self.visualizer_bars:
            bar.setColor(color)
            
        # Start or stop animation
        if self.is_active:
            if not self.timer.isActive():
                self.timer.start(35)  # ~30fps for smoother animation
        else:
            # Stop timers but don't hide
            if self.timer.isActive():
                self.timer.stop()
            
            # Reset visualizer bars
            for bar in self.visualizer_bars:
                bar.setValue(0)
            self.audio_level_bar.setValue(0)
            
            # We'll keep the widget visible for better UI consistency
    
    def set_audio_level(self, level):
        """
        Set the current audio level.
        
        Args:
            level: Audio level from 0.0 to 1.0
        """
        self.target_audio_level = max(0.0, min(1.0, level))
    
    def update_visualization(self):
        """Update the visualization based on current audio level."""
        if not self.is_active:
            return
            
        # Smoothly update audio level
        self.audio_level += (self.target_audio_level - self.audio_level) * self.animation_speed
        
        # Update audio level meter with easing
        self.audio_level_bar.setValue(int(self.audio_level * 100))
        
        # Update visualizer bars with smooth animation and wave effect
        time_factor = self.timer.interval() / 1000 * 8  # For wave animation
        
        for i, bar in enumerate(self.visualizer_bars):
            # Calculate position factor (0..1) for wave effect
            pos = i / (self.bars_count - 1)
            
            # Base factor - higher in the center
            center_factor = 1.1 - 1.2 * abs(pos - 0.5)
            
            # Wave movement factor
            wave = 0.25 * math.sin(time_factor * 4 + i * 0.5) 
            
            # Second wave for complexity
            wave2 = 0.15 * math.sin(time_factor * 2.5 + i * 0.8)
            
            # Final target height calculation
            target_height = (
                self.audio_level * 0.6 +  # Base level
                wave * self.audio_level +  # Primary wave
                wave2 * self.audio_level +  # Secondary wave
                center_factor * self.audio_level * 0.4  # Center emphasis
            )
            
            # Add randomness for a more natural look
            if random.random() < 0.1:  # 10% chance
                target_height += random.uniform(-0.05, 0.05) * self.audio_level
                
            # Smoothly update bar height with variable speed (faster for increases)
            current = self.bar_heights[i]
            speed = self.animation_speed * (1.5 if target_height > current else 1.0)
            self.bar_heights[i] += (target_height - current) * speed
            
            # Clamp and set value
            value = int(max(0, min(100, self.bar_heights[i] * 100)))
            bar.setValue(value)
        
        # Simply reset target audio level for next C++ update
        self.target_audio_level = 0.0
            
        # Ensure the widget stays visible
        if not self.isVisible():
            self.setVisible(True) 