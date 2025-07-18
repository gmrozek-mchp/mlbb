#ifndef BALL_H
#define	BALL_H

#include <stdbool.h>
#include <stdint.h>


typedef struct {
    bool detected;
    int16_t x;
    int16_t y;
} ball_data_t;


void BALL_Initialize( void );

ball_data_t BALL_Position_Get( void );


#endif	/* BALL_H */
