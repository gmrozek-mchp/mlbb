#ifndef BALANCE_FUZZY_H
#define	BALANCE_FUZZY_H


#include <stdbool.h>
#include "arm_math_types.h"


void BALANCE_FUZZY_Initialize( void );

void BALANCE_FUZZY_Reset( void );

void BALANCE_FUZZY_Run( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y );

void BALANCE_FUZZY_DataVisualizer( q15_t target_x, q15_t target_y, bool ball_detected, q15_t ball_x, q15_t ball_y );


#endif	/* BALANCE_FUZZY_H */ 