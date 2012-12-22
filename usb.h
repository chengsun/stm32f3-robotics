#ifndef USB_H
#define USB_H

#include "usb_lib.h"

void USBHID_Init(void);
void Get_SerialNum(void);
void USB_Interrupts_Config(void);
void USB_Cable_Config (FunctionalState NewState);

void USB_LP_CAN1_RX0_IRQHandler(void);
void USB_LP_IRQHandler(void);
void USBWakeUp_IRQHandler(void);
void USBWakeUp_RMP_IRQHandler(void);



#endif
