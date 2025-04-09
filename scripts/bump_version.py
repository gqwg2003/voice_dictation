#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Utility script to bump the project version.
This script is a convenience wrapper around the version_manager module.
"""

import os
import sys
import argparse
import subprocess

# Add the parent directory to sys.path to allow importing from utils
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(script_dir)
sys.path.insert(0, project_root)

from utils import version_manager, version

def main():
    parser = argparse.ArgumentParser(
        description="Utility script to bump the project version",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Show current version
  python scripts/bump_version.py

  # Bump patch version (0.0.1 -> 0.0.2)
  python scripts/bump_version.py patch

  # Bump minor version (0.1.0 -> 0.2.0)
  python scripts/bump_version.py minor

  # Bump major version (1.0.0 -> 2.0.0)
  python scripts/bump_version.py major

  # Bump to a prerelease version
  python scripts/bump_version.py prerelease beta
  
  # Automatically determine version bump from commit messages
  python scripts/bump_version.py auto
  
  # Only display what the new version would be without creating a tag
  python scripts/bump_version.py patch --dry-run
  
  # Bump version with a custom tag message
  python scripts/bump_version.py minor --message "New feature release"
"""
    )
    
    parser.add_argument('bump_type', nargs='?', choices=['major', 'minor', 'patch', 'prerelease', 'auto', 'current'],
                       help='Type of version bump (default: show current version)', default='current')
    parser.add_argument('prerelease_id', nargs='?', 
                       help='Prerelease identifier (required for prerelease type)')
    parser.add_argument('--message', '-m', help='Git tag message')
    parser.add_argument('--dry-run', '-d', action='store_true', 
                      help='Show the new version without creating a tag')
    
    args = parser.parse_args()
    
    # Show current version
    current_version = version.get_version()
    print(f"Current version: {current_version}")
    
    # If only showing current version, exit
    if args.bump_type == 'current':
        return
    
    # Check if prerelease_id is provided for prerelease bump
    if args.bump_type == 'prerelease' and not args.prerelease_id:
        parser.error("--prerelease-id is required for prerelease bump")
    
    # Get new version
    try:
        if args.bump_type == 'auto':
            new_version = version_manager.auto_bump_from_commits()
            if not new_version:
                print("No version bump needed")
                return
        else:
            new_version = version_manager.bump_version(args.bump_type, args.prerelease_id)
        
        print(f"New version: {new_version}")
        
        # If dry run, exit
        if args.dry_run:
            return
            
        # Update version in git
        if version_manager.update_version_in_git(new_version, args.message):
            print(f"Successfully created git tag {new_version}")
            
            # Ask if user wants to push the tag
            push = input("Do you want to push the tag to the remote repository? [y/N] ").lower()
            if push == 'y' or push == 'yes':
                try:
                    subprocess.run(["git", "push", "origin", new_version], check=True)
                    print(f"Successfully pushed tag {new_version} to remote")
                except subprocess.SubprocessError as e:
                    print(f"Failed to push tag: {e}")
        
    except ValueError as e:
        parser.error(str(e))
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 