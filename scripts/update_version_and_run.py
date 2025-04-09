#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Script to update version information and run the application.
This is useful for development and testing to ensure version is updated before each run.
"""

import os
import sys
import subprocess
import argparse

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
project_dir = os.path.dirname(script_dir)
sys.path.insert(0, project_dir)

from utils.version_updater import update_version

def main():
    """Update version and run application."""
    parser = argparse.ArgumentParser(description="Update version and run application")
    parser.add_argument("--no-run", action="store_true", help="Only update version, don't run app")
    parser.add_argument("--build-number", type=int, help="Set specific build number for testing")
    args = parser.parse_args()
    
    # Set build number in environment if provided
    if args.build_number is not None:
        os.environ["OVERRIDE_BUILD_NUMBER"] = str(args.build_number)
    
    # Update version
    print("Updating version information...")
    version_data = update_version()
    print(f"Version: {version_data['display_version']}")
    print(f"Build Number: {version_data['build_number']}")
    
    if args.no_run:
        print("Version updated. Application not started.")
        return
    
    # Run application
    print("Starting application...")
    app_path = os.path.join(project_dir, "app.py")
    
    # Use Python executable from current environment
    python_exe = sys.executable
    
    try:
        # Run application
        process = subprocess.Popen([python_exe, app_path])
        process.wait()
    except KeyboardInterrupt:
        print("Application stopped by user")
    except Exception as e:
        print(f"Error running application: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 