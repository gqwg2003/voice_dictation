#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Logging module for the voice dictation application.
Provides secure logging functionality with data filtering and log rotation.
"""

# ==== IMPORTS ====
import logging
import os
import sys
import re
import stat
from logging.handlers import RotatingFileHandler
from datetime import datetime

# ==== CONSTANTS ====
MAX_LOG_SIZE = 5 * 1024 * 1024  # 5MB
BACKUP_COUNT = 3
SENSITIVE_PATTERNS = [
    r'(password)\s*=\s*\S+',
    r'(token)\s*=\s*\S+',
    r'(secret)\s*=\s*\S+',
    r'(key)\s*=\s*\S+',
    r'((api[-_]?key)|apikey)\s*=\s*\S+',
    r'(access[-_]?token)\s*=\s*\S+'
]

# ==== FILTER CLASS ====
class SensitiveDataFilter(logging.Filter):
    """Filter to remove sensitive data from log messages."""
    
    def __init__(self, patterns=SENSITIVE_PATTERNS):
        """
        Initialize the filter with patterns to match sensitive data.
        
        Args:
            patterns (list): List of regex patterns for sensitive data
        """
        super().__init__()
        self.patterns = [re.compile(pattern, re.IGNORECASE) for pattern in patterns]
    
    def filter(self, record):
        """
        Filter log records to remove sensitive information.
        
        Args:
            record: Log record to filter
            
        Returns:
            bool: True to include the record (after sanitizing)
        """
        if record.getMessage():
            # Make a copy of the message to sanitize
            message = record.getMessage()
            
            # Replace sensitive data with [REDACTED]
            for pattern in self.patterns:
                message = pattern.sub(r'\1=[REDACTED]', message)
            
            # If message was changed, update the record
            if message != record.getMessage():
                record.msg = message
                # Update args if present
                if record.args:
                    record.args = ()
        
        return True

# ==== LOGGER SETUP ====
def setup_logger(name='voice_dictation', level=logging.INFO):
    """
    Set up and configure a secure logger for the application.
    
    Args:
        name (str): Logger name
        level (int): Logging level
        
    Returns:
        logging.Logger: Configured logger instance
    """
    # Create logger
    logger = logging.getLogger(name)
    logger.setLevel(level)
    
    # Remove existing handlers if any
    for handler in logger.handlers[:]:
        logger.removeHandler(handler)
    
    # Create formatter
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    
    # Add sensitive data filter
    sensitive_filter = SensitiveDataFilter()
    logger.addFilter(sensitive_filter)
    
    # Create console handler
    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)
    
    # Create logs directory if it doesn't exist
    logs_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'logs')
    os.makedirs(logs_dir, exist_ok=True)
    
    # Log file with date in filename
    log_filename = os.path.join(logs_dir, f'voice_dictation_{datetime.now().strftime("%Y%m%d")}.log')
    
    # Use rotating file handler to limit file size and manage backups
    file_handler = RotatingFileHandler(
        log_filename,
        maxBytes=MAX_LOG_SIZE,
        backupCount=BACKUP_COUNT
    )
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)
    
    # Secure log files (0600 permissions - owner read/write only)
    try:
        # Set secure permissions on the log file
        os.chmod(log_filename, stat.S_IRUSR | stat.S_IWUSR)
    except Exception as e:
        # Log but continue if we can't set permissions
        print(f"Warning: Could not set secure permissions on log file: {e}")
    
    return logger 