# Neural Network Implementation for Balance Control

This document describes the fully connected neural network implementation using CMSIS-NN for the ball balancing platform control system.

## Overview

The neural network is designed to control a ball balancing platform by taking error-based inputs and outputting platform control signals in ABC coordinates. It uses CMSIS-NN optimized functions for efficient execution on ARM Cortex-M processors.

## Architecture

### Network Structure
- **Input Layer**: 6 neurons
  - X error (target_x - ball_x)
  - Y error (target_y - ball_y)
  - X integral (accumulated error)
  - Y integral (accumulated error)
  - X derivative (error rate of change)
  - Y derivative (error rate of change)
- **Hidden Layer 1**: 12 neurons with ReLU activation
- **Hidden Layer 2**: 12 neurons with ReLU activation
- **Output Layer**: 3 neurons (platform_a, platform_b, platform_c)

### Data Flow
```
Error Calculation → PID Processing → Input Normalization → FC Layer 1 → ReLU → FC Layer 2 → ReLU → FC Layer 3 → Denormalization → ABC Platform Control
```

## Implementation Details

### Quantization
- **Input/Output**: 16-bit fixed point (s16)
- **Weights**: 8-bit signed integers (s8)
- **Biases**: 64-bit signed integers (s64)
- **Scaling**: Integer bit shifts (no floating point)

### CMSIS-NN Functions Used
- `arm_fully_connected_s16()`: Main fully connected layer computation
- Quantization parameters for proper scaling
- ReLU activation function implementation

### Key Features
1. **Real-time Processing**: Optimized for embedded systems
2. **Integer Arithmetic**: No floating-point operations for efficiency
3. **Memory Efficient**: Minimal RAM usage with optimized buffers
4. **PID-like Control**: Includes error, integral, and derivative components
5. **Direct ABC Control**: Outputs platform servo positions directly

## File Structure

### Source Files
- `balance_nn.c`: Main neural network implementation
- `balance_nn.h`: Header file with function declarations

### Test Files
- `training/scripts/simple_nn_test.py`: Python test script
- `training/scripts/test_nn_implementation.py`: Comprehensive test script

## API Functions

### Initialization
```c
void BALANCE_NN_Initialize(void);
```
Initializes the neural network state and buffers.

### Reset
```c
void BALANCE_NN_Reset(void);
```
Resets the neural network state and error tracking.

### Main Processing
```c
void BALANCE_NN_Run(q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y);
```
Runs the neural network inference and applies platform control.

### Debug/Visualization
```c
void BALANCE_NN_DataVisualizer(q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y);
```
Provides debug output and data visualization (placeholder).

## Test Commands

The implementation includes comprehensive test commands for debugging and validation:

### Available Commands
- **`nn`**: Display network state (initialization, error index, integral values)
- **`nni`**: Show current input data (6 error-based inputs)
- **`nno`**: Show current output data (3 ABC platform outputs)
- **`nnt`**: Test network inference with custom inputs
- **`nnr`**: Reset network state and error tracking

### Command Usage Examples
```bash
# Test with default values
nnt

# Test with custom inputs (x_error, y_error, x_integral, y_integral, x_derivative, y_derivative)
nnt 100 -50 200 -100 10 -5

# Check network state
nn

# View current inputs/outputs
nni
nno

# Reset network
nnr
```

### Test Command Features
- **Flexible Input Testing**: Accept custom 6-parameter inputs or use defaults
- **State Inspection**: Monitor network initialization and error tracking
- **Input/Output Validation**: Verify data preparation and network outputs
- **Complete Pipeline Testing**: Full end-to-end inference testing
- **Reset Capability**: Clean state for repeatable testing

## Configuration

### Network Parameters
```c
#define NN_INPUT_SIZE          6   // Error-based inputs
#define NN_HIDDEN1_SIZE        12  // First hidden layer neurons
#define NN_HIDDEN2_SIZE        12  // Second hidden layer neurons
#define NN_OUTPUT_SIZE         3   // ABC output neurons
#define NN_BUFFER_SIZE         512 // Working buffer size
```

