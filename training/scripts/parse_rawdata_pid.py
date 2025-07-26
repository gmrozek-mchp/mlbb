#!/usr/bin/env python3
"""
Script to parse raw data CSV files and generate new files with PID-related calculations.
error_x = target_x - ball_x
error_y = target_y - ball_y
integral_x = running sum of error_x
integral_y = running sum of error_y
derivative_x = average of two consecutive differences in error_x
derivative_y = average of two consecutive differences in error_y
"""

import csv
import os
import sys
import statistics

def parse_human01_csv(input_file, output_file):
    """
    Parse CSV files and generate new files with PID-related calculations.
    Filters based on filename: mode 72 for files containing "human", mode 80 for files containing "pid".
    
    Args:
        input_file (str): Path to the input CSV file
        output_file (str): Path to the output CSV file
    """
    try:
        # Read the CSV file
        print(f"Reading {input_file}...")
        
        with open(input_file, 'r', newline='') as infile:
            reader = csv.DictReader(infile)
            
            # Check if required columns exist
            required_columns = ['ball_x', 'target_x', 'ball_y', 'target_y']
            missing_columns = [col for col in required_columns if col not in reader.fieldnames]
            
            if missing_columns:
                print(f"Error: Missing required columns: {missing_columns}")
                print(f"Available columns: {reader.fieldnames}")
                return False
            
            # Determine filter mode based on filename
            filename = os.path.basename(input_file).lower()
            if 'human' in filename:
                target_mode = 72
                print(f"Filtering for mode {target_mode} (human data)")
            elif 'pid' in filename:
                target_mode = 80
                print(f"Filtering for mode {target_mode} (pid data)")
            else:
                # Default to mode 72 if neither keyword is found
                target_mode = 72
                print(f"No specific filter found in filename, defaulting to mode {target_mode}")
            
            # Read all rows and calculate error_x, error_y, integrals, and derivatives
            rows = []
            error_x_values = []
            error_y_values = []
            integral_x = 0.0
            integral_y = 0.0
            
            # Keep track of previous error values for derivative calculation
            prev_error_x = []
            prev_error_y = []
            
            for row in reader:
                # Filter based on determined target mode
                if int(row['mode']) != target_mode:
                    continue
                
                # Calculate error_x = target_x - ball_x
                ball_x = float(row['ball_x'])
                target_x = float(row['target_x'])
                error_x = target_x - ball_x
                
                # Calculate error_y = target_y - ball_y
                ball_y = float(row['ball_y'])
                target_y = float(row['target_y'])
                error_y = target_y - ball_y
                
                # Calculate running integrals
                integral_x += error_x
                integral_y += error_y
                
                # Calculate derivatives (average of two consecutive differences)
                derivative_x = 0.0
                derivative_y = 0.0
                
                if len(prev_error_x) >= 2:
                    # Calculate average of two consecutive differences:
                    # 1. Current - Previous
                    # 2. Previous - Before that
                    diff1_x = error_x - prev_error_x[-1]
                    diff2_x = prev_error_x[-1] - prev_error_x[-2]
                    derivative_x = (diff1_x + diff2_x) / 2.0
                    
                    diff1_y = error_y - prev_error_y[-1]
                    diff2_y = prev_error_y[-1] - prev_error_y[-2]
                    derivative_y = (diff1_y + diff2_y) / 2.0
                elif len(prev_error_x) == 1:
                    # Only one previous row available
                    derivative_x = error_x - prev_error_x[-1]
                    derivative_y = error_y - prev_error_y[-1]
                else:
                    # No previous rows available, derivative is 0
                    derivative_x = 0.0
                    derivative_y = 0.0
                
                # Update previous error lists (keep only last 2)
                prev_error_x.append(error_x)
                prev_error_y.append(error_y)
                if len(prev_error_x) > 2:
                    prev_error_x.pop(0)
                    prev_error_y.pop(0)
                
                # Add error_x, error_y, integrals, and derivatives to the row
                row['error_x'] = error_x
                row['error_y'] = error_y
                row['integral_x'] = integral_x
                row['integral_y'] = integral_y
                row['derivative_x'] = derivative_x
                row['derivative_y'] = derivative_y
                rows.append(row)
                error_x_values.append(error_x)
                error_y_values.append(error_y)
            
            # Write the new file
            print(f"Saving to {output_file}...")
            with open(output_file, 'w', newline='') as outfile:
                fieldnames = reader.fieldnames + ['error_x', 'error_y', 'integral_x', 'integral_y', 'derivative_x', 'derivative_y']
                writer = csv.DictWriter(outfile, fieldnames=fieldnames)
                writer.writeheader()
                writer.writerows(rows)
            
            print(f"Successfully processed {len(rows)} rows")
            print(f"Output saved to: {output_file}")
            
            # Display some statistics
            print("\nError statistics:")
            print("Error X:")
            print(f"  Mean error_x: {statistics.mean(error_x_values):.2f}")
            print(f"  Std error_x: {statistics.stdev(error_x_values):.2f}")
            print(f"  Min error_x: {min(error_x_values):.2f}")
            print(f"  Max error_x: {max(error_x_values):.2f}")
            print("Error Y:")
            print(f"  Mean error_y: {statistics.mean(error_y_values):.2f}")
            print(f"  Std error_y: {statistics.stdev(error_y_values):.2f}")
            print(f"  Min error_y: {min(error_y_values):.2f}")
            print(f"  Max error_y: {max(error_y_values):.2f}")
            print("Integral statistics:")
            print(f"  Final integral_x: {integral_x:.2f}")
            print(f"  Final integral_y: {integral_y:.2f}")
            
            # Calculate derivative statistics
            derivative_x_values = [row['derivative_x'] for row in rows]
            derivative_y_values = [row['derivative_y'] for row in rows]
            print("Derivative statistics:")
            print("Derivative X:")
            print(f"  Mean derivative_x: {statistics.mean(derivative_x_values):.2f}")
            print(f"  Std derivative_x: {statistics.stdev(derivative_x_values):.2f}")
            print(f"  Min derivative_x: {min(derivative_x_values):.2f}")
            print(f"  Max derivative_x: {max(derivative_x_values):.2f}")
            print("Derivative Y:")
            print(f"  Mean derivative_y: {statistics.mean(derivative_y_values):.2f}")
            print(f"  Std derivative_y: {statistics.stdev(derivative_y_values):.2f}")
            print(f"  Min derivative_y: {min(derivative_y_values):.2f}")
            print(f"  Max derivative_y: {max(derivative_y_values):.2f}")
            
            return True
        
    except FileNotFoundError:
        print(f"Error: File {input_file} not found")
        return False
    except Exception as e:
        print(f"Error processing file: {e}")
        return False

