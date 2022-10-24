/*
 * deferred_interrupt.c
 * Created for Milestone 1 demo.
 *  Created on: Oct 19, 2022
 *      Author: gzm20
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "driverlib.h"


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

    xTaskCreate(mainTask, "mainTask", 128, NULL, 2, NULL );
    xTaskCreate(deferTask, "deferTask", 128, NULL, 1, NULL );

    vTaskStartScheduler();
    while(1);
}

static void mainTask    (void* pvParams){
    (void) pvParams;
    TickType_t xNextWakeTime = xTaskGetTickCount();

    for(;;){
        CPU_work(8000, GPIO_PORT_P1, GPIO_PIN0);
        vTaskDelayUntil(&xNextWakeTime, 1000 / portTICK_PERIOD_MS);
    }
}

#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{
    xSemaphoreGiveFromISR(xISRSemaphore, NULL);
    GPIO_clearInterrupt(GPIO_PORT_P4, GPIO_PIN1);
}


static void deferTask(void* pvParams){
    (void) pvParams;
    for(;;){
        if(xSemaphoreTake(xISRSemaphore, (TickType_t) ~-1) == pdTRUE){
            CPU_work(32000, GPIO_PORT_P6, GPIO_PIN6);
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
