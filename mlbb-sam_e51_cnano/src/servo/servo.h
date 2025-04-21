#ifndef SERVO_H
#define	SERVO_H


#include "arm_math.h"


typedef enum {
    SERVO_ID_A = 0,
    SERVO_ID_B,
    SERVO_ID_C,

    SERVO_ID_NUM_SERVOS
} servo_id_t;


void  SERVO_Initialize( void );

q15_t SERVO_Position_Get_q15angle( servo_id_t servo_id );
q15_t SERVO_Position_Command_Get_q15angle( servo_id_t servo_id );
void  SERVO_Position_Command_Set_q15angle( servo_id_t servo_id, q15_t angle );
void  SERVO_Position_Zero_Set( servo_id_t servo_id );


// **************************************************************
//  TOUCH Command portal functions
// **************************************************************
void SERVO_CMD_Position_GetSet( void );
void SERVO_CMD_Position_Zero( void );


#endif	/* SERVO_H */

