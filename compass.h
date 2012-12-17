#ifndef COMPASS_H
#define COMPASS_H

void Compass_Init(void);

/**
* @brief Read LSM303DLHC output register, and calculate the acceleration ACC=(1/SENSITIVITY)* (out_h*256+out_l)/16 (12 bit rappresentation)
* @param pnData: pointer to float buffer where to store data
* @retval None
*/
void Compass_ReadAcc(float* pfData);

/**
  * @brief  calculate the magnetic field Magn.
* @param  pfData: pointer to the data out
  * @retval None
  */
void Compass_ReadMag (float* pfData);


#endif
