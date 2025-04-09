#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Script to install Git hooks for commit message validation and version management.
"""

import os
import sys
import shutil
import stat
import subprocess

# Add the parent directory to sys.path
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(script_dir)
sys.path.insert(0, project_root)

def is_git_repository():
    """Check if the current directory is a git repository."""
    try:
        subprocess.run(
            ["git", "rev-parse", "--is-inside-work-tree"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True
        )
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def get_git_hooks_dir():
    """Get the git hooks directory path."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--git-path", "hooks"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return os.path.join(project_root, ".git", "hooks")

def create_commit_msg_hook():
    """Create the commit-msg hook for validating commit messages."""
    hook_content = """#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import sys
import os
import subprocess

# Add project root to sys.path
script_dir = os.path.dirname(os.path.abspath(__file__))
git_dir = os.path.dirname(script_dir)
project_root = os.path.dirname(git_dir)
sys.path.insert(0, project_root)

# Conventional Commits regex pattern
CONVENTIONAL_COMMIT_REGEX = r'^(build|chore|ci|docs|feat|fix|perf|refactor|revert|style|test)(\([a-z-]+\))?!?: .+'
BREAKING_CHANGE_REGEX = r'^BREAKING CHANGE:'
FOOTER_REGEX = r'^[a-zA-Z-]+(\([a-zA-Z-]+\))?!?: '

def validate_commit_message(file_path):
    # Read the commit message
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()
    
    # Remove comment lines
    message_lines = [line for line in lines if not line.startswith('#')]
    if not message_lines:
        print('Error: Empty commit message')
        return False
    
    # Check subject line (first line)
    subject = message_lines[0].strip()
    if not re.match(CONVENTIONAL_COMMIT_REGEX, subject) and not re.match(BREAKING_CHANGE_REGEX, subject):
        print('Error: Commit message subject does not follow Conventional Commits format.')
        print('Format should be: type[(scope)]: description')
        print('Types: build, chore, ci, docs, feat, fix, perf, refactor, revert, style, test')
        print('Example: feat(ui): add login button')
        return False
    
    # Check for max line length
    for i, line in enumerate(message_lines):
        if len(line) > 100:  # Max 100 chars per line
            print(f'Error: Line {i+1} exceeds 100 characters')
            return False
    
    # Check if there's a blank line between subject and body (if body exists)
    if len(message_lines) > 1 and message_lines[1].strip():
        print('Error: Missing blank line between subject and body')
        return False
    
    # If commit has a body, check that footer has proper format
    if len(message_lines) > 2:
        for i, line in enumerate(message_lines[2:], 3):
            # Check for footer format
            if re.match(BREAKING_CHANGE_REGEX, line.strip()) or re.match(FOOTER_REGEX, line.strip()):
                # Found a footer, make sure there's a blank line before it
                if i > 3 and message_lines[i-2].strip():
                    print('Error: Missing blank line before footer')
                    return False
    
    return True

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Error: Commit message file path is required')
        sys.exit(1)
    
    commit_msg_file = sys.argv[1]
    if not validate_commit_message(commit_msg_file):
        print('\nSee https://www.conventionalcommits.org/ for more details')
        result = input('Do you want to continue with this commit message anyway? [y/N] ').lower()
        if result != 'y' and result != 'yes':
            sys.exit(1)
    
    # Analyze version bump type based on the commit message
    try:
        from utils import version_manager
        
        with open(commit_msg_file, 'r', encoding='utf-8') as file:
            commit_msg = file.read()
        
        bump_type = version_manager.get_commit_type_from_message(commit_msg)
        if bump_type:
            print(f'\\nThis commit will trigger a {bump_type} version bump')
    except ImportError:
        # Module not available, skip version bump analysis
        pass
"""
    
    hooks_dir = get_git_hooks_dir()
    os.makedirs(hooks_dir, exist_ok=True)
    
    hook_path = os.path.join(hooks_dir, "commit-msg")
    with open(hook_path, 'w', encoding='utf-8') as f:
        f.write(hook_content)
    
    # Make the hook executable
    os.chmod(hook_path, os.stat(hook_path).st_mode | stat.S_IEXEC)
    
    return hook_path

def create_post_commit_hook():
    """Create the post-commit hook for suggesting version bumps."""
    hook_content = """#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import subprocess

# Add project root to sys.path
script_dir = os.path.dirname(os.path.abspath(__file__))
git_dir = os.path.dirname(script_dir)
project_root = os.path.dirname(git_dir)
sys.path.insert(0, project_root)

try:
    from utils import version, version_manager
    
    # Get the last commit message
    result = subprocess.run(
        ["git", "log", "-1", "--pretty=%B"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=True
    )
    
    commit_msg = result.stdout.strip()
    bump_type = version_manager.get_commit_type_from_message(commit_msg)
    
    if bump_type:
        current_version = version.get_version()
        new_version = version_manager.bump_version(bump_type)
        
        print(f"\\n===== Version Bump Suggestion =====")
        print(f"Commit type detected: {bump_type}")
        print(f"Current version: {current_version}")
        print(f"Suggested new version: {new_version}")
        print(f"\\nTo apply this version bump, run:")
        print(f"python scripts/bump_version.py {bump_type}")
        print(f"=====================================")
except (ImportError, subprocess.SubprocessError):
    # Module not available or git command failed, skip version bump suggestion
    pass
"""
    
    hooks_dir = get_git_hooks_dir()
    os.makedirs(hooks_dir, exist_ok=True)
    
    hook_path = os.path.join(hooks_dir, "post-commit")
    with open(hook_path, 'w', encoding='utf-8') as f:
        f.write(hook_content)
    
    # Make the hook executable
    os.chmod(hook_path, os.stat(hook_path).st_mode | stat.S_IEXEC)
    
    return hook_path

def main():
    if not is_git_repository():
        print("Error: Not a git repository")
        sys.exit(1)
    
    # Create commit-msg hook
    commit_msg_hook_path = create_commit_msg_hook()
    print(f"Installed commit-msg hook: {commit_msg_hook_path}")
    
    # Create post-commit hook
    post_commit_hook_path = create_post_commit_hook()
    print(f"Installed post-commit hook: {post_commit_hook_path}")
    
    print("\nGit hooks installed successfully!")
    print("These hooks will:")
    print("1. Validate commit messages to follow Conventional Commits format")
    print("2. Suggest version bumps after each commit")

if __name__ == "__main__":
    main() 