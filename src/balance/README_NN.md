# Neural Network Implementation for Balance Control

This document describes the fully connected neural network implementation using CMSIS-NN for the ball balancing platform control system.

## Overview

The neural network is designed to control a ball balancing platform by taking sensor inputs and outputting platform control signals. It uses CMSIS-NN optimized functions for efficient execution on ARM Cortex-M processors.

## Architecture

### Network Structure
- **Input Layer**: 8 neurons
  - Ball position (x, y)
  - Target position (x, y)
  - Error (x, y)
  - Integral error (x, y)
- **Hidden Layer**: 16 neurons with ReLU activation
- **Output Layer**: 2 neurons (platform_x, platform_y)

### Data Flow
```
Sensor Data → Input Normalization → FC Layer 1 → ReLU → FC Layer 2 → Denormalization → Platform Control
```

## Implementation Details

### Quantization
- **Input/Output**: 16-bit fixed point (s16)
- **Weights**: 8-bit signed integers (s8)
- **Biases**: 64-bit signed integers (s64)
- **Scaling**: Input scale 0.001, Output scale 100.0

### CMSIS-NN Functions Used
- `arm_fully_connected_s16()`: Main fully connected layer computation
- Quantization parameters for proper scaling
- ReLU activation function implementation

### Key Features
1. **Real-time Processing**: Optimized for embedded systems
2. **Fixed-point Arithmetic**: No floating-point operations
3. **Memory Efficient**: Minimal RAM usage with optimized buffers
4. **Robust Control**: Includes integral error tracking for stability

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
void BALANCE_NN_Run(q15_t target_x, q15_t target_y);
```
Runs the neural network inference and applies platform control.

### Debug/Visualization
```c
void BALANCE_NN_DataVisualizer(void);
```
Provides debug output and data visualization (placeholder).

## Configuration

### Network Parameters
```c
#define NN_INPUT_SIZE          8   // Input features
#define NN_HIDDEN_SIZE         16  // Hidden layer neurons
#define NN_OUTPUT_SIZE         2   // Output neurons
#define NN_BUFFER_SIZE         256 // Working buffer size
```

### Scaling Parameters
```c
#define INPUT_SCALE            0.001f  // Input normalization
#define OUTPUT_SCALE           100.0f  // Output denormalization
#define ACTIVATION_MIN         -32768  // ReLU min value
#define ACTIVATION_MAX         32767   // ReLU max value
```

## Training Data Analysis

Based on the training data analysis, the system handles:
- **Ball Position Range**: ~2800-3400 (x, y coordinates)
- **Target Position Range**: ~2800-3100 (x, y coordinates)
- **Platform Control Range**: -1500 to 1500 (servo positions)
- **Error Range**: -500 to 500 (position error)

## Performance Considerations

### Memory Usage
- **Static Weights**: ~256 bytes (8-bit weights)
- **Working Buffer**: 256 bytes (CMSIS-NN buffer)
- **State Variables**: ~64 bytes (error tracking, etc.)
- **Total RAM**: ~576 bytes

### Computational Complexity
- **Input Layer**: 8 × 16 = 128 multiply-accumulate operations
- **Hidden Layer**: 16 × 2 = 32 multiply-accumulate operations
- **Total**: ~160 MAC operations per inference

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
    // Get target position
    q15_t target_x = get_target_x();
    q15_t target_y = get_target_y();
    
    // Run neural network inference
    BALANCE_NN_Run(target_x, target_y);
    
    // Wait for next control cycle
    delay_ms(10);
}
```

## Training and Optimization

### Current Status
- **Weights**: Placeholder values (need training)
- **Architecture**: Fixed structure
- **Quantization**: Manual scaling

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
- Input data preparation
- Neural network inference
- Output denormalization
- Platform control limits

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

## References

- [CMSIS-NN Documentation](https://arm-software.github.io/CMSIS_5/NN/html/index.html)
- [ARM Cortex-M Optimization Guide](https://developer.arm.com/documentation/101748/0612/)
- [Fixed-point Neural Networks](https://arxiv.org/abs/1712.05877)

## License

This implementation is part of the MLBB (Machine Learning Ball Balancer) project. 