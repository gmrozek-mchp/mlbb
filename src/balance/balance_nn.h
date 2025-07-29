#ifndef BALANCE_NN_H
#define	BALANCE_NN_H


#include <stdbool.h>
#include "arm_math_types.h"


void BALANCE_NN_Initialize( void );

void BALANCE_NN_Reset( void );

void BALANCE_NN_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y );

void BALANCE_NN_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y );


#endif	/* BALANCE_NN_H */

