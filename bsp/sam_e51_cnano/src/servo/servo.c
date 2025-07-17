// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "servo.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "peripheral/port/plib_port.h"
#include "peripheral/tc/plib_tc1.h"
#include "peripheral/tc/plib_tc4.h"

#include "command/command.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define SERVO_RTOS_PRIORITY       (3)
#define SERVO_RTOS_STACK_SIZE     (configMINIMAL_STACK_SIZE)

#define SERVO_POWER_UP_DELAY_mS   (100)

#define SERVO_MOTOR_STEPS_PER_REVOLUTION  (200)
#define SERVO_DRIVE_MICROSTEPS            (16)
#define SERVO_DRIVE_STEPS_PER_REVOLUTION  (SERVO_MOTOR_STEPS_PER_REVOLUTION * SERVO_DRIVE_MICROSTEPS)
#define SERVO_DRIVE_ANGLE_PER_STEP_Q15    (INT16_MAX / SERVO_DRIVE_STEPS_PER_REVOLUTION)
#define SERVO_DRIVE_SPEED_MAX             (1)

#define SERVO_STEP_COMPARE_VALUE    (10)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************
typedef struct {
    q15_t command_angle;
    q15_t limit_angle_min;
    q15_t limit_angle_max;
    int16_t command_steps_buffer;
    int16_t command_steps;
    int16_t position_steps;
    int16_t velocity;
    int16_t acceleration_delay;
} servo_state_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************
static TaskHandle_t servo_taskHandle = NULL;
static StaticTask_t servo_taskBuffer;
static StackType_t  servo_taskStack[ SERVO_RTOS_STACK_SIZE ];

static servo_state_t servos[SERVO_ID_NUM_SERVOS];

// All pins of PCA9538A on Stepper 19 Click set to outputs
static const uint8_t servo_stepper19_cmd_config[] = { 0x03, 0x00 };
// P0 -> M0 
// P1 -> M1 --> M1:M0 = 0b11 = 1/8 microstepping
// P2 -> DEC0
// P3 -> DEC1 --> DEC1:DEC0 = 0b00 = Smart Tune Dynamic Decay
// P4 -> TOFF --> TOFF = 0b1 = 16uS
// P5 -> STEP (Not Connected)
// P6 -> DIR (Not Connected)
// P7 -> N/C
static const uint8_t servo_stepper19_cmd_output[] = { 0x01, 0x13 };


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void SERVO_RTOS_Task( void * pvParameters );

static void SERVO_TC1_CompareCallback( TC_COMPARE_STATUS status, uintptr_t context );
static void SERVO_TC4_CompareCallback( TC_COMPARE_STATUS status, uintptr_t context );

static void SERVO_Drive( servo_id_t servo_id );
static void SERVO_Drive_StepAndDirection( servo_id_t servo_id, bool step, bool direction );


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************
static void SERVO_CMD_Position_GetSet_q15angle( void );
static void SERVO_CMD_Position_GetSet_steps( void );
static void SERVO_CMD_Position_Zero( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void SERVO_Initialize( void )
{
    servo_id_t index;
    
    for( index = 0; index < SERVO_ID_NUM_SERVOS; index++ )
    {
        servos[index].command_angle = 0;
        servos[index].limit_angle_min = INT16_MIN;
        servos[index].limit_angle_max = INT16_MAX;
        servos[index].command_steps_buffer = 0;
        servos[index].command_steps = 0;
        servos[index].position_steps = 0;
        servos[index].velocity = 0;
        servos[index].acceleration_delay = 0;
    }

    CMD_RegisterCommand( "servo", SERVO_CMD_Position_GetSet_q15angle );
    CMD_RegisterCommand( "servo-steps", SERVO_CMD_Position_GetSet_steps );
    CMD_RegisterCommand( "servo-zero", SERVO_CMD_Position_Zero );

    servo_taskHandle = xTaskCreateStatic(
        SERVO_RTOS_Task,        /* Function that implements the task. */
        "Servo",                /* Text name for the task. */
        SERVO_RTOS_STACK_SIZE,  /* Number of indexes in the stack. */
        NULL,                     /* Parameter passed into the task. */
        SERVO_RTOS_PRIORITY,    /* Priority at which the task is created. */
        servo_taskStack,         /* Array to use as the task's stack. */
        &servo_taskBuffer        /* Variable to hold the task's data structure. */
    );
}

void SERVO_Disable( void )
{
    STEPPER_nENABLE_Set();
}

void SERVO_Enable( void )
{
    STEPPER_nENABLE_Clear();
}

q15_t SERVO_Position_Get_q15angle( servo_id_t servo_id )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return 0;
    }

    q31_t angle;
    
    angle = ((q31_t)servos[servo_id].position_steps * 0x8000) / SERVO_DRIVE_STEPS_PER_REVOLUTION;
    
    // check if angle is within rounding error of command.
    if( (angle + (SERVO_DRIVE_ANGLE_PER_STEP_Q15 + 1) > servos[servo_id].command_angle) &&
        (angle - (SERVO_DRIVE_ANGLE_PER_STEP_Q15 + 1) < servos[servo_id].command_angle) )
    {
        angle = servos[servo_id].command_angle;
    }
    
    return (q15_t)angle;
}

