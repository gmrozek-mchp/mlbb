#ifndef AR1100_H
#define	AR1100_H


#include <stdbool.h>
#include <stdint.h>


typedef struct {
    bool down;
    uint16_t x;
    uint16_t y;
} ar1100_touchdata_t;

typedef void (*ar1100_touch_callback_t)( ar1100_touchdata_t touchData );


void AR1100_Initialize( void );

void AR1100_TouchCallback_Register( ar1100_touch_callback_t callback );

ar1100_touchdata_t AR1100_TouchData_Get( void );


// **************************************************************
//  TOUCH Command portal functions
// **************************************************************
void AR1100_CMD_Print_TouchData( void );


#endif	/* AR1100_H */