### Scaling Parameters
```c
#define INPUT_SCALE_SHIFT      10   // Input normalization (1/1024)
#define OUTPUT_SCALE_SHIFT     7    // Output denormalization (128x)
#define ACTIVATION_MIN         -32768  // ReLU min value
#define ACTIVATION_MAX         32767   // ReLU max value
```

## Training Data Analysis

Based on the training data analysis, the system handles:
- **Ball Position Range**: ~2800-3400 (x, y coordinates)
- **Target Position Range**: ~2800-3100 (x, y coordinates)
- **Platform Control Range**: -1500 to 1500 (ABC servo positions)
- **Error Range**: -500 to 500 (position error)

## Performance Considerations

### Memory Usage
- **Static Weights**: ~432 bytes (6×12 + 12×12 + 12×3 weights)
- **Working Buffer**: 512 bytes (CMSIS-NN buffer)
- **State Variables**: ~64 bytes (error tracking, etc.)
- **Total RAM**: ~1008 bytes

### Computational Complexity
- **Input Layer**: 6 × 12 = 72 multiply-accumulate operations
- **Hidden Layer 1**: 12 × 12 = 144 multiply-accumulate operations
- **Hidden Layer 2**: 12 × 3 = 36 multiply-accumulate operations
- **Total**: ~252 MAC operations per inference

### Timing
- **Inference Time**: < 1ms on ARM Cortex-M4/M7
- **Update Rate**: 100Hz+ recommended for smooth control

## Integration

### Dependencies
- CMSIS-NN library
- Ball sensor interface (`ball.h`)
- Platform control interface (`platform.h`)

### Usage Example
```c
// Initialize the neural network
BALANCE_NN_Initialize();

// Main control loop
while (1) {
    // Get ball position
    ball_data_t ball_data = BALL_Position_Get();
    
    if (ball_data.detected) {
        // Get target position
        q15_t target_x = get_target_x();
        q15_t target_y = get_target_y();
        
        // Run neural network inference
        BALANCE_NN_Run(target_x, target_y, true, ball_data.x, ball_data.y);
    }
    
    // Wait for next control cycle
    delay_ms(10);
}
```

## Training and Optimization

### Current Status
- **Weights**: Placeholder values (need training)
- **Architecture**: Fixed 6-12-12-3 structure
- **Quantization**: Integer scaling with fixed shifts

### Future Improvements
1. **Online Training**: Implement online learning capabilities
2. **Adaptive Scaling**: Dynamic quantization based on data
3. **Multi-layer Networks**: Add more hidden layers if needed
4. **Recurrent Connections**: Add LSTM/GRU for temporal modeling

### Training Process
1. Collect training data from human demonstrations
2. Train network using TensorFlow/Keras
3. Quantize weights using CMSIS-NN tools
4. Generate C code with optimized weights
5. Test on target hardware

## Testing

### Unit Tests
- Input data preparation with error-based inputs
- Neural network inference with 3-layer architecture
- Output denormalization with integer math
- Platform control limits for ABC coordinates
- Command portal function testing
- Custom input parameter validation

### Integration Tests
- End-to-end control loop
- Real-time performance
- Memory usage validation

### Test Scripts
```bash
cd training/scripts
python3 simple_nn_test.py
```

## Troubleshooting

### Common Issues
1. **Overflow**: Check input scaling and weight ranges
2. **Underflow**: Verify quantization parameters
3. **Memory**: Ensure buffer sizes are sufficient
4. **Timing**: Monitor inference time for real-time constraints

### Debug Features
- Input/output data logging
- Weight value inspection
- Performance profiling
- Memory usage tracking
- Command portal for real-time debugging
- Network state inspection
- Custom input testing capabilities

## References

- [CMSIS-NN Documentation](https://arm-software.github.io/CMSIS_5/NN/html/index.html)
- [ARM Cortex-M Optimization Guide](https://developer.arm.com/documentation/101748/0612/)
- [Fixed-point Neural Networks](https://arxiv.org/abs/1712.05877)

## License

This implementation is part of the MLBB (Machine Learning Ball Balancer) project. 