def main():
    """Main function to run the script."""
    # Define file paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    rawdata_dir = os.path.join(project_root, "rawdata")
    
    # Create processed directory if it doesn't exist
    processed_dir = os.path.join(project_root, "processed")
    if not os.path.exists(processed_dir):
        os.makedirs(processed_dir)
        print(f"Created processed directory: {processed_dir}")
    
    # Check if rawdata directory exists
    if not os.path.exists(rawdata_dir):
        print(f"Error: Raw data directory not found at {rawdata_dir}")
        sys.exit(1)
    
    # Find all CSV files in rawdata directory
    csv_files = [f for f in os.listdir(rawdata_dir) if f.endswith('.csv')]
    
    if not csv_files:
        print(f"No CSV files found in {rawdata_dir}")
        sys.exit(1)
    
    print(f"Found {len(csv_files)} CSV file(s) to process:")
    for csv_file in csv_files:
        print(f"  - {csv_file}")
    print()
    
    # Process each CSV file
    successful_files = 0
    failed_files = 0
    
    for csv_file in csv_files:
        input_file = os.path.join(rawdata_dir, csv_file)
        output_filename = csv_file.replace('.csv', '_with_error.csv')
        output_file = os.path.join(processed_dir, output_filename)
        
        print(f"Processing: {csv_file}")
        success = parse_human01_csv(input_file, output_file)
        
        if success:
            successful_files += 1
            print(f"✓ Successfully processed {csv_file}\n")
        else:
            failed_files += 1
            print(f"✗ Failed to process {csv_file}\n")
    
    # Summary
    print("=" * 50)
    print("PROCESSING SUMMARY")
    print("=" * 50)
    print(f"Total files found: {len(csv_files)}")
    print(f"Successfully processed: {successful_files}")
    print(f"Failed to process: {failed_files}")
    
    if failed_files > 0:
        print("\nSome files failed to process. Check the error messages above.")
        sys.exit(1)
    else:
        print("\nAll files processed successfully!")

if __name__ == "__main__":
    main() 