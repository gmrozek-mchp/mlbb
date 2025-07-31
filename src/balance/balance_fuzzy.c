// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "balance_fuzzy.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arm_math_types.h"

#include "command/command.h"
#include "platform/platform.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

// Fuzzy logic parameters
#define FUZZY_ERROR_RANGE          (8192)  // Q15 format: ±0.25
#define FUZZY_ERROR_DOT_RANGE      (4096)  // Q15 format: ±0.125
#define FUZZY_OUTPUT_RANGE         (8192)  // Q15 format: ±0.25

// Membership function linguistic variables
#define FUZZY_SETS_COUNT           (5)
#define FUZZY_RULES_COUNT          (25)

// Default scaling factors - tuned for conservative control to eliminate overshoot
#define FUZZY_ERROR_SCALE_DEFAULT      (1200)    // Conservative error scale to eliminate overshoot
#define FUZZY_ERROR_DOT_SCALE_DEFAULT  (2500)   // Increased damping to stop oscillations
#define FUZZY_OUTPUT_SCALE_DEFAULT     (170)    // Reduced to prevent aggressive response


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

// Linguistic variables for fuzzy sets
typedef enum {
    FUZZY_NEGATIVE_LARGE = 0,
    FUZZY_NEGATIVE_SMALL,
    FUZZY_ZERO,
    FUZZY_POSITIVE_SMALL,
    FUZZY_POSITIVE_LARGE
} fuzzy_set_t;

// Triangular membership function
typedef struct {
    q15_t left_peak;
    q15_t center_peak;
    q15_t right_peak;
} fuzzy_membership_t;

// Fuzzy rule structure
typedef struct {
    fuzzy_set_t error_set;
    fuzzy_set_t error_dot_set;
    fuzzy_set_t output_set;
} fuzzy_rule_t;

// Fuzzy controller instance
typedef struct {
    // Scaling factors
    uint16_t error_scale;
    uint16_t error_dot_scale;
    uint16_t output_scale;
    
    // Membership functions for error
    fuzzy_membership_t error_mf[FUZZY_SETS_COUNT];
    
    // Membership functions for error_dot
    fuzzy_membership_t error_dot_mf[FUZZY_SETS_COUNT];
    
    // Membership functions for output
    fuzzy_membership_t output_mf[FUZZY_SETS_COUNT];
    
    // Fuzzy rules
    fuzzy_rule_t rules[FUZZY_RULES_COUNT];
    
    // Previous error for derivative calculation
    q15_t prev_error;
    
    // Error history for filtering
    q15_t error_history[5];
    size_t error_history_index;
} fuzzy_controller_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************

static fuzzy_controller_t fuzzy_x;
static fuzzy_controller_t fuzzy_y;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void BALANCE_FUZZY_Initialize_Instance( fuzzy_controller_t *fuzzy );
static void BALANCE_FUZZY_Reset_Instance( fuzzy_controller_t *fuzzy );
static q15_t BALANCE_FUZZY_Run_Instance( fuzzy_controller_t *fuzzy, q15_t target, q15_t actual );
static q15_t BALANCE_FUZZY_Calculate_Membership( q15_t value, fuzzy_membership_t *mf );
static void BALANCE_FUZZY_Initialize_Membership_Functions( fuzzy_controller_t *fuzzy );
static void BALANCE_FUZZY_Initialize_Rules( fuzzy_controller_t *fuzzy );
static q15_t BALANCE_FUZZY_Defuzzify( q15_t *memberships, q15_t *outputs, size_t count );


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************

static void BALANCE_FUZZY_CMD_Print_State( void );
static void BALANCE_FUZZY_CMD_Print_Scaling( void );
static void BALANCE_FUZZY_CMD_Set_ErrorScale( void );
static void BALANCE_FUZZY_CMD_Set_ErrorDotScale( void );
static void BALANCE_FUZZY_CMD_Set_OutputScale( void );
static void BALANCE_FUZZY_CMD_Reset( void );
static void BALANCE_FUZZY_CMD_Print_Debug( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void BALANCE_FUZZY_Initialize( void )
{
    BALANCE_FUZZY_Initialize_Instance( &fuzzy_x );
    BALANCE_FUZZY_Initialize_Instance( &fuzzy_y );

    CMD_RegisterCommand( "fuzzy", BALANCE_FUZZY_CMD_Print_State );
    CMD_RegisterCommand( "fuzzys", BALANCE_FUZZY_CMD_Print_Scaling );
    CMD_RegisterCommand( "fes", BALANCE_FUZZY_CMD_Set_ErrorScale );
    CMD_RegisterCommand( "feds", BALANCE_FUZZY_CMD_Set_ErrorDotScale );
    CMD_RegisterCommand( "fos", BALANCE_FUZZY_CMD_Set_OutputScale );
    CMD_RegisterCommand( "fuzzyreset", BALANCE_FUZZY_CMD_Reset );
    CMD_RegisterCommand( "fuzzydbg", BALANCE_FUZZY_CMD_Print_Debug );
}

void BALANCE_FUZZY_Reset( void )
{
    BALANCE_FUZZY_Reset_Instance( &fuzzy_x );
    BALANCE_FUZZY_Reset_Instance( &fuzzy_y );
}

void BALANCE_FUZZY_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    if( ball_detected )
    {
        q15_t platform_x = BALANCE_FUZZY_Run_Instance( &fuzzy_x, target_x, ball_x );
        q15_t platform_y = BALANCE_FUZZY_Run_Instance( &fuzzy_y, target_y, ball_y );

        PLATFORM_Position_XY_Set( platform_x, platform_y );
    }
    else
    {
        BALANCE_FUZZY_Reset();
        PLATFORM_Position_XY_Set( 0, 0 );
    }   
}

void BALANCE_FUZZY_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y )
{
    static uint8_t dv_data[22];

    platform_xy_t platform_xy = PLATFORM_Position_XY_Get();
    platform_abc_t platform_abc = PLATFORM_Position_ABC_Get();

    dv_data[0] = 0x03;

    dv_data[1] = (uint8_t)'F';  // 'F' for Fuzzy

    dv_data[2] = (uint8_t)ball_detected;

    dv_data[3] = (uint8_t)target_x;
    dv_data[4] = (uint8_t)(target_x >> 8);
    dv_data[5] = (uint8_t)target_y;
    dv_data[6] = (uint8_t)(target_y >> 8);

    dv_data[7] = (uint8_t)ball_x;
    dv_data[8] = (uint8_t)(ball_x >> 8);
    dv_data[9] = (uint8_t)ball_y;
    dv_data[10] = (uint8_t)(ball_y >> 8);

    dv_data[11] = (uint8_t)platform_xy.x;
    dv_data[12] = (uint8_t)(platform_xy.x >> 8);
    dv_data[13] = (uint8_t)platform_xy.y;
    dv_data[14] = (uint8_t)(platform_xy.y >> 8);

    dv_data[15] = (uint8_t)platform_abc.a;
    dv_data[16] = (uint8_t)(platform_abc.a >> 8);
    dv_data[17] = (uint8_t)platform_abc.b;
    dv_data[18] = (uint8_t)(platform_abc.b >> 8);
    dv_data[19] = (uint8_t)platform_abc.c;
    dv_data[20] = (uint8_t)(platform_abc.c >> 8);

    dv_data[21] = ~0x03;

    CMD_PrintByteArray( dv_data, sizeof(dv_data), false );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void BALANCE_FUZZY_Initialize_Instance( fuzzy_controller_t *fuzzy )
{
    fuzzy->error_scale = FUZZY_ERROR_SCALE_DEFAULT;
    fuzzy->error_dot_scale = FUZZY_ERROR_DOT_SCALE_DEFAULT;
    fuzzy->output_scale = FUZZY_OUTPUT_SCALE_DEFAULT;
    
    BALANCE_FUZZY_Initialize_Membership_Functions( fuzzy );
    BALANCE_FUZZY_Initialize_Rules( fuzzy );
    BALANCE_FUZZY_Reset_Instance( fuzzy );
}

static void BALANCE_FUZZY_Reset_Instance( fuzzy_controller_t *fuzzy )
{
    fuzzy->prev_error = 0;
    fuzzy->error_history_index = 0;
    
    for( size_t i = 0; i < 5; i++ )
    {
        fuzzy->error_history[i] = 0;
    }
}

static void BALANCE_FUZZY_Initialize_Membership_Functions( fuzzy_controller_t *fuzzy )
{
    // Error membership functions (triangular)
    // NL: [-8192, -4096, 0]
    fuzzy->error_mf[FUZZY_NEGATIVE_LARGE].left_peak = -8192;
    fuzzy->error_mf[FUZZY_NEGATIVE_LARGE].center_peak = -4096;
    fuzzy->error_mf[FUZZY_NEGATIVE_LARGE].right_peak = 0;
    
    // NS: [-3584, -2048, -512] - Adjusted for less overlap
    fuzzy->error_mf[FUZZY_NEGATIVE_SMALL].left_peak = -3584;
    fuzzy->error_mf[FUZZY_NEGATIVE_SMALL].center_peak = -2048;
    fuzzy->error_mf[FUZZY_NEGATIVE_SMALL].right_peak = -512;
    
    // ZE: [-1024, 0, 1024] - Reduced overlap for better precision
    fuzzy->error_mf[FUZZY_ZERO].left_peak = -1024;
    fuzzy->error_mf[FUZZY_ZERO].center_peak = 0;
    fuzzy->error_mf[FUZZY_ZERO].right_peak = 1024;
    
    // PS: [512, 2048, 3584] - Adjusted for less overlap
    fuzzy->error_mf[FUZZY_POSITIVE_SMALL].left_peak = 512;
    fuzzy->error_mf[FUZZY_POSITIVE_SMALL].center_peak = 2048;
    fuzzy->error_mf[FUZZY_POSITIVE_SMALL].right_peak = 3584;
    
    // PL: [0, 4096, 8192]
    fuzzy->error_mf[FUZZY_POSITIVE_LARGE].left_peak = 0;
    fuzzy->error_mf[FUZZY_POSITIVE_LARGE].center_peak = 4096;
    fuzzy->error_mf[FUZZY_POSITIVE_LARGE].right_peak = 8192;
    
    // Error_dot membership functions (smaller range)
    // NL: [-4096, -2048, 0]
    fuzzy->error_dot_mf[FUZZY_NEGATIVE_LARGE].left_peak = -4096;
    fuzzy->error_dot_mf[FUZZY_NEGATIVE_LARGE].center_peak = -2048;
    fuzzy->error_dot_mf[FUZZY_NEGATIVE_LARGE].right_peak = 0;
    
    // NS: [-2048, -1024, 0]
    fuzzy->error_dot_mf[FUZZY_NEGATIVE_SMALL].left_peak = -2048;
    fuzzy->error_dot_mf[FUZZY_NEGATIVE_SMALL].center_peak = -1024;
    fuzzy->error_dot_mf[FUZZY_NEGATIVE_SMALL].right_peak = 0;
    
    // ZE: [-128, 0, 128] - Tight zero region for conservative damping
    fuzzy->error_dot_mf[FUZZY_ZERO].left_peak = -128;
    fuzzy->error_dot_mf[FUZZY_ZERO].center_peak = 0;
    fuzzy->error_dot_mf[FUZZY_ZERO].right_peak = 128;
    
    // PS: [0, 1024, 2048]
    fuzzy->error_dot_mf[FUZZY_POSITIVE_SMALL].left_peak = 0;
    fuzzy->error_dot_mf[FUZZY_POSITIVE_SMALL].center_peak = 1024;
    fuzzy->error_dot_mf[FUZZY_POSITIVE_SMALL].right_peak = 2048;
    
    // PL: [0, 2048, 4096]
    fuzzy->error_dot_mf[FUZZY_POSITIVE_LARGE].left_peak = 0;
    fuzzy->error_dot_mf[FUZZY_POSITIVE_LARGE].center_peak = 2048;
    fuzzy->error_dot_mf[FUZZY_POSITIVE_LARGE].right_peak = 4096;
    
    // Output membership functions
    // NL: [-8192, -4096, 0]
    fuzzy->output_mf[FUZZY_NEGATIVE_LARGE].left_peak = -8192;
    fuzzy->output_mf[FUZZY_NEGATIVE_LARGE].center_peak = -4096;
    fuzzy->output_mf[FUZZY_NEGATIVE_LARGE].right_peak = 0;
    
    // NS: [-4096, -2048, 0]
    fuzzy->output_mf[FUZZY_NEGATIVE_SMALL].left_peak = -4096;
    fuzzy->output_mf[FUZZY_NEGATIVE_SMALL].center_peak = -2048;
    fuzzy->output_mf[FUZZY_NEGATIVE_SMALL].right_peak = 0;
    
    // ZE: [-2048, 0, 2048]
    fuzzy->output_mf[FUZZY_ZERO].left_peak = -2048;
    fuzzy->output_mf[FUZZY_ZERO].center_peak = 0;
    fuzzy->output_mf[FUZZY_ZERO].right_peak = 2048;
    
    // PS: [0, 2048, 4096]
    fuzzy->output_mf[FUZZY_POSITIVE_SMALL].left_peak = 0;
    fuzzy->output_mf[FUZZY_POSITIVE_SMALL].center_peak = 2048;
    fuzzy->output_mf[FUZZY_POSITIVE_SMALL].right_peak = 4096;
    
    // PL: [0, 4096, 8192]
    fuzzy->output_mf[FUZZY_POSITIVE_LARGE].left_peak = 0;
    fuzzy->output_mf[FUZZY_POSITIVE_LARGE].center_peak = 4096;
    fuzzy->output_mf[FUZZY_POSITIVE_LARGE].right_peak = 8192;
}

static void BALANCE_FUZZY_Initialize_Rules( fuzzy_controller_t *fuzzy )
{
    // Initialize fuzzy rules table
    // Rule format: IF error IS X AND error_dot IS Y THEN output IS Z
    
    // Row 1: error = NL - Conservative control to eliminate oscillations
    fuzzy->rules[0] = (fuzzy_rule_t){FUZZY_NEGATIVE_LARGE, FUZZY_NEGATIVE_LARGE, FUZZY_NEGATIVE_LARGE};
    fuzzy->rules[1] = (fuzzy_rule_t){FUZZY_NEGATIVE_LARGE, FUZZY_NEGATIVE_SMALL, FUZZY_NEGATIVE_LARGE};
    fuzzy->rules[2] = (fuzzy_rule_t){FUZZY_NEGATIVE_LARGE, FUZZY_ZERO, FUZZY_NEGATIVE_SMALL};
    fuzzy->rules[3] = (fuzzy_rule_t){FUZZY_NEGATIVE_LARGE, FUZZY_POSITIVE_SMALL, FUZZY_ZERO};
    fuzzy->rules[4] = (fuzzy_rule_t){FUZZY_NEGATIVE_LARGE, FUZZY_POSITIVE_LARGE, FUZZY_ZERO};
    
    // Row 2: error = NS - Conservative control to eliminate oscillations
    fuzzy->rules[5] = (fuzzy_rule_t){FUZZY_NEGATIVE_SMALL, FUZZY_NEGATIVE_LARGE, FUZZY_NEGATIVE_SMALL};
    fuzzy->rules[6] = (fuzzy_rule_t){FUZZY_NEGATIVE_SMALL, FUZZY_NEGATIVE_SMALL, FUZZY_NEGATIVE_SMALL};
    fuzzy->rules[7] = (fuzzy_rule_t){FUZZY_NEGATIVE_SMALL, FUZZY_ZERO, FUZZY_ZERO};
    fuzzy->rules[8] = (fuzzy_rule_t){FUZZY_NEGATIVE_SMALL, FUZZY_POSITIVE_SMALL, FUZZY_ZERO};
    fuzzy->rules[9] = (fuzzy_rule_t){FUZZY_NEGATIVE_SMALL, FUZZY_POSITIVE_LARGE, FUZZY_ZERO};
    
    // Row 3: error = ZE - Conservative control to eliminate oscillations
    fuzzy->rules[10] = (fuzzy_rule_t){FUZZY_ZERO, FUZZY_NEGATIVE_LARGE, FUZZY_NEGATIVE_SMALL};
    fuzzy->rules[11] = (fuzzy_rule_t){FUZZY_ZERO, FUZZY_NEGATIVE_SMALL, FUZZY_ZERO};
    fuzzy->rules[12] = (fuzzy_rule_t){FUZZY_ZERO, FUZZY_ZERO, FUZZY_ZERO};
    fuzzy->rules[13] = (fuzzy_rule_t){FUZZY_ZERO, FUZZY_POSITIVE_SMALL, FUZZY_ZERO};
    fuzzy->rules[14] = (fuzzy_rule_t){FUZZY_ZERO, FUZZY_POSITIVE_LARGE, FUZZY_POSITIVE_SMALL};
    
    // Row 4: error = PS - Conservative control to eliminate oscillations
    fuzzy->rules[15] = (fuzzy_rule_t){FUZZY_POSITIVE_SMALL, FUZZY_NEGATIVE_LARGE, FUZZY_ZERO};
    fuzzy->rules[16] = (fuzzy_rule_t){FUZZY_POSITIVE_SMALL, FUZZY_NEGATIVE_SMALL, FUZZY_ZERO};
    fuzzy->rules[17] = (fuzzy_rule_t){FUZZY_POSITIVE_SMALL, FUZZY_ZERO, FUZZY_POSITIVE_SMALL};
    fuzzy->rules[18] = (fuzzy_rule_t){FUZZY_POSITIVE_SMALL, FUZZY_POSITIVE_SMALL, FUZZY_POSITIVE_SMALL};
    fuzzy->rules[19] = (fuzzy_rule_t){FUZZY_POSITIVE_SMALL, FUZZY_POSITIVE_LARGE, FUZZY_POSITIVE_SMALL};
    
    // Row 5: error = PL - Conservative control to eliminate oscillations
    fuzzy->rules[20] = (fuzzy_rule_t){FUZZY_POSITIVE_LARGE, FUZZY_NEGATIVE_LARGE, FUZZY_ZERO};
    fuzzy->rules[21] = (fuzzy_rule_t){FUZZY_POSITIVE_LARGE, FUZZY_NEGATIVE_SMALL, FUZZY_ZERO};
    fuzzy->rules[22] = (fuzzy_rule_t){FUZZY_POSITIVE_LARGE, FUZZY_ZERO, FUZZY_POSITIVE_SMALL};
    fuzzy->rules[23] = (fuzzy_rule_t){FUZZY_POSITIVE_LARGE, FUZZY_POSITIVE_SMALL, FUZZY_POSITIVE_LARGE};
    fuzzy->rules[24] = (fuzzy_rule_t){FUZZY_POSITIVE_LARGE, FUZZY_POSITIVE_LARGE, FUZZY_POSITIVE_LARGE};
}

static q15_t BALANCE_FUZZY_Run_Instance( fuzzy_controller_t *fuzzy, q15_t target, q15_t actual )
{
    q15_t error, error_dot;
    q15_t error_memberships[FUZZY_SETS_COUNT];
    q15_t error_dot_memberships[FUZZY_SETS_COUNT];
    q15_t rule_outputs[FUZZY_RULES_COUNT];
    q15_t rule_strengths[FUZZY_RULES_COUNT];
    
    // Calculate error
    error = target - actual;
    
    // Apply scaling to error
    error = (q15_t)((q31_t)error * fuzzy->error_scale / 256);
    
    // Calculate error derivative (filtered)
    q15_t error_delta = error - fuzzy->prev_error;
    fuzzy->error_history[fuzzy->error_history_index] = error_delta;
    fuzzy->error_history_index = (fuzzy->error_history_index + 1) % 5;
    
    // Calculate filtered error_dot
    q31_t error_dot_sum = 0;
    for( size_t i = 0; i < 5; i++ )
    {
        error_dot_sum += fuzzy->error_history[i];
    }
    error_dot = (q15_t)(error_dot_sum / 5);
    
    // Apply scaling to error_dot
    error_dot = (q15_t)((q31_t)error_dot * fuzzy->error_dot_scale / 256);
    
    // Calculate membership degrees for error
    for( size_t i = 0; i < FUZZY_SETS_COUNT; i++ )
    {
        error_memberships[i] = BALANCE_FUZZY_Calculate_Membership( error, &fuzzy->error_mf[i] );
    }
    
    // Calculate membership degrees for error_dot
    for( size_t i = 0; i < FUZZY_SETS_COUNT; i++ )
    {
        error_dot_memberships[i] = BALANCE_FUZZY_Calculate_Membership( error_dot, &fuzzy->error_dot_mf[i] );
    }
    
    // Apply fuzzy rules and calculate rule outputs
    for( size_t i = 0; i < FUZZY_RULES_COUNT; i++ )
    {
        fuzzy_rule_t *rule = &fuzzy->rules[i];
        
        // Calculate rule strength using MIN operator
        q15_t strength1 = error_memberships[rule->error_set];
        q15_t strength2 = error_dot_memberships[rule->error_dot_set];
        rule_strengths[i] = (strength1 < strength2) ? strength1 : strength2;
        
        // Rule output is the center of the output membership function
        rule_outputs[i] = fuzzy->output_mf[rule->output_set].center_peak;
    }
    
    // Defuzzify using center of gravity method
    q15_t output = BALANCE_FUZZY_Defuzzify( rule_strengths, rule_outputs, FUZZY_RULES_COUNT );
    
    // Apply output scaling
    output = (q15_t)((q31_t)output * fuzzy->output_scale / 256);
    
    // Clamp output
    if( output > 16383 )
    {
        output = 16383;
    }
    else if( output < -16384 )
    {
        output = -16384;
    }
    
    fuzzy->prev_error = error;
    
    return output;
}

static q15_t BALANCE_FUZZY_Calculate_Membership( q15_t value, fuzzy_membership_t *mf )
{
    q15_t membership = 0;
    
    // Check if value is within the triangular membership function
    if( value >= mf->left_peak && value <= mf->right_peak )
    {
        if( value <= mf->center_peak )
        {
            // Rising edge of triangle
            if( mf->center_peak != mf->left_peak )
            {
                membership = (q15_t)((q31_t)(value - mf->left_peak) * 32767 / (mf->center_peak - mf->left_peak));
            }
        }
        else
        {
            // Falling edge of triangle
            if( mf->right_peak != mf->center_peak )
            {
                membership = (q15_t)((q31_t)(mf->right_peak - value) * 32767 / (mf->right_peak - mf->center_peak));
            }
        }
    }
    
    return membership;
}

static q15_t BALANCE_FUZZY_Defuzzify( q15_t *memberships, q15_t *outputs, size_t count )
{
    q31_t numerator = 0;
    q31_t denominator = 0;
    
    for( size_t i = 0; i < count; i++ )
    {
        numerator += (q31_t)memberships[i] * outputs[i];
        denominator += memberships[i];
    }
    
    if( denominator == 0 )
    {
        return 0;
    }
    
    return (q15_t)(numerator / denominator);
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void BALANCE_FUZZY_CMD_Print_State( void )
{
    q15_t ex = fuzzy_x.prev_error;
    q15_t ey = fuzzy_y.prev_error;
    platform_xy_t platform_xy = PLATFORM_Position_XY_Get();

    CMD_PrintString( "ex: ", true );
    CMD_PrintHex_U16( (uint16_t)ex, true );
    CMD_PrintString( " ey: ", true );
    CMD_PrintHex_U16( (uint16_t)ey, true );
    CMD_PrintString( " px: ", true );
    CMD_PrintHex_U16( (uint16_t)platform_xy.x, true );
    CMD_PrintString( " py: ", true );
    CMD_PrintHex_U16( (uint16_t)platform_xy.y, true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_FUZZY_CMD_Print_Scaling( void )
{
    CMD_PrintString( "Error Scale: 0x", true );
    CMD_PrintHex_U16( (uint16_t)fuzzy_x.error_scale, true );
    CMD_PrintString( " Error Dot Scale: 0x", true );
    CMD_PrintHex_U16( (uint16_t)fuzzy_x.error_dot_scale, true );
    CMD_PrintString( " Output Scale: 0x", true );
    CMD_PrintHex_U16( (uint16_t)fuzzy_x.output_scale, true );
    CMD_PrintString( "\r\n", true );
}

static void BALANCE_FUZZY_CMD_Set_ErrorScale( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        uint16_t error_scale;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        error_scale = (uint16_t)atoi( arg_buffer );

        fuzzy_x.error_scale = error_scale;
        fuzzy_y.error_scale = error_scale;
    }

    BALANCE_FUZZY_CMD_Print_Scaling();
}

static void BALANCE_FUZZY_CMD_Set_ErrorDotScale( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        uint16_t error_dot_scale;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        error_dot_scale = (uint16_t)atoi( arg_buffer );

        fuzzy_x.error_dot_scale = error_dot_scale;
        fuzzy_y.error_dot_scale = error_dot_scale;
    }

    BALANCE_FUZZY_CMD_Print_Scaling();
}

static void BALANCE_FUZZY_CMD_Set_OutputScale( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        uint16_t output_scale;
        
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        output_scale = (uint16_t)atoi( arg_buffer );

        fuzzy_x.output_scale = output_scale;
        fuzzy_y.output_scale = output_scale;
    }

    BALANCE_FUZZY_CMD_Print_Scaling();
} 

static void BALANCE_FUZZY_CMD_Reset( void )
{
    BALANCE_FUZZY_Reset();
    CMD_PrintString( "Fuzzy controller reset\r\n", true );
} 

static void BALANCE_FUZZY_CMD_Print_Debug( void )
{
    // Calculate current error and error_dot for X axis
    q15_t error_x = fuzzy_x.prev_error;
    q15_t error_dot_x = 0;
    for( size_t i = 0; i < 5; i++ )
    {
        error_dot_x += fuzzy_x.error_history[i];
    }
    error_dot_x = (q15_t)(error_dot_x / 5);
    
    // Calculate membership values for X axis
    q15_t error_memberships[FUZZY_SETS_COUNT];
    q15_t error_dot_memberships[FUZZY_SETS_COUNT];
    
    for( size_t i = 0; i < FUZZY_SETS_COUNT; i++ )
    {
        error_memberships[i] = BALANCE_FUZZY_Calculate_Membership( error_x, &fuzzy_x.error_mf[i] );
        error_dot_memberships[i] = BALANCE_FUZZY_Calculate_Membership( error_dot_x, &fuzzy_x.error_dot_mf[i] );
    }
    
    CMD_PrintString( "Error: ", true );
    CMD_PrintHex_U16( (uint16_t)error_x, true );
    CMD_PrintString( " Error_dot: ", true );
    CMD_PrintHex_U16( (uint16_t)error_dot_x, true );
    CMD_PrintString( "\r\n", true );
    
    CMD_PrintString( "Error memberships: ", true );
    for( size_t i = 0; i < FUZZY_SETS_COUNT; i++ )
    {
        CMD_PrintHex_U16( (uint16_t)error_memberships[i], true );
        CMD_PrintString( " ", true );
    }
    CMD_PrintString( "\r\n", true );
    
    CMD_PrintString( "Error_dot memberships: ", true );
    for( size_t i = 0; i < FUZZY_SETS_COUNT; i++ )
    {
        CMD_PrintHex_U16( (uint16_t)error_dot_memberships[i], true );
        CMD_PrintString( " ", true );
    }
    CMD_PrintString( "\r\n", true );
} 