q15_t SERVO_Position_Command_Get_q15angle( servo_id_t servo_id )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return 0;
    }

    return servos[servo_id].command_angle;
}

void SERVO_Position_Command_Set_q15angle( servo_id_t servo_id, q15_t angle )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return;
    }

    if( angle >= servos[servo_id].limit_angle_max )
    {
        angle = servos[servo_id].limit_angle_max;
    }
    else if( angle <= servos[servo_id].limit_angle_min )
    {
        angle = servos[servo_id].limit_angle_min;
    }
    
    servos[servo_id].command_angle = angle;
    servos[servo_id].command_steps_buffer = (q15_t)(((q31_t)angle * SERVO_DRIVE_STEPS_PER_REVOLUTION) >> 15);
}

void SERVO_Position_Zero_Set( servo_id_t servo_id )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return;
    }
    
    servos[servo_id].position_steps = 0;
    servos[servo_id].command_angle = 0;
    servos[servo_id].command_steps_buffer = 0;
}


int16_t SERVO_Position_Get_steps( servo_id_t servo_id )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return 0;
    }

    return servos[servo_id].position_steps;
}

int16_t SERVO_Position_Command_Get_steps( servo_id_t servo_id )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return 0;
    }

    return servos[servo_id].command_steps;
}

void SERVO_Position_Command_Set_steps( servo_id_t servo_id, int16_t steps )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return;
    }

    servos[servo_id].command_steps_buffer = steps;    
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void SERVO_RTOS_Task( void * pvParameters )
{
    (void)pvParameters;
    
    vTaskDelay( pdMS_TO_TICKS(SERVO_POWER_UP_DELAY_mS) );

    TC1_CompareCallbackRegister( SERVO_TC1_CompareCallback, (uintptr_t)NULL );
    TC1_CompareStart();

    TC4_CompareCallbackRegister( SERVO_TC4_CompareCallback, (uintptr_t)NULL );
    TC4_CompareStart();
    
    // Servo module is completely interrupt driven. Don't need the task running any more.
    vTaskSuspend( NULL );
}

static void SERVO_TC1_CompareCallback( TC_COMPARE_STATUS status, uintptr_t context )
{
    (void)status;
    (void)context;

    SERVO_Drive( SERVO_ID_A );
    SERVO_Drive( SERVO_ID_B );
}

static void SERVO_TC4_CompareCallback( TC_COMPARE_STATUS status, uintptr_t context )
{
    (void)status;
    (void)context;
    
    SERVO_Drive( SERVO_ID_C );    
}


