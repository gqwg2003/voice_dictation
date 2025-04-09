#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Hotkey manager module for the voice dictation application.
Provides secure global hotkey functionality with input validation and error handling.
"""

# ==== IMPORTS ====
import logging
import threading
from pynput import keyboard
from PyQt6.QtCore import QObject, pyqtSignal

# ==== LOGGER SETUP ====
logger = logging.getLogger('voice_dictation.hotkey_manager')

# ==== HOTKEY MANAGER CLASS ====
class HotkeyManager(QObject):
    """
    Module for securely managing global hotkeys with input validation.
    """
    # Signal emitted when a registered hotkey is triggered
    hotkey_triggered = pyqtSignal(str)
    
    def __init__(self):
        """Initialize the hotkey manager."""
        super().__init__()
        self.listener = None
        self.hotkeys = {}
        self.current_keys = set()
        self.is_active = False
        self._listener_lock = threading.Lock()  # Add thread safety
    
    # ==== PUBLIC METHODS ====
    def add_hotkey(self, key_combination, action_name):
        """
        Register a hotkey combination with input validation.
        
        Args:
            key_combination (tuple): Tuple of keyboard keys
            action_name (str): Name of the action to be triggered
            
        Returns:
            bool: True if hotkey was added successfully, False otherwise
        """
        # Input validation
        if not isinstance(key_combination, (tuple, list)):
            logger.error(f"Invalid key combination type: {type(key_combination)}")
            return False
        
        if not key_combination:
            logger.error("Empty key combination")
            return False
            
        if not isinstance(action_name, str) or not action_name:
            logger.error(f"Invalid action name: {action_name}")
            return False
        
        # Convert all keys to strings for consistency
        key_combo = tuple(str(k).lower() if isinstance(k, str) else str(k) for k in key_combination)
        
        # Add the hotkey
        try:
            self.hotkeys[key_combo] = action_name
            logger.debug(f"Added hotkey {key_combo} for action '{action_name}'")
            return True
        except Exception as e:
            logger.error(f"Error adding hotkey: {e}")
            return False
    
    def remove_hotkey(self, key_combination):
        """
        Remove a registered hotkey with input validation.
        
        Args:
            key_combination (tuple): Tuple of keyboard keys to remove
            
        Returns:
            bool: True if hotkey was removed successfully, False otherwise
        """
        # Input validation
        if not isinstance(key_combination, (tuple, list)):
            logger.error(f"Invalid key combination type: {type(key_combination)}")
            return False
        
        # Convert all keys to strings for consistency
        key_combo = tuple(str(k).lower() if isinstance(k, str) else str(k) for k in key_combination)
        
        # Remove the hotkey if it exists
        try:
            if key_combo in self.hotkeys:
                action = self.hotkeys[key_combo]
                del self.hotkeys[key_combo]
                logger.debug(f"Removed hotkey {key_combo} for action '{action}'")
                return True
            else:
                logger.warning(f"Hotkey {key_combo} not found")
                return False
        except Exception as e:
            logger.error(f"Error removing hotkey: {e}")
            return False
    
    def start_listening(self):
        """
        Start listening for keyboard events with thread safety.
        
        Returns:
            bool: True if started successfully, False otherwise
        """
        with self._listener_lock:
            if not self.is_active:
                try:
                    self.listener = keyboard.Listener(
                        on_press=self._on_press, 
                        on_release=self._on_release
                    )
                    self.listener.start()
                    self.is_active = True
                    logger.debug("Hotkey listener started")
                    return True
                except Exception as e:
                    logger.error(f"Error starting hotkey listener: {e}")
                    self.is_active = False
                    return False
            return True  # Already active
    
    def stop_listening(self):
        """
        Stop listening for keyboard events with thread safety.
        
        Returns:
            bool: True if stopped successfully, False otherwise
        """
        with self._listener_lock:
            if self.is_active and self.listener:
                try:
                    self.listener.stop()
                    self.is_active = False
                    self.current_keys.clear()
                    logger.debug("Hotkey listener stopped")
                    return True
                except Exception as e:
                    logger.error(f"Error stopping hotkey listener: {e}")
                    return False
            return True  # Already inactive
    
    # ==== PRIVATE METHODS ====
    def _on_press(self, key):
        """
        Handle key press events with error handling.
        
        Args:
            key: The key that was pressed
            
        Returns:
            bool: True to continue listening, False to stop
        """
        try:
            # Convert key to a hashable representation
            key_str = self._convert_key_to_string(key)
            if not key_str:
                return True
                
            logger.debug(f"Key pressed: {key_str}")
            self.current_keys.add(key_str)
            logger.debug(f"Current keys: {self.current_keys}")
            
            # Check if any registered hotkey combination is pressed
            for hotkey_combo, action in self.hotkeys.items():
                logger.debug(f"Checking hotkey combo: {hotkey_combo}")
                # Only trigger if all keys in the combo are in the current keys
                # and the number of keys is the same (exact match)
                if set(hotkey_combo).issubset(self.current_keys) and len(hotkey_combo) == len(self.current_keys):
                    logger.debug(f"Hotkey triggered: {hotkey_combo} -> {action}")
                    self.hotkey_triggered.emit(action)
                    # Prevent from emitting multiple times
                    break
            
            return True
        except Exception as e:
            # Log the error but don't stop the listener
            logger.error(f"Error in key press event: {e}")
            return True
    
    def _on_release(self, key):
        """
        Handle key release events with error handling.
        
        Args:
            key: The key that was released
            
        Returns:
            bool: True to continue listening, False to stop
        """
        try:
            # Convert key to a hashable representation
            key_str = self._convert_key_to_string(key)
            if not key_str:
                return True
                
            if key_str in self.current_keys:
                self.current_keys.remove(key_str)
            
            return True
        except Exception as e:
            # Log the error but don't stop the listener
            logger.error(f"Error in key release event: {e}")
            return True
    
    def _convert_key_to_string(self, key):
        """
        Convert a key to a consistent string representation.
        
        Args:
            key: Key to convert
            
        Returns:
            str: String representation of the key, or None if conversion fails
        """
        try:
            if isinstance(key, keyboard.Key):
                # Remove 'Key.' prefix from key names
                return str(key).replace('Key.', '').lower()
            elif hasattr(key, 'char') and key.char:
                return key.char.lower()
            elif hasattr(key, 'vk'):
                # Handle virtual keys
                return str(key.vk).lower()
            else:
                return str(key).lower()
        except Exception as e:
            logger.error(f"Error converting key to string: {e}")
            return None
    
    # ==== CLEANUP ====
    def __del__(self):
        """Cleanup when object is destroyed."""
        self.stop_listening() 