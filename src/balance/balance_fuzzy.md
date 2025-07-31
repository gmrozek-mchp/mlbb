# Fuzzy Logic Balance Module

## Overview

The fuzzy logic balance module (`balance_fuzzy.c` and `balance_fuzzy.h`) implements a Mamdani-style fuzzy logic controller for the ball balancing system. This module provides an alternative to the PID controller with potentially better handling of non-linear dynamics and uncertainty.

## Architecture

### Fuzzy Sets
The controller uses 5 linguistic variables for each input and output:
- **NL** (Negative Large)
- **NS** (Negative Small) 
- **ZE** (Zero)
- **PS** (Positive Small)
- **PL** (Positive Large)

### Input Variables
1. **Error** (e): The difference between target and actual ball position
2. **Error Dot** (ė): The rate of change of error (filtered derivative)

### Output Variable
- **Control Output**: Platform position command

### Membership Functions
Triangular membership functions are used for all variables:
- Error range: ±8192 (Q15 format: ±0.25)
- Error_dot range: ±4096 (Q15 format: ±0.125)  
- Output range: ±8192 (Q15 format: ±0.25)

### Fuzzy Rules
A 5×5 rule matrix (25 rules) implements the control logic:

| Error\Error_dot | NL | NS | ZE | PS | PL |
|----------------|----|----|----|----|----|
| **NL** | NL | NL | NS | ZE | PS |
| **NS** | NL | NS | NS | ZE | PS |
| **ZE** | NS | NS | ZE | PS | PS |
| **PS** | NS | ZE | PS | PS | PL |
| **PL** | NS | ZE | PS | PL | PL |

### Inference Method
- **Fuzzification**: Triangular membership functions
- **Rule Evaluation**: MIN operator for AND conditions
- **Defuzzification**: Center of Gravity method

## Configuration

### Scaling Factors
- `error_scale`: Scales the error input (default: 256)
- `error_dot_scale`: Scales the error derivative input (default: 128)
- `output_scale`: Scales the output (default: 256)

### Command Interface
- `fuzzy`: Print current state (error, platform position)
- `fuzzys`: Print current scaling factors
- `fes <value>`: Set error scale factor
- `feds <value>`: Set error dot scale factor  
- `fos <value>`: Set output scale factor

## Usage

The fuzzy logic controller is integrated into the balance system and can be selected by cycling through balance modes using the nunchuk C button:

1. **OFF** → **PID** → **NN** → **FUZZY** → **OFF**

The controller automatically handles:
- Ball detection debouncing
- Error filtering and derivative calculation
- Output clamping to prevent saturation
- Platform position control

## Advantages over PID

1. **Non-linear handling**: Better performance with non-linear system dynamics
2. **Robustness**: Less sensitive to parameter variations
3. **Intuitive tuning**: Linguistic rules are easier to understand and modify
4. **Multi-objective control**: Can incorporate multiple control objectives

## Tuning Guidelines

1. **Start with default scaling factors**
2. **Adjust error_scale** to control responsiveness to position errors
3. **Adjust error_dot_scale** to control damping behavior
4. **Adjust output_scale** to control overall control effort
5. **Monitor system response** and fine-tune as needed

## Performance Considerations

- **Computational overhead**: Higher than PID due to rule evaluation
- **Memory usage**: ~2KB for membership functions and rules
- **Real-time performance**: Designed for 100Hz control loop
- **Fixed-point arithmetic**: All calculations use Q15 format for efficiency 