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

extern __IO uint8_t currentlyReadingI2C;
extern __IO uint32_t timeReadI2C;
extern __IO uint32_t totalTime;


const uint32_t leds[8] = {LED3, LED4, LED6, LED8, LED10, LED9, LED7, LED5};

/* axes is a rotation matrix from board coords to world coords */
float axes[3][3];
float *const Xaxis = axes[0], *const Yaxis = axes[1], *const Zaxis = axes[2];
float accs[2][3], vels[2][3], poss[2][3], angRates[2][3], angs[2][3], mags[2][3];
float zeroAngRate[3];


void vecCross(float *o, float *a, float *b)
{
    o[0] = a[1]*b[2] - a[2]*b[1];
    o[1] = a[2]*b[0] - a[0]*b[2];
    o[2] = a[0]*b[1] - a[1]*b[0];
}

void vecNegate(float *v)
{
    int i;
    for (i = 0; i < 3; ++i) {
        v[i] = -v[i];
    }
}

float vecDot(const float *a, const float *b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

float vecLen(const float *v)
{
    return sqrt(vecDot(v, v));
}

void vecNorm(float *v)
{
    float len = vecLen(v);
    int i;
    for (i = 0; i < 3; ++i) {
        v[i] /= len;
    }
}

void vecMul(float x[3][3], float *v)
{
    int i;
    float vcopy[3];
    memcpy(vcopy, v, sizeof(vcopy));
    for (i = 0; i < 3; ++i) {
        v[i] = vecDot(vcopy, x[i]);
    }
}

void vecMulTrans(float x[3][3], float *v)
{
    int i, j;
    float vcopy[3];
    memcpy(vcopy, v, sizeof(vcopy));
    for (i = 0; i < 3; ++i) {
        float xtrans[3];
        for (j = 0; j < 3; ++j)
            xtrans[j] = x[j][i];
        v[i] = vecDot(vcopy, xtrans);
    }
}

/*
void vecMulMat(float o[3][3], float x[3][3], float y[3][3])
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            o[i][j] =
                x[i][0]*y[0][j] +
                x[i][1]*y[1][j] +
                x[i][2]*y[2][j];
        }
    }
}

void vecMulMatTrans(float o[3][3], float x[3][3], float y[3][3])
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            o[i][j] =
                x[0][i]*y[0][j] +
                x[1][i]*y[1][j] +
                x[2][i]*y[2][j];
        }
    }
}
*/

void Compass_ReadAccAvg(float *v, int n)
{
    float vals[3] = {};
    int i = n, x;
    memset(v, 0, sizeof(float[3]));
    while (i--) {
        Compass_ReadAcc(vals);
        for (x = 0; x < 3; ++x)
            v[x] += vals[x];
    }
    for (x = 0; x < 3; ++x)
        v[x] /= n;
}

void Compass_ReadMagAvg(float *v, int n)
{
    float vals[3] = {};
    int i = n, x;
    memset(v, 0, sizeof(float[3]));
    while (i--) {
        Compass_ReadMag(vals);
        for (x = 0; x < 3; ++x)
            v[x] += vals[x];
    }
    for (x = 0; x < 3; ++x)
        v[x] /= n;
}

void Gyro_ReadAngRateAvg(float *v, int n)
{
    float vals[3] = {};
    int i = n, x;
    memset(v, 0, sizeof(float[3]));
    while (i--) {
        Gyro_ReadAngRate(vals);
        for (x = 0; x < 3; ++x)
            v[x] += vals[x];
    }
    for (x = 0; x < 3; ++x)
        v[x] /= n;
}

void calibrate()
{
    // TODO: wait for things to stabilise
    //printf("Calibrating\n");
    Compass_ReadAccAvg(Zaxis, 1000);
    vecNorm(Zaxis);
    //printf("Z: %9.3f %9.3f %9.3f\n", Zaxis[0], Zaxis[1], Zaxis[2]);

    Compass_ReadMag(Xaxis);
    vecNorm(Xaxis);
    //printf("X: %9.3f %9.3f %9.3f\n", Xaxis[0], Xaxis[1], Xaxis[2]);
    vecCross(Yaxis, Zaxis, Xaxis);
    vecNorm(Yaxis);
    vecCross(Xaxis, Yaxis, Zaxis);
    vecNorm(Xaxis);

    Gyro_ReadAngRateAvg(zeroAngRate, 100);

    //printf("X: %9.3f %9.3f %9.3f\n", Xaxis[0], Xaxis[1], Xaxis[2]);
    //printf("Y: %9.3f %9.3f %9.3f\n", Yaxis[0], Yaxis[1], Yaxis[2]);
    //printf("Z: %9.3f %9.3f %9.3f\n", Zaxis[0], Zaxis[1], Zaxis[2]);
}


int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

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

    //printf("Starting\n");
    USART1_flush();

    /*
    printf("Initialising USB\n");
    USBHID_Init();
    printf("Initialising USB HID\n");
    Joystick_init();
    */
    
    /* Initialise LEDs */
    //printf("Initialising LEDs\n");
    int i;
    for (i = 0; i < 8; ++i) {
        STM_EVAL_LEDInit(leds[i]);
        STM_EVAL_LEDOff(leds[i]);
    }

    /* Initialise gyro */
    //printf("Initialising gyroscope\n");
    Gyro_Init();

    /* Initialise compass */
    //printf("Initialising compass\n");
    Compass_Init();

    Delay(100);
    calibrate();

    int C = 0, noAccelCount = 0;

    while (1) {
        float *acc = accs[C&1],
              *prevAcc = accs[(C&1)^1],
              *vel = vels[C&1],
              *prevVel = vels[(C&1)^1],
              *pos = poss[C&1],
              *prevPos = poss[(C&1)^1],
              *angRate = angRates[C&1],
              *prevAngRate = angRates[(C&1)^1],
              *ang = angs[C&1],
              *prevAng = angs[(C&1)^1],
              *mag = mags[C&1],
              *prevmag = mags[(C&1)^1];

        /* Wait for data ready */

#if 0
        Compass_ReadAccAvg(acc, 10);
        vecMul(axes, acc);
        //printf("X: %9.3f Y: %9.3f Z: %9.3f\n", acc[0], acc[1], acc[2]);
        
        float grav = acc[2];
        acc[2] = 0;
        
        if (noAccelCount++ > 50) {
            for (i = 0; i < 2; ++i) {
                vel[i] = 0;
                prevVel[i] = 0;
            }
            noAccelCount = 0;
        }

        if (vecLen(acc) > 50.f) {
            for (i = 0; i < 2; ++i) {
                vel[i] += prevAcc[i] + (acc[i]-prevAcc[i])/2.f;
                pos[i] += prevVel[i] + (vel[i]-prevVel[i])/2.f;
            }
            noAccelCount = 0;
        }

        C += 1;
        if (((C) & 0x7F) == 0) {
            printf("%9.3f %9.3f %9.3f %9.3f %9.3f\n", vel[0], vel[1], pos[0], pos[1], grav);
            //printf("%3.1f%% %d %5.1f %6.3f\n", (float) timeReadI2C*100.f / totalTime, C, (float) C*100.f / (totalTime), grav);
        }
#endif

        Compass_ReadMagAvg(mag, 2);
        vecMul(axes, mag);
        float compassAngle = atan2f(mag[1], mag[0]) * 180.f / PI;
        if (compassAngle > 180.f) compassAngle -= 360.f;
        //vecNorm(mag);
        Gyro_ReadAngRateAvg(angRate, 2);
        printf("c%6.3f\ng%6.3f\n", compassAngle, angRate[2]);

#if 0
        Gyro_ReadAngRateAvg(angRate, 2);
        angRate[0] *= 180.f / PI;
        angRate[1] *= 180.f / PI;
        angRate[2] *= 180.f / PI;
        float s[3] = {sin(angRate[0]), sin(angRate[1]), sin(angRate[2])};
        float c[3] = {cos(angRate[0]), cos(angRate[1]), cos(angRate[2])};
        float gyroMat[3][3] = {
            {c[0]*c[1], c[0]*s[1], -s[1]},
            {c[0]*s[1]*s[2]-s[0]*c[2], c[0]*c[2]+s[0]*s[1]*s[2], c[1]*s[2]},
            {c[0]*s[1]*c[2]+s[0]*s[2], -c[0]*s[2]+s[0]*s[1]*c[2], c[1]*c[2]}};
        /*
        float gyroWorldMat[3][3];
        vecMulMatTrans(gyroWorldMat, axes, gyroMat);
        *ang = gyroWorldMat[2][0];
        *ang += gyroWorldMat[2][1];
        *ang += gyroWorldMat[2][2];
        *ang /= 300.f;
        static const float ANGALPHA = 0.0f;
        *ang += ANGALPHA*(compassAngle - *ang);
        */
        float rotObsVec[3];
        memcpy(rotObsVec, axes[0], sizeof(rotObsVec));
        vecMul(gyroMat, rotObsVec);
        vecMul(axes, rotObsVec);
        rotObsVec[2] = 0.f;
        vecNorm(rotObsVec);
        float angDelta = acos(rotObsVec[0]);

        if (((++C) & 0x7) == 0) {
            printf("%6.3f\n", compassAngle);
            printf("%6.3f %6.3f %6.3f %6.3f\n", rotObsVec[0], rotObsVec[1], rotObsVec[2], angDelta);
        }
#endif


#if 0
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
#endif
    }

    return 1;
}



