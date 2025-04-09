#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Cleanup script for the voice dictation application.
Removes temporary files, logs, and cache files to maintain security and clean workspace.
"""

import os
import sys
import shutil
import argparse
import logging
from pathlib import Path
from datetime import datetime, timedelta

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger('cleanup')

# Get project root directory
PROJECT_ROOT = Path(__file__).resolve().parent.parent

# Default paths to clean relative to project root
DEFAULT_CLEANUP_PATTERNS = [
    # Python cache files
    "**/__pycache__",
    "**/*.py[cod]",
    
    # Log files (older than specified days)
    "logs/*.log*",
    
    # Build and temp directories
    "build/",
    "dist/",
    "temp/",
    "tmp/",
    
    # Temporary editor files
    "**/*.bak",
    "**/*.swp",
    "**/*.tmp",
    "**/*~",
]

def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description="Clean temporary and unnecessary files")
    parser.add_argument(
        "--log-age", type=int, default=30,
        help="Delete logs older than specified days (default: 30)"
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Show what would be deleted without actually deleting"
    )
    parser.add_argument(
        "--all-logs", action="store_true",
        help="Delete all logs, regardless of age"
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true",
        help="Show detailed information about deleted files"
    )
    return parser.parse_args()

def is_safe_to_delete(path, log_max_age, delete_all_logs):
    """
    Check if it's safe to delete a file.
    
    Args:
        path (Path): Path to file
        log_max_age (int): Maximum age for log files in days
        delete_all_logs (bool): Whether to delete all logs regardless of age
        
    Returns:
        bool: True if safe to delete, False otherwise
    """
    # Don't delete directories directly, we'll handle their contents
    if path.is_dir():
        return False

    # For log files, check age
    if "logs" in path.parts and path.suffix == ".log" and not delete_all_logs:
        try:
            file_time = datetime.fromtimestamp(path.stat().st_mtime)
            if datetime.now() - file_time < timedelta(days=log_max_age):
                return False
        except (OSError, ValueError) as e:
            logger.warning(f"Could not check age of {path}: {e}")
            return False
            
    return True

def cleanup_files(args):
    """
    Clean up temporary and unnecessary files.
    
    Args:
        args: Command line arguments
    """
    deleted_count = 0
    total_size = 0
    
    for pattern in DEFAULT_CLEANUP_PATTERNS:
        for path in Path(PROJECT_ROOT).glob(pattern):
            if path.exists():
                if path.is_dir():
                    # For directories, check all files inside
                    for file_path in path.rglob("*"):
                        if file_path.is_file() and is_safe_to_delete(file_path, args.log_age, args.all_logs):
                            try:
                                size = file_path.stat().st_size
                                
                                if args.verbose:
                                    logger.info(f"Deleting file: {file_path.relative_to(PROJECT_ROOT)}")
                                
                                if not args.dry_run:
                                    file_path.unlink()
                                
                                deleted_count += 1
                                total_size += size
                            except OSError as e:
                                logger.error(f"Error deleting {file_path}: {e}")
                    
                    # Delete empty directories
                    if not args.dry_run and not any(path.iterdir()):
                        try:
                            path.rmdir()
                            if args.verbose:
                                logger.info(f"Deleted empty directory: {path.relative_to(PROJECT_ROOT)}")
                        except OSError as e:
                            logger.error(f"Error deleting directory {path}: {e}")
                
                else:  # It's a file
                    if is_safe_to_delete(path, args.log_age, args.all_logs):
                        try:
                            size = path.stat().st_size
                            
                            if args.verbose:
                                logger.info(f"Deleting file: {path.relative_to(PROJECT_ROOT)}")
                            
                            if not args.dry_run:
                                path.unlink()
                            
                            deleted_count += 1
                            total_size += size
                        except OSError as e:
                            logger.error(f"Error deleting {path}: {e}")
    
    # Convert total size to human-readable format
    size_str = format_size(total_size)
    
    if args.dry_run:
        logger.info(f"Dry run completed. Would delete {deleted_count} files ({size_str})")
    else:
        logger.info(f"Cleanup completed. Deleted {deleted_count} files ({size_str})")

def format_size(size_bytes):
    """
    Format size in bytes to human-readable string.
    
    Args:
        size_bytes (int): Size in bytes
        
    Returns:
        str: Human-readable size
    """
    if size_bytes == 0:
        return "0 B"
    
    size_names = ("B", "KB", "MB", "GB")
    i = 0
    while size_bytes >= 1024 and i < len(size_names) - 1:
        size_bytes /= 1024
        i += 1
        
    return f"{size_bytes:.2f} {size_names[i]}"

if __name__ == "__main__":
    try:
        args = parse_args()
        cleanup_files(args)
    except KeyboardInterrupt:
        logger.info("Cleanup interrupted by user")
        sys.exit(1)
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        sys.exit(1) 