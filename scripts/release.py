#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Release script for Voice Dictation application.
This script helps to create new releases following Semantic Versioning.
"""

import os
import re
import sys
import argparse
import subprocess
from typing import Tuple, Optional

# Import our version module
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from utils.version import parse_version, is_prerelease

VERSION_TYPES = ['major', 'minor', 'patch', 'alpha', 'beta', 'rc']

def get_current_version() -> str:
    """Get the current version from the latest git tag."""
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False
        )
        
        if result.returncode == 0:
            tag = result.stdout.strip()
            # Ensure the tag has 'v' prefix
            if not tag.startswith('v'):
                tag = f"v{tag}"
            return tag
        
        # If no tag found, return default
        return "v0.1.0"
        
    except (FileNotFoundError, subprocess.SubprocessError):
        # Git not available or error running git commands
        return "v0.1.0"

def bump_version(current_version: str, version_type: str) -> str:
    """
    Bump the version according to semantic versioning.
    
    Args:
        current_version (str): Current version string
        version_type (str): Type of version bump ('major', 'minor', 'patch', 'alpha', 'beta', 'rc')
    
    Returns:
        str: New version string
    """
    try:
        # Parse the current version
        major, minor, patch, prerelease, build = parse_version(current_version)
        
        # Handle version type
        if version_type == 'major':
            return f"v{major + 1}.0.0"
        elif version_type == 'minor':
            return f"v{major}.{minor + 1}.0"
        elif version_type == 'patch':
            return f"v{major}.{minor}.{patch + 1}"
        elif version_type in ['alpha', 'beta', 'rc']:
            # If already a prerelease of the same type, increment its number
            if prerelease and prerelease.startswith(version_type):
                match = re.search(r'(\d+)$', prerelease)
                if match:
                    num = int(match.group(1))
                    return f"v{major}.{minor}.{patch}-{version_type}.{num + 1}"
                else:
                    return f"v{major}.{minor}.{patch}-{version_type}.1"
            else:
                return f"v{major}.{minor}.{patch}-{version_type}.1"
        else:
            raise ValueError(f"Unknown version type: {version_type}")
            
    except ValueError as e:
        print(f"Error: {e}")
        return current_version

def create_git_tag(version: str, message: Optional[str] = None) -> bool:
    """
    Create a git tag for the release.
    
    Args:
        version (str): Version to tag
        message (Optional[str]): Tag message
    
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        cmd = ["git", "tag", version]
        if message:
            cmd = ["git", "tag", "-a", version, "-m", message]
            
        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False
        )
        
        return result.returncode == 0
        
    except (FileNotFoundError, subprocess.SubprocessError):
        return False

def push_git_tag(version: str) -> bool:
    """
    Push the git tag to remote.
    
    Args:
        version (str): Version tag to push
    
    Returns:
        bool: True if successful, False otherwise
    """
    try:
        result = subprocess.run(
            ["git", "push", "origin", version],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False
        )
        
        return result.returncode == 0
        
    except (FileNotFoundError, subprocess.SubprocessError):
        return False

def main():
    """Main function."""
    parser = argparse.ArgumentParser(description="Release script for Voice Dictation application")
    parser.add_argument('version_type', choices=VERSION_TYPES, help='Type of version to bump')
    parser.add_argument('--message', '-m', help='Tag message')
    parser.add_argument('--push', '-p', action='store_true', help='Push tag to remote')
    args = parser.parse_args()
    
    # Get the current version
    current_version = get_current_version()
    print(f"Current version: {current_version}")
    
    # Bump the version
    new_version = bump_version(current_version, args.version_type)
    print(f"New version: {new_version}")
    
    # Create a git tag
    message = args.message or f"Release {new_version}"
    print(f"Creating git tag {new_version}...")
    if create_git_tag(new_version, message):
        print(f"Successfully created tag {new_version}")
    else:
        print(f"Failed to create tag {new_version}")
        return 1
    
    # Push the tag if requested
    if args.push:
        print(f"Pushing tag {new_version} to remote...")
        if push_git_tag(new_version):
            print(f"Successfully pushed tag {new_version}")
        else:
            print(f"Failed to push tag {new_version}")
            return 1
    
    print(f"\nRelease {new_version} created successfully!")
    if is_prerelease(new_version):
        print("Note: This is a pre-release version.")
    
    return 0

if __name__ == "__main__":
    sys.exit(main()) 