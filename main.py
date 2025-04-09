#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Voice Dictation Application

This application allows you to dictate text using voice recognition and
easily copy it to the clipboard for use in games, social media, etc.
"""

# ==== IMPORTS ====
# System imports
import sys
import logging
import signal
import subprocess
import os
import atexit

# Qt imports
from PyQt6.QtWidgets import QApplication

# Project modules
from utils.logger import setup_logger
from utils.version import get_version
from utils.version_updater import update_version, get_runtime_version, get_runtime_app_info
from gui.main_window import MainWindow

# ==== SIGNAL HANDLING ====
def signal_handler(sig, frame):
    """Handle termination signals."""
    logger.info(f"Received signal {sig}, exiting gracefully")
    sys.exit(0)

# ==== VERSION MANAGEMENT ====
def force_update_version():
    """
    Force update version from Git tags, regardless of cache.
    This ensures the version is always up-to-date on application start.
    """
    try:
        # Force git to update tags
        subprocess.run(["git", "fetch", "--tags"], check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        # Update version information
        return update_version()
    except Exception as e:
        print(f"Warning: Failed to force update version: {e}")
        # Fallback to regular update
        return update_version()

# ==== CLEANUP ====
def cleanup():
    """Clean up resources upon application exit."""
    # This ensures any remaining resources get cleaned up
    pass

# ==== APPLICATION INITIALIZATION ====
def main():
    """Main entry point for the application."""
    # Initialize version information
    print("Updating version information...")
    version_data = force_update_version()
    print(f"Version: {version_data['display_version']}")
    print(f"Build Number: {version_data['build_number']}")
    
    # Get runtime version info
    app_version = get_runtime_version()
    app_info = get_runtime_app_info()
    
    # Setup logging
    global logger
    logger = setup_logger(level=logging.INFO)
    logger.info(f"Starting Voice Dictation Application v{app_version}")
    
    # Register signal handlers for graceful shutdown
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Register cleanup function
    atexit.register(cleanup)
    
    # Initialize Qt application
    app = QApplication(sys.argv)
    app.setStyle('Fusion')  # Using Fusion style for a more professional look
    
    # Create and show main window
    window = MainWindow()
    window.show()
    
    # Start application event loop
    return app.exec()

# ==== DIRECT EXECUTION ====
if __name__ == '__main__':
    sys.exit(main()) 