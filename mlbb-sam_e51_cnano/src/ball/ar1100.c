#include "ar1100.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "peripheral/sercom/usart/plib_sercom0_usart.h"


#define AR1100_READ_BUFFER_SIZE     (5)

static uint8_t ar1100_readBuffer[AR1100_READ_BUFFER_SIZE];
static size_t ar1100_readBufferIndex;

// Set up ping-pong buffer to store most recent touch data. Using buffer
// will ensure consistent data for AR1100_TouchData_Get(), even if it is
// interrupted with reception of a fresh data packet.
static volatile ar1100_touchdata_t ar1100_touchData[2];
static volatile uint8_t ar1100_touchData_readIndex = 0;

static ar1100_touch_callback_t ar1100_touchCallback = NULL;

static void ar1100_UartReadCallback( uintptr_t context );


void AR1100_Initialize( void )
{
    ar1100_touchData[0].down = false;
    ar1100_touchData[0].x = 0;
    ar1100_touchData[0].y = 0;
    
    ar1100_touchData[1].down = false;
    ar1100_touchData[1].x = 0;
    ar1100_touchData[1].y = 0;

    ar1100_readBufferIndex = 0;    
    SERCOM0_USART_ReadCallbackRegister( ar1100_UartReadCallback, (uintptr_t)NULL );
    SERCOM0_USART_Read( &ar1100_readBuffer[ar1100_readBufferIndex], 1 );
}

void AR1100_TouchCallback_Register( ar1100_touch_callback_t callback )
{
    ar1100_touchCallback = callback;
}


ar1100_touchdata_t AR1100_TouchData_Get( void )
{
    return ar1100_touchData[ar1100_touchData_readIndex];
}


static void ar1100_UartReadCallback( uintptr_t context )
{
    (void)context;
    
    switch( ar1100_readBufferIndex )
    {
        case 0:
        {
            if( ar1100_readBuffer[ar1100_readBufferIndex] & 0x80 )
            {
                ar1100_readBufferIndex++;
            }
            break;
        }
        case 1:
        case 2:
        case 3:
        {
            if( !(ar1100_readBuffer[ar1100_readBufferIndex] & 0x80) )
            {
                ar1100_readBufferIndex++;
            }
            else
            {
                ar1100_readBufferIndex = 0;
            }
            break;
        }
        case 4:
        {
            if( !(ar1100_readBuffer[ar1100_readBufferIndex] & 0x80) )
            {
                uint8_t writeIndex = (ar1100_touchData_readIndex + 1) % 2;

                ar1100_touchData[writeIndex].down = ((ar1100_readBuffer[0] & 0x01) != 0);
                ar1100_touchData[writeIndex].x = ((ar1100_readBuffer[2] & 0x1f) << 7) | (ar1100_readBuffer[1] & 0x7f);
                ar1100_touchData[writeIndex].y = ((ar1100_readBuffer[4] & 0x1f) << 7) | (ar1100_readBuffer[3] & 0x7f);
                
                if( ar1100_touchCallback != NULL )
                {
                    ar1100_touchCallback( ar1100_touchData[writeIndex] );
                }
                
                ar1100_touchData_readIndex = writeIndex;
            }
            ar1100_readBufferIndex = 0;
            break;            
        }
        default:
        {
            ar1100_readBufferIndex = 0;
            break;
        }
    }
    
    SERCOM0_USART_Read( &ar1100_readBuffer[ar1100_readBufferIndex], 1 );
}
