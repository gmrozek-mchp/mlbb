/*******************************************************************************
 System Tasks File

  File Name:
    freertos_hooks.c

  Summary:
    This file contains source code necessary for FreeRTOS hooks

  Description:

  Remarks:
 *******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"


void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
   ( void ) pcTaskName;
   ( void ) xTask;

   /* Run time task stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a task stack overflow is detected. Note the system/interrupt
   stack is not checked. */
   taskDISABLE_INTERRUPTS();
   for( ;; )
   {
       /* Do Nothing */
   }
}


void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
   volatile unsigned long ul = 0;

   ( void ) pcFile;
   ( void ) ulLine;

   taskENTER_CRITICAL();
   {
      /* Set ul to a non-zero value using the debugger to step out of this
         function. */
      while( ul == 0U )
      {
         portNOP();
      }
   }
   taskEXIT_CRITICAL();
}
