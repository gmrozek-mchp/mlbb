# Fuzzy Logic Tuning Guide

## Current Oscillation Issue

The slow oscillation around the target is caused by several factors:

1. **Insufficient damping** - The error derivative scaling was too low
2. **Overlapping membership functions** - Too much overlap in the zero region
3. **Aggressive output scaling** - Too high output gain causing overshoot
4. **Slow derivative response** - 5-point moving average might be too slow

## Current Default Parameters (Working Well)

The fuzzy controller has been tuned to provide reasonable control response:

- **Error Scale**: 1200 (moderate) - Good position responsiveness without overshoot
- **Error Dot Scale**: 2500 (strong damping) - Excellent damping for ball approach
- **Output Scale**: 170 (balanced) - Appropriate control effort for stable response

## Membership Function Improvements

### Error Membership Functions
- **Zero region**: Reduced from ±2048 to ±1024 for better precision
- **Small regions**: Adjusted overlap to reduce oscillation
- **Large regions**: Unchanged for large error handling

### Error Dot Membership Functions  
- **Zero region**: Set to ±128 for good damping sensitivity
- **Small regions**: Well-tuned for responsive derivative control

### Enhanced Fuzzy Rules
- **Zero error region**: Balanced damping response for good control
- **Small error regions**: Effective damping for ball approach
- **Balanced stability**: Good balance between stability and responsiveness

## Tuning Commands

### Basic Commands
- `fuzzy` - Print current state (error, platform position)
- `fuzzys` - Print current scaling factors
- `fuzzyreset` - Reset the fuzzy controller

### Tuning Commands
- `fes <value>` - Set error scale factor
- `feds <value>` - Set error dot scale factor
- `fos <value>` - Set output scale factor

## Tuning Strategy

### Step 1: Fine-tune Current Performance
1. **Adjust error_scale** around 1000-1400 for position responsiveness
2. **Adjust error_dot_scale** around 2000-3000 for damping control
3. **Adjust output_scale** around 150-200 for control effort

### Step 2: Improve Response
1. **Increase error_scale** to 220-250 for faster response
2. **Fine-tune error_dot_scale** to balance damping vs responsiveness
3. **Adjust output_scale** to 160-200 for optimal control effort

### Step 3: Fine-tune
1. **Monitor system response** using `fuzzy` command
2. **Adjust parameters incrementally** (10-20% changes)
3. **Test with different target positions**

## Recommended Parameter Ranges

| Parameter | Min | Default | Max | Effect |
|-----------|-----|---------|-----|--------|
| error_scale | 800 | 1200 | 1500 | Position responsiveness |
| error_dot_scale | 1500 | 2500 | 3000 | Damping/oscillation control |
| output_scale | 120 | 170 | 220 | Control effort |

## Troubleshooting

### Too Much Oscillation
- Increase `error_dot_scale` by 500-800
- Decrease `output_scale` by 80-120
- Decrease `error_scale` by 50-80

### Too Slow Response
- Increase `error_scale` by 20-30
- Decrease `error_dot_scale` by 50-100
- Increase `output_scale` by 20-30

### Oscillations
- Decrease `error_scale` by 100-200
- Increase `error_dot_scale` by 300-500
- Decrease `output_scale` by 20-50

### Steady-state Error
- Increase `error_scale` by 50-100
- Increase `output_scale` by 30-50
- Decrease `error_dot_scale` by 100-200

## Example Tuning Session

```
# Check current parameters
fuzzys

# If oscillations, reduce error scale
fes 1000

# If still oscillating, increase error dot scale
feds 3000

# If too slow response, increase output scale
fos 200

# If response is too slow, increase error scale
fes 220

# Reset if needed
fuzzyreset
```

## Performance Monitoring

Use the `fuzzy` command to monitor:
- Error magnitude (should decrease over time)
- Platform position (should stabilize)
- Overall system behavior

The goal is to achieve:
- Fast initial response
- Minimal overshoot
- Quick settling time
- No sustained oscillation

## Current Performance Characteristics

The current fuzzy logic controller (error_scale=1200, error_dot_scale=2500, output_scale=170) provides:
- **Good position tracking** without excessive overshoot
- **Effective damping** during ball approach
- **Stable steady-state** performance
- **Responsive control** for dynamic movements
- **Balanced response** between aggressiveness and stability

These parameters have been tested and provide reasonable control response for the ball balancing system. 