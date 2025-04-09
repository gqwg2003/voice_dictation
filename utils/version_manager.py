#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Version management module for automatic semantic versioning.
This module provides functionality to bump version numbers according to SemVer guidelines.
"""

import os
import re
import subprocess
from typing import Tuple, Optional, Literal
from . import version

VersionType = Literal["major", "minor", "patch", "prerelease"]

def bump_version(bump_type: VersionType, prerelease_id: Optional[str] = None) -> str:
    """
    Bump the version according to semantic versioning rules.
    
    Args:
        bump_type: Type of version bump (major, minor, patch, prerelease)
        prerelease_id: Identifier for prerelease (e.g., 'alpha', 'beta', 'rc')
    
    Returns:
        str: New version string
    
    Raises:
        ValueError: If bump_type is invalid or git operations fail
    """
    # Get current version
    current_version = version.get_version()
    
    try:
        # Parse current version
        major, minor, patch, current_prerelease, build = version.parse_version(current_version)
        
        # Determine new version parts based on bump type
        if bump_type == "major":
            new_version = f"v{major + 1}.0.0"
        elif bump_type == "minor":
            new_version = f"v{major}.{minor + 1}.0"
        elif bump_type == "patch":
            new_version = f"v{major}.{minor}.{patch + 1}"
        elif bump_type == "prerelease":
            if not prerelease_id:
                raise ValueError("Prerelease identifier must be provided for prerelease bump")
            
            # If current version is not already a prerelease, increment patch version
            if not current_prerelease:
                new_version = f"v{major}.{minor}.{patch + 1}-{prerelease_id}.1"
            else:
                # Extract prerelease identifier and version
                prerelease_parts = current_prerelease.split('.')
                
                if len(prerelease_parts) >= 2 and prerelease_parts[0] == prerelease_id:
                    # Increment prerelease version
                    try:
                        pre_version = int(prerelease_parts[1])
                        new_version = f"v{major}.{minor}.{patch}-{prerelease_id}.{pre_version + 1}"
                    except (IndexError, ValueError):
                        new_version = f"v{major}.{minor}.{patch}-{prerelease_id}.1"
                else:
                    # New prerelease identifier
                    new_version = f"v{major}.{minor}.{patch}-{prerelease_id}.1"
        else:
            raise ValueError(f"Invalid bump type: {bump_type}")
        
        # Preserve build metadata if it exists
        if build:
            new_version = f"{new_version}+{build}"
            
        return new_version
        
    except ValueError as e:
        # If parsing fails, create a new version based on default
        if bump_type == "major":
            return "v1.0.0"
        elif bump_type == "minor":
            return "v0.1.0"
        elif bump_type == "patch":
            return "v0.0.1"
        elif bump_type == "prerelease":
            if not prerelease_id:
                raise ValueError("Prerelease identifier must be provided for prerelease bump")
            return f"v0.0.1-{prerelease_id}.1"
        else:
            raise ValueError(f"Invalid bump type: {bump_type}")

def update_version_in_git(new_version: str, message: Optional[str] = None) -> bool:
    """
    Update version in git by creating a new tag.
    
    Args:
        new_version: New version string to tag
        message: Optional commit message
    
    Returns:
        bool: True if successful, False otherwise
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
            print("Not in a git repository.")
            return False
        
        # Ensure the new version has a 'v' prefix
        if not new_version.startswith('v'):
            new_version = f"v{new_version}"
        
        # Create tag message
        tag_message = message or f"Version {new_version}"
        
        # Create an annotated tag
        subprocess.run(
            ["git", "tag", "-a", new_version, "-m", tag_message],
            check=True
        )
        
        print(f"Successfully created tag: {new_version}")
        return True
        
    except subprocess.SubprocessError as e:
        print(f"Failed to create tag: {e}")
        return False
    except Exception as e:
        print(f"Error updating version in git: {e}")
        return False

