#include "servo.h"

#include <xc.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "arm_math.h"

#include "peripheral/port/plib_port.h"
#include "peripheral/tc/plib_tc1.h"
#include "peripheral/tc/plib_tc4.h"


#define SERVO_MOTOR_STEPS_PER_REVOLUTION  (200)
#define SERVO_DRIVE_MICROSTEPS            (8)
#define SERVO_DRIVE_STEPS_PER_REVOLUTION  (SERVO_MOTOR_STEPS_PER_REVOLUTION * SERVO_DRIVE_MICROSTEPS)
#define SERVO_DRIVE_ANGLE_PER_STEP_Q15    (INT16_MAX / SERVO_DRIVE_STEPS_PER_REVOLUTION)
#define SERVO_DRIVE_SPEED_MAX             (8)

#define SERVO_STEP_COMPARE_VALUE    (10)


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

static servo_state_t servos[SERVO_ID_NUM_SERVOS];


static void SERVO_TC1_CompareCallback( TC_COMPARE_STATUS status, uintptr_t context );
static void SERVO_TC4_CompareCallback( TC_COMPARE_STATUS status, uintptr_t context );

static void SERVO_Drive( servo_id_t servo_id );
static void SERVO_Drive_StepAndDirection( servo_id_t servo_id, bool step, bool direction );


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

    MIKROBUS1_RST_Set();
    MIKROBUS1_CS_Set();
            
    MIKROBUS2_RST_Set();
    MIKROBUS2_CS_Set();
            
    MIKROBUS3_RST_Set();
    MIKROBUS3_CS_Set();
            
    TC1_CompareCallbackRegister( SERVO_TC1_CompareCallback, (uintptr_t)NULL );
    TC4_CompareCallbackRegister( SERVO_TC4_CompareCallback, (uintptr_t)NULL );
    
    TC1_CompareStart();
    TC4_CompareStart();
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

void SERVO_Angle_Zero_Set( servo_id_t servo_id )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return;
    }
    
    servos[servo_id].position_steps = 0;
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
            direction ? MIKROBUS1_DIR_Set() : MIKROBUS1_DIR_Clear();
            TC1_Compare8bitMatch0Set( step ? SERVO_STEP_COMPARE_VALUE : 0 );
            break;
        }
        case SERVO_ID_B:
        {
            direction ? MIKROBUS2_DIR_Set() : MIKROBUS2_DIR_Clear();
            TC1_Compare8bitMatch1Set( step ? SERVO_STEP_COMPARE_VALUE : 0 );
            break;
        }
        case SERVO_ID_C:
        {
            direction ? MIKROBUS3_DIR_Set() : MIKROBUS3_DIR_Clear();
            TC4_Compare8bitMatch1Set( step ? SERVO_STEP_COMPARE_VALUE : 0 );
            break;
        }
        default:
        {
            break;
        }
    }
}
