/* Rodrigo Friesen
*  Yuri Andreiko
*
* Laboratorio 2 - Sistemas embarcados  criado em 09/10/2019
*
*/
#define PART_TM4C1294NCPDT

#include <stdint.h>
#include <stdbool.h>
#include <intrinsics.h>
// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"  
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "system_TM4C1294.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"

uint32_t vetor[3] = {0,0,0};
uint32_t tempoSubida = 0;
uint32_t tempoDescida = 0;
uint32_t contadorTimeOut_subida = 0;
uint32_t contadorTimeOut_descida = 0;

//delay em ms (aproximado)
void sys_delay(uint32_t temp)
{
#ifdef FREQ_120M
  temp *= 15000;
#else
  temp *= 3000;
#endif
  while(temp--)
  {}
}

void TIMER2A_Handler(void)
{
    //
    // Clear the timer interrupt.
    TimerIntClear(TIMER2_BASE, TIMER_CAPA_EVENT);
    
    contadorTimeOut_subida = 0;
    
    vetor[0] = TimerValueGet(TIMER2_BASE, TIMER_A);
    if(vetor[0] > vetor[1])
    {
      tempoSubida = vetor[0] - vetor[1];
    }
    else
      tempoSubida = vetor[0] + 0xFFFF - vetor[1];
    
    vetor[1] = vetor[0];
    
//TODO: verificar fontes de interrup��o, implementar verifica��o de estouro de timer (incrementar variavel) 
}

void TIMER2B_Handler(void)
{
    // Clear the timer interrupt.
    TimerIntClear(TIMER2_BASE, TIMER_CAPB_EVENT);
    
    contadorTimeOut_descida = 0;
    
    vetor[2] = TimerValueGet(TIMER2_BASE, TIMER_B);
    if(vetor[2] > vetor[0])
    {
      tempoDescida = vetor[2] - vetor[0];
    }
    else
      tempoDescida = vetor[2] + 0xFFFF - vetor[0];
   
//TODO: verificar fontes de interrup��o, implementar verifica��o de estouro de timer (incrementar variavel) 
}

//fun�ao que envia string para a serial
void UART_send_string(uint8_t *string, uint16_t qtd)
{
  while(qtd--)
    UARTCharPut(UART0_BASE, *string++);
}

void calculo_tempo(uint32_t tl_copy, uint32_t th_copy)
{
  uint64_t duty_cycle;
  uint64_t frequencia;
  uint8_t c = 0;
  uint16_t offset = 0;
  
  uint8_t duty[3]; //vetor para converter para caracteres
  uint8_t freq[6]; //vetor para converter para caracteres
  

  
  if(contadorTimeOut_subida > 1000000 || contadorTimeOut_descida > 1000000)
  {
    frequencia = 0;
    if(contadorTimeOut_subida < contadorTimeOut_descida)
    {
      duty_cycle = 99999;
    }
    else
    {
      duty_cycle = 0;
    }
  }
  else
  {
    frequencia = (uint64_t)100000000000/(tempoSubida * 8333); //nanosegundos *1000;
    offset = (frequencia/10);
  
    duty_cycle = (uint64_t)((tempoDescida)*100000);
    duty_cycle /= (uint64_t)tempoSubida;
    duty_cycle += offset;
  }
  
  //etapa que converte valor decimal para caractres ASCII
  freq[0] = (frequencia/100000);     //milh�o
  frequencia -= freq[0] * 100000;
  freq[1] = (frequencia/10000);     //milhar
  frequencia -= freq[1] * 10000;
  freq[2] = (frequencia/1000);     //centena
  frequencia -= freq[2] * 1000;
  freq[3] = (frequencia/100);      //dezena
  frequencia -= freq[3] * 100;
  freq[4] = (frequencia/10);       //unidade
  frequencia -= freq[4] * 10;
  freq[5] = frequencia;          //decimal
  
  for(c = 0; c < 6; c++)          //converte para ascii
    freq[c] += 0x30;
  
  duty[0] = (duty_cycle/10000);      //dezena
  duty_cycle -= duty[0] * 10000;
  duty[1] = (duty_cycle/1000);       //unidade
  duty_cycle -= duty[1] * 1000;
  duty[2] = duty_cycle/100;
  
  duty[0] += 0x30;                 //converte para ascii
  duty[1] += 0x30;
  duty[2] += 0x30;
  
  //escreve na serial a frequencia
  UART_send_string("freq = ", 7);
  UART_send_string(freq, 4);
  UART_send_string(".", 1);
  UART_send_string(&freq[4], 2); 
  UART_send_string("kHz", 3);
  UART_send_string("\r\n", 2);

  //escreve na serial o duty cycle
  UART_send_string("duty cycle = ", 13);
  UART_send_string(duty, 2);
  UART_send_string(".", 1);
  UART_send_string(&duty[2], 1);
  UART_send_string("%", 1);
  UART_send_string("\r\n", 2);
} 

