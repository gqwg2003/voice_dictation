#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Version tracking module for Voice Dictation application.
This module provides functions for obtaining and managing version information.
Implements Semantic Versioning (SemVer) standard - https://semver.org/
"""

import os
import re
import subprocess
import datetime
from typing import Tuple, Optional

# Default version to use if no version control information is available
DEFAULT_VERSION = "v2.8.7"

# Regular expression for SemVer validation
SEMVER_REGEX = r"^v?(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?(?:\+([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?$"

def get_version() -> str:
    """
    Get the current application version.
    
    The version is determined using the following methods in order:
    1. From git tags if available (must follow SemVer format with 'v' prefix)
    2. From git commits if available 
    3. Default version with date stamp if no git information
    
    Returns:
        str: Version string in format "vX.Y.Z" or "vX.Y.Z-prerelease" or "vX.Y.Z-gitSHA" or "vX.Y.Z-date"
    """
    # Try to get version from git
    git_version = _get_git_version()
    if git_version:
        return git_version
        
    # Fallback to date-based version if not in git repository
    return _get_date_version()

def parse_version(version_str: str) -> Tuple[int, int, int, Optional[str], Optional[str]]:
    """
    Parse a semantic version string into its components.
    
    Args:
        version_str (str): Version string to parse
    
    Returns:
        Tuple[int, int, int, Optional[str], Optional[str]]: 
            (major, minor, patch, prerelease, build)
    
    Raises:
        ValueError: If the version string does not match SemVer format
    """
    match = re.match(SEMVER_REGEX, version_str)
    if not match:
        raise ValueError(f"Invalid semantic version: {version_str}")
    
    major = int(match.group(1))
    minor = int(match.group(2))
    patch = int(match.group(3))
    prerelease = match.group(4)
    build = match.group(5)
    
    return major, minor, patch, prerelease, build

def _get_git_version() -> Optional[str]:
    """
    Get version information from git.
    
    Returns:
        Optional[str]: Git-based version string or None if not in a git repository
    """
    try:
        # Check if we're in a git repository
        result = subprocess.run(
            ["git", "rev-parse", "--is-inside-work-tree"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False
        )
        
        if result.returncode != 0:
            return None
            
        # Try to get the latest tag
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False
        )
        
        if result.returncode == 0:
            tag = result.stdout.strip()
            
            # Ensure tag follows SemVer format
            if not re.match(SEMVER_REGEX, tag):
                # If tag doesn't match SemVer format but has digits, try to convert it
                digits_match = re.search(r'(\d+)\.(\d+)\.(\d+)', tag)
                if digits_match:
                    tag = f"v{digits_match.group(0)}"
                else:
                    # Use default version if tag doesn't match SemVer and can't be converted
                    tag = DEFAULT_VERSION
            
            # Add 'v' prefix if missing
            if not tag.startswith('v'):
                tag = f"v{tag}"
            
            # Check if we're exactly on a tag
            commit_result = subprocess.run(
                ["git", "rev-parse", "HEAD"],
                stdout=subprocess.PIPE, 
                stderr=subprocess.PIPE,
                text=True,
                check=False
            )
            
            tag_commit_result = subprocess.run(
                ["git", "rev-list", "-n", "1", tag],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False
            )
            
            if (commit_result.returncode == 0 and 
                tag_commit_result.returncode == 0 and
                commit_result.stdout.strip() == tag_commit_result.stdout.strip()):
                # We're exactly on a tagged commit
                return tag
                
            # We're not on a tag, so add commit info
            commit_info = subprocess.run(
                ["git", "rev-parse", "--short", "HEAD"],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False
            )
            
            if commit_info.returncode == 0:
                return f"{tag}-{commit_info.stdout.strip()}"
            
            return tag
            
        # No tag found, use commit hash
        commit_result = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False
        )
        
        if commit_result.returncode == 0:
            return f"{DEFAULT_VERSION}-{commit_result.stdout.strip()}"
            
        return None
        
    except (FileNotFoundError, subprocess.SubprocessError):
        # Git not available or error running git commands
        return None

def _get_date_version() -> str:
    """
    Generate a version string based on the current date.
    
    Returns:
        str: Version string in format "vX.Y.Z-YYYYMMDD"
    """
    date_str = datetime.datetime.now().strftime("%Y%m%d")
    return f"{DEFAULT_VERSION}-{date_str}"

def is_prerelease(version_str: str) -> bool:
    """
    Check if a version is a pre-release version.
    
    Args:
        version_str (str): Version string to check
    
    Returns:
        bool: True if the version is a pre-release, False otherwise
    """
    try:
        _, _, _, prerelease, _ = parse_version(version_str)
        return prerelease is not None
    except ValueError:
        return False

def get_app_info() -> dict:
    """
    Get comprehensive application information.
    
    Returns:
        dict: Dictionary containing version and other app info
    """
    version = get_version()
    
    # Create a clean display version (without build metadata)
    display_version = version
    try:
        major, minor, patch, prerelease, _ = parse_version(version)
        if prerelease:
            display_version = f"v{major}.{minor}.{patch}-{prerelease}"
        else:
            display_version = f"v{major}.{minor}.{patch}"
    except ValueError:
        pass
    
    return {
        "version": display_version,
        "version_full": version,
        "is_prerelease": is_prerelease(version),
        "name": "Голосовой Ввод",
        "copyright": f"© {datetime.datetime.now().year}",
        "website": "https://github.com/gqwg2003/voice-multilingual"
    }

# Module-level version variable
__version__ = get_version() 