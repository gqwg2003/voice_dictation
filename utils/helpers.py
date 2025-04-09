#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Helper functions for the voice dictation application.
Provides utility functions for common operations.
"""

import os
import sys
import logging
import platform
import subprocess
from typing import Optional, List, Tuple
from pathlib import Path

# ==== LOGGER SETUP ====
logger = logging.getLogger('voice_dictation.helpers')

# ==== HELPER FUNCTIONS ====
def get_app_path() -> str:
    """
    Get the application's base directory path.
    
    Returns:
        str: Application path
    """
    if getattr(sys, 'frozen', False):
        # Running as compiled executable
        return os.path.dirname(sys.executable)
    else:
        # Running as script
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        
def get_resource_path(relative_path: str) -> str:
    """
    Get absolute path to a resource file.
    
    Args:
        relative_path (str): Relative path to resource
        
    Returns:
        str: Absolute resource path
    """
    base_path = get_app_path()
    return os.path.join(base_path, 'resources', relative_path)
    
def get_log_path() -> str:
    """
    Get the path to the logs directory.
    
    Returns:
        str: Logs directory path
    """
    return os.path.join(get_app_path(), 'logs')
    
def ensure_directories() -> bool:
    """
    Ensure required directories exist.
    
    Returns:
        bool: Success status
    """
    try:
        directories = [
            get_log_path(),
            get_resource_path(''),
            os.path.join(get_app_path(), 'temp')
        ]
        
        for directory in directories:
            os.makedirs(directory, exist_ok=True)
        return True
    except Exception as e:
        logger.error(f"Error creating directories: {e}")
        return False
        
def is_windows() -> bool:
    """
    Check if running on Windows.
    
    Returns:
        bool: True if Windows
    """
    return platform.system().lower() == 'windows'
    
def is_linux() -> bool:
    """
    Check if running on Linux.
    
    Returns:
        bool: True if Linux
    """
    return platform.system().lower() == 'linux'
    
def is_mac() -> bool:
    """
    Check if running on macOS.
    
    Returns:
        bool: True if macOS
    """
    return platform.system().lower() == 'darwin'
    
def get_system_info() -> dict:
    """
    Get system information.
    
    Returns:
        dict: System information
    """
    return {
        'platform': platform.system(),
        'platform_version': platform.version(),
        'platform_release': platform.release(),
        'architecture': platform.machine(),
        'processor': platform.processor(),
        'python_version': platform.python_version()
    }
    
def run_command(command: List[str], cwd: Optional[str] = None) -> Tuple[int, str, str]:
    """
    Run a shell command.
    
    Args:
        command (List[str]): Command and arguments
        cwd (Optional[str]): Working directory
        
    Returns:
        Tuple[int, str, str]: (returncode, stdout, stderr)
    """
    try:
        process = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=cwd,
            text=True
        )
        stdout, stderr = process.communicate()
        return process.returncode, stdout, stderr
    except Exception as e:
        logger.error(f"Error running command '{' '.join(command)}': {e}")
        return -1, "", str(e)
        
def format_size(size_bytes: int) -> str:
    """
    Format size in bytes to human readable string.
    
    Args:
        size_bytes (int): Size in bytes
        
    Returns:
        str: Formatted size string
    """
    for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
        if size_bytes < 1024.0:
            return f"{size_bytes:.1f} {unit}"
        size_bytes /= 1024.0
    return f"{size_bytes:.1f} PB"
    
def format_time(seconds: float) -> str:
    """
    Format time in seconds to human readable string.
    
    Args:
        seconds (float): Time in seconds
        
    Returns:
        str: Formatted time string
    """
    if seconds < 60:
        return f"{seconds:.1f} seconds"
    minutes = seconds / 60
    if minutes < 60:
        return f"{minutes:.1f} minutes"
    hours = minutes / 60
    if hours < 24:
        return f"{hours:.1f} hours"
    days = hours / 24
    return f"{days:.1f} days"
    
def sanitize_filename(filename: str) -> str:
    """
    Sanitize a filename to be safe for all platforms.
    
    Args:
        filename (str): Original filename
        
    Returns:
        str: Sanitized filename
    """
    # Replace invalid characters
    invalid_chars = '<>:"/\\|?*'
    for char in invalid_chars:
        filename = filename.replace(char, '_')
        
    # Remove leading/trailing spaces and dots
    filename = filename.strip('. ')
    
    # Ensure filename is not empty
    if not filename:
        filename = 'unnamed'
        
    return filename 