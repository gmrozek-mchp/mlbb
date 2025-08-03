# Neural Network Control System for Ball Balancing

## System Overview

The neural network controller in `balance_nn.c` implements an intelligent control system for balancing a ball on a platform using a trained neural network. The system processes ball position errors and produces platform control commands through a forward pass of the neural network.

## 1. Neural Network Architecture

### Input Layer (4 inputs)
The neural network takes 4 inputs for each axis (X and Y):

1. **Error (error_x, error_y)**: Current position error = target - ball_position
2. **Error Delta (error_delta_x, error_delta_y)**: Rate of change of error (filtered derivative)

### Output Layer (2 outputs)
The neural network produces 2 outputs:

1. **Platform X Position**: Target platform position in X-axis
2. **Platform Y Position**: Target platform position in Y-axis

### Network Structure
```
Input Layer (4) → Hidden Layer(s) → Output Layer (2)
```

The actual network architecture and weights are defined in `balance_nn_weights.h` and implemented in `balance_nn_weights.c`.

## 2. Input Processing

### Error Calculation
```c
float error_x = (float)target_x - (float)ball_x;
float error_y = (float)target_y - (float)ball_y;
```

### Error Delta Calculation (Filtered Derivative)
```c
// Calculate error delta using historical values
int idx_prev = (nn_state.error_history_index - NN_ERROR_DELTA_FILTER_SIZE + NN_ERROR_HISTORY_SIZE) % NN_ERROR_HISTORY_SIZE;
float error_prev_x = nn_state.error_history_x[idx_prev];
float error_prev_y = nn_state.error_history_y[idx_prev];

float error_delta_x = error_x - error_prev_x;
float error_delta_y = error_y - error_prev_y;
```

### Integral Anti-Windup Logic
The system includes intelligent integral control with anti-windup:

```c
// Only update integral when ball is near target and moving slow
bool near_target = (fabs(error_x) < NN_NEAR_TARGET_THRESHOLD) && (fabs(error_y) < NN_NEAR_TARGET_THRESHOLD);
bool moving_slow = (fabs(error_delta_x) < NN_SLOW_MOVEMENT_THRESHOLD) && (fabs(error_delta_y) < NN_SLOW_MOVEMENT_THRESHOLD);
bool integral_enabled = near_target && moving_slow;

if (integral_enabled) {
    nn_state.error_sum_x += error_x;
    nn_state.error_sum_y += error_y;
}
```

**Thresholds:**
- `NN_NEAR_TARGET_THRESHOLD`: 512 (Q15 units) - Ball is "near" target
- `NN_SLOW_MOVEMENT_THRESHOLD`: 5 (Q15 units) - Ball is moving "slow"

## 3. Neural Network Forward Pass

### Input Preparation
```c
// Prepare neural network inputs (convert to float)
float nn_inputs[NN_INPUT_SIZE];
nn_prepare_inputs(target_x, target_y, ball_x, ball_y, nn_inputs);
```

### Network Execution
```c
// Run neural network forward pass (float)
float nn_outputs[NN_OUTPUT_SIZE];
nn_forward(nn_inputs, nn_outputs);
```

### Output Processing
```c
// Apply output to platform X control
float platform_x = nn_outputs[NN_OUTPUT_PLATFORM_X];
float platform_y = nn_outputs[NN_OUTPUT_PLATFORM_Y];

// Clamp outputs to Q15 range
if (platform_x > Q15_MAX) {
    nn_state.output_x = Q15_MAX;
} else if (platform_x < Q15_MIN) {
    nn_state.output_x = Q15_MIN;
} else {
    nn_state.output_x = (q15_t)platform_x;
}
```

## 4. Data Structures

### Neural Network State
```c
typedef struct {
    float error_history_x[NN_ERROR_HISTORY_SIZE];  // Previous error values
    float error_history_y[NN_ERROR_HISTORY_SIZE];
    float error_sum_x;                             // Integral term for X
    float error_sum_y;                             // Integral term for Y
    uint8_t error_history_index;                   // Circular buffer index
    q15_t output_x;                               // Current platform X output
    q15_t output_y;                               // Current platform Y output
} balance_nn_state_t;
```

