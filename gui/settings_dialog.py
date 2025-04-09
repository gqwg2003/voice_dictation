#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from PyQt6.QtCore import Qt, QSettings
from PyQt6.QtGui import QKeySequence, QFont
from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QFormLayout, QGroupBox,
    QLabel, QCheckBox, QComboBox, QPushButton, QDialogButtonBox,
    QKeySequenceEdit, QSlider
)

class SettingsDialog(QDialog):
    """Dialog for configuring application settings."""
    
    def __init__(self, settings, hotkey_manager, parent=None):
        super().__init__(parent)
        
        self.settings = settings
        self.hotkey_manager = hotkey_manager
        
        self.setWindowTitle("Настройки")
        self.setMinimumWidth(400)
        
        # Set up interface
        self.setup_ui()
        
        # Load settings
        self.load_settings()
    
    def setup_ui(self):
        """Set up the UI components."""
        main_layout = QVBoxLayout(self)
        main_layout.setSpacing(15)
        
        # Appearance group
        appearance_group = QGroupBox("Внешний вид")
        appearance_layout = QFormLayout(appearance_group)
        
        # Theme
        self.theme_combo = QComboBox()
        self.theme_combo.addItem("Светлая тема", "light")
        self.theme_combo.addItem("Тёмная тема", "dark")
        appearance_layout.addRow("Тема:", self.theme_combo)
        
        # Font size
        self.font_size_combo = QComboBox()
        for size in [10, 11, 12, 14, 16]:
            self.font_size_combo.addItem(f"{size} пт", size)
        appearance_layout.addRow("Размер шрифта:", self.font_size_combo)
        
        main_layout.addWidget(appearance_group)
        
        # Hotkey settings group
        hotkeys_group = QGroupBox("Горячие клавиши")
        hotkeys_layout = QFormLayout(hotkeys_group)
        
        # Record hotkey
        self.record_hotkey_edit = QKeySequenceEdit()
        self.record_hotkey_edit.setMinimumWidth(150)
        hotkeys_layout.addRow("Записать:", self.record_hotkey_edit)
        
        # Copy hotkey
        self.copy_hotkey_edit = QKeySequenceEdit()
        self.copy_hotkey_edit.setMinimumWidth(150)
        hotkeys_layout.addRow("Копировать:", self.copy_hotkey_edit)
        
        # Clear hotkey
        self.clear_hotkey_edit = QKeySequenceEdit()
        self.clear_hotkey_edit.setMinimumWidth(150)
        hotkeys_layout.addRow("Очистить:", self.clear_hotkey_edit)
        
        main_layout.addWidget(hotkeys_group)
        
        # Behavior settings group
        behavior_group = QGroupBox("Поведение")
        behavior_layout = QFormLayout(behavior_group)
        
        # Start minimized
        self.start_minimized_checkbox = QCheckBox("Запускать свёрнутым в трей")
        behavior_layout.addRow("", self.start_minimized_checkbox)
        
        # Minimize to tray
        self.minimize_to_tray_checkbox = QCheckBox("Сворачивать в трей при закрытии")
        behavior_layout.addRow("", self.minimize_to_tray_checkbox)
        
        # Auto copy to clipboard
        self.auto_clipboard_checkbox = QCheckBox("Автоматически копировать в буфер обмена")
        behavior_layout.addRow("", self.auto_clipboard_checkbox)
        
        main_layout.addWidget(behavior_group)
        
        # Speech recognition settings group
        recognition_group = QGroupBox("Настройки распознавания речи")
        recognition_layout = QFormLayout(recognition_group)
        
        # Energy threshold slider
        self.energy_threshold_slider = QSlider(Qt.Orientation.Horizontal)
        self.energy_threshold_slider.setMinimum(100)
        self.energy_threshold_slider.setMaximum(1000)
        self.energy_threshold_slider.setSingleStep(50)
        self.energy_threshold_label = QLabel("300")
        
        energy_layout = QHBoxLayout()
        energy_layout.addWidget(self.energy_threshold_slider)
        energy_layout.addWidget(self.energy_threshold_label)
        
        recognition_layout.addRow("Порог энергии:", energy_layout)
        
        # Dynamic energy threshold
        self.dynamic_energy_checkbox = QCheckBox("Динамический порог энергии")
        recognition_layout.addRow("", self.dynamic_energy_checkbox)
        
        # Quiet speech boost
        self.energy_adjustment_slider = QSlider(Qt.Orientation.Horizontal)
        self.energy_adjustment_slider.setMinimum(10)
        self.energy_adjustment_slider.setMaximum(30)
        self.energy_adjustment_slider.setSingleStep(1)
        self.energy_adjustment_label = QLabel("1.5x")
        
        adjustment_layout = QHBoxLayout()
        adjustment_layout.addWidget(self.energy_adjustment_slider)
        adjustment_layout.addWidget(self.energy_adjustment_label)
        
        recognition_layout.addRow("Усиление тихой речи:", adjustment_layout)
        
        # Pause threshold
        self.pause_threshold_slider = QSlider(Qt.Orientation.Horizontal)
        self.pause_threshold_slider.setMinimum(5)
        self.pause_threshold_slider.setMaximum(20)
        self.pause_threshold_slider.setSingleStep(1)
        self.pause_threshold_label = QLabel("0.8 сек")
        
        pause_layout = QHBoxLayout()
        pause_layout.addWidget(self.pause_threshold_slider)
        pause_layout.addWidget(self.pause_threshold_label)
        
        recognition_layout.addRow("Пауза для окончания:", pause_layout)
        
        # Apply audio normalization
        self.audio_normalization_checkbox = QCheckBox("Нормализация громкости (улучшает распознавание тихой речи)")
        recognition_layout.addRow("", self.audio_normalization_checkbox)
        
        # Phrase similarity threshold
        self.similarity_threshold_slider = QSlider(Qt.Orientation.Horizontal)
        self.similarity_threshold_slider.setMinimum(50)
        self.similarity_threshold_slider.setMaximum(90)
        self.similarity_threshold_slider.setSingleStep(5)
        self.similarity_threshold_label = QLabel("65%")
        
        similarity_layout = QHBoxLayout()
        similarity_layout.addWidget(self.similarity_threshold_slider)
        similarity_layout.addWidget(self.similarity_threshold_label)
        
        recognition_layout.addRow("Порог схожести фраз:", similarity_layout)
        
        # Connect signals for updating labels
        self.energy_threshold_slider.valueChanged.connect(
            lambda value: self.energy_threshold_label.setText(str(value))
        )
        self.energy_adjustment_slider.valueChanged.connect(
            lambda value: self.energy_adjustment_label.setText(f"{value/10:.1f}x")
        )
        self.pause_threshold_slider.valueChanged.connect(
            lambda value: self.pause_threshold_label.setText(f"{value/10:.1f} сек")
        )
        self.similarity_threshold_slider.valueChanged.connect(
            lambda value: self.similarity_threshold_label.setText(f"{value}%")
        )
        
        main_layout.addWidget(recognition_group)
        
        # Dialog buttons
        button_box = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | 
            QDialogButtonBox.StandardButton.Cancel
        )
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)
        
        main_layout.addWidget(button_box)
    
    def load_settings(self):
        """Load settings from QSettings."""
        # Theme
        theme = self.settings.value("theme", "light")
        index = self.theme_combo.findData(theme)
        if index >= 0:
            self.theme_combo.setCurrentIndex(index)
        
        # Font size
        font_size = self.settings.value("font_size", 12, type=int)
        index = self.font_size_combo.findData(font_size)
        if index >= 0:
            self.font_size_combo.setCurrentIndex(index)
        
        # Hotkeys
        record_hotkey = self.settings.value("hotkey_record", "Ctrl+Alt+R")
        self.record_hotkey_edit.setKeySequence(QKeySequence(record_hotkey))
        
        copy_hotkey = self.settings.value("hotkey_copy", "Ctrl+Alt+C")
        self.copy_hotkey_edit.setKeySequence(QKeySequence(copy_hotkey))
        
        clear_hotkey = self.settings.value("hotkey_clear", "Ctrl+Alt+X")
        self.clear_hotkey_edit.setKeySequence(QKeySequence(clear_hotkey))
        
        # Behavior
        start_minimized = self.settings.value("start_minimized", False, type=bool)
        self.start_minimized_checkbox.setChecked(start_minimized)
        
        minimize_to_tray = self.settings.value("minimize_to_tray", True, type=bool)
        self.minimize_to_tray_checkbox.setChecked(minimize_to_tray)
        
        auto_clipboard = self.settings.value("auto_clipboard", False, type=bool)
        self.auto_clipboard_checkbox.setChecked(auto_clipboard)
        
        # Recognition settings
        energy_threshold = self.settings.value("recognition_energy_threshold", 300, type=int)
        self.energy_threshold_slider.setValue(energy_threshold)
        
        dynamic_energy = self.settings.value("recognition_dynamic_energy", True, type=bool)
        self.dynamic_energy_checkbox.setChecked(dynamic_energy)
        
        energy_adjustment = int(self.settings.value("recognition_energy_adjustment", 15, type=int))
        self.energy_adjustment_slider.setValue(energy_adjustment)
        
        pause_threshold = int(self.settings.value("recognition_pause_threshold", 8, type=int))
        self.pause_threshold_slider.setValue(pause_threshold)
        
        audio_normalization = self.settings.value("recognition_audio_normalization", True, type=bool)
        self.audio_normalization_checkbox.setChecked(audio_normalization)
        
        similarity_threshold = int(self.settings.value("recognition_similarity_threshold", 65, type=int))
        self.similarity_threshold_slider.setValue(similarity_threshold)
    
    def accept(self):
        """Save settings when OK is clicked."""
        # Theme
        self.settings.setValue("theme", self.theme_combo.currentData())
        
        # Font size
        self.settings.setValue("font_size", self.font_size_combo.currentData())
        
        # Hotkeys
        self.settings.setValue("hotkey_record", self.record_hotkey_edit.keySequence().toString())
        self.settings.setValue("hotkey_copy", self.copy_hotkey_edit.keySequence().toString())
        self.settings.setValue("hotkey_clear", self.clear_hotkey_edit.keySequence().toString())
        
        # Behavior
        self.settings.setValue("start_minimized", self.start_minimized_checkbox.isChecked())
        self.settings.setValue("minimize_to_tray", self.minimize_to_tray_checkbox.isChecked())
        self.settings.setValue("auto_clipboard", self.auto_clipboard_checkbox.isChecked())
        
        # Recognition settings
        self.settings.setValue("recognition_energy_threshold", self.energy_threshold_slider.value())
        self.settings.setValue("recognition_dynamic_energy", self.dynamic_energy_checkbox.isChecked())
        self.settings.setValue("recognition_energy_adjustment", self.energy_adjustment_slider.value())
        self.settings.setValue("recognition_pause_threshold", self.pause_threshold_slider.value())
        self.settings.setValue("recognition_audio_normalization", self.audio_normalization_checkbox.isChecked())
        self.settings.setValue("recognition_similarity_threshold", self.similarity_threshold_slider.value())
        
        # Call parent accept
        super().accept() 