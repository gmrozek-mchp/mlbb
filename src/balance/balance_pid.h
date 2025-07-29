#ifndef BALANCE_PID_H
#define	BALANCE_PID_H


#include <stdbool.h>
#include "arm_math_types.h"


void BALANCE_PID_Initialize( void );

void BALANCE_PID_Reset( void );

void BALANCE_PID_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y );

void BALANCE_PID_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y );


#endif	/* BALANCE_PID_H */

