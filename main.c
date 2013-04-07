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
    // at rest the accelerometer reports an acceleration straight upwards
    // due to gravity. use this as our Z axis
    Compass_ReadAccAvg(Zaxis, 500);
    vecNorm(Zaxis);

    // the magnetic strength is greatest towards magnetic north. use this as
    // our X axis
    Compass_ReadMagAvg(Xaxis, 10);
    vecNorm(Xaxis);

    // create a Y axis perpendicular to the X and Z axes
    vecCross(Yaxis, Zaxis, Xaxis);
    vecNorm(Yaxis);
    // ensire that the X axis is actually perpendicular to the Y and Z axes
    vecCross(Xaxis, Yaxis, Zaxis);
    vecNorm(Xaxis);

    // calibrate the zero error on the gyroscope
    Gyro_ReadAngRateAvg(zeroAngRate, 100);
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
    // perform calibration
    calibrate();


    while (1) {
        float angRate[3], mag[3];

        // read average compass values
        Compass_ReadMagAvg(mag, 2);
        // rotate the compass values so that they are aligned with Earth
        vecMul(axes, mag);
        // calculate the heading through inverse tan of the Y/X magnetic strength
        float compassAngle = atan2f(mag[1], mag[0]) * 180.f / PI;
        // fix heading to be in range -180 to 180
        if (compassAngle > 180.f) compassAngle -= 360.f;
        // read average gyro values
        Gyro_ReadAngRateAvg(angRate, 2);
        // print out everything
        printf("c%6.3f\ng%6.3f\n", compassAngle, angRate[2]-zeroAngRate[2]);

    }

    return 1;
}



