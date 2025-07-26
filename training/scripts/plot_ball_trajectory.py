#!/usr/bin/env python3
"""
Ball Trajectory Plotter

This script reads CSV files containing ball position data and creates XY plots
showing the ball trajectory over time. It can handle multiple files and provides
various visualization options.

Usage:
    python plot_ball_trajectory.py [csv_file] [options]
    
Examples:
    python plot_ball_trajectory.py human01_with_error.csv
    python plot_ball_trajectory.py --all --save
    python plot_ball_trajectory.py --compare human01_with_error.csv human02_with_error.csv
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import os
import glob
from pathlib import Path

def load_csv_data(file_path):
    """
    Load CSV data and extract ball position and target information.
    
    Args:
        file_path (str): Path to the CSV file
        
    Returns:
        pandas.DataFrame: DataFrame with ball_x, ball_y, target_x, target_y, and timestamp columns
    """
    try:
        df = pd.read_csv(file_path)
        
        # Check if required columns exist
        required_cols = ['ball_x', 'ball_y']
        missing_cols = [col for col in required_cols if col not in df.columns]
        
        if missing_cols:
            raise ValueError(f"Missing required columns: {missing_cols}")
        
        # Select the columns we need
        result_df = df[['ball_x', 'ball_y']].copy()
        
        # Add target information if available
        if 'target_x' in df.columns:
            result_df['target_x'] = df['target_x']
        if 'target_y' in df.columns:
            result_df['target_y'] = df['target_y']
        
        # Add timestamp if available
        if 'timestamp' in df.columns:
            result_df['timestamp'] = df['timestamp']
        
        return result_df
        
    except Exception as e:
        print(f"Error loading {file_path}: {e}")
        return None

def plot_single_trajectory(df, title="Ball Trajectory", save_path=None, show_plot=True):
    """
    Plot a single ball trajectory.
    
    Args:
        df (pandas.DataFrame): DataFrame with ball_x and ball_y columns
        title (str): Plot title
        save_path (str): Path to save the plot (optional)
        show_plot (bool): Whether to display the plot
    """
    plt.figure(figsize=(10, 8))
    
    # Create the scatter plot
    plt.scatter(df['ball_x'], df['ball_y'], c=range(len(df)), 
                cmap='viridis', alpha=0.6, s=20)
    
    # Add colorbar to show time progression
    cbar = plt.colorbar()
    cbar.set_label('Time Step')
    
    # Add start and end markers
    plt.scatter(df['ball_x'].iloc[0], df['ball_y'].iloc[0], 
                color='green', s=100, marker='o', label='Start', zorder=5)
    plt.scatter(df['ball_x'].iloc[-1], df['ball_y'].iloc[-1], 
                color='red', s=100, marker='x', label='End', zorder=5)
    
    # Add trajectory line
    plt.plot(df['ball_x'], df['ball_y'], 'b-', alpha=0.3, linewidth=1)
    
    plt.xlabel('Ball X Position')
    plt.ylabel('Ball Y Position')
    plt.title(title)
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.axis('equal')  # Equal aspect ratio
    
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Plot saved to: {save_path}")
    
    if show_plot:
        plt.show()
    else:
        plt.close()



def plot_trajectory_with_time(df, title="Ball Trajectory Over Time", save_path=None, show_plot=True):
    """
    Plot ball trajectory with time information and target positions in time plot only.
    
    Args:
        df (pandas.DataFrame): DataFrame with ball_x, ball_y, target_x, target_y, and timestamp columns
        title (str): Plot title
        save_path (str): Path to save the plot (optional)
        show_plot (bool): Whether to display the plot
    """
    if 'timestamp' not in df.columns:
        print("Warning: No timestamp column found. Using index as time.")
        df = df.copy()
        df['timestamp'] = range(len(df))
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))
    
    # Top plot: XY trajectory
    scatter = ax1.scatter(df['ball_x'], df['ball_y'], 
                         c=df['timestamp'], cmap='viridis', alpha=0.6, s=20)
    ax1.scatter(df['ball_x'].iloc[0], df['ball_y'].iloc[0], 
               color='green', s=100, marker='o', label='Start', zorder=5)
    ax1.scatter(df['ball_x'].iloc[-1], df['ball_y'].iloc[-1], 
               color='red', s=100, marker='x', label='End', zorder=5)
    ax1.plot(df['ball_x'], df['ball_y'], 'b-', alpha=0.3, linewidth=1)
    

    
    ax1.set_xlabel('Ball X Position')
    ax1.set_ylabel('Ball Y Position')
    ax1.set_title(f'{title} - XY Trajectory')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax1.axis('equal')
    
    # Add colorbar
    cbar = plt.colorbar(scatter, ax=ax1)
    cbar.set_label('Timestamp')
    
    # Bottom plot: Position vs Time
    ax2.plot(df['timestamp'], df['ball_x'], 'b-', label='Ball X Position', alpha=0.7, linewidth=2)
    ax2.plot(df['timestamp'], df['ball_y'], 'r-', label='Ball Y Position', alpha=0.7, linewidth=2)
    
    # Add target lines if available
    if 'target_x' in df.columns and 'target_y' in df.columns:
        # Plot target X and Y over time
        ax2.plot(df['timestamp'], df['target_x'], 'b--', label='Target X Position', alpha=0.8, linewidth=2)
        ax2.plot(df['timestamp'], df['target_y'], 'r--', label='Target Y Position', alpha=0.8, linewidth=2)
    
    ax2.set_xlabel('Time')
    ax2.set_ylabel('Position')
    ax2.set_title(f'{title} - Position vs Time')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Plot saved to: {save_path}")
    
    if show_plot:
        plt.show()
    else:
        plt.close()

def main():
    parser = argparse.ArgumentParser(description='Plot ball trajectories from CSV data')
    parser.add_argument('csv_files', nargs='*', help='CSV files to plot')
    parser.add_argument('--all', action='store_true', 
                       help='Plot all CSV files in the processed directory')

    parser.add_argument('--save', action='store_true',
                       help='Save plots to files')
    parser.add_argument('--no-show', action='store_true',
                       help='Don\'t display plots (only save)')
    parser.add_argument('--output-dir', default='../plots',
                       help='Output directory for saved plots')
    parser.add_argument('--with-time', action='store_true',
                       help='Include time-based plots')
    
    args = parser.parse_args()
    
    # Create output directory if saving
    if args.save:
        os.makedirs(args.output_dir, exist_ok=True)
    
    # Get list of CSV files
    csv_files = []
    if args.all:
        # Get all CSV files from processed directory
        processed_dir = Path(__file__).parent.parent / 'processed'
        csv_files = list(processed_dir.glob('*.csv'))
    elif args.csv_files:
        csv_files = args.csv_files
    else:
        print("No CSV files specified. Use --all or provide file names.")
        return
    
    # Load data
    data_dict = {}
    for csv_file in csv_files:
        df = load_csv_data(csv_file)
        if df is not None:
            filename = Path(csv_file).stem
            data_dict[filename] = df
            print(f"Loaded {filename}: {len(df)} data points")
    
    if not data_dict:
        print("No valid CSV files found.")
        return
    
    # Generate plots
    for filename, df in data_dict.items():
        print(f"Plotting {filename}...")
        
        # Basic trajectory plot
        plot_single_trajectory(
            df,
            title=f"Ball Trajectory - {filename}",
            save_path=f"{args.output_dir}/{filename}_trajectory.png" if args.save else None,
            show_plot=not args.no_show
        )
        
        # Time-based plot if requested
        if args.with_time:
            plot_trajectory_with_time(
                df,
                title=f"Ball Trajectory - {filename}",
                save_path=f"{args.output_dir}/{filename}_with_time.png" if args.save else None,
                show_plot=not args.no_show
            )

if __name__ == "__main__":
    main() 