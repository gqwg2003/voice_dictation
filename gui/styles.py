#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Application styles and themes.
Provides CSS styles for the application UI.
"""

# Dark theme style
DARK_THEME = """
/* Global styles */
QWidget {
    background-color: #2D2D30;
    color: #FFFFFF;
    font-family: 'Segoe UI', Arial, sans-serif;
}

/* Main window */
QMainWindow {
    background-color: #1E1E1E;
}

/* Status bar */
QStatusBar {
    background-color: #007ACC;
    color: white;
}

QStatusBar QLabel {
    padding: 3px;
    color: white;
}

/* Menu and toolbar */
QMenuBar {
    background-color: #323233;
}

QMenu {
    background-color: #1E1E1E;
    border: 1px solid #3F3F46;
}

QMenu::item:selected {
    background-color: #007ACC;
}

QToolBar {
    background-color: #333333;
    border-bottom: 1px solid #3F3F46;
}

QToolButton {
    background-color: transparent;
    border: none;
    padding: 6px;
}

QToolButton:hover {
    background-color: #3E3E40;
}

QToolButton:pressed {
    background-color: #007ACC;
}

/* Text edit */
QTextEdit {
    background-color: #1E1E1E;
    color: #DCDCDC;
    border: 1px solid #3F3F46;
    border-radius: 4px;
    padding: 8px;
    selection-background-color: #264F78;
}

/* Buttons */
QPushButton {
    background-color: #0E639C;
    color: white;
    border: none;
    border-radius: 4px;
    padding: 8px 16px;
    text-align: center;
}

QPushButton:hover {
    background-color: #1177BB;
}

QPushButton:pressed {
    background-color: #0E5A8C;
}

QPushButton:disabled {
    background-color: #43434A;
    color: #9D9D9E;
}

QPushButton:checked {
    background-color: #C82829; /* Red for recording state */
}

QPushButton#copyButton {
    background-color: #3C8527;
}

QPushButton#copyButton:hover {
    background-color: #4D9637;
}

QPushButton#clearButton {
    background-color: #6C6C6C;
}

QPushButton#clearButton:hover {
    background-color: #7D7D7D;
}

/* Combo Box */
QComboBox {
    background-color: #3C3C3C;
    color: white;
    border: 1px solid #3F3F46;
    border-radius: 4px;
    padding: 4px;
    min-width: 6em;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border-left: 1px solid #3F3F46;
}

QComboBox::down-arrow {
    image: url(resources/dropdown.png);
}

QComboBox QAbstractItemView {
    background-color: #1E1E1E;
    color: white;
    border: 1px solid #3F3F46;
    selection-background-color: #007ACC;
}

/* Labels */
QLabel {
    color: #DCDCDC;
}

/* Progress bars for visualizer */
QProgressBar {
    background-color: #1E1E1E;
    border: 1px solid #3F3F46;
    border-radius: 2px;
    text-align: center;
}

QProgressBar::chunk {
    background-color: #007ACC;
    width: 1px;
}

/* Audio level indicator */
QProgressBar#audioLevelBar {
    background-color: transparent;
    border: none;
    text-align: right;
}

QProgressBar#audioLevelBar::chunk {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00AA00, stop:0.6 #AAAA00, stop:1 #AA0000);
    border-radius: 2px;
}

/* Recognition status indicator */
QLabel#statusIndicator {
    font-size: 14px;
    font-weight: bold;
    padding: 5px;
    border-radius: 3px;
}
"""

# Light theme style
LIGHT_THEME = """
/* Global styles */
QWidget {
    background-color: #F0F0F0;
    color: #212121;
    font-family: 'Segoe UI', Arial, sans-serif;
}

/* Main window */
QMainWindow {
    background-color: #FFFFFF;
}

/* Status bar */
QStatusBar {
    background-color: #0078D7;
    color: white;
}

QStatusBar QLabel {
    padding: 3px;
    color: white;
}

/* Menu and toolbar */
QMenuBar {
    background-color: #F5F5F5;
}

QMenu {
    background-color: #FFFFFF;
    border: 1px solid #DDDDDD;
}

QMenu::item:selected {
    background-color: #0078D7;
    color: white;
}

QToolBar {
    background-color: #F5F5F5;
    border-bottom: 1px solid #DDDDDD;
}

QToolButton {
    background-color: transparent;
    border: none;
    padding: 6px;
}

QToolButton:hover {
    background-color: #E0E0E0;
}

QToolButton:pressed {
    background-color: #0078D7;
}

/* Text edit */
QTextEdit {
    background-color: #FFFFFF;
    color: #212121;
    border: 1px solid #DDDDDD;
    border-radius: 4px;
    padding: 8px;
    selection-background-color: #ADD6FF;
}

/* Buttons */
QPushButton {
    background-color: #0078D7;
    color: white;
    border: none;
    border-radius: 4px;
    padding: 8px 16px;
    text-align: center;
}

QPushButton:hover {
    background-color: #1A88E1;
}

QPushButton:pressed {
    background-color: #0067C0;
}

QPushButton:disabled {
    background-color: #CCCCCC;
    color: #888888;
}

QPushButton:checked {
    background-color: #E81123; /* Red for recording state */
}

QPushButton#copyButton {
    background-color: #107C10;
}

QPushButton#copyButton:hover {
    background-color: #218E21;
}

QPushButton#clearButton {
    background-color: #6C6C6C;
}

QPushButton#clearButton:hover {
    background-color: #7D7D7D;
}

/* Combo Box */
QComboBox {
    background-color: #FFFFFF;
    color: #212121;
    border: 1px solid #DDDDDD;
    border-radius: 4px;
    padding: 4px;
    min-width: 6em;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 20px;
    border-left: 1px solid #DDDDDD;
}

QComboBox::down-arrow {
    image: url(resources/dropdown.png);
}

QComboBox QAbstractItemView {
    background-color: #FFFFFF;
    color: #212121;
    border: 1px solid #DDDDDD;
    selection-background-color: #0078D7;
    selection-color: white;
}

/* Labels */
QLabel {
    color: #212121;
}

/* Progress bars for visualizer */
QProgressBar {
    background-color: #F0F0F0;
    border: 1px solid #DDDDDD;
    border-radius: 2px;
    text-align: center;
}

QProgressBar::chunk {
    background-color: #0078D7;
    width: 1px;
}

/* Audio level indicator */
QProgressBar#audioLevelBar {
    background-color: transparent;
    border: none;
    text-align: right;
}

QProgressBar#audioLevelBar::chunk {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00AA00, stop:0.6 #AAAA00, stop:1 #AA0000);
    border-radius: 2px;
}

/* Recognition status indicator */
QLabel#statusIndicator {
    font-size: 14px;
    font-weight: bold;
    padding: 5px;
    border-radius: 3px;
}
"""

# Style for recognition status indicator
STATUS_IDLE = "background-color: transparent; color: #888888;"
STATUS_LISTENING = "background-color: #C82829; color: white;"
STATUS_PROCESSING = "background-color: #FF9900; color: white;"
STATUS_SUCCESS = "background-color: #00AA00; color: white;"
STATUS_ERROR = "background-color: #C82829; color: white;" 