def get_commit_type_from_message(commit_msg: str) -> Optional[VersionType]:
    """
    Determine the type of version bump from a commit message based on conventional commits.
    
    Args:
        commit_msg: Git commit message
    
    Returns:
        Optional[VersionType]: Type of version bump or None if undetermined
    """
    # Conventional commits regex: https://www.conventionalcommits.org/
    if re.search(r'(?i)^BREAKING CHANGE:|^[a-z]+(\([a-z]+\))?!:', commit_msg):
        return "major"
    elif re.search(r'(?i)^feat(\([a-z]+\))?:', commit_msg):
        return "minor"
    elif re.search(r'(?i)^fix(\([a-z]+\))?:|^docs(\([a-z]+\))?:|^style(\([a-z]+\))?:|^refactor(\([a-z]+\))?:|^perf(\([a-z]+\))?:|^test(\([a-z]+\))?:|^chore(\([a-z]+\))?:', commit_msg):
        return "patch"
    return None

def auto_bump_from_commits() -> Optional[str]:
    """
    Automatically determine the version bump type from git commits since the last tag.
    
    Returns:
        Optional[str]: New version string or None if no bump is needed
    """
    try:
        # Get the latest tag
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False
        )
        
        if result.returncode != 0:
            # No tags yet, start with initial version
            return bump_version("patch")
        
        latest_tag = result.stdout.strip()
        
        # Get commits since the last tag
        result = subprocess.run(
            ["git", "log", f"{latest_tag}..HEAD", "--pretty=format:%s"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False
        )
        
        if result.returncode != 0:
            print("Failed to retrieve commit history")
            return None
        
        commit_messages = result.stdout.strip().split('\n')
        if not commit_messages or (len(commit_messages) == 1 and not commit_messages[0]):
            print("No new commits since last tag")
            return None
        
        # Determine the highest bump type needed
        bump_type = None
        for msg in commit_messages:
            commit_bump_type = get_commit_type_from_message(msg)
            if commit_bump_type == "major":
                bump_type = "major"
                break
            elif commit_bump_type == "minor" and bump_type != "major":
                bump_type = "minor"
            elif commit_bump_type == "patch" and bump_type not in ("major", "minor"):
                bump_type = "patch"
        
        # Default to patch if no specific bump type was determined
        bump_type = bump_type or "patch"
        
        # Bump version according to determined type
        return bump_version(bump_type)
        
    except Exception as e:
        print(f"Error determining version from commits: {e}")
        return None

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Manage semantic versioning for the project")
    
    subparsers = parser.add_subparsers(dest="command", help="Commands")
    
    # Bump command
    bump_parser = subparsers.add_parser("bump", help="Bump version")
    bump_parser.add_argument("type", choices=["major", "minor", "patch", "prerelease"], 
                            help="Type of version bump")
    bump_parser.add_argument("--prerelease-id", "-p", help="Prerelease identifier (required for prerelease bump)")
    bump_parser.add_argument("--message", "-m", help="Tag message")
    
    # Auto command
    auto_parser = subparsers.add_parser("auto", help="Automatically bump version based on commits")
    auto_parser.add_argument("--message", "-m", help="Tag message")
    
    # Get current version command
    subparsers.add_parser("current", help="Get current version")
    
    args = parser.parse_args()
    
    if args.command == "bump":
        if args.type == "prerelease" and not args.prerelease_id:
            parser.error("--prerelease-id is required for prerelease bump")
        
        new_version = bump_version(args.type, args.prerelease_id)
        print(f"Bumped version to: {new_version}")
        
        update_version_in_git(new_version, args.message)
        
    elif args.command == "auto":
        new_version = auto_bump_from_commits()
        if new_version:
            print(f"Auto-bumped version to: {new_version}")
            update_version_in_git(new_version, args.message)
        else:
            print("No version bump needed")
            
    elif args.command == "current":
        print(f"Current version: {version.get_version()}")
        
    else:
        parser.print_help() 