static void SERVO_Drive( servo_id_t servo_id )
{
    // Private function, no range check of parameters
    
    servo_state_t* servo = &servos[servo_id];

    servo->command_steps = servo->command_steps_buffer;

    if( servo->acceleration_delay > 0 )
    {
        // Waiting for acceleration counter to expire. NO STEP
        servo->acceleration_delay--;
        SERVO_Drive_StepAndDirection( servo_id, false, false );
    }
    else
    {
        q31_t error = servo->command_steps - servo->position_steps;

        if( error > 0 )
        {
            if( servo->velocity > error )
            {
                servo->velocity--;
            }
            else if( servo->velocity < error )
            {
                servo->velocity++;
            }
        }
        else if( error < 0 )
        {
            if( servo->velocity < error )
            {
                servo->velocity++;
            }
            else if( servo->velocity > error )
            {
                servo->velocity--;
            }
        }
        else
        {
            if( servo->velocity > 0 )
            {
                servo->velocity--;
            }
            else if( servo->velocity < 0 )
            {
                servo->velocity++;                
            }
        }

        // Clamp to maximum speed
        if( servo->velocity > SERVO_DRIVE_SPEED_MAX )
        {
            servo->velocity = SERVO_DRIVE_SPEED_MAX;
        }
        else if( servo->velocity < -SERVO_DRIVE_SPEED_MAX )
        {
            servo->velocity = -SERVO_DRIVE_SPEED_MAX;        
        }

        if( servo->velocity > 0 )
        {
            servo->position_steps++;
            servo->acceleration_delay = SERVO_DRIVE_SPEED_MAX - servo->velocity;
        }
        else if( servo->velocity < 0 )
        {
            servo->position_steps--;
            servo->acceleration_delay = SERVO_DRIVE_SPEED_MAX + servo->velocity;
        }
        else
        {
            servo->acceleration_delay = 0;
        }

        SERVO_Drive_StepAndDirection( servo_id, (servo->velocity != 0), (servo->velocity >= 0) );
    }
}

static void SERVO_Drive_StepAndDirection( servo_id_t servo_id, bool step, bool direction )
{
    switch( servo_id )
    {
        case SERVO_ID_A:
        {
            if( step )
            {
                if( direction )
                {
                    STEPPER1_DIRECTION_Set();
                }
                else
                {
                    STEPPER1_DIRECTION_Clear();
                }
                TC1_Compare8bitMatch0Set( SERVO_STEP_COMPARE_VALUE );
            }
            else
            {
                TC1_Compare8bitMatch0Set( 0 );                
            }
            break;
        }
        case SERVO_ID_B:
        {
            if( step )
            {
                if( direction )
                {
                    STEPPER2_DIRECTION_Set();
                }
                else
                {
                    STEPPER2_DIRECTION_Clear();
                }
                TC1_Compare8bitMatch1Set( SERVO_STEP_COMPARE_VALUE );
            }
            else
            {
                TC1_Compare8bitMatch1Set( 0 );                
            }
            break;
        }
        case SERVO_ID_C:
        {
            if( step )
            {
                if( direction )
                {
                    STEPPER3_DIRECTION_Set();
                }
                else
                {
                    STEPPER3_DIRECTION_Clear();
                }
                TC4_Compare8bitMatch1Set( SERVO_STEP_COMPARE_VALUE );
            }
            else
            {
                TC4_Compare8bitMatch1Set( 0 );                
            }
            break;
        }
        default:
        {
            break;
        }
    }
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void SERVO_CMD_Position_GetSet_q15angle( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        servo_id_t servo_id;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        servo_id = (servo_id_t)atoi( arg_buffer );
        
        if (CMD_GetArgc() == 2 )
        {
            CMD_PrintString( "ANGLE: 0x", true );
            CMD_PrintHex_U16( SERVO_Position_Get_q15angle((servo_id_t)atoi( arg_buffer )), true );
            CMD_PrintString( "\r\n", true );
        }
        else
        {
            q15_t angle;
            
            CMD_GetArgv( 2, arg_buffer, sizeof(arg_buffer) );
            angle = (q15_t)atoi( arg_buffer );

            SERVO_Position_Command_Set_q15angle( servo_id, angle );
        }
    }
}

static void SERVO_CMD_Position_GetSet_steps( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        servo_id_t servo_id;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        servo_id = (servo_id_t)atoi( arg_buffer );
        
        if (CMD_GetArgc() == 2 )
        {
            CMD_PrintString( "STEPS: 0x", true );
            CMD_PrintHex_U16( SERVO_Position_Get_steps(servo_id), true );
            CMD_PrintString( "\r\n", true );
        }
        else
        {
            int16_t steps;
            
            CMD_GetArgv( 2, arg_buffer, sizeof(arg_buffer) );
            steps = (int16_t)atoi( arg_buffer );

            SERVO_Position_Command_Set_steps( servo_id, steps);
        }
    }
}

static void SERVO_CMD_Position_Zero( void )
{
    char arg_buffer[10];
    
    if( CMD_GetArgc() >= 2 )
    {
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        SERVO_Position_Zero_Set(  (servo_id_t)atoi( arg_buffer ) );
    }
}
