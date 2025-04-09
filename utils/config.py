#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Configuration manager for the voice dictation application.
Provides centralized configuration management with persistence.
"""

import os
import json
import logging
from typing import Any, Dict, Optional
from PyQt6.QtCore import QSettings

# ==== LOGGER SETUP ====
logger = logging.getLogger('voice_dictation.config')

# ==== CONFIGURATION MANAGER CLASS ====
class ConfigManager:
    """
    Manages application configuration with persistence.
    """
    def __init__(self):
        """Initialize the configuration manager."""
        self.settings = QSettings("VoiceDictation", "Settings")
        self._default_config = {
            "appearance": {
                "theme": "light",
                "font_size": 12,
                "font_family": "Segoe UI"
            },
            "hotkeys": {
                "start_recognition": "Ctrl+Shift+R",
                "stop_recognition": "Ctrl+Shift+S",
                "copy_to_clipboard": "Ctrl+C"
            },
            "recognition": {
                "language": "ru-RU",
                "energy_threshold": 300,
                "dynamic_energy_threshold": True,
                "pause_threshold": 0.8
            },
            "behavior": {
                "auto_copy": True,
                "auto_clear": False,
                "show_notifications": True
            }
        }
        
    def get(self, key: str, default: Any = None) -> Any:
        """
        Get a configuration value.
        
        Args:
            key (str): Configuration key
            default (Any): Default value if key not found
            
        Returns:
            Any: Configuration value
        """
        try:
            value = self.settings.value(key)
            if value is None:
                return self._get_default(key, default)
            return value
        except Exception as e:
            logger.error(f"Error getting config value for key '{key}': {e}")
            return self._get_default(key, default)
            
    def set(self, key: str, value: Any) -> bool:
        """
        Set a configuration value.
        
        Args:
            key (str): Configuration key
            value (Any): Configuration value
            
        Returns:
            bool: Success status
        """
        try:
            self.settings.setValue(key, value)
            self.settings.sync()
            return True
        except Exception as e:
            logger.error(f"Error setting config value for key '{key}': {e}")
            return False
            
    def reset(self) -> bool:
        """
        Reset all configuration to defaults.
        
        Returns:
            bool: Success status
        """
        try:
            self.settings.clear()
            self.settings.sync()
            return True
        except Exception as e:
            logger.error(f"Error resetting configuration: {e}")
            return False
            
    def _get_default(self, key: str, default: Any = None) -> Any:
        """
        Get default value for a key.
        
        Args:
            key (str): Configuration key
            default (Any): Default value if key not found
            
        Returns:
            Any: Default value
        """
        try:
            keys = key.split('/')
            current = self._default_config
            for k in keys:
                if k in current:
                    current = current[k]
                else:
                    return default
            return current
        except Exception as e:
            logger.error(f"Error getting default value for key '{key}': {e}")
            return default
            
    def export_config(self, file_path: str) -> bool:
        """
        Export configuration to a file.
        
        Args:
            file_path (str): Path to export file
            
        Returns:
            bool: Success status
        """
        try:
            config = {}
            for key in self.settings.allKeys():
                config[key] = self.settings.value(key)
                
            with open(file_path, 'w', encoding='utf-8') as f:
                json.dump(config, f, indent=4)
            return True
        except Exception as e:
            logger.error(f"Error exporting configuration: {e}")
            return False
            
    def import_config(self, file_path: str) -> bool:
        """
        Import configuration from a file.
        
        Args:
            file_path (str): Path to import file
            
        Returns:
            bool: Success status
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                config = json.load(f)
                
            for key, value in config.items():
                self.settings.setValue(key, value)
            self.settings.sync()
            return True
        except Exception as e:
            logger.error(f"Error importing configuration: {e}")
            return False 