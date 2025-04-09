#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Main application window for the voice dictation application.
Provides the user interface and integrates all core components.
"""

# ==== IMPORTS ====
# System imports
import os
import logging

# Qt imports
from PyQt6.QtCore import Qt, QSettings, QSize, QTimer
from PyQt6.QtGui import QIcon, QAction, QFont
from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
    QPushButton, QTextEdit, QLabel, QStatusBar, 
    QToolBar, QMenu, QSystemTrayIcon, QMessageBox,
    QComboBox, QSizePolicy, QApplication
)

# Core modules
from core.speech_recognizer import SpeechRecognizer
from core.clipboard_manager import ClipboardManager
from core.hotkey_manager import HotkeyManager

# GUI modules
from gui.settings_dialog import SettingsDialog
from gui.audio_visualizer import AudioVisualizer
from gui.styles import DARK_THEME, LIGHT_THEME

# Utility modules
from utils.version_updater import get_runtime_app_info

# ==== LOGGER SETUP ====
logger = logging.getLogger('voice_dictation.main_window')
logger.setLevel(logging.DEBUG)

# Add file handler
log_file = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'voice_dictation.log')
file_handler = logging.FileHandler(log_file, encoding='utf-8')
file_handler.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
file_handler.setFormatter(formatter)
logger.addHandler(file_handler)

# ==== MAIN WINDOW CLASS ====
class MainWindow(QMainWindow):
    """Main application window."""
    
    def __init__(self, hybrid_mode_info=None, parent=None):
        """Initialize the main window and all its components."""
        super().__init__(parent)
        
        # Сохраняем информацию о режиме работы
        self.hybrid_mode_info = hybrid_mode_info or {}
        self.mode = self.hybrid_mode_info.get('mode', 'pure_python')
        
        # Initialize core components
        self._init_core_components()
        
        # Setup settings
        self.settings = QSettings("VoiceApp", "VoiceDictation")
        
        # Setup UI
        self._init_ui()
        
        # Connect signals
        self.connect_signals()
        
        # Load settings
        self._load_settings()
        
        # Apply theme
        self.apply_theme()
        
        # Start hotkey listener
        logger.debug("Starting hotkey listener...")
        self.hotkey_manager.start_listening()
        
        # Обновляем информацию о производительности в интерфейсе
        self._update_performance_info()
    
    # ==== INITIALIZATION METHODS ====
    def _init_core_components(self):
        """Initialize the core components of the application."""
        self.speech_recognizer = SpeechRecognizer()
        self.clipboard_manager = ClipboardManager()
        self.hotkey_manager = HotkeyManager()
    
    # ==== UI SETUP METHODS ====
    def get_icon(self, name):
        """
        Get icon from resources, with fallback if file doesn't exist.
        
        Args:
            name (str): Icon filename without path
            
        Returns:
            QIcon: Icon object
        """
        # Full path to resource
        path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 
                           'resources', name)
        
        # Check if file exists
        if os.path.exists(path):
            return QIcon(path)
        else:
            # Return empty icon as fallback
            return QIcon()
    
    def _init_ui(self):
        """Set up the user interface components."""
        # Get app info
        app_info = get_runtime_app_info()
        
        # Window properties
        self._setup_window_properties(app_info)
        
        # Create central widget and main layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(15, 15, 15, 15)
        main_layout.setSpacing(10)
        
        # Create toolbar
        self.setup_toolbar()
        
        # Header with language selection
        self._setup_header(main_layout, app_info)
        
        # Audio visualizer widget
        self.audio_visualizer = AudioVisualizer()
        self.audio_visualizer.setMinimumHeight(120)
        main_layout.addWidget(self.audio_visualizer)
        
        # Text area
        self._setup_text_area(main_layout)
        
        # Button row
        self._setup_buttons(main_layout)
        
        # Status bar
        self._setup_status_bar(app_info)
        
        # System tray
        self.setup_system_tray()
    
    def _setup_window_properties(self, app_info):
        """Set up the main window properties."""
        self.setWindowTitle(f"Голосовой Ввод {app_info['version']}")
        self.setMinimumSize(650, 500)
        self.setWindowIcon(self.get_icon("app.png"))
    
    def _setup_header(self, main_layout, app_info):
        """Set up the header with app title and language selection."""
        header_layout = QHBoxLayout()
        
        # App logo and title
        logo_label = QLabel()
        logo_label.setPixmap(self.get_icon("app.png").pixmap(QSize(32, 32)))
        header_layout.addWidget(logo_label)
        
        title_label = QLabel(f"Голосовой Ввод {app_info['version']}")
        title_label.setFont(QFont("Segoe UI", 14, QFont.Weight.Bold))
        header_layout.addWidget(title_label)
        
        header_layout.addStretch()
        
        # Language selection
        lang_label = QLabel("Язык:")
        lang_label.setFont(QFont("Segoe UI", 10))
        self.language_combo = QComboBox()
        self.language_combo.setFont(QFont("Segoe UI", 10))
        self.language_combo.addItem("Русский", "ru-RU")
        self.language_combo.addItem("English", "en-US")
        self.language_combo.setMinimumWidth(120)
        header_layout.addWidget(lang_label)
        header_layout.addWidget(self.language_combo)
        
        main_layout.addLayout(header_layout)
    
    def _setup_text_area(self, main_layout):
        """Set up the text editing area."""
        self.text_edit = QTextEdit()
        self.text_edit.setPlaceholderText("Нажмите кнопку 'Записать' для начала диктовки...")
        self.text_edit.setFont(QFont("Segoe UI", 12))
        main_layout.addWidget(self.text_edit)
    
    def _setup_buttons(self, main_layout):
        """Set up the control buttons."""
        button_layout = QHBoxLayout()
        button_layout.setSpacing(10)
        
        # Record button
        self.record_button = QPushButton("Записать")
        self.record_button.setIcon(self.get_icon("mic.png"))
        self.record_button.setCheckable(True)
        self.record_button.setMinimumHeight(50)
        self.record_button.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self.record_button.setFont(QFont("Segoe UI", 11, QFont.Weight.Bold))
        self.record_button.setObjectName("recordButton")
        
        # Copy button
        self.copy_button = QPushButton("Копировать")
        self.copy_button.setIcon(self.get_icon("copy.png"))
        self.copy_button.setMinimumHeight(50)
        self.copy_button.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self.copy_button.setFont(QFont("Segoe UI", 11))
        self.copy_button.setObjectName("copyButton")
        
        # Clear button
        self.clear_button = QPushButton("Очистить")
        self.clear_button.setIcon(self.get_icon("clear.png"))
        self.clear_button.setMinimumHeight(50)
        self.clear_button.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self.clear_button.setFont(QFont("Segoe UI", 11))
        self.clear_button.setObjectName("clearButton")
        
        button_layout.addWidget(self.record_button)
        button_layout.addWidget(self.copy_button)
        button_layout.addWidget(self.clear_button)
        
        main_layout.addLayout(button_layout)
    
    def _setup_status_bar(self, app_info):
        """Set up the status bar."""
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        
        # Create permanent version label in status bar
        version_text = app_info['version']
        if app_info['is_prerelease']:
            version_label = QLabel(f"Версия: {version_text} (тестовая)")
        else:
            version_label = QLabel(f"Версия: {version_text}")
        self.status_bar.addPermanentWidget(version_label)
        
        self.status_bar.showMessage("Готов к работе")
    
    def setup_toolbar(self):
        """Set up toolbar with actions."""
        toolbar = QToolBar()
        toolbar.setMovable(False)
        toolbar.setIconSize(QSize(24, 24))
        toolbar.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonTextBesideIcon)
        
        # Settings action
        settings_action = QAction(self.get_icon("settings.png"), "Настройки", self)
        settings_action.triggered.connect(self.show_settings)
        toolbar.addAction(settings_action)
        
        # Custom terms action
        custom_terms_action = QAction(self.get_icon("copy.png"), "Термины", self)
        custom_terms_action.triggered.connect(self.show_custom_terms_dialog)
        toolbar.addAction(custom_terms_action)
        
        toolbar.addSeparator()
        
        # Help action
        help_action = QAction(self.get_icon("help.png"), "Помощь", self)
        help_action.triggered.connect(self.show_help)
        toolbar.addAction(help_action)
        
        # About action
        about_action = QAction("О программе", self)
        about_action.triggered.connect(self.show_about)
        toolbar.addAction(about_action)
        
        self.addToolBar(toolbar)
    
    def setup_system_tray(self):
        """Set up system tray icon and menu."""
        self.tray_icon = QSystemTrayIcon(self.get_icon("app.png"), self)
        
        tray_menu = QMenu()
        
        show_action = QAction("Показать", self)
        show_action.triggered.connect(self.show)
        tray_menu.addAction(show_action)
        
        record_action = QAction("Записать", self)
        record_action.triggered.connect(lambda: self.record_button.click())
        tray_menu.addAction(record_action)
        
        tray_menu.addSeparator()
        
        about_action = QAction("О программе", self)
        about_action.triggered.connect(self.show_about)
        tray_menu.addAction(about_action)
        
        exit_action = QAction("Выход", self)
        exit_action.triggered.connect(self.close)
        tray_menu.addAction(exit_action)
        
        self.tray_icon.setContextMenu(tray_menu)
        self.tray_icon.activated.connect(self.tray_icon_activated)
        self.tray_icon.show()
    
    def connect_signals(self):
        """Connect signals and slots."""
        # Button signals
        self.record_button.clicked.connect(self.toggle_recording)
        self.copy_button.clicked.connect(self.copy_text)
        self.clear_button.clicked.connect(self.clear_text)
        
        # Speech recognizer signals
        self.speech_recognizer.recognized_text.connect(self.on_text_recognized)
        self.speech_recognizer.error_occurred.connect(self.on_recognition_error)
        self.speech_recognizer.listening_started.connect(self.on_listening_started)
        self.speech_recognizer.listening_ended.connect(self.on_listening_ended)
        self.speech_recognizer.processing_started.connect(self.on_processing_started)
        self.speech_recognizer.processing_ended.connect(self.on_processing_ended)
        self.speech_recognizer.audio_level.connect(self.on_audio_level)
        
        # Clipboard manager signals
        self.clipboard_manager.text_copied.connect(lambda text: self.status_bar.showMessage("Текст скопирован", 3000))
        
        # Hotkey manager signals
        self.hotkey_manager.hotkey_triggered.connect(self.on_hotkey_triggered)
        
        # Language combo signal
        self.language_combo.currentIndexChanged.connect(self.on_language_changed)
    
    def toggle_recording(self):
        """Start or stop voice recording."""
        if not self.record_button.isChecked():
            # Stopping recording if it's active
            self.speech_recognizer.stop_listening()
            return
            
        # Start recording
        self.speech_recognizer.language = self.language_combo.currentData()
        self.speech_recognizer.start()
    
    def on_text_recognized(self, text):
        """Handle recognized text."""
        current_text = self.text_edit.toPlainText()
        if current_text:
            self.text_edit.setPlainText(f"{current_text} {text}")
        else:
            self.text_edit.setPlainText(text)
        
        self.status_bar.showMessage("Текст распознан", 3000)
        
        # Automatically copy text to clipboard if enabled in settings
        if self.settings.value("auto_clipboard", False, type=bool):
            self.copy_text()
            # Update status bar to inform user
            self.status_bar.showMessage("Текст распознан и скопирован в буфер обмена", 3000)
    
    def on_recognition_error(self, error_msg):
        """Handle recognition errors."""
        self.status_bar.showMessage(error_msg, 3000)
        self.audio_visualizer.set_status("error", error_msg)
        self.record_button.setChecked(False)
    
    def on_listening_started(self):
        """Handle listening start."""
        self.status_bar.showMessage("Слушаю...", 0)
        self.record_button.setText("Остановить")
        self.audio_visualizer.set_status("listening", "Слушаю...")
    
    def on_listening_ended(self):
        """Handle listening end."""
        self.record_button.setText("Записать")
        self.record_button.setChecked(False)
    
    def on_processing_started(self):
        """Handle processing start."""
        self.status_bar.showMessage("Обработка...", 0)
        self.audio_visualizer.set_status("processing", "Обработка...")
    
    def on_processing_ended(self, success):
        """Handle processing end."""
        if success:
            self.audio_visualizer.set_status("success", "Успешно!")
        self.status_bar.showMessage("Готов к работе", 3000)
    
    def on_audio_level(self, level):
        """Handle audio level updates."""
        self.audio_visualizer.set_audio_level(level)
    
    def copy_text(self):
        """Copy text to clipboard."""
        text = self.text_edit.toPlainText()
        if text:
            self.clipboard_manager.copy_to_clipboard(text)
            # Visual feedback
            original_stylesheet = self.copy_button.styleSheet()
            self.copy_button.setStyleSheet("QPushButton { background-color: #00AA00; color: white; }")
            # Reset style after a short delay
            QTimer.singleShot(500, lambda: self.copy_button.setStyleSheet(original_stylesheet))
    
    def clear_text(self):
        """Clear the text area."""
        self.text_edit.clear()
    
    def on_hotkey_triggered(self, action_name):
        """Handle hotkey triggers."""
        if action_name == "start_recording":
            self.record_button.click()
        elif action_name == "copy_text":
            self.copy_text()
        elif action_name == "clear_text":
            self.clear_text()
    
    def on_language_changed(self, index):
        """Handle language selection change."""
        language_code = self.language_combo.currentData()
        self.speech_recognizer.set_language(language_code)
        self.statusBar().showMessage(f"Язык распознавания: {self.language_combo.currentText()}", 3000)
    
    def show_settings(self):
        """Show settings dialog."""
        dialog = SettingsDialog(self.settings, self.hotkey_manager, self)
        if dialog.exec():
            self._load_settings()
            # Apply recognition settings immediately in case they were changed
            self.apply_recognition_settings()
    
    def show_help(self):
        """Show help dialog."""
        app_info = get_runtime_app_info()
        
        help_text = f"""
        <h2 style='color: #007ACC;'>Голосовой Ввод v{app_info['version']}</h2>
        <p>Приложение для голосового ввода текста</p>
        <p><b>Основные команды:</b></p>
        <ul>
            <li><b>Записать</b> (Ctrl+Alt+R) - Начать/остановить запись</li>
            <li><b>Копировать</b> (Ctrl+Alt+C) - Копировать текст в буфер обмена</li>
            <li><b>Очистить</b> (Ctrl+Alt+X) - Очистить текст</li>
        </ul>
        <p>Изменить горячие клавиши можно в настройках.</p>
        <p>{app_info['copyright']} {app_info['website']}</p>
        """
        
        QMessageBox.information(self, "Справка", help_text)
    
    def show_about(self):
        """Show about dialog with version information."""
        app_info = get_runtime_app_info()
        version_info = app_info['version']
        
        # Add prerelease indicator if needed
        if app_info['is_prerelease']:
            version_info += " <span style='color:#E81123'>(тестовая версия)</span>"
        
        QMessageBox.about(
            self,
            "О программе",
            f"""
            <h2 style='color: #007ACC;'>Голосовой Ввод {app_info['version']}</h2>
            <p>Многоязычное приложение для распознавания речи</p>
            <p>Версия: <b>{version_info}</b></p>
            <p>{app_info['copyright']}</p>
            <p><a href='{app_info['website']}'>Домашняя страница</a></p>
            """
        )
    
    def tray_icon_activated(self, reason):
        """Handle tray icon activation."""
        if reason == QSystemTrayIcon.ActivationReason.DoubleClick:
            if self.isVisible():
                self.hide()
            else:
                self.show()
                self.setWindowState(self.windowState() & ~Qt.WindowState.WindowMinimized | Qt.WindowState.WindowActive)
                self.activateWindow()
    
    def toggle_theme(self):
        """Toggle between light and dark theme."""
        current_theme = self.settings.value("theme", "light")
        new_theme = "dark" if current_theme == "light" else "light"
        self.settings.setValue("theme", new_theme)
        self.apply_theme()
    
    def apply_theme(self):
        """Apply the selected theme to the application."""
        theme = self.settings.value("theme", "light")
        
        if theme == "dark":
            QApplication.instance().setStyleSheet(DARK_THEME)
        else:
            QApplication.instance().setStyleSheet(LIGHT_THEME)
    
    def _load_settings(self):
        """Load application settings."""
        # Load window geometry
        geometry = self.settings.value("geometry")
        if geometry:
            self.restoreGeometry(geometry)
        
        # Load language
        language = self.settings.value("language", "ru-RU")
        index = self.language_combo.findData(language)
        if index >= 0:
            self.language_combo.setCurrentIndex(index)
        
        # Load theme
        self.apply_theme()
        
        # Load hotkey settings
        self.configure_hotkeys()
        
        # Apply recognition settings to speech recognizer
        self.apply_recognition_settings()
        
        # Check if should start minimized
        start_minimized = self.settings.value("start_minimized", False, type=bool)
        if start_minimized:
            self.hide()
    
    def configure_hotkeys(self):
        """Configure hotkey manager with saved settings."""
        # Clear existing hotkeys
        self.hotkey_manager.hotkeys = {}
        
        # Map PyQt key names to pynput key names
        key_map = {
            "Ctrl": "ctrl",
            "Alt": "alt",
            "Shift": "shift",
            "Meta": "cmd",
            "Return": "enter",
            "Escape": "esc",
            "Tab": "tab",
            "Backspace": "backspace",
            "Space": "space",
            "F1": "f1",
            "F2": "f2",
            "F3": "f3",
            "F4": "f4",
            "F5": "f5",
            "F6": "f6",
            "F7": "f7",
            "F8": "f8",
            "F9": "f9",
            "F10": "f10",
            "F11": "f11",
            "F12": "f12",
        }
        
        # Convert QKeySequence to pynput format
        def convert_key_sequence(key_seq_str):
            if not key_seq_str:
                return []
            keys = key_seq_str.split("+")
            pynput_keys = []
            for key in keys:
                key = key.strip()
                if key in key_map:
                    pynput_keys.append(key_map[key])
                else:
                    # For single character keys (a, b, c, etc.)
                    pynput_keys.append(key.lower())
            return tuple(pynput_keys)
        
        # Add hotkeys from settings
        record_hotkey = self.settings.value("hotkey_record", "Ctrl+Alt+R")
        if record_hotkey:
            pynput_seq = convert_key_sequence(record_hotkey)
            if pynput_seq:
                self.hotkey_manager.add_hotkey(pynput_seq, "start_recording")
        
        copy_hotkey = self.settings.value("hotkey_copy", "Ctrl+Alt+C")
        if copy_hotkey:
            pynput_seq = convert_key_sequence(copy_hotkey)
            if pynput_seq:
                self.hotkey_manager.add_hotkey(pynput_seq, "copy_text")
        
        clear_hotkey = self.settings.value("hotkey_clear", "Ctrl+Alt+X")
        if clear_hotkey:
            pynput_seq = convert_key_sequence(clear_hotkey)
            if pynput_seq:
                self.hotkey_manager.add_hotkey(pynput_seq, "clear_text")
    
    def apply_recognition_settings(self):
        """Apply speech recognition settings from QSettings to the speech recognizer."""
        recognition_settings = {
            'energy_threshold': self.settings.value("recognition_energy_threshold", 300, type=int),
            'dynamic_energy_threshold': self.settings.value("recognition_dynamic_energy", True, type=bool),
            'energy_adjustment_ratio': self.settings.value("recognition_energy_adjustment", 15, type=int) / 10.0,
            'pause_threshold': self.settings.value("recognition_pause_threshold", 8, type=int) / 10.0,
            'apply_audio_normalization': self.settings.value("recognition_audio_normalization", True, type=bool),
            'phrase_similarity_threshold': self.settings.value("recognition_similarity_threshold", 65, type=int) / 100.0
        }
        
        self.speech_recognizer.update_recognition_settings(recognition_settings)
    
    def show_custom_terms_dialog(self):
        """Show dialog for managing custom terms."""
        from PyQt6.QtWidgets import (
            QDialog, QVBoxLayout, QHBoxLayout, QLabel, 
            QPushButton, QListWidget, QLineEdit, QComboBox, 
            QMessageBox
        )
        
        dialog = QDialog(self)
        dialog.setWindowTitle("Управление специальными терминами")
        dialog.setMinimumSize(500, 400)
        
        layout = QVBoxLayout(dialog)
        
        # Language selection
        lang_layout = QHBoxLayout()
        lang_label = QLabel("Язык:")
        lang_combo = QComboBox()
        lang_combo.addItem("Русский", "ru-RU")
        lang_combo.addItem("English", "en-US")
        
        # Set current language to match main window
        current_index = lang_combo.findData(self.speech_recognizer.language)
        if current_index >= 0:
            lang_combo.setCurrentIndex(current_index)
            
        lang_layout.addWidget(lang_label)
        lang_layout.addWidget(lang_combo)
        lang_layout.addStretch()
        
        layout.addLayout(lang_layout)
        
        # List of terms
        terms_list = QListWidget()
        layout.addWidget(QLabel("Специальные термины:"))
        layout.addWidget(terms_list)
        
        # Add term controls
        add_layout = QHBoxLayout()
        term_input = QLineEdit()
        term_input.setPlaceholderText("Введите специальный термин...")
        add_button = QPushButton("Добавить")
        remove_button = QPushButton("Удалить")
        remove_button.setEnabled(False)
        
        add_layout.addWidget(term_input)
        add_layout.addWidget(add_button)
        add_layout.addWidget(remove_button)
        
        layout.addLayout(add_layout)
        
        # Instructions
        help_text = """
        <p>Добавьте специальные термины, которые часто используются в вашей речи, 
        но редко встречаются в обычных текстах (например, специальные технические термины, 
        названия продуктов, имена собственные).</p>
        
        <p>Система будет улучшать распознавание этих терминов при обработке речи.</p>
        """
        help_label = QLabel(help_text)
        help_label.setWordWrap(True)
        layout.addWidget(help_label)
        
        # Dialog buttons
        button_layout = QHBoxLayout()
        close_button = QPushButton("Закрыть")
        button_layout.addStretch()
        button_layout.addWidget(close_button)
        
        layout.addLayout(button_layout)
        
        # Load initial terms
        def load_terms():
            terms_list.clear()
            language_code = lang_combo.currentData()
            terms = self.speech_recognizer.get_custom_terms(language_code)
            for term in terms:
                terms_list.addItem(term)
                
        load_terms()
        
        # Connect signals
        def add_term():
            term = term_input.text().strip()
            if not term:
                return
                
            if len(term) < 3:
                QMessageBox.warning(dialog, "Предупреждение", 
                                    "Термин должен содержать не менее 3 символов")
                return
                
            language_code = lang_combo.currentData()
            self.speech_recognizer.add_custom_term(term, language_code)
            load_terms()
            term_input.clear()
            
        def remove_term():
            selected_items = terms_list.selectedItems()
            if not selected_items:
                return
                
            term = selected_items[0].text()
            language_code = lang_combo.currentData()
            self.speech_recognizer.remove_custom_term(term, language_code)
            load_terms()
            
        def on_selection_changed():
            remove_button.setEnabled(len(terms_list.selectedItems()) > 0)
            
        def on_language_changed():
            load_terms()
            
        add_button.clicked.connect(add_term)
        remove_button.clicked.connect(remove_term)
        close_button.clicked.connect(dialog.accept)
        term_input.returnPressed.connect(add_term)
        terms_list.itemSelectionChanged.connect(on_selection_changed)
        lang_combo.currentIndexChanged.connect(on_language_changed)
        
        dialog.exec()
    
    def closeEvent(self, event):
        """Handle window close events."""
        # Save settings
        self.settings.setValue("geometry", self.saveGeometry())
        
        # Clean up
        self.hotkey_manager.stop_listening()
        
        # Close to tray if minimizing to tray is enabled
        minimize_to_tray = self.settings.value("minimize_to_tray", True, type=bool)
        if minimize_to_tray and not self.isHidden():
            event.ignore()
            self.hide()
            self.tray_icon.showMessage(
                "Голосовой Ввод",
                "Приложение продолжает работать в фоне",
                QSystemTrayIcon.MessageIcon.Information,
                2000
            )
        else:
            # Ensure all threads are properly stopped before accepting the event
            if self.speech_recognizer.isRunning():
                self.speech_recognizer.stop_listening()
                self.speech_recognizer.wait()  # Wait for thread to finish
            
            # Make sure system tray is properly cleaned up
            if hasattr(self, 'tray_icon') and self.tray_icon is not None:
                self.tray_icon.hide()
            
            event.accept()
    
    def _update_performance_info(self):
        """Обновляет информацию о режиме работы и производительности в интерфейсе."""
        if not hasattr(self, 'status_bar') or not self.hybrid_mode_info:
            return
            
        mode = self.hybrid_mode_info.get('mode', 'unknown')
        perf_level = self.hybrid_mode_info.get('performance_level', 0)
        
        # Добавляем информацию о режиме в статус-бар
        mode_label = self.status_bar.findChild(QLabel, "mode_label")
        if not mode_label:
            mode_label = QLabel()
            mode_label.setObjectName("mode_label")
            self.status_bar.addPermanentWidget(mode_label)
            
        # Форматируем сообщение о режиме
        mode_names = {
            'hybrid_full': 'Полный гибридный режим',
            'hybrid_partial': 'Частичный гибридный режим',
            'pure_python': 'Чистый Python',
            'light': 'Легкий режим'
        }
        
        mode_text = mode_names.get(mode, f"Режим: {mode}")
        perf_icons = ['⚠️', '▪', '▪▪', '▪▪▪', '▪▪▪▪', '▪▪▪▪▪']
        perf_icon = perf_icons[min(perf_level, 5)]
        
        mode_label.setText(f"{mode_text} | Производительность: {perf_icon}")
        
        # Устанавливаем подсказку с деталями
        extensions = [k for k, v in self.hybrid_mode_info.get('extensions_available', {}).items() if v]
        system_info = self.hybrid_mode_info.get('system_info', {})
        
        tooltip = f"Режим: {mode_text}\n"
        tooltip += f"Уровень производительности: {perf_level}/5\n"
        tooltip += f"Доступные расширения: {', '.join(extensions) if extensions else 'нет'}\n"
        tooltip += f"ОС: {system_info.get('os', 'неизвестно')}\n"
        tooltip += f"Python: {system_info.get('python_version', 'неизвестно')}"
        
        mode_label.setToolTip(tooltip) 