# Ball Trajectory Plotter

This directory contains Python scripts for visualizing ball trajectory data from CSV files.

## Files

- `plot_ball_trajectory.py` - Main plotting script
- `example_usage.py` - Example usage demonstrations
- `requirements.txt` - Python dependencies
- `README.md` - This file

## Setup

1. Create a virtual environment:
   ```bash
   python3 -m venv venv
   source venv/bin/activate
   ```

2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

## Usage

### Basic Usage

Plot a single CSV file:
```bash
python plot_ball_trajectory.py ../processed/human01_with_error.csv
```

### Command Line Options

- `--all` - Plot all CSV files in the processed directory
- `--save` - Save plots to files (default: `../plots` directory)
- `--no-show` - Don't display plots (only save)
- `--output-dir DIR` - Specify output directory for saved plots (default: `../plots`)
- `--with-time` - Include time-based plots

### Examples

1. **Plot single file with save:**
   ```bash
   python plot_ball_trajectory.py ../processed/human01_with_error.csv --save --no-show
   ```

2. **Plot with time information:**
   ```bash
   python plot_ball_trajectory.py ../processed/human01_with_error.csv --with-time --save
   ```

3. **Plot all files:**
   ```bash
   python plot_ball_trajectory.py --all --save --no-show
   ```

4. **Run all examples:**
   ```bash
   python example_usage.py
   ```

## Output

The script generates several types of plots:

1. **Basic Trajectory Plot** - XY scatter plot showing ball position over time
   - Green dot: Start position
   - Red X: End position
   - Color gradient: Time progression
   - Blue line: Trajectory path

2. **Time-based Plot** (with `--with-time`)
   - Top: XY trajectory with timestamp colorbar (ball movement only)
   - Bottom: X and Y position vs time with target position over time
   - Dashed lines: Target X and Y positions over time (if target_x/target_y available)



## CSV Format

The script expects CSV files with the following columns:
- `ball_x` - Ball X position (required)
- `ball_y` - Ball Y position (required)
- `target_x` - Target X position (optional, shown in time-based plots)
- `target_y` - Target Y position (optional, shown in time-based plots)
- `timestamp` - Time information (optional, used for time-based plots)

## Generated Files

When using `--save`, plots are saved to the output directory (default: `../plots`):
- `{filename}_trajectory.png` - Basic trajectory plot
- `{filename}_with_time.png` - Time-based plot (if `--with-time`) 