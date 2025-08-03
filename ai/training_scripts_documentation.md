# Training Scripts Documentation

This document provides comprehensive documentation for all scripts in the `training/scripts` directory, which are used for training, converting, and analyzing neural network models for the ball balancing control system.

## Overview

The training scripts provide a complete pipeline for:
1. **Data Visualization**: Plotting ball trajectories from CSV data
2. **Model Training**: Training neural networks on human demonstration data
3. **Model Conversion**: Converting trained models to C code for embedded deployment
4. **Weight Extraction**: Analyzing and comparing model weights

## Script 1: `plot_ball_trajectory.py`

### Purpose
Visualizes ball trajectory data from CSV files to analyze human demonstration data and model performance.

### Features
- **Multiple Plot Types**: Basic trajectory plots and time-based plots
- **Batch Processing**: Process all CSV files in a directory
- **Comparison Mode**: Compare multiple trajectory files
- **Export Options**: Save plots as PNG files

### Usage
```bash
# Plot a single file
python plot_ball_trajectory.py human01_with_error.csv

# Plot all CSV files in processed directory
python plot_ball_trajectory.py --all --save

# Compare multiple files
python plot_ball_trajectory.py --compare human01_with_error.csv human02_with_error.csv

# Include time-based plots
python plot_ball_trajectory.py --with-time human01_with_error.csv

# Save plots without displaying
python plot_ball_trajectory.py --all --save --no-show
```

### Key Functions

#### `load_csv_data(file_path)`
- Loads CSV data and extracts ball position information
- Handles missing columns gracefully
- Returns DataFrame with ball_x, ball_y, target_x, target_y, and timestamp

#### `plot_single_trajectory(df, title, save_path, show_plot)`
- Creates scatter plot with color-coded time progression
- Shows start (green) and end (red) markers
- Includes trajectory line overlay

#### `plot_trajectory_with_time(df, title, save_path, show_plot)`
- Creates dual-panel plot: XY trajectory and position vs time
- Shows target positions in time plot
- Includes color-coded timestamp information

### Input Data Format
Expected CSV columns:
- `ball_x`, `ball_y`: Ball position coordinates
- `target_x`, `target_y`: Target position coordinates (optional)
- `timestamp`: Time information (optional)

### Output
- **Interactive plots**: Displayed on screen
- **PNG files**: Saved to `../plots/` directory (when using `--save`)
- **File naming**: `{filename}_trajectory.png` and `{filename}_with_time.png`

---

## Script 2: `train_q15_control.py`

### Purpose
Trains a neural network model for ball balancing control using human demonstration data.

### Architecture
```
Input Layer (4) → Hidden Layer 1 (12) → Hidden Layer 2 (6) → Output Layer (2)
```

**Inputs:**
- `error_x`: X-axis position error
- `error_delta_x`: X-axis error rate of change
- `error_y`: Y-axis position error  
- `error_delta_y`: Y-axis error rate of change

**Outputs:**
- `platform_x`: Target platform X position
- `platform_y`: Target platform Y position

### Features
- **Data Preprocessing**: Filters problematic samples and handles missing data
- **Adaptive Learning Rate**: Implements learning rate scheduling
- **Robust Training**: Uses Nadam optimizer with MAE loss
- **Comprehensive Evaluation**: Provides detailed model testing and metrics

### Usage
```bash
python train_q15_control.py
```

### Key Functions

#### `create_neural_network()`
- Creates 4-layer neural network (4→12→6→2)
- Uses ReLU activation for hidden layers
- Linear activation for input and output layers
- Configures Nadam optimizer with MAE loss

#### `load_and_preprocess_data(csv_files, input_cols, output_cols)`
- Loads CSV files from `../rawdata/human*.csv`
- Filters out rows with NaN values
- Removes problematic samples (large errors with small control responses)
- Splits data into 80% training, 20% validation
- Randomizes data order for better training

#### `train_model(model, X_train, y_train, X_val, y_val, epochs=100)`
- Implements learning rate scheduling (decay factor: 0.95)
- Uses batch size of 16 for optimal gradient updates
- Provides detailed training progress

#### `test_model(model, X_val, y_val, input_cols, output_cols)`
- Calculates MSE and MAE metrics
- Shows sample predictions for verification
- Displays input/output format information

### Data Requirements
- CSV files in `../rawdata/` directory
- Required columns: `error_x`, `error_delta_x`, `error_y`, `error_delta_y`, `platform_x`, `platform_y`
- Files should be named `human*.csv`

### Output
- **Trained Model**: `ball_balance_control_model.keras`
- **Training Plots**: Loss and MAE curves
- **Performance Metrics**: Test MSE and MAE values
- **Sample Predictions**: Verification of model behavior

---

## Script 3: `convert_model_to_c.py`

### Purpose
Converts trained Keras neural network models to C code for embedded deployment.

### Features
- **Weight Extraction**: Extracts weights and biases from trained model
- **C Code Generation**: Creates header and implementation files
- **Architecture Detection**: Automatically detects model structure
- **Optimized Implementation**: Generates efficient C matrix operations

### Usage
```bash
python convert_model_to_c.py
```

### Key Functions

#### `convert_model_to_c(model_path, output_file)`
- Loads trained Keras model
- Extracts weights and biases from all layers
- Generates C header file with network architecture
- Creates C implementation file with forward pass

#### `generate_c_header(output_file, layer_info)`
- Creates `nn_weights.h` header file
- Defines network architecture constants
- Declares weight and bias arrays
- Provides function declarations

#### `generate_c_implementation(output_file)`
- Creates `nn_weights.c` implementation file
- Implements matrix-vector multiplication
- Provides ReLU activation function
- Generates complete forward pass implementation

### Generated Files

#### `nn_weights.h`
```c
// Network architecture constants
#define NN_INPUT_SIZE 4
#define NN_HIDDEN1_SIZE 12
#define NN_HIDDEN2_SIZE 6
#define NN_OUTPUT_SIZE 2

// Weight and bias arrays
static const float INPUT_WEIGHTS[] = {...};
static const float INPUT_BIAS[] = {...};
// ... additional layers

// Function declarations
void nn_forward(const float* input, float* output);
float nn_relu(float x);
```

#### `nn_weights.c`
```c
// Matrix multiplication implementation
static void nn_matmul_float(const float* weights, const float* input, 
                           float* output, uint32_t rows, uint32_t cols);

// Complete forward pass
void nn_forward(const float* input, float* output) {
    // Layer-by-layer computation with ReLU activations
}
```

### Supported Architectures
- **2-layer model**: 4→12→6→2
- **3-layer model**: 4→4→24→12→8→2
- **Custom architectures**: Automatically detected from model

---

## Script 4: `extract_c_weights.py`

### Purpose
Extracts and analyzes neural network weights from generated C code and compares them with the original Keras model.

### Features
- **Weight Extraction**: Parses C header files to extract weights
- **Model Comparison**: Compares C weights with Keras weights
- **Verification**: Tests forward pass computation
- **Debugging**: Helps identify conversion issues

### Usage
```bash
python extract_c_weights.py
```

### Key Functions

#### `extract_c_weights()`
- Parses `balance_nn_weights.h` file
- Extracts weights and biases using regex
- Reshapes arrays to correct dimensions
- Returns structured weight data

#### `compare_with_keras()`
- Loads original Keras model
- Compares weights between C and Keras implementations
- Tests forward pass with sample input
- Reports differences and verification results

### Output
- **Weight Shapes**: Dimensions of extracted weight arrays
- **Weight Values**: Actual weight values from C code
- **Comparison Results**: Differences between C and Keras weights
- **Verification**: Forward pass test results

### Verification Process
1. Extracts weights from C header file
2. Loads corresponding Keras model
3. Compares weight arrays element-by-element
4. Tests forward pass with sample input
5. Reports maximum differences and verification results

---

## Script 5: `requirements.txt`

### Purpose
Defines Python package dependencies for all training scripts.

### Key Dependencies
- **TensorFlow 2.19.0**: Neural network training and model handling
- **Keras 3.10.0**: High-level neural network API
- **NumPy 2.1.3**: Numerical computations
- **Pandas 2.3.1**: Data manipulation and CSV handling
- **Matplotlib 3.10.3**: Plotting and visualization
- **Additional packages**: Supporting libraries for data processing and model conversion

### Installation
```bash
pip install -r requirements.txt
```

---

## Complete Training Pipeline

### Step 1: Data Preparation
```bash
# Visualize training data
python plot_ball_trajectory.py --all --save
```

### Step 2: Model Training
```bash
# Train neural network
python train_q15_control.py
```

### Step 3: Model Conversion
```bash
# Convert to C code
python convert_model_to_c.py
```

### Step 4: Verification
```bash
# Verify conversion accuracy
python extract_c_weights.py
```

### Step 5: Deployment
- Copy generated `nn_weights.h` and `nn_weights.c` to embedded project
- Integrate with `balance_nn.c` implementation
- Test on target hardware

---

## File Structure
```
training/scripts/
├── plot_ball_trajectory.py      # Data visualization
├── train_q15_control.py         # Model training
├── convert_model_to_c.py        # C code generation
├── extract_c_weights.py         # Weight verification
├── requirements.txt             # Dependencies
└── ball_balance_control_model.keras  # Trained model
```

## Best Practices

### Data Quality
- Ensure CSV files have consistent column names
- Remove outliers and problematic samples
- Validate data ranges and formats

### Model Training
- Monitor training curves for overfitting
- Use appropriate learning rate scheduling
- Validate model performance on test data

### Code Generation
- Verify weight extraction accuracy
- Test generated C code thoroughly
- Ensure numerical precision compatibility

### Deployment
- Test converted model on target hardware
- Validate performance against original model
- Monitor real-time execution characteristics

This training pipeline provides a complete workflow from data visualization to embedded deployment, ensuring robust and accurate neural network control for the ball balancing system. 