### Configuration Constants
```c
#define NN_ERROR_HISTORY_SIZE (10)           // Number of historical error values
#define NN_ERROR_DELTA_FILTER_SIZE (5)       // Filter size for derivative calculation
#define NN_NEAR_TARGET_THRESHOLD (512)       // Q15 units - ball is "near" target
#define NN_SLOW_MOVEMENT_THRESHOLD (5)       // Q15 units - ball is moving "slow"
```

## 5. Control Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    Ball Position Input                     │
│  (target_x, target_y, ball_x, ball_y)                    │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Error Calculation                       │
│  error_x = target_x - ball_x                             │
│  error_y = target_y - ball_y                             │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                Error Delta Calculation                     │
│  error_delta_x = error_x - error_prev_x                  │
│  error_delta_y = error_y - error_prev_y                  │
│  (using filtered historical values)                       │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                Integral Anti-Windup Logic                  │
│  IF (near_target AND moving_slow) THEN                   │
│    update_integral_terms                                  │
│  END IF                                                  │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                Neural Network Forward Pass                 │
│  nn_forward(nn_inputs, nn_outputs)                       │
│  - Processes 4 inputs (error_x, error_delta_x,           │
│    error_y, error_delta_y)                               │
│  - Produces 2 outputs (platform_x, platform_y)           │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Output Processing                       │
│  - Clamp outputs to Q15 range [-16384, 16383]            │
│  - Apply to platform position control                     │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Platform Control                        │
│  PLATFORM_Position_XY_Set(platform_x, platform_y)        │
└─────────────────────────────────────────────────────────────┘
```

## 6. Key Features

### Intelligent Integral Control
- **Anti-windup logic**: Only updates integral terms when conditions are met
- **Near target detection**: Prevents integral windup when ball is far from target
- **Slow movement detection**: Prevents integral windup during rapid movements

### Historical Error Tracking
- **Circular buffer**: Maintains history of error values
- **Filtered derivative**: Uses historical values for smooth derivative calculation
- **Configurable history size**: Adjustable buffer size for different response characteristics

### Neural Network Integration
- **Float precision**: Uses floating-point arithmetic for neural network calculations
- **Q15 conversion**: Converts between float and fixed-point formats
- **Output clamping**: Ensures outputs stay within valid platform range

### Robust Error Handling
- **Ball detection**: Handles cases where ball is not detected
- **Output validation**: Clamps outputs to prevent invalid platform positions
- **State management**: Maintains consistent state across control cycles

## 7. Command Interface

### Available Commands
- `nntest`: Run neural network test with known inputs
- Provides detailed output showing input/output relationships

### Test Scenarios
The test command evaluates the neural network with:
1. **Small errors**: Tests response to minor position errors
2. **Large errors**: Tests response to significant position errors  
3. **Zero inputs**: Tests baseline behavior
4. **Negative values**: Tests response to negative errors

## 8. Data Visualization

### Data Visualizer Format
The system provides real-time data visualization with:
- Ball detection status
- Target and ball positions (X, Y)
- Platform output positions (X, Y)
- Platform ABC coordinates
- Neural network identifier ('N')

### Data Structure
```c
static uint8_t dv_data[22];
// Format: [header][type][ball_detected][target_x][target_y][ball_x][ball_y]
//         [output_x][output_y][platform_a][platform_b][platform_c][checksum]
```

## 9. Performance Characteristics

### Real-time Operation
- **Deterministic execution**: Fixed computational complexity
- **Efficient memory usage**: Static allocation with circular buffers
- **Minimal latency**: Direct forward pass without complex optimization

### Adaptive Behavior
- **Learned responses**: Neural network provides learned control patterns
- **Non-linear mapping**: Can handle complex, non-linear control relationships
- **Generalization**: Trained network can handle various input scenarios

### Robustness
- **Graceful degradation**: Handles edge cases and invalid inputs
- **State consistency**: Maintains consistent state across control cycles
- **Error recovery**: Recovers from temporary sensor failures

## 10. Integration with Platform

### Platform Interface
- **Direct position control**: Sets platform X/Y positions directly
- **Q15 format**: Uses fixed-point format compatible with platform hardware
- **Range validation**: Ensures platform positions stay within valid range

### Control Coordination
- **Independent X/Y control**: Each axis controlled independently
- **Synchronized updates**: Both axes updated simultaneously
- **State preservation**: Maintains platform state across control cycles

This neural network implementation provides intelligent, adaptive control for the ball balancing system, leveraging learned patterns to achieve smooth and responsive ball positioning while maintaining robustness and real-time performance. 