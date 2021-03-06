#include "pca9685.h"
/*!
 *  @brief  Sends a reset command to the PCA9685 chip over I2C
 */
HAL_StatusTypeDef PCA9685_Reset(I2C_HandleTypeDef *hi2c)
{
	uint8_t restart = PCA9685_MODE1_RESTART;
  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, PCA9685_REG_MODE1_ADDR, 1, &restart, 1, 1) != HAL_OK)
	{
    return HAL_ERROR;
  }
  else
  {
    HAL_Delay(10);
    return HAL_OK;
  }
}

/*!
 *  @brief  Initialize PCA9685 chip over I2C
 */
HAL_StatusTypeDef PCA9685_Init(I2C_HandleTypeDef *hi2c)
{
  // Check if device ready
	if (HAL_I2C_IsDeviceReady(hi2c, PCA9685_ADDRESS, 1000, 1) != HAL_OK)
	{
    return HAL_ERROR;
	}
	// Reset chip
	if (PCA9685_Reset(hi2c) != HAL_OK)
	{
		return HAL_ERROR;
	}
  return HAL_OK;
}

/*!
 *  @brief  Sets the PWM output frequency, up to ~1.6 KHz
 *  output_freq = clock_freq/((prescale+1)*4096)
 *  clock_freq can be internal or external oscillator
 *  @param  Frequency Floating point frequency that we will attempt to match
 */
HAL_StatusTypeDef PCA9685_Set_PWM_Freq(I2C_HandleTypeDef *hi2c, float Frequency)
{
  if (Frequency < 1)
    Frequency = 1;
  if (Frequency > 3500)
    Frequency = 3500;

  // Calculate prescale value
  float prescaleval = (PCA9685_INTERNAL_FREQ / (Frequency * 4096.0f)) - 1;
  if (prescaleval < PCA9685_PRESCALE_MIN)
    prescaleval = PCA9685_PRESCALE_MIN;
  if (prescaleval > PCA9685_PRESCALE_MAX)
    prescaleval = PCA9685_PRESCALE_MAX;
  uint8_t prescale = (uint8_t) prescaleval;

  // Write prescale value to PCA9685
  uint8_t oldmode;
  if (HAL_I2C_Mem_Read(hi2c, PCA9685_ADDRESS, PCA9685_REG_MODE1_ADDR, 1, &oldmode, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  uint8_t newmode = (oldmode & ~PCA9685_MODE1_RESTART) | PCA9685_MODE1_SLEEP; // sleep value

  // Go to sleep
  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, PCA9685_REG_MODE1_ADDR, 1, &newmode, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  // Write prescale value
  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, PCA9685_REG_PRESCALE_ADDR, 1, &prescale, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  // Wake up to normal mode
  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, PCA9685_REG_MODE1_ADDR, 1, &oldmode, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  HAL_Delay(5);

  // This sets the MODE1 register to turn on auto increment.
  oldmode = oldmode | PCA9685_MODE1_RESTART | PCA9685_MODE1_AI;
  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, PCA9685_REG_MODE1_ADDR, 1, &oldmode, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}

/*!
 *  @brief  Sets the PWM output pulse
 *  @param  Channel PCA9685 channel number (0-15)
 *  @param  On High output pulse start (0-4095)
 *  @param  Off Low output pulse start (0-4095)
 */
HAL_StatusTypeDef PCA9685_Set_PWM(I2C_HandleTypeDef *hi2c, uint8_t Channel, uint16_t On, uint16_t Off)
{
	if (On > 4095)
		On = 4095;
	if (Off > 4095)
		Off = 4095;
  // LED0_ON_L REG address start from 0x06 - ON (LOW, HIGH) - OFF (LOW, HIGH) is 4 bytes
  uint8_t reg_on_low = PCA9685_REG_LED0_ON_L + Channel*4;
  uint8_t reg_on_high = reg_on_low + 1;
  uint8_t reg_off_low = reg_on_high + 1;
  uint8_t reg_off_high = reg_off_low + 1;

  uint8_t on_low = On & 0xFF;
  uint8_t on_high = (On >> 8) & 0xFF;
  uint8_t off_low =  Off & 0xFF;
  uint8_t off_high = (Off >> 8) & 0xFF;

  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, reg_on_low, 1, &on_low, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, reg_on_high, 1, &on_high, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, reg_off_low, 1, &off_low, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_I2C_Mem_Write(hi2c, PCA9685_ADDRESS, reg_off_high, 1, &off_high, 1, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}

/*!
 *  @brief  Sets the Servo throttle
 *  @param  Channel PCA9685 channel number (0-15)
 *  @param  Throttle Throttle percent (-100 - 100)
 */
HAL_StatusTypeDef Servo_Set_Throttle(I2C_HandleTypeDef *hi2c, uint8_t Channel, int8_t Throttle)
{
	if (Throttle < -100)
		Throttle = -100;
	if (Throttle > 100)
		Throttle = 100;

	uint16_t pulselen = SERVO_PULSE_NEUTRAL + Throttle*(SERVO_PULSE_MAX - SERVO_PULSE_MIN)/200.0f;
	if (pulselen < SERVO_PULSE_MIN)
		pulselen = SERVO_PULSE_MIN;
	if (pulselen > SERVO_PULSE_MAX)
		pulselen = SERVO_PULSE_MAX;

	if (PCA9685_Set_PWM(hi2c, Channel, 0, pulselen) != HAL_OK)
	{
		return HAL_ERROR;
	}

	return HAL_OK;
}
