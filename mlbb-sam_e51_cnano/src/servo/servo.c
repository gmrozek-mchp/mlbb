#include "servo.h"

#include <xc.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "arm_math.h"

#include "peripheral/tc/plib_tc1.h"
#include "peripheral/tc/plib_tc4.h"


typedef struct {
    q15_t command;
    q15_t actual;
    q15_t limit_min;
    q15_t limit_max;
    int16_t speed;
} servo_state_t;

static servo_state_t servo_state[SERVO_NUM_SERVOS];


void SERVO_TC1_TimerCallback( TC_TIMER_STATUS status, uintptr_t context );
void SERVO_TC4_TimerCallback( TC_TIMER_STATUS status, uintptr_t context );

void SERVO_DoStepDirection( servo_t servo,  );


void SERVO_Initialize( void )
{
    servo_t servo;
    
    for( servo = 0; servo < SERVO_NUM_SERVOS; servo++ )
    {
        servo_state[servo].command = 0;
        servo_state[servo].actual = 0;
        servo_state[servo].limit_min = INT16_MIN;
        servo_state[servo].limit_max = INT16_MAX;
        servo_state[servo].speed = 0;
    }

    TC1_TimerCallbackRegister( SERVO_TC1_TimerCallback, (uintptr_t)NULL );
    TC4_TimerCallbackRegister( SERVO_TC4_TimerCallback, (uintptr_t)NULL );
}

q15_t SERVO_Angle_Get( servo_t servo )
{
    return servo_state[servo].actual;
}

void SERVO_Angle_Set( servo_t servo, q15_t angle )
{
    if( servo >= SERVO_NUM_SERVOS )
    {
        return;
    }

    if( angle >= servo_state[servo].limit_max )
    {
        angle = servo_state[servo].limit_max;
    }
    else if( angle <= servo_state[servo].limit_min )
    {
        angle = servo_state[servo].limit_min;
    }
    
    servo_state[servo].command = angle;
}

void SERVO_Angle_Zero( servo_t servo )
{
    if( servo >= SERVO_NUM_SERVOS )
    {
        return;
    }
    
    servo_state[servo].actual = 0;
}


void SERVO_TC1_TimerCallback( TC_TIMER_STATUS status, uintptr_t context )
{
    SERVO_DoStepDirection( SERVO_A );
    SERVO_DoStepDirection( SERVO_B );
}

void SERVO_TC4_TimerCallback( TC_TIMER_STATUS status, uintptr_t context )
{
    SERVO_DoStepDirection( SERVO_C );    
}
