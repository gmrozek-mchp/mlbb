#!/usr/bin/env python3
"""
Example usage of the ball trajectory plotter.

This script demonstrates how to use the plot_ball_trajectory.py script
with different options and configurations.
"""

import subprocess
import sys
import os
from pathlib import Path

def run_plot_script(args):
    """Run the plot script with given arguments."""
    script_path = Path(__file__).parent / "plot_ball_trajectory.py"
    cmd = [sys.executable, str(script_path)] + args
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout)
    if result.stderr:
        print("STDERR:", result.stderr)
    return result.returncode == 0

def main():
    print("Ball Trajectory Plotter - Example Usage")
    print("=" * 50)
    
    # Example 1: Plot a single file
    print("\n1. Plotting single file (human01_with_error.csv)...")
    success = run_plot_script([
        "../processed/human01_with_error.csv",
        "--save",
        "--no-show"
    ])
    
    if success:
        print("✓ Single file plot completed successfully!")
    else:
        print("✗ Single file plot failed!")
    
    # Example 2: Plot with time information
    print("\n2. Plotting with time information...")
    success = run_plot_script([
        "../processed/human01_with_error.csv",
        "--save",
        "--no-show",
        "--with-time"
    ])
    
    if success:
        print("✓ Time-based plot completed successfully!")
    else:
        print("✗ Time-based plot failed!")
    
    # Example 3: Plot all files
    print("\n3. Plotting all CSV files...")
    success = run_plot_script([
        "--all",
        "--save",
        "--no-show"
    ])
    
    if success:
        print("✓ All files plot completed successfully!")
    else:
        print("✗ All files plot failed!")
    
    print("\n" + "=" * 50)
    print("Example usage completed!")
    print("Check the '../plots' directory for generated images.")

if __name__ == "__main__":
    main() 