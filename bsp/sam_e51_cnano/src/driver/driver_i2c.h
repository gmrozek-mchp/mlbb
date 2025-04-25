#ifndef DRIVER_I2C_H
#define	DRIVER_I2C_H


#include <stdint.h>


void DRIVER_I2C_Initialize( void );

void DRIVER_I2C_Write(uint16_t address, uint8_t* wrData, uint32_t wrLength);

void DRIVER_I2C_Read(uint16_t address, uint8_t* rdData, uint32_t rdLength);


#endif	/* DRIVER_I2C_H */

