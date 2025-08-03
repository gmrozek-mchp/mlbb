# Fuzzy Logic Visual Diagram

## Membership Functions Visualization

### Error Membership Functions
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

### Error_dot Membership Functions
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

## Fuzzy Rule Base Matrix

```
Error_dot →
Error ↓    NL    NS    ZE    PS    PL
NL        NL    NL    NS    ZE    ZE
NS        NS    NS    ZE    ZE    ZE  
ZE        NS    ZE    ZE    ZE    PS
PS        ZE    ZE    PS    PS    PS
PL        ZE    ZE    PS    PL    PL
```

## Control Flow

```
Input: (target, actual)
    ↓
Calculate: error = target - actual
    ↓
Calculate: error_dot = filtered_derivative(error)
    ↓
Scale inputs with adaptive factors
    ↓
Calculate membership degrees for error and error_dot
    ↓
Evaluate 25 fuzzy rules (MIN operator)
    ↓
Defuzzify using Center of Gravity
    ↓
Apply anti-overshoot mechanisms
    ↓
Output: platform position
```

## Key Features

- **Conservative Control**: Prevents oscillations
- **Anti-overshoot**: Multiple mechanisms
- **Adaptive Scaling**: Dynamic response adjustment
- **Rate Limiting**: Smooth control changes 