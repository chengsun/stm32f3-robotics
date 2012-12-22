#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "usb.h"
#include "math.h"
#include "usart.h"
#include "gyro.h"
#include "compass.h"
#include "interrupts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ABS(x)         (x < 0) ? (-x) : x
#define PI             ((float) 3.14159265f)

  RCC_ClocksTypeDef RCC_Clocks;
__IO uint32_t UserButtonPressed = 0;
__IO float HeadingValue = 0.0f;  

__IO uint8_t DataReady = 0;
__IO uint8_t PrevXferComplete = 1;

const uint32_t leds[8] = {LED3, LED4, LED6, LED8, LED10, LED9, LED7, LED5};

int main()
{
    /*!< At this stage the microcontroller clock setting is already configured, 
      this is done through SystemInit() function which is called from startup
      file (startup_stm32f30x.s) before to branch to application main.
      To reconfigure the default setting of SystemInit() function, refer to
      system_stm32f30x.c file
      */ 

    /* SysTick end of count event each 10ms */
    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);

    /* initialise USART1 debug output (TX on pin PA9 and RX on pin PA10) */
    USART1_Init();

    printf("Starting\n");
    fflush(stdout);
    USART1_flush();

    printf("Initialising USB\n");
    USBHID_Init();
    Joystick_init();
    
    /* Initialise LEDs */
    printf("Initialising LEDs\n");
    int i;
    for (i = 0; i < 8; ++i) {
        STM_EVAL_LEDInit(leds[i]);
        STM_EVAL_LEDOff(leds[i]);
    }

    /* Initialise gyro */
    printf("Initialising gyroscope\n");
    Gyro_Init();

    /* Initialise compass */
    printf("Initialising compass\n");
    Compass_Init();


    while (1) {
      /* Wait for data ready */
        Delay(20);

        float angRate[3];
      
      /* Read Gyro Angular data */
      Gyro_ReadAngRate(angRate);

      printf("X: %f Y: %f Z: %f\n", angRate[0], angRate[1], angRate[2]);

        float MagBuffer[3] = {0.0f}, AccBuffer[3] = {0.0f};
        float fNormAcc,fSinRoll,fCosRoll,fSinPitch,fCosPitch = 0.0f, RollAng = 0.0f, PitchAng = 0.0f;
        float fTiltedX,fTiltedY = 0.0f;


      Compass_ReadMag(MagBuffer);
      Compass_ReadAcc(AccBuffer);
      
      for(i=0;i<3;i++)
        AccBuffer[i] /= 100.0f;
      
      fNormAcc = sqrt((AccBuffer[0]*AccBuffer[0])+(AccBuffer[1]*AccBuffer[1])+(AccBuffer[2]*AccBuffer[2]));
      
      fSinRoll = -AccBuffer[1]/fNormAcc;
      fCosRoll = sqrt(1.0-(fSinRoll * fSinRoll));
      fSinPitch = AccBuffer[0]/fNormAcc;
      fCosPitch = sqrt(1.0-(fSinPitch * fSinPitch));
     if ( fSinRoll >0)
     {
       if (fCosRoll>0)
       {
         RollAng = acos(fCosRoll)*180/PI;
       }
       else
       {
         RollAng = acos(fCosRoll)*180/PI + 180;
       }
     }
     else
     {
       if (fCosRoll>0)
       {
         RollAng = acos(fCosRoll)*180/PI + 360;
       }
       else
       {
         RollAng = acos(fCosRoll)*180/PI + 180;
       }
     }
     
      if ( fSinPitch >0)
     {
       if (fCosPitch>0)
       {
            PitchAng = acos(fCosPitch)*180/PI;
       }
       else
       {
          PitchAng = acos(fCosPitch)*180/PI + 180;
       }
     }
     else
     {
       if (fCosPitch>0)
       {
            PitchAng = acos(fCosPitch)*180/PI + 360;
       }
       else
       {
          PitchAng = acos(fCosPitch)*180/PI + 180;
       }
     }

      if (RollAng >=360)
      {
        RollAng = RollAng - 360;
      }
      
      if (PitchAng >=360)
      {
        PitchAng = PitchAng - 360;
      }
      
      fTiltedX = MagBuffer[0]*fCosPitch+MagBuffer[2]*fSinPitch;
      fTiltedY = MagBuffer[0]*fSinRoll*fSinPitch+MagBuffer[1]*fCosRoll-MagBuffer[1]*fSinRoll*fCosPitch;
      
      HeadingValue = (float) ((atan2f((float)fTiltedY,(float)fTiltedX))*180)/PI;
 
        printf("Compass heading: %f\n", HeadingValue);
    }

    return 1;
}



