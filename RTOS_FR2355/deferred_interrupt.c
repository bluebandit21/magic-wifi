/*
 * deferred_interrupt.c
 *
 *  Created on: Oct 19, 2022
 *      Author: gzm20
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

///* Demo includes. */
//#include "partest.h"

/* TI includes. */
#include "driverlib.h"

#define mainTASK_PERIOD_MS  (1000)
#define mainTASK_LENGTH_MS  (500)

#define intTASK_LENGTH_MS   (2000)

SemaphoreHandle_t xISRSemaphore;

void main_defer_interrupt(void);

static void mainTask    (void* pvParams);
static void deferTask   (void* pvParams);
static void CPU_work(int, uint8_t, uint16_t);

void main_defer_interrupt(void){
    if(xISRSemaphore == NULL){
        xISRSemaphore = xSemaphoreCreateBinary();
    }

    P4DIR &= ~BIT1; //input dir
    P4REN |= BIT1; //Pull enabled
    P4OUT |= BIT1; //Pull up
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN1);
    GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN1);


    //xTaskCreate()
    xTaskCreate(
        mainTask
        ,  "mainTask"   // A name just for humans
        ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
        ,  NULL
        ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,  NULL );

       //deferred ISR
      xTaskCreate(
        deferTask
        ,  "deferTask"   // A name just for humans
        ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
        ,  NULL
        ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        ,  NULL );

      vTaskStartScheduler();

      while(1);
}

//xSemaphoreGiveFromISR(xISRSemaphore, NULL);

static void mainTask    (void* pvParams){
    (void) pvParams;
    TickType_t xNextWakeTime = xTaskGetTickCount();

    for(;;){
        CPU_work(500, GPIO_PORT_P1, GPIO_PIN0);
        vTaskDelayUntil(&xNextWakeTime, 1000 / portTICK_PERIOD_MS);
    }
}

#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{
    xSemaphoreGiveFromISR(xISRSemaphore, NULL);
    GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN1);
}


static void deferTask   (void* pvParams){
    (void) pvParams;
    for(;;){
        if(xSemaphoreTake(xISRSemaphore, (TickType_t) ~-1) == pdTRUE){
            CPU_work(2000, GPIO_PORT_P6, GPIO_PIN6);
        }
    }
}


static void CPU_work(int time, uint8_t port, uint16_t pin)
{ //time is ms.
  volatile int i,j,k;
  for(i=0;i<time;i++){
    for(j=0;j<2;j++){
      GPIO_toggleOutputOnPin(port, pin);
      for(k=0;k<25;k++);
    }
  }
}
