/**
  Command Processor implementation file

  @Company:
    Microchip Technology Inc.

  @File Name:
    command.c

  @Summary:
    This is the implementation file for Command Processor

  @Description:
    This file contains the implementation for the Command Processor.
    It provides a way to interact with the Command Processor subsystem to manage
    the ASCII command requests from the user.
*/

/*
    (c) 2017 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/


// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "command.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "command_config.h"
#include "command_hal.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************

#define CMD_USE_CIRCUILAR_BUFFER    ((CMD_ENABLE_HISTORY == 1) || (CMD_ENABLE_STREAM == 1))

#define CMD_ENABLE_ESC_SEQUENCES    (CMD_ENABLE_HISTORY == 1)


#define CMD_BACKSPACE_STR           "\b \b"
#define CMD_BELL_STR                "\a"

#define CMD_NULL_CHAR               0x00
#define CMD_BACKSPACE_CHAR          0x08
#define CMD_LINEFEED_CHAR           0x0A
#define CMD_CARRIAGE_RETURN_CHAR    0x0D
#define CMD_ESCAPE_CHAR             0x1B
#define CMD_SEPARATOR_CHAR          0x1F
#define CMD_SPACE_CHAR              0x20

#define CMD_CSI_CHAR1               0x1B
#define CMD_CSI_CHAR2               '['
#define CMD_CSI_CODE_CURSOR_UP      'A'
#define CMD_CSI_CODE_CURSOR_DOWN    'B'


#define CMD_STATE_TRANSITION(state)                \
    do { s_current_state = state; } while(0)

#define CMD_STATE_TRANSITION_STRING(state,string)  \
    do { s_current_state = state;                  \
         s_tx_string = string; } while(0)


#if CMD_USE_CIRCUILAR_BUFFER
#define CMD_BUFFER_NOT_EMPTY()  (s_cmd_end_index != s_cmd_start_index)
#define CMD_BUFFER_NOT_FULL()   (s_cmd_end_index != s_cmd_buffer_full_index)

#define INCREMENT_CMD_BUFFER_INDEX(index)    \
    do { index++;                            \
         if( index >= sizeof(s_cmd_buffer) ) \
         {                                   \
             index = 0;                      \
         } } while(0)

#define DECREMENT_CMD_BUFFER_INDEX(index)    \
    do { if( index > 0 )                     \
         {                                   \
             index--;                        \
         }                                   \
         else                                \
         {                                   \
             index = sizeof(s_cmd_buffer)-1; \
         } } while(0)
#else
#define CMD_BUFFER_NOT_EMPTY()  (s_cmd_end_index > 0)
#define CMD_BUFFER_NOT_FULL()   (s_cmd_end_index < sizeof(s_cmd_buffer))

#define INCREMENT_CMD_BUFFER_INDEX(index)    \
    do { index++; } while(0)

#define DECREMENT_CMD_BUFFER_INDEX(index)    \
    do { index--; } while(0)
#endif


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************

typedef enum
{
    CMD_STATE_INIT = 0,
#if (CMD_ENABLE_PASSWORD == 1)
    CMD_STATE_LOCKED,
#endif
    CMD_STATE_PROMPT,
    CMD_STATE_WAIT_RX,
    CMD_STATE_RX_CHAR,
    CMD_STATE_RX_CHAR_ECHO,
#if (CMD_ENABLE_HISTORY == 1)
    CMD_STATE_COMMAND_CLEAR,
    CMD_STATE_COMMAND_CLEAR_BACKSPACE,
    CMD_STATE_COMMAND_LOAD,
    CMD_STATE_COMMAND_LOAD_ECHO,
#endif
#if (CMD_ENABLE_STREAM == 1)
    CMD_STATE_STREAM,
#endif
    CMD_STATE_EXECUTE,
    CMD_STATE_COMMAND_RESET,

    CMD_STATE_INVALID
} CMD_MODULE_STATE_t;

typedef struct cmd_struct
{
    const char* string;
    cmd_function_t function;
} cmd_descriptor_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************

static CMD_MODULE_STATE_t s_current_state;

static const char* s_tx_string;

#if (CMD_BUFFER_SIZE > 256)
#error "CMD_BUFFER_SIZE cannot exceed 256"
#endif
static char s_cmd_buffer[CMD_BUFFER_SIZE];

static uint8_t s_cmd_end_index;

#if CMD_USE_CIRCUILAR_BUFFER
static uint8_t s_cmd_start_index;
static uint8_t s_cmd_buffer_full_index;
#endif
#if (CMD_ENABLE_HISTORY == 1)
static uint8_t s_cmd_history_index;
#endif
#if CMD_ENABLE_ESC_SEQUENCES
static bool s_in_escape;
#endif
#if (CMD_ENABLE_STREAM == 1)
static cmd_function_t s_previous_command;
#endif

#if (s_cmd_list_size > 256)
#error "s_cmd_list_size cannot exceed 256"
#endif
static cmd_descriptor_t s_cmd_list[CMD_COMMAND_LIST_SIZE];
static uint8_t s_cmd_list_size = 0;


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************

static void Handler_Init(void);
#if (CMD_ENABLE_PASSWORD == 1)
static void Handler_Locked(void);
#endif
static void Handler_Prompt(void);
static void Handler_WaitRx(void);
static void Handler_RxChar(void);
static void Handler_RxCharEcho(void);
#if (CMD_ENABLE_HISTORY == 1)
static void Handler_CommandClear(void);
static void Handler_CommandClearBackspace(void);
static void Handler_CommandLoad(void);
static void Handler_CommandLoadEcho(void);
#endif
static void Handler_Execute(void);
#if (CMD_ENABLE_STREAM == 1)
static void Handler_Stream(void);
#endif
static void Handler_CommandReset(void);


#if ((CMD_ENABLE_HISTORY == 1) || (CMD_ENABLE_STREAM == 1))
static uint8_t SearchPreviousCommand( uint8_t start_index, uint8_t stop_index );
#endif
#if (CMD_ENABLE_HISTORY == 1)
static uint8_t SearchNextCommand( uint8_t start_index, uint8_t stop_index );
#endif

static bool CheckCommandMatch( const char* command );

#if (CMD_ENABLE_HELP == 1)
static void DumpCommandList(void);
#endif

#if (CMD_ENABLE_PASSWORD == 1)
static void LockCommandProcessor(void);
#endif

#if (CMD_ENABLE_STREAM == 1)
static void EnableStream(void);
#endif


// ******************************************************************
// Section: Public Functions
// ******************************************************************

bool CMD_RegisterCommand( const char* string, cmd_function_t function )
{
    if( (string == NULL) || (function == NULL) || (s_cmd_list_size >= CMD_COMMAND_LIST_SIZE) )
    {
        return false;
    }

    s_cmd_list[s_cmd_list_size].string = string;
    s_cmd_list[s_cmd_list_size].function = function;

    s_cmd_list_size++;

    return true;
}

char* CMD_PrintString( const char* string, bool block )
{
#if (CMD_ENABLE_PASSWORD == 1)
    if( s_current_state <= CMD_STATE_LOCKED )
    {
        return NULL;
    }
#endif
    
    while( (string != NULL) &&
           (block || !CMD_HAL_IO_TxBufferFull()) )
    {
        CMD_HAL_IO_Write(*string);
        string++;
        if( *string == CMD_NULL_CHAR )
        {
            string = NULL;
        }        
    }
    
    return (char*)string;
}

uint8_t* CMD_PrintByteArray( const uint8_t* bytes, uint16_t length, bool block )
{
#if (CMD_ENABLE_PASSWORD == 1)
    if( s_current_state <= CMD_STATE_LOCKED )
    {
        return NULL;
    }
#endif
    
    while( (length > 0) &&
           (block || !CMD_HAL_IO_TxBufferFull()) )
    {
        CMD_HAL_IO_Write(*bytes++);
        length--;
    }
    
    if( length == 0 )
    {
        return NULL;
    }
    else
    {
        return (uint8_t*)bytes;	
    }
}


static const char HEX2CHAR[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void CMD_PrintHex_U8( uint8_t value, bool block )
{
	#if (CMD_ENABLE_PASSWORD == 1)
	if( s_current_state <= CMD_STATE_LOCKED )
	{
		return;
	}
	#endif
	
	char hexString[3];
	char* ptrChar = hexString + sizeof(hexString) - 1;
    *ptrChar = '\0';
	
	*ptrChar = CMD_NULL_CHAR;
	while( ptrChar > hexString )
	{
		ptrChar--;
		*ptrChar = HEX2CHAR[(uint8_t)value & 0x0F];
		value >>= 4;
	}
	
	(void)CMD_PrintString( hexString, block );
}

void CMD_PrintHex_U16( uint16_t value, bool block )
{
	#if (CMD_ENABLE_PASSWORD == 1)
	if( s_current_state <= CMD_STATE_LOCKED )
	{
		return;
	}
	#endif
	
	char hexString[5];
	char* ptrChar = hexString + sizeof(hexString) - 1;
    *ptrChar = '\0';
	
	*ptrChar = CMD_NULL_CHAR;
	while( ptrChar > hexString )
	{
		ptrChar--;
		*ptrChar = HEX2CHAR[(uint8_t)value & 0x0F];
		value >>= 4;
	}
	
	(void)CMD_PrintString( hexString, block );
}

void CMD_PrintHex_U32( uint32_t value, bool block )
{
#if (CMD_ENABLE_PASSWORD == 1)
    if( s_current_state <= CMD_STATE_LOCKED )
    {
        return;
    }
#endif
    
	char hexString[9];
	char* ptrChar = hexString + sizeof(hexString) - 1;
    *ptrChar = '\0';
	
	*ptrChar = CMD_NULL_CHAR;
	while( ptrChar > hexString )
	{
		ptrChar--;
		*ptrChar = HEX2CHAR[(uint8_t)value & 0x0F];
		value >>= 4;
    }
	
	(void)CMD_PrintString( hexString, block );	
}

void CMD_PrintDecimal_U32( uint32_t value, bool zero_blank, uint8_t width, bool block )
{
#if (CMD_ENABLE_PASSWORD == 1)
	if( s_current_state <= CMD_STATE_LOCKED )
	{
		return;
	}
#endif

	char decimalString[12];
	char* ptrChar = decimalString + sizeof(decimalString) - 1;
    *ptrChar = '\0';
    
	if( (value == 0) && (ptrChar > decimalString) )
	{
		ptrChar--;
		*ptrChar = '0';
		
		if( width > 0 )
		{
			width--;
		}
	}

	while( ((value > 0) || (width > 0)) && (ptrChar > decimalString) )
	{
		ptrChar--;
        if( value == 0 )
		{
            if( zero_blank )
            {
    			*ptrChar = ' ';
            }
            else
            {
    			*ptrChar = '0';
            }
		}
        else
        {
    		*ptrChar = '0' + (value % 10);
        }
		
		if( value >= 10 )
		{
			value = value / 10;
		}
		else
		{
			value = 0;
		}

		if( width > 0 )
		{
			width--;
		}
	}

	(void)CMD_PrintString( ptrChar, block );
}


void CMD_PrintFixedPoint_U32( uint32_t value, uint8_t fractional_bits,
							  bool zero_blank, uint8_t width, uint8_t precision, bool block )
{
#if (CMD_ENABLE_PASSWORD == 1)
	if( s_current_state <= CMD_STATE_LOCKED )
	{
		return;
	}
#endif

	value = value >> fractional_bits; 
	CMD_PrintDecimal_U32( value, zero_blank, width, block );

	if( precision > 0 )
	{
		uint64_t fraction = value & ((1ULL << fractional_bits) - 1);
		uint8_t power = precision;
		
		while( power > 0 )
		{
			fraction *= 10;
			power--;
		}
		
		fraction = fraction >> fractional_bits;

		(void)CMD_PrintString( ".", block );
		CMD_PrintDecimal_U32( (uint32_t)fraction, false, precision, block );
	}
}


#if (CMD_ENABLE_ARGS == 1)
uint8_t CMD_GetArgc( void )
{
    uint8_t argument_count = 0;
#if CMD_USE_CIRCUILAR_BUFFER
    uint8_t search_index = s_cmd_start_index;
#else
    uint8_t search_index = 0;
#endif
    uint8_t previous_char = CMD_NULL_CHAR;
    
    while( search_index != s_cmd_end_index )
    {
        // Start of each argument is indicated by a non-null character
        // preceded by a null character.
        if( (s_cmd_buffer[search_index] != CMD_NULL_CHAR) &&
            (previous_char == CMD_NULL_CHAR) )
        {
            argument_count++;
        }

        previous_char = s_cmd_buffer[search_index];
        INCREMENT_CMD_BUFFER_INDEX(search_index);
    }

    return argument_count;
}

uint8_t CMD_GetArgv( uint8_t argv_index, 
                     char* arg_buffer, uint8_t arg_buffer_size )
{
    uint8_t string_length = 0;
#if CMD_USE_CIRCUILAR_BUFFER
    uint8_t search_index = s_cmd_start_index;
#else
    uint8_t search_index = 0;
#endif
    uint8_t previous_char = CMD_NULL_CHAR;
    
    // check for valid parameters
    if( (arg_buffer == NULL) || (arg_buffer_size == 0) )
    {
        return 0;
    }
    
    // argv_index is 0-indexed.
    // Increment argv_index by 1 to accommodate search logic.
    argv_index++;
    // Find start of requested argument.  
    while( (argv_index > 0) && (search_index != s_cmd_end_index) )
    {    
        // Start of each argument is indicated by a non-null character
        // preceded by a null character.
        if( (s_cmd_buffer[search_index] != CMD_NULL_CHAR) &&
            (previous_char == CMD_NULL_CHAR) )
        {
            argv_index--;
        }
        
        previous_char = s_cmd_buffer[search_index];
        if( argv_index > 0 )
        {
            INCREMENT_CMD_BUFFER_INDEX(search_index);
        }
    }

    if( argv_index == 0 )
    {
        // Requested argument found. Copy argument to return buffer.
        do
        {
            *arg_buffer = s_cmd_buffer[search_index];
            arg_buffer++;
            INCREMENT_CMD_BUFFER_INDEX(search_index);
            string_length++;
        } while( (s_cmd_buffer[search_index] != CMD_NULL_CHAR) &&
                 (string_length < (arg_buffer_size-1)) );
        
        // force null termination of string
        *arg_buffer = CMD_NULL_CHAR;
    }
    else
    {
        // Requested argument not found.  Return empty string.
        *arg_buffer = CMD_NULL_CHAR;
        string_length = 0;
    }
    
    return string_length;
}
#endif

#if (CMD_ENABLE_STREAM == 1)
bool CMD_CheckEscape(void)
{
    // check for escape character to exit stream mode.
    return ( !CMD_HAL_IO_RxBufferEmpty() && (CMD_HAL_IO_Read() == CMD_ESCAPE_CHAR) );
}
#endif

void CMD_Initialize(void)
{
    CMD_HAL_Initialize();

    CMD_STATE_TRANSITION_STRING(CMD_STATE_INIT, NULL);
}

void CMD_Task(void)
{
    CMD_MODULE_STATE_t previous_state;
    
    CMD_HAL_Tasks();
          
    do
    {
        previous_state = s_current_state;
                
        if( s_tx_string != NULL )
        {
            s_tx_string = CMD_PrintString(s_tx_string, false);
        }

        if( s_tx_string == NULL )
        {
            switch( s_current_state )
            {
                case CMD_STATE_INIT:
                    Handler_Init();
                    break;
#if (CMD_ENABLE_PASSWORD == 1)
                case CMD_STATE_LOCKED:
                    Handler_Locked();
                    break;
#endif
                case CMD_STATE_PROMPT:
                    Handler_Prompt();
                    break;
                case CMD_STATE_WAIT_RX:
                    Handler_WaitRx();
                    break;
                case CMD_STATE_RX_CHAR:
                    Handler_RxChar();
                    break;
                case CMD_STATE_RX_CHAR_ECHO:
                    Handler_RxCharEcho();
                    break;
#if (CMD_ENABLE_HISTORY == 1)
                case CMD_STATE_COMMAND_CLEAR:
                    Handler_CommandClear();
                    break;
                case CMD_STATE_COMMAND_CLEAR_BACKSPACE:
                    Handler_CommandClearBackspace();
                    break;
                case CMD_STATE_COMMAND_LOAD:
                    Handler_CommandLoad();
                    break;
                case CMD_STATE_COMMAND_LOAD_ECHO:
                    Handler_CommandLoadEcho();
                    break;
#endif
                case CMD_STATE_EXECUTE:
                    Handler_Execute();
                    break;
#if (CMD_ENABLE_STREAM == 1)
                case CMD_STATE_STREAM:
                    Handler_Stream();
                    break;
#endif                    
                case CMD_STATE_COMMAND_RESET:
                    Handler_CommandReset();
                    break;
                default:
                    CMD_STATE_TRANSITION(CMD_STATE_INIT);
                    break;
            }
        }
    } while( s_current_state != previous_state );
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void Handler_Init(void)
{
    s_cmd_end_index = 0;

#if CMD_USE_CIRCUILAR_BUFFER
    for( s_cmd_buffer_full_index=0;
         s_cmd_buffer_full_index<(sizeof(s_cmd_buffer)-1);
         s_cmd_buffer_full_index++ )
    {
        s_cmd_buffer[s_cmd_buffer_full_index] = CMD_NULL_CHAR;
    }
    s_cmd_buffer[s_cmd_buffer_full_index] = CMD_SEPARATOR_CHAR;

    s_cmd_start_index = 0;
#endif
    
#if (CMD_ENABLE_HISTORY == 1)
    s_cmd_history_index = 0;
#endif
    
#if CMD_ENABLE_ESC_SEQUENCES
    s_in_escape = false;
#endif
    
#if (CMD_ENABLE_STREAM == 1)
    s_previous_command = NULL;
#endif

#if (CMD_ENABLE_PASSWORD == 1)
    CMD_STATE_TRANSITION(CMD_STATE_LOCKED);
#else
    CMD_STATE_TRANSITION_STRING(CMD_STATE_PROMPT, CMD_INITIALIZE_STR);
#endif    
}

#if (CMD_ENABLE_PASSWORD == 1)
static void Handler_Locked(void)
{
    bool unlock = false;
    const char* const unlock_string = CMD_UNLOCK_COMMAND_STR;

    while( !CMD_HAL_IO_RxBufferEmpty() && !unlock )
    {
        if( CMD_HAL_IO_Read() == unlock_string[s_cmd_end_index] )
        {
            s_cmd_end_index++;
            if( s_cmd_end_index >= (sizeof(CMD_UNLOCK_COMMAND_STR)-1) )
            {
                unlock = true;
            }
        }
        else
        {
            s_cmd_end_index = 0;
        }
    }
    
    if( unlock )
    {
        s_cmd_end_index = 0;
        CMD_STATE_TRANSITION_STRING(CMD_STATE_PROMPT, CMD_INITIALIZE_STR);
    }
}
#endif

static void Handler_Prompt(void)
{
    CMD_STATE_TRANSITION_STRING(CMD_STATE_WAIT_RX, CMD_COMMAND_PROMPT_STR);
}

static void Handler_WaitRx(void)
{
    if( !CMD_HAL_IO_RxBufferEmpty() )
    {
        CMD_STATE_TRANSITION(CMD_STATE_RX_CHAR);
    }                    
}

static void Handler_RxChar(void)
{
    static char previous_char = 0;
    char rx_char = CMD_HAL_IO_Read();
    
    if( (rx_char == CMD_LINEFEED_CHAR) && 
        (previous_char == CMD_CARRIAGE_RETURN_CHAR) )
    {
        // Ignore linefeed following carriage return - we already acted on the
        // previous carriage return.
        CMD_STATE_TRANSITION(CMD_STATE_WAIT_RX);        
    }
    else if( (rx_char == CMD_CARRIAGE_RETURN_CHAR) ||
             (rx_char == CMD_LINEFEED_CHAR) )
    {
        if( CMD_BUFFER_NOT_EMPTY() )
        {
            CMD_STATE_TRANSITION_STRING(CMD_STATE_EXECUTE, CMD_LINE_TERMINATOR_STR);
        }
        else
        {
            CMD_STATE_TRANSITION_STRING(CMD_STATE_PROMPT, CMD_LINE_TERMINATOR_STR);                    
        }
    }
    else if( rx_char == CMD_BACKSPACE_CHAR )
    {
        if( CMD_BUFFER_NOT_EMPTY() )
        {
            DECREMENT_CMD_BUFFER_INDEX(s_cmd_end_index);

#if (CMD_ENABLE_HISTORY == 1)
            // reset history index to active command
            s_cmd_history_index = s_cmd_start_index;
#endif

            CMD_STATE_TRANSITION_STRING(CMD_STATE_WAIT_RX, CMD_BACKSPACE_STR);
        }
        else
        {
            CMD_STATE_TRANSITION_STRING(CMD_STATE_WAIT_RX, CMD_BELL_STR);
        }
    }
#if CMD_ENABLE_ESC_SEQUENCES
    else if( (rx_char == CMD_CSI_CHAR1) && !s_in_escape )
    {
        s_in_escape = true;
        CMD_STATE_TRANSITION(CMD_STATE_WAIT_RX);
    }
    else if( s_in_escape )
    {
        if( (rx_char == CMD_CSI_CHAR2) && (previous_char == CMD_CSI_CHAR1) )
        {
            // Still good. Stay in escape mode and wait for next character
            CMD_STATE_TRANSITION(CMD_STATE_WAIT_RX);
        }
        else if( previous_char == CMD_CSI_CHAR2 )
        {
            switch( rx_char )
            {
#if (CMD_ENABLE_HISTORY == 1)
                case CMD_CSI_CODE_CURSOR_UP:
                {
                    uint8_t previousCommandIndex = SearchPreviousCommand(s_cmd_history_index, s_cmd_end_index);
                    if( previousCommandIndex != s_cmd_history_index )
                    {
                        // Found previous command.
                        s_cmd_history_index = previousCommandIndex;
                        CMD_STATE_TRANSITION(CMD_STATE_COMMAND_CLEAR);                                
                    }
                    else
                    {
                        // No previous command.
                        CMD_STATE_TRANSITION_STRING(CMD_STATE_WAIT_RX, CMD_BELL_STR);
                    }                    
                    break;
                }
                case CMD_CSI_CODE_CURSOR_DOWN:
                {
                    uint8_t nextCommandIndex = SearchNextCommand(s_cmd_history_index, s_cmd_start_index);
                    if( nextCommandIndex != s_cmd_history_index )
                    {
                        // Found next command.
                        s_cmd_history_index = nextCommandIndex;
                        CMD_STATE_TRANSITION(CMD_STATE_COMMAND_CLEAR);                                
                    }
                    else
                    {
                        // No next command.
                        CMD_STATE_TRANSITION_STRING(CMD_STATE_WAIT_RX, CMD_BELL_STR);
                    }                    
                    break;
                }
#endif
                default:
                {
                    // Unknown Command Sequence Code
                    CMD_STATE_TRANSITION(CMD_STATE_WAIT_RX);
                }
            }
            
            s_in_escape = false;
        }
        else
        {
            // invalid control sequence, drop out of escape
            s_in_escape = false;
            CMD_STATE_TRANSITION(CMD_STATE_WAIT_RX);            
        }
    }
#endif
    else if( (rx_char >= 0x20) && (rx_char <= 0x7E) )
    {
        if( CMD_BUFFER_NOT_FULL() )
        {
            s_cmd_buffer[s_cmd_end_index] = rx_char;

#if (CMD_ENABLE_HISTORY == 1)
            // reset history index to point to active command
            s_cmd_history_index = s_cmd_start_index;
#endif

            CMD_STATE_TRANSITION(CMD_STATE_RX_CHAR_ECHO);
        }
        else
        {
            // ignore character - buffer full
            CMD_STATE_TRANSITION_STRING(CMD_STATE_WAIT_RX, CMD_BELL_STR);
        }
    }
    else
    {
        // ignore character - we don't know what to do with it.
        CMD_STATE_TRANSITION(CMD_STATE_WAIT_RX);
    }
    
    previous_char = rx_char;
}

static void Handler_RxCharEcho(void)
{
    if( !CMD_HAL_IO_TxBufferFull() )
    {
        CMD_HAL_IO_Write(s_cmd_buffer[s_cmd_end_index]);        

        // terminate argument strings in buffer by replacing space character
        // with null character
        if( s_cmd_buffer[s_cmd_end_index] == CMD_SPACE_CHAR )
        {
            s_cmd_buffer[s_cmd_end_index] = CMD_NULL_CHAR;
        }
        
        INCREMENT_CMD_BUFFER_INDEX(s_cmd_end_index);
        
        CMD_STATE_TRANSITION(CMD_STATE_WAIT_RX);
    }
}

#if (CMD_ENABLE_HISTORY == 1)
static void Handler_CommandClear(void)
{
    if( CMD_BUFFER_NOT_EMPTY() )
    {
        DECREMENT_CMD_BUFFER_INDEX(s_cmd_end_index);
        CMD_STATE_TRANSITION(CMD_STATE_COMMAND_CLEAR_BACKSPACE);
    }
    else
    {
        CMD_STATE_TRANSITION(CMD_STATE_COMMAND_LOAD);
    }
}

static void Handler_CommandClearBackspace(void)
{
    CMD_STATE_TRANSITION_STRING(CMD_STATE_COMMAND_CLEAR, CMD_BACKSPACE_STR);
}

static void Handler_CommandLoad(void)
{
    if( s_cmd_buffer[s_cmd_history_index] != CMD_SEPARATOR_CHAR )
    {
        s_cmd_buffer[s_cmd_end_index] = s_cmd_buffer[s_cmd_history_index];
        CMD_STATE_TRANSITION(CMD_STATE_COMMAND_LOAD_ECHO);        
    }
    else
    {
        // back up history index to point to end of command that was just loaded.
        DECREMENT_CMD_BUFFER_INDEX(s_cmd_history_index);
        CMD_STATE_TRANSITION(CMD_STATE_WAIT_RX);
    }
}

static void Handler_CommandLoadEcho(void)
{
    if( !CMD_HAL_IO_TxBufferFull() )
    {
        if( s_cmd_buffer[s_cmd_end_index] == CMD_NULL_CHAR )
        {
            CMD_HAL_IO_Write(CMD_SPACE_CHAR);
        }
        else
        {
            CMD_HAL_IO_Write(s_cmd_buffer[s_cmd_end_index]);
        }

        INCREMENT_CMD_BUFFER_INDEX(s_cmd_end_index);
        INCREMENT_CMD_BUFFER_INDEX(s_cmd_history_index);
                
        CMD_STATE_TRANSITION(CMD_STATE_COMMAND_LOAD);
    }
}
#endif

static void Handler_Execute(void)
{
    cmd_function_t command = NULL;
    uint8_t command_list_index = 0;

    // Terminate the active command string
    s_cmd_buffer[s_cmd_end_index] = CMD_NULL_CHAR;

#if (CMD_ENABLE_STREAM == 1)
    if( CheckCommandMatch(CMD_STREAM_COMMAND_STR) )
    {
        EnableStream();
        return;
    }

    s_previous_command = NULL;
#endif
    
#if (CMD_ENABLE_HELP == 1)
    if( CheckCommandMatch(CMD_HELP_COMMAND_STR) )
    {
        DumpCommandList();
        return;
    }
#endif

#if (CMD_ENABLE_PASSWORD == 1)
    if( CheckCommandMatch(CMD_LOCK_COMMAND_STR) )
    {
        LockCommandProcessor();
        return;
    }
#endif

    while( (command == NULL) && 
           (command_list_index < s_cmd_list_size) )
    {
        if( CheckCommandMatch(s_cmd_list[command_list_index].string) )
        {
            command = s_cmd_list[command_list_index].function;
        }
        else
        {
            command_list_index++;
        }
    }

    if( command != NULL )
    {
        command();
#if (CMD_ENABLE_STREAM == 1)
        s_previous_command = command;
#endif
        CMD_STATE_TRANSITION(CMD_STATE_COMMAND_RESET);
    }
    else
    {
        CMD_STATE_TRANSITION_STRING(CMD_STATE_COMMAND_RESET, CMD_COMMAND_NOT_FOUND_STR);         
    }
}

#if (CMD_ENABLE_STREAM == 1)
static void Handler_Stream(void)
{
    // check for escape character to exit stream mode.
    if( !CMD_HAL_IO_RxBufferEmpty() && (CMD_HAL_IO_Read() == CMD_ESCAPE_CHAR) )
    {
        CMD_HAL_TIMER_Stop();
        CMD_STATE_TRANSITION(CMD_STATE_COMMAND_RESET);
    }                    
    else
    {
        if( CMD_HAL_TIMER_IsTimerExpired() )
        {
            s_previous_command();
        }
    }
}
#endif

static void Handler_CommandReset(void)
{
#if CMD_USE_CIRCUILAR_BUFFER
    // replace final null terminator character of previous command with separator
    // character in order to support history search through command buffer.
    s_cmd_buffer[s_cmd_end_index] = CMD_SEPARATOR_CHAR;
    
    // Save buffer full index to allow for quick check of buffer full condition
    s_cmd_buffer_full_index = s_cmd_end_index;

    // Initialize indexes to prepare for reception of next command
    INCREMENT_CMD_BUFFER_INDEX(s_cmd_end_index);
    s_cmd_start_index = s_cmd_end_index;
#else
    s_cmd_end_index = 0;
#endif

#if (CMD_ENABLE_HISTORY == 1)    
    s_cmd_history_index = s_cmd_end_index;
#endif

#if CMD_ENABLE_ESC_SEQUENCES
    s_in_escape = false;
#endif
    
    CMD_STATE_TRANSITION_STRING(CMD_STATE_PROMPT, CMD_LINE_TERMINATOR_STR);                    
}


#if ((CMD_ENABLE_HISTORY == 1) || (CMD_ENABLE_STREAM == 1))
static uint8_t SearchPreviousCommand( uint8_t start_index, uint8_t stop_index )
{
    uint8_t separator_count = 0;
    uint8_t search_index = start_index;
    
    // Need to bump stop index by one to support search logic
    INCREMENT_CMD_BUFFER_INDEX(stop_index);
    
    // We need to find 2 command separator characters in order to find
    // complete previous command.
    //  - 1st separator indicates the end of a complete previous command
    //  - 2nd separator indicates the beginning of the previous command
    // Search until we either find 2 separator characters or bump into end
    // of currently active command.
    while( (separator_count < 2) && (search_index != stop_index) )
    {
        DECREMENT_CMD_BUFFER_INDEX(search_index);
        
        if( s_cmd_buffer[search_index] == CMD_SEPARATOR_CHAR )
        {
            separator_count++;
        }
    }

    if( separator_count >= 2 )
    {
        // Found previous command.
        // Return value is index to first character of previous command.
        INCREMENT_CMD_BUFFER_INDEX(search_index);
    }
    else
    {
        // No previous command.
        // Return value is start_index as indication of command not found.        
        search_index = start_index;
    }                    
    
    return search_index;
}
#endif

#if (CMD_ENABLE_HISTORY == 1)
static uint8_t SearchNextCommand( uint8_t start_index, uint8_t stop_index )
{
    uint8_t separator_count = 0;
    uint8_t search_index = start_index;
    uint8_t next_command_index = start_index;

    // We need to find 2 command separator characters in order to find
    // complete next command.
    //  - 1st separator indicates the start of a next command
    //  - 2nd separator indicates the end of a complete next command
    // Search until we either find 2 separator characters or bump into
    // end of buffer.
    while( (separator_count < 2) && (search_index != stop_index) )
    {
        INCREMENT_CMD_BUFFER_INDEX(search_index);
        
        if( s_cmd_buffer[search_index] == CMD_SEPARATOR_CHAR )
        {
            separator_count++;
            if( separator_count == 1 )
            {
                // Found start of next command. Make note of start index. Still
                // need to find end of command for it to be valid next command.
                next_command_index = search_index;
                INCREMENT_CMD_BUFFER_INDEX(next_command_index);        
            }
        }
    }
    
    if( separator_count < 2 )
    {
        // No next command. 
        // Return value is start_index as indication of command not found.        
        next_command_index = start_index;
    }                    
    
    return next_command_index;
}
#endif

static bool CheckCommandMatch(const char* command)
{
    bool found = false;

#if CMD_USE_CIRCUILAR_BUFFER
    uint8_t match_index = s_cmd_start_index;
#else
    uint8_t match_index = 0;
#endif
    
    while( !found && (*command == s_cmd_buffer[match_index]) )
    {
        if( *command == CMD_NULL_CHAR )
        {
            found = true;
        }
        else
        {
            command++;
            INCREMENT_CMD_BUFFER_INDEX(match_index);
        }
    }

    return found;
}

#if (CMD_ENABLE_HELP == 1)
static void DumpCommandList(void)
{
    uint8_t command_list_index;
    
#if (CMD_ENABLE_PASSWORD == 1)
    (void)CMD_PrintString(CMD_LOCK_COMMAND_STR, true);
    (void)CMD_PrintString(CMD_LINE_TERMINATOR_STR, true);
#endif

#if (CMD_ENABLE_STREAM == 1)
    (void)CMD_PrintString(CMD_STREAM_COMMAND_STR, true);
    (void)CMD_PrintString(CMD_LINE_TERMINATOR_STR, true);
#endif

    for( command_list_index = 0; 
         command_list_index < s_cmd_list_size;
         command_list_index++ )
    {
        (void)CMD_PrintString(s_cmd_list[command_list_index].string, true);
        (void)CMD_PrintString(CMD_LINE_TERMINATOR_STR, true);
    }
    
    CMD_STATE_TRANSITION(CMD_STATE_COMMAND_RESET);
}
#endif

#if (CMD_ENABLE_PASSWORD == 1)
static void LockCommandProcessor(void)
{
    (void)CMD_PrintString(CMD_LOCK_MESSAGE_STR, false);
    CMD_STATE_TRANSITION(CMD_STATE_INIT);
}
#endif

#if (CMD_ENABLE_STREAM == 1)
static void EnableStream(void)
{
    uint32_t period = 0;
    uint8_t previous_command_index = SearchPreviousCommand( s_cmd_start_index, s_cmd_end_index );

    // check if previous command is still available in buffer
    if( (s_previous_command != NULL) && 
        (previous_command_index != s_cmd_start_index) )
    {
        uint8_t parse_index = s_cmd_start_index;
        
        // skip over argv[0] in the command buffer
        while( (s_cmd_buffer[parse_index] != CMD_NULL_CHAR) &&
               (parse_index != s_cmd_end_index) )
        {
            INCREMENT_CMD_BUFFER_INDEX(parse_index);                        
        }

        // skip over space before argv[1]
        while( (s_cmd_buffer[parse_index] == CMD_NULL_CHAR) &&
               (parse_index != s_cmd_end_index) )
        {
            INCREMENT_CMD_BUFFER_INDEX(parse_index);                                    
        }
        
        // parse argv[1] for desired period
        while( (s_cmd_buffer[parse_index] >= '0') &&
               (s_cmd_buffer[parse_index] <= '9') )
        {
            period *= 10;
            period += (s_cmd_buffer[parse_index] - '0');
            INCREMENT_CMD_BUFFER_INDEX(parse_index);

            // clamp working value to max if it grows too large (i.e. > 16 bits)
            if( period > UINT16_MAX )
            {
                period = UINT16_MAX;
            }
        }
    }
    else
    {
        s_previous_command = NULL;
    }

    // Reset buffer indexes to point to previous command.
    // This is necessary to support arguments for streamed command.
    s_cmd_start_index = previous_command_index;
    s_cmd_end_index = s_cmd_buffer_full_index;
    s_cmd_buffer[s_cmd_end_index] = CMD_NULL_CHAR;

    if( period > 0 )
    {
        CMD_HAL_TIMER_Start( (uint16_t)period );

        // Call previous command.
        s_previous_command();

        CMD_STATE_TRANSITION(CMD_STATE_STREAM);        
    }
    else
    {
        if( s_previous_command == NULL )
        {
            CMD_STATE_TRANSITION_STRING(CMD_STATE_COMMAND_RESET, CMD_NO_STREAM_MESSAGE_STR);
        }
        else
        {
            CMD_STATE_TRANSITION_STRING(CMD_STATE_COMMAND_RESET, CMD_NO_TICK_MESSAGE_STR);            
        }
    }
}
#endif
