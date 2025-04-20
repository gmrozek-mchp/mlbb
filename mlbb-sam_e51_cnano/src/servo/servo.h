#ifndef SERVO_H
#define	SERVO_H


#include "arm_math.h"


typedef enum {
    SERVO_A = 0,
    SERVO_B,
    SERVO_C,

    SERVO_NUM_SERVOS
} servo_t;


void SERVO_Initialize( void );

q15_t SERVO_Angle_Get( servo_t servo );
void SERVO_Angle_Set( servo_t servo, q15_t angle );
void SERVO_Angle_Zero( servo_t servo );


// **************************************************************
//  TOUCH Command portal functions
// **************************************************************
void SERVO_CMD_Angle_GetSet( void );
void SERVO_CMD_Angle_Zero( void );


#endif	/* SERVO_H */

