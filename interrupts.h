#ifndef __STM32F30X_IT_H
#define __STM32F30X_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f30x.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void EXTI0_IRQHandler(void);

void TimingDelay_Decrement(void);
void Delay(__IO uint32_t nTime);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F30X_IT_H */
