# Fuzzy Logic Control System for Ball Balancing

## System Overview

The fuzzy logic controller in `balance_fuzzy.c` implements a sophisticated control system for balancing a ball on a platform using fuzzy logic principles. The system uses two inputs (error and error_dot) and produces one output (platform position) for each axis (X and Y).

## 1. Membership Functions

### Error Membership Functions (Triangular)
```
Membership Degree
   1.0 |     NL     NS     ZE     PS     PL
       |    /\      /\      /\      /\      /\
       |   /  \    /  \    /  \    /  \    /  \
       |  /    \  /    \  /    \  /    \  /    \
       | /      \/      \/      \/      \/      \
   0.0 |-----------------------------------------------
       -8192  -4096  -2048    0   2048  4096  8192
                    Error Value (Q15)
```

**Linguistic Variables:**
- **NL (Negative Large)**: [-8192, -4096, 0]
- **NS (Negative Small)**: [-3584, -2048, -512]
- **ZE (Zero)**: [-1024, 0, 1024]
- **PS (Positive Small)**: [512, 2048, 3584]
- **PL (Positive Large)**: [0, 4096, 8192]

### Error_dot Membership Functions (Smaller Range)
```
Membership Degree
   1.0 |     NL     NS     ZE     PS     PL
       |    /\      /\      /\      /\      /\
       |   /  \    /  \    /  \    /  \    /  \
       |  /    \  /    \  /    \  /    \  /    \
       | /      \/      \/      \/      \/      \
   0.0 |-----------------------------------------------
       -4096  -2048  -1024    0   1024  2048  4096
                  Error_dot Value (Q15)
```

**Linguistic Variables:**
- **NL (Negative Large)**: [-4096, -2048, 0]
- **NS (Negative Small)**: [-2048, -1024, 0]
- **ZE (Zero)**: [-128, 0, 128] (Tight zero region)
- **PS (Positive Small)**: [0, 1024, 2048]
- **PL (Positive Large)**: [0, 2048, 4096]

### Output Membership Functions
```
Membership Degree
   1.0 |     NL     NS     ZE     PS     PL
       |    /\      /\      /\      /\      /\
       |   /  \    /  \    /  \    /  \    /  \
       |  /    \  /    \  /    \  /    \  /    \
       | /      \/      \/      \/      \/      \
   0.0 |-----------------------------------------------
       -8192  -4096  -2048    0   2048  4096  8192
                    Output Value (Q15)
```

## 2. Fuzzy Rule Base (5x5 Matrix)

The rule base implements conservative control to eliminate oscillations:

| Error \ Error_dot | NL | NS | ZE | PS | PL |
|-------------------|----|----|----|----|----|
| **NL** | NL | NL | NS | ZE | ZE |
| **NS** | NS | NS | ZE | ZE | ZE |
| **ZE** | NS | ZE | ZE | ZE | PS |
| **PS** | ZE | ZE | PS | PS | PS |
| **PL** | ZE | ZE | PS | PL | PL |

**Rule Interpretation:**
- **Conservative Control**: Rules favor smaller outputs to prevent overshoot
- **Damping**: Large error_dot values reduce output magnitude
- **Smooth Transitions**: Gradual changes between linguistic variables

## 3. Control Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    Ball Position Input                     │
│  (target_x, target_y, ball_x, ball_y)                    │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Error Calculation                       │
│  error = target - actual                                  │
│  error_dot = filtered_derivative(error)                   │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Input Scaling                           │
│  error *= error_scale / 256                               │
│  error_dot *= error_dot_scale / 256                       │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                Membership Function Calculation              │
│  For each linguistic variable:                            │
│  - Calculate triangular membership degree                 │
│  - Apply MIN operator for rule strength                   │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Rule Evaluation                         │
│  For each rule (25 total):                                │
│  - IF error IS X AND error_dot IS Y THEN output IS Z     │
│  - Rule strength = MIN(error_mf, error_dot_mf)           │
│  - Rule output = center of output membership function     │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Defuzzification                         │
│  Center of Gravity method:                                │
│  output = Σ(rule_strength × rule_output) / Σ(rule_strength) │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Output Processing                       │
│  - Apply adaptive output scaling                          │
│  - Rate limiting (max_delta = 1536)                      │
│  - Anti-overshoot logic                                   │
│  - Clamp to Q15 range [-16384, 16383]                    │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    Platform Control                        │
│  PLATFORM_Position_XY_Set(platform_x, platform_y)        │
└─────────────────────────────────────────────────────────────┘
```

## 4. Key Features

### Adaptive Scaling
- **Error Scale**: 600 (default) - Controls responsiveness to position errors
- **Error_dot Scale**: 3500 (default) - Controls damping to stop oscillations  
- **Output Scale**: 120 (default) - Controls overall output magnitude

### Anti-Overshoot Mechanisms
1. **Adaptive Output Scaling**: Reduces output for large errors
2. **Rate Limiting**: Maximum change of 1536 per cycle
3. **Error Change Detection**: Reduces output when approaching target
4. **Conservative Rule Base**: Favors smaller outputs

### Error Filtering
- **5-point weighted average** for error_dot calculation
- **More weight to recent values** (weights: 5,4,3,2,1)
- **Prevents noise amplification** in derivative calculation

## 5. Implementation Details

### Data Structures
```c
typedef struct {
    q15_t left_peak;
    q15_t center_peak; 
    q15_t right_peak;
} fuzzy_membership_t;

typedef struct {
    fuzzy_set_t error_set;
    fuzzy_set_t error_dot_set;
    fuzzy_set_t output_set;
} fuzzy_rule_t;

typedef struct {
    uint16_t error_scale;
    uint16_t error_dot_scale;
    uint16_t output_scale;
    fuzzy_membership_t error_mf[5];
    fuzzy_membership_t error_dot_mf[5];
    fuzzy_membership_t output_mf[5];
    fuzzy_rule_t rules[25];
    q15_t prev_error;
    q15_t error_history[5];
    size_t error_history_index;
} fuzzy_controller_t;
```

### Key Functions
- `BALANCE_FUZZY_Run_Instance()`: Main control loop
- `BALANCE_FUZZY_Calculate_Membership()`: Triangular membership calculation
- `BALANCE_FUZZY_Defuzzify()`: Center of gravity defuzzification
- `BALANCE_FUZZY_Initialize_Membership_Functions()`: Setup membership functions
- `BALANCE_FUZZY_Initialize_Rules()`: Setup rule base

## 6. Performance Characteristics

### Conservative Design
- **Eliminates oscillations** through conservative rule base
- **Prevents overshoot** with multiple anti-overshoot mechanisms
- **Smooth control** with rate limiting and filtering

### Real-time Operation
- **Fixed-point arithmetic** (Q15 format) for efficiency
- **Deterministic execution** time
- **Memory efficient** with static allocation

### Tunable Parameters
- **Scaling factors** adjustable via command interface
- **Membership functions** can be modified for different behaviors
- **Rule base** can be optimized for specific applications

This fuzzy logic implementation provides robust, oscillation-free control for the ball balancing system while maintaining real-time performance and tunability. 