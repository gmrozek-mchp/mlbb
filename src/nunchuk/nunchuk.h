#ifndef NUNCHUK_H
#define	NUNCHUK_H


#include <stdbool.h>
#include <stdint.h>


typedef struct {
    int16_t joystick_x;
    int16_t joystick_y;
    bool button_c;
    bool button_z;
} nunchuk_data_t;

typedef void (*nunchuk_data_callback_t)( nunchuk_data_t data );


void NUNCHUK_Initialize( void );

void NUNCHUK_DataCallback_Register( nunchuk_data_callback_t callback );

nunchuk_data_t NUNCHUK_Data_Get( void );

void NUNCHUK_Zero_Set( void );


#endif	/* NUNCHUK_H */