void SysTick_Handler(void)
{
  calculo_tempo(0,0);
} // SysTick_Handler

void main(void)
{
  __disable_interrupt();
  //
  // HABILITA UART0.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  sys_delay(10);

  //
  // Set GPIO A0 and A1 as UART pins.
  //
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  sys_delay(10);

  //
  // Configure the UART for 115,200, 8-N-1 operation.
  //
  UARTConfigSetExpClk(UART0_BASE, SystemCoreClock , 115200,
                          (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                           UART_CONFIG_PAR_NONE));
  sys_delay(5);
  UART_send_string("inicio", 6);
  UART_send_string("\r\n", 2);
  
  SysTickPeriodSet(4290000000); // f = 1Hz para clock = 24MHz
  
  SysTickIntEnable();
  SysTickEnable();
  
  //
  // Enable the peripherals used by this example.
  //
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
  
  //
  // Configure PM0 as the CCP0 pin for timer 2.
  //
  // TODO: This is set up to use GPIO PM0 and PM1 which can be configured
  // as the CCP0 and CCP1 pin for Timer2 and also happens to be attached to
  // a switch on the EK-LM4F232 board.  Change this configuration to
  // correspond to the correct pin for your application.
  //
  GPIOPinTypeTimer(GPIO_PORTM_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  GPIOPinConfigure(GPIO_PM0_T2CCP0);
  GPIOPinConfigure(GPIO_PM1_T2CCP1);
  
  //
  // Set the pin to use the internal pull-up.
  //
  //
  //GPIOPadConfigSet(GPIO_PORTM_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

  //
  // Enable processor interrupts.
  //
  //IntMasterEnable();
  
  //
  // Configure the timers in both edges count mode.
  //
  //
  TimerConfigure(TIMER2_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP | TIMER_CFG_B_CAP_TIME_UP)); //quando evento ocorre, da trigger no timer
  TimerControlEvent(TIMER2_BASE, TIMER_A, TIMER_EVENT_POS_EDGE); //borda positiva
  TimerControlEvent(TIMER2_BASE, TIMER_B, TIMER_EVENT_NEG_EDGE); //borda negativa
  
//  TimerLoadSet(TIMER2_BASE, TIMER_A, 0);
//  TimerLoadSet(TIMER2_BASE, TIMER_B, 0);

  //
  // Setup the interrupt for the edge capture timer.
  // 
  IntEnable(INT_TIMER2A);
  IntEnable(INT_TIMER2B);
  TimerIntEnable(TIMER2_BASE, TIMER_CAPA_EVENT | TIMER_TIMA_TIMEOUT);
  TimerIntEnable(TIMER2_BASE, TIMER_CAPB_EVENT | TIMER_TIMB_TIMEOUT);
 
  //
  // Enable the timer.
  //
  //
  TimerEnable(TIMER2_BASE, TIMER_A);
  TimerEnable(TIMER2_BASE, TIMER_B);
  
  __enable_interrupt();
  while(1)
  { 
    contadorTimeOut_subida++;
    contadorTimeOut_descida++;
  }
   
    
} 
