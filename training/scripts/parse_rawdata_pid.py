#!/usr/bin/env python3
"""
Script to parse raw data CSV files and generate new files with PID-related calculations.
error_x = target_x - ball_x
error_y = target_y - ball_y
integral_x = running sum of error_x
integral_y = running sum of error_y
error_x_prev1 = error_x from previous row
error_x_prev2 = error_x from two rows back
error_y_prev1 = error_y from previous row
error_y_prev2 = error_y from two rows back
derivative_x_n = sum of n consecutive differences in error_x (n=1 to 10)
derivative_y_n = sum of n consecutive differences in error_y (n=1 to 10)
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
            integral_x = 0
            integral_y = 0
            
            # Keep track of previous error values for derivative calculation and previous error columns
            prev_error_x = []
            prev_error_y = []
            
            for row in reader:
                # Filter based on determined target mode and active flag
                if int(row['mode']) != target_mode or int(row['active']) != 1:
                    continue
                
                # Calculate error_x = target_x - ball_x
                ball_x = int(row['ball_x'])
                target_x = int(row['target_x'])
                error_x = target_x - ball_x
                
                # Calculate error_y = target_y - ball_y
                ball_y = int(row['ball_y'])
                target_y = int(row['target_y'])
                error_y = target_y - ball_y
                
                # Calculate running integrals
                integral_x += error_x
                integral_y += error_y
                
                # Calculate multiple derivative terms as sum of n consecutive differences (n=1 to 10)
                derivative_terms_x = []
                derivative_terms_y = []
                
                for n in range(1, 11):  # n ranges from 1 to 10
                    if len(prev_error_x) >= n:
                        derivative_terms_x.append(error_x - prev_error_x[-n])
                        derivative_terms_y.append(error_y - prev_error_y[-n])
                    elif len(prev_error_x) >= 1:
                        derivative_terms_x.append(error_x - prev_error_x[-len(prev_error_x)])
                        derivative_terms_y.append(error_y - prev_error_y[-len(prev_error_y)])
                    else:
                        derivative_terms_x.append(0)
                        derivative_terms_y.append(0)
                
                # Add error_x, error_y, integrals, and derivatives to the row
                row['error_x'] = error_x
                row['error_y'] = error_y

                # Add previous error columns (check length before adding current error)
                for n in range(1, 11):
                    if len(prev_error_x) >= n:
                        row[f'error_x_prev{n}'] = prev_error_x[-n]
                        row[f'error_y_prev{n}'] = prev_error_y[-n]
                    else:
                        row[f'error_x_prev{n}'] = 0
                        row[f'error_y_prev{n}'] = 0
                                
                row['integral_x'] = integral_x
                row['integral_y'] = integral_y

                # Add multiple derivative terms (n=1 to 10)
                for n in range(1, 11):
                    row[f'derivative_x_{n}'] = derivative_terms_x[n-1]
                    row[f'derivative_y_{n}'] = derivative_terms_y[n-1]

                rows.append(row)
            
                # Update previous error lists (keep only last 10 for previous error columns and derivative calculations)
                prev_error_x.append(error_x)
                prev_error_y.append(error_y)
                if len(prev_error_x) > 10:
                    prev_error_x.pop(0)
                    prev_error_y.pop(0)
                
            # Write the new file
            print(f"Saving to {output_file}...")
            
            # Create fieldnames including all derivative terms
            base_fieldnames = ['error_x', 'error_y', 'integral_x', 'integral_y']
            
            # Add previous error fieldnames for n=1 to 10
            prev_error_fieldnames = []
            for n in range(1, 11):
                prev_error_fieldnames.extend([f'error_x_prev{n}', f'error_y_prev{n}'])
            
            # Add derivative terms for n=1 to 10
            derivative_fieldnames = []
            for n in range(1, 11):
                derivative_fieldnames.extend([f'derivative_x_{n}', f'derivative_y_{n}'])
            
            fieldnames = reader.fieldnames + base_fieldnames + prev_error_fieldnames + derivative_fieldnames
            
            with open(output_file, 'w', newline='') as outfile:
                writer = csv.DictWriter(outfile, fieldnames=fieldnames)
                writer.writeheader()
                writer.writerows(rows)
            
            print(f"Successfully processed {len(rows)} rows")
            print(f"Output saved to: {output_file}")
            
            # Only display statistics if we have data
            if len(rows) > 0:
                # Extract error values from rows for statistics
                error_x_values = [row['error_x'] for row in rows]
                error_y_values = [row['error_y'] for row in rows]
                
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
                
                # Calculate statistics for derivative terms (n=1 to 10)
                print("\nAdditional derivative terms statistics:")
                for n in range(1, 11):
                    if n == 2:  # Skip n=2 as it's already shown above
                        continue
                        
                    derivative_x_n_values = [row[f'derivative_x_{n}'] for row in rows]
                    derivative_y_n_values = [row[f'derivative_y_{n}'] for row in rows]
                    
                    print(f"Derivative X (n={n}):")
                    print(f"  Mean: {statistics.mean(derivative_x_n_values):.2f}")
                    print(f"  Std: {statistics.stdev(derivative_x_n_values):.2f}")
                    print(f"  Min: {min(derivative_x_n_values):.2f}")
                    print(f"  Max: {max(derivative_x_n_values):.2f}")
                    print(f"Derivative Y (n={n}):")
                    print(f"  Mean: {statistics.mean(derivative_y_n_values):.2f}")
                    print(f"  Std: {statistics.stdev(derivative_y_n_values):.2f}")
                    print(f"  Min: {min(derivative_y_n_values):.2f}")
                    print(f"  Max: {max(derivative_y_n_values):.2f}")
                    print()
            else:
                print("No rows processed - no statistics available")
            
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