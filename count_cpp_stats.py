#!/usr/bin/env python3
"""
Count C/C++ files and lines in the project.
Works on both Linux and Windows.
"""

import os
import sys
from pathlib import Path

# C/C++ file extensions
CPP_EXTENSIONS = {'.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx'}


def is_cpp_file(filepath: Path) -> bool:
    """Check if file has a C/C++ extension."""
    return filepath.suffix.lower() in CPP_EXTENSIONS


def count_lines_in_file(filepath: Path) -> int:
    """Count lines in a single file."""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            return sum(1 for _ in f)
    except (IOError, OSError):
        return 0


def collect_cpp_files(root_dir: Path) -> list[Path]:
    """Collect all C/C++ files recursively from root directory."""
    cpp_files = []
    for path in root_dir.rglob('*'):
        if path.is_file() and is_cpp_file(path):
            cpp_files.append(path)
    return cpp_files


def get_directory_stats(root_dir: Path, cpp_files: list[Path]) -> dict[str, dict]:
    """Get statistics grouped by immediate subdirectories."""
    stats = {}
    
    # Initialize stats for immediate subdirectories
    for item in root_dir.iterdir():
        if item.is_dir():
            stats[item.name] = {'files': 0, 'lines': 0}
    
    # Also count files directly in root
    stats['.'] = {'files': 0, 'lines': 0}
    
    for filepath in cpp_files:
        try:
            # Get relative path from root
            rel_path = filepath.relative_to(root_dir)
            
            # Determine which immediate subdirectory this file belongs to
            if len(rel_path.parts) == 1:
                # File is directly in root
                dir_key = '.'
            else:
                # File is in a subdirectory
                dir_key = rel_path.parts[0]
            
            if dir_key in stats:
                line_count = count_lines_in_file(filepath)
                stats[dir_key]['files'] += 1
                stats[dir_key]['lines'] += line_count
        except (ValueError, IndexError):
            continue
    
    return stats


def main():
    # Use current directory as root, or accept a path argument
    if len(sys.argv) > 1:
        root_dir = Path(sys.argv[1]).resolve()
    else:
        root_dir = Path.cwd()
    
    if not root_dir.exists():
        print(f"Error: Directory '{root_dir}' does not exist.")
        sys.exit(1)
    
    print(f"Scanning C/C++ files in: {root_dir}")
    print("=" * 60)
    
    # Collect all C/C++ files
    cpp_files = collect_cpp_files(root_dir)
    
    if not cpp_files:
        print("No C/C++ files found.")
        sys.exit(0)
    
    # Get directory statistics
    dir_stats = get_directory_stats(root_dir, cpp_files)
    
    # Print per-directory statistics
    print("\nPer-directory C/C++ statistics:")
    print("-" * 60)
    print(f"{'Directory':<30} {'Files':>10} {'Lines':>15}")
    print("-" * 60)
    
    total_files = 0
    total_lines = 0
    
    # Sort directories: root first, then alphabetically
    sorted_dirs = sorted(dir_stats.keys(), key=lambda x: (x != '.', x.lower()))
    
    for dir_name in sorted_dirs:
        stat = dir_stats[dir_name]
        if stat['files'] > 0:
            display_name = '(root)' if dir_name == '.' else dir_name
            print(f"{display_name:<30} {stat['files']:>10} {stat['lines']:>15,}")
            total_files += stat['files']
            total_lines += stat['lines']
    
    print("-" * 60)
    print(f"{'TOTAL':<30} {total_files:>10} {total_lines:>15,}")
    print("=" * 60)


if __name__ == '__main__':
    main()
