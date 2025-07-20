// ******************************************************************
// Section: Included Files
// ******************************************************************

#include "servo.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "peripheral/tcc/plib_tcc1.h"

#include "command/command.h"


// ******************************************************************
// Section: Macro Declarations
// ******************************************************************
#define SERVO_RTOS_PRIORITY         (3)
#define SERVO_RTOS_STACK_SIZE       (configMINIMAL_STACK_SIZE)

#define SERVO_POWER_UP_DELAY_mS     (100)

#define ANGLE_DEG_to_Q15(deg)       ((q15_t)((deg * 65536) / 360))

#define SERVO_ANGLE_LIMIT_MINIMUM   ANGLE_DEG_to_Q15(-40)
#define SERVO_ANGLE_LIMIT_MAXIMUM   ANGLE_DEG_to_Q15(40)

#define SERVO_PULSE_WIDTH_ZERO      (90000)
#define SERVO_PULSE_WIDTH_NEG_90    (30000)
#define SERVO_PULSE_WIDTH_POS_90    (150000)

#define SERVO_A_ZERO_OFFSET         (int32_t)(-1400)
#define SERVO_B_ZERO_OFFSET         (int32_t)(4000)
#define SERVO_C_ZERO_OFFSET         (int32_t)(900)


// ******************************************************************
// Section: Data Type Definitions
// ******************************************************************
typedef struct {
    q15_t command_angle;
} servo_state_t;


// ******************************************************************
// Section: Private Variables
// ******************************************************************
static TaskHandle_t servo_taskHandle = NULL;
static StaticTask_t servo_taskBuffer;
static StackType_t  servo_taskStack[ SERVO_RTOS_STACK_SIZE ];

static servo_state_t servos[SERVO_ID_NUM_SERVOS];


// ******************************************************************
// Section: Private Function Declarations
// ******************************************************************
static void SERVO_RTOS_Task( void * pvParameters );


// ******************************************************************
// Section: Command Portal Function Declarations
// ******************************************************************
static void SERVO_CMD_Position_GetSet_q15angle( void );
static void SERVO_CMD_Position_Zero( void );


// ******************************************************************
// Section: Public Functions
// ******************************************************************

void SERVO_Initialize( void )
{
    servo_id_t index;
    
    for( index = 0; index < SERVO_ID_NUM_SERVOS; index++ )
    {
        servos[index].command_angle = 0;
    }

    CMD_RegisterCommand( "servo", SERVO_CMD_Position_GetSet_q15angle );
    CMD_RegisterCommand( "servo-zero", SERVO_CMD_Position_Zero );

    servo_taskHandle = xTaskCreateStatic(
        SERVO_RTOS_Task,        /* Function that implements the task. */
        "Servo",                /* Text name for the task. */
        SERVO_RTOS_STACK_SIZE,  /* Number of indexes in the stack. */
        NULL,                     /* Parameter passed into the task. */
        SERVO_RTOS_PRIORITY,    /* Priority at which the task is created. */
        servo_taskStack,         /* Array to use as the task's stack. */
        &servo_taskBuffer        /* Variable to hold the task's data structure. */
    );
}

void SERVO_Disable( void )
{
}

void SERVO_Enable( void )
{
}

q15_t SERVO_Position_Command_Get_q15angle( servo_id_t servo_id )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return 0;
    }

    return servos[servo_id].command_angle;
}

void SERVO_Position_Command_Set_q15angle( servo_id_t servo_id, q15_t angle )
{
    int32_t duty;

    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return;
    }

    if( angle >= SERVO_ANGLE_LIMIT_MAXIMUM )
    {
        angle = SERVO_ANGLE_LIMIT_MAXIMUM;
    }
    else if( angle <= SERVO_ANGLE_LIMIT_MINIMUM )
    {
        angle = SERVO_ANGLE_LIMIT_MINIMUM;
    }
    
    servos[servo_id].command_angle = angle;

    duty = (((int64_t)angle * (SERVO_PULSE_WIDTH_POS_90 - SERVO_PULSE_WIDTH_NEG_90)) >> 15) + SERVO_PULSE_WIDTH_ZERO;

    switch( servo_id )
    {
        case SERVO_ID_A:
        {
            TCC1_PWM24bitDutySet( TCC1_CHANNEL0, (uint32_t)(duty + SERVO_A_ZERO_OFFSET) );
            break;
        }
        case SERVO_ID_B:
        {
            TCC1_PWM24bitDutySet( TCC1_CHANNEL1, (uint32_t)(duty + SERVO_B_ZERO_OFFSET) );
            break;
        }
        case SERVO_ID_C:
        {
            TCC1_PWM24bitDutySet( TCC1_CHANNEL2, (uint32_t)(duty + SERVO_C_ZERO_OFFSET) );
            break;
        }
        default:
        {
            break;
        }
    }
}

void SERVO_Position_Zero_Set( servo_id_t servo_id )
{
    if( servo_id >= SERVO_ID_NUM_SERVOS )
    {
        return;
    }
    
    servos[servo_id].command_angle = 0;
}


// ******************************************************************
// Section: Private Functions
// ******************************************************************

static void SERVO_RTOS_Task( void * pvParameters )
{
    (void)pvParameters;
    
    vTaskDelay( pdMS_TO_TICKS(SERVO_POWER_UP_DELAY_mS) );

    TCC1_PWM24bitDutySet( TCC1_CHANNEL0, SERVO_PULSE_WIDTH_ZERO + SERVO_A_ZERO_OFFSET );
    TCC1_PWM24bitDutySet( TCC1_CHANNEL1, SERVO_PULSE_WIDTH_ZERO + SERVO_B_ZERO_OFFSET );
    TCC1_PWM24bitDutySet( TCC1_CHANNEL2, SERVO_PULSE_WIDTH_ZERO + SERVO_C_ZERO_OFFSET );
    TCC1_PWMStart();

    // Servo module is completely interrupt driven. Don't need the task running any more.
    vTaskSuspend( NULL );
}


// ******************************************************************
// Section: Command Portal Functions
// ******************************************************************

static void SERVO_CMD_Position_GetSet_q15angle( void )
{
    char arg_buffer[10];

    if( CMD_GetArgc() >= 2 )
    {
        servo_id_t servo_id;

        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        servo_id = (servo_id_t)atoi( arg_buffer );
        
        if (CMD_GetArgc() == 2 )
        {
            CMD_PrintString( "ANGLE: 0x", true );
            CMD_PrintHex_U16( SERVO_Position_Command_Get_q15angle((servo_id_t)atoi( arg_buffer )), true );
            CMD_PrintString( "\r\n", true );
        }
        else
        {
            q15_t angle;
            
            CMD_GetArgv( 2, arg_buffer, sizeof(arg_buffer) );
            angle = (q15_t)atoi( arg_buffer );

            SERVO_Position_Command_Set_q15angle( servo_id, angle );
        }
    }
}

static void SERVO_CMD_Position_Zero( void )
{
    char arg_buffer[10];
    
    if( CMD_GetArgc() >= 2 )
    {
        CMD_GetArgv( 1, arg_buffer, sizeof(arg_buffer) );
        SERVO_Position_Zero_Set(  (servo_id_t)atoi( arg_buffer ) );
    }
}
