#ifndef AR1100_H
#define	AR1100_H


#include <stdbool.h>
#include <stdint.h>


typedef struct {
    bool down;
    uint16_t x;
    uint16_t y;
} ar1100_touchdata_t;


void AR1100_Initialize( void );

ar1100_touchdata_t AR1100_TouchData_Get( void );

bool AR1100_Touch_Down_Get( void );
uint16_t AR1100_Touch_X_Get( void );
uint16_t AR1100_Touch_Y_Get( void );

// **************************************************************
//  TOUCH Command portal functions
// **************************************************************
void AR1100_CMD_Print_TouchData( void );


#endif	/* AR1100_H */

