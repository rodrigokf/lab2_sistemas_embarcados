/* Rodrigo Friesen
*  Yuri Andreiko
*
* Laboratorio 2 - Sistemas embarcados  criado em 09/10/2019
*
*/
#define PART_TM4C1294NCPDT

#include <stdint.h>
#include <stdbool.h>
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

void Timer2IntHandler(void)
{
    //
    // Clear the timer interrupt.
    //
    //
    TimerIntClear(TIMER2_BASE, TIMER_CAPA_EVENT);
//TODO: verificar fontes de interrupção, implementar verificação de estouro de timer (incrementar variavel) 


}

//funçao que envia string para a serial
void UART_send_string(uint8_t *string, uint16_t qtd)
{
  while(qtd--)
    UARTCharPut(UART0_BASE, *string++);
}

void calculo_tempo(uint32_t tl_copy, uint32_t th_copy)
{
  uint64_t periodo;
  uint64_t duty_cycle;
  uint64_t frequencia;
  uint64_t tl;
  uint64_t th;
  uint8_t c = 0;
  
  uint8_t duty[3]; //vetor para converter para caracteres
  uint8_t freq[6]; //vetor para converter para caracteres

  
  
  //etapa que converte valor decimal para caractres ASCII
  freq[0] = (frequencia/100000);     //milhão
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
  
  //espera cerca de 2 segundos para fazer nova medição
  sys_delay(2000);
} 

void main(void){
  
  //
  // Enable the peripherals used by this example.
  //
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);

  //
  // Configure PM0 as the CCP0 pin for timer 2.
  //
  // TODO: This is set up to use GPIO PM0 which can be configured
  // as the CCP0 pin for Timer2 and also happens to be attached to
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
  GPIOPadConfigSet(GPIO_PORTM_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

  //
  // Enable processor interrupts.
  //
  IntMasterEnable();

  //
  // Configure the timers in both edges count mode.
  //
  //
  TimerConfigure(TIMER2_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME | TIMER_CFG_B_CAP_TIME)); //quando evento ocorre, da trigger no timer
  TimerControlEvent(TIMER2_BASE, TIMER_A, TIMER_EVENT_POS_EDGE); //borda positiva
  TimerControlEvent(TIMER2_BASE, TIMER_B, TIMER_EVENT_NEG_EDGE); //borda negativa
  
  TimerLoadSet(TIMER2_BASE, TIMER_A, 0);
  TimerLoadSet(TIMER2_BASE, TIMER_B, 0);

  //
  // Setup the interrupt for the edge capture timer.
  // 
  IntEnable(INT_TIMER2A | INT_TIMER2B);
  TimerIntEnable(TIMER2_BASE, TIMER_CAPA_EVENT | TIMER_TIMA_TIMEOUT);
  TimerIntEnable(TIMER2_BASE, TIMER_CAPB_EVENT | TIMER_TIMB_TIMEOUT);

  //
  // Enable the timer.
  //
  //
  TimerEnable(TIMER2_BASE, TIMER_A);
  TimerEnable(TIMER2_BASE, TIMER_B);
    
  
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


  while(1)
  { 
   
  }
   
    
} 
