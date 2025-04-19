#ifndef PLATFORM_H
#define	PLATFORM_H


#include <stdint.h>

#include "arm_math.h"


typedef struct {
    q15_t x;
    q15_t y;
} platform_xy_t;

typedef struct {
    q15_t a;
    q15_t b;
    q15_t c;    
} platform_abc_t;


void PLATFORM_Initialize( void );

platform_xy_t PLATFORM_Position_XY_Get( void );
void PLATFORM_Position_XY_Set( platform_xy_t  );

platform_abc_t PLATFORM_Position_ABC_Get( void );
void PLATFORM_Position_ABC_Set( platform_abc_t  );


// **************************************************************
//  TOUCH Command portal functions
// **************************************************************
void PLATFORM_CMD_Position_XY( void );
void PLATFORM_CMD_Position_ABC( void );


#endif	/* PLATFORM_H */

