#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Version updater module for automatic version synchronization.
This module synchronizes the version number used by the application at runtime.
"""

import os
import json
import datetime
from typing import Dict, Any, Optional
from . import version

# Path to version data file relative to this file
VERSION_DATA_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'version_data.json')

def get_cached_version_data() -> Dict[str, Any]:
    """
    Get cached version data from file.
    If file doesn't exist or is corrupted, returns empty dict.
    
    Returns:
        Dict[str, Any]: Cached version data or empty dict
    """
    try:
        if os.path.exists(VERSION_DATA_FILE):
            with open(VERSION_DATA_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
    except (json.JSONDecodeError, IOError, OSError) as e:
        print(f"Error reading version data: {e}")
    
    return {}

def save_version_data(data: Dict[str, Any]) -> bool:
    """
    Save version data to cache file.
    
    Args:
        data: Version data dictionary
        
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        # Create directory if it doesn't exist
        os.makedirs(os.path.dirname(VERSION_DATA_FILE), exist_ok=True)
        
        with open(VERSION_DATA_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)
        return True
    except (IOError, OSError) as e:
        print(f"Error saving version data: {e}")
        return False

def update_version() -> Dict[str, Any]:
    """
    Update version information in the application.
    Retrieves current version and saves to cache file.
    
    Returns:
        Dict[str, Any]: Updated version information
    """
    # Get current version from Git
    current_version = version.get_version()
    app_info = version.get_app_info()
    
    # Create version data object
    version_data = {
        "version": current_version,
        "display_version": app_info["version"],
        "is_prerelease": app_info["is_prerelease"],
        "updated_at": datetime.datetime.now().isoformat(),
        "build_number": _get_build_number(),
        "app_info": app_info
    }
    
    # Save to cache file
    save_version_data(version_data)
    
    return version_data

def _get_build_number() -> int:
    """
    Get build number from environment variables or increment existing.
    
    Returns:
        int: Build number
    """
    # Check for override from our own script
    if 'OVERRIDE_BUILD_NUMBER' in os.environ:
        return int(os.environ['OVERRIDE_BUILD_NUMBER'])
        
    # Check for CI/CD environment variables
    if 'CI_PIPELINE_ID' in os.environ:
        return int(os.environ['CI_PIPELINE_ID'])
    elif 'GITHUB_RUN_NUMBER' in os.environ:
        return int(os.environ['GITHUB_RUN_NUMBER'])
    
    # If not in CI/CD, increment existing build number
    cached_data = get_cached_version_data()
    build_number = cached_data.get('build_number', 0) + 1
    
    return build_number

def ensure_version_updated() -> Dict[str, Any]:
    """
    Ensure version information is up to date.
    Always refreshes version from Git if running in development mode.
    
    Returns:
        Dict[str, Any]: Current version information
    """
    cached_data = get_cached_version_data()
    
    # Check if we're in a development environment
    # In development, we always want the latest version
    if os.environ.get('VOICE_DICTATION_DEV', '').lower() in ('true', '1', 'yes'):
        return update_version()
    
    # Check if we need to update
    # We'll check based on time and Git state
    need_update = True
    if cached_data and 'updated_at' in cached_data:
        try:
            last_update = datetime.datetime.fromisoformat(cached_data['updated_at'])
            time_diff = datetime.datetime.now() - last_update
            
            # Update if last update was more than 6 hours ago (reduced from 1 day)
            need_update = time_diff.total_seconds() >= 6 * 60 * 60  # 6 hours
        except (ValueError, TypeError):
            # If date parsing fails, update anyway
            need_update = True
    
    # Update if needed
    if need_update:
        return update_version()
    
    return cached_data

def get_runtime_version() -> str:
    """
    Get the version for runtime use.
    This uses the cached version if available, or fetches it.
    Use this function in the application to ensure fast startup.
    
    Returns:
        str: Version string for runtime use
    """
    # Ensure we have updated version data
    version_data = ensure_version_updated()
    
    # Return display version or fallback to default
    return version_data.get('display_version', version.DEFAULT_VERSION)

def get_runtime_app_info() -> Dict[str, Any]:
    """
    Get application info for runtime use.
    This uses the cached info if available, or fetches it.
    
    Returns:
        Dict[str, Any]: Application info for runtime use
    """
    # Ensure we have updated version data
    version_data = ensure_version_updated()
    
    # Return app info or fallback to generating it
    if 'app_info' in version_data:
        return version_data['app_info']
    
    return version.get_app_info()

if __name__ == "__main__":
    # When run as script, update version and print info
    version_data = update_version()
    print(f"Updated version to: {version_data['display_version']}")
    print(f"Build number: {version_data['build_number']}")
    print(f"Updated at: {version_data['updated_at']}")
    print(f"Is prerelease: {version_data['is_prerelease']}") 