#ifndef BALANCE_HUMAN_H
#define	BALANCE_HUMAN_H

#include <stdbool.h>

#include "arm_math_types.h"


void BALANCE_HUMAN_Initialize( void );

void BALANCE_HUMAN_Reset( void );

void BALANCE_HUMAN_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y );

void BALANCE_HUMAN_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y );


#endif	/* BALANCE_HUMAN_H */
