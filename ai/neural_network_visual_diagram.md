# Neural Network Visual Diagram

## Network Architecture

```
Input Layer (4)     Hidden Layer(s)     Output Layer (2)
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│ Error X     │────▶│             │────▶│ Platform X  │
│ Error Y     │────▶│   Neural    │────▶│ Platform Y  │
│ Error ΔX    │────▶│  Network    │     │             │
│ Error ΔY    │────▶│             │     │             │
└─────────────┘     └─────────────┘     └─────────────┘
```

## Data Flow Diagram

```
Ball Position Input
    (target_x, target_y, ball_x, ball_y)
           │
           ▼
    Error Calculation
    error_x = target_x - ball_x
    error_y = target_y - ball_y
           │
           ▼
    Error Delta Calculation
    (using historical values)
           │
           ▼
    Integral Anti-Windup Logic
    IF (near_target AND moving_slow)
       update_integral_terms
           │
           ▼
    Neural Network Forward Pass
    nn_forward(inputs, outputs)
           │
           ▼
    Output Processing
    (clamp to Q15 range)
           │
           ▼
    Platform Control
    PLATFORM_Position_XY_Set()
```

## Input Processing

### Error History Buffer (Circular)
```
Index:  0  1  2  3  4  5  6  7  8  9
Error: [E][E][E][E][E][E][E][E][E][E]
        ↑
    Current Index
```

### Anti-Windup Logic
```
┌─────────────────────────────────────┐
│ Integral Anti-Windup Conditions    │
├─────────────────────────────────────┤
│ Near Target: |error| < 512        │
│ Slow Movement: |error_delta| < 5   │
│                                         │
│ IF (near_target AND moving_slow)       │
│   THEN update_integral_terms            │
│ ELSE skip_integral_update               │
└─────────────────────────────────────┘
```

## Neural Network Input/Output

### Input Vector (4 elements)
```
┌─────────────┬─────────────┬─────────────┬─────────────┐
│ Error X     │ Error Y     │ Error ΔX    │ Error ΔY    │
│ (float)     │ (float)     │ (float)     │ (float)     │
└─────────────┴─────────────┴─────────────┴─────────────┘
```

### Output Vector (2 elements)
```
┌─────────────┬─────────────┐
│ Platform X  │ Platform Y  │
│ (float)     │ (float)     │
└─────────────┴─────────────┘
```

## State Management

### Neural Network State Structure
```
┌─────────────────────────────────────────────────────────┐
│ balance_nn_state_t                                    │
├─────────────────────────────────────────────────────────┤
│ float error_history_x[10]    // Circular buffer       │
│ float error_history_y[10]    // Circular buffer       │
│ float error_sum_x            // Integral term X        │
│ float error_sum_y            // Integral term Y        │
│ uint8_t error_history_index  // Buffer index          │
│ q15_t output_x              // Current output X       │
│ q15_t output_y              // Current output Y       │
└─────────────────────────────────────────────────────────┘
```

## Key Features

### Intelligent Control
- **Learned Responses**: Neural network provides trained control patterns
- **Non-linear Mapping**: Handles complex control relationships
- **Adaptive Behavior**: Responds differently to various input scenarios

### Robust Processing
- **Anti-windup Logic**: Prevents integral term saturation
- **Historical Filtering**: Smooth derivative calculation
- **Output Clamping**: Ensures valid platform positions

### Real-time Performance
- **Fixed Complexity**: Deterministic execution time
- **Efficient Memory**: Static allocation with circular buffers
- **Direct Integration**: Minimal latency control loop 