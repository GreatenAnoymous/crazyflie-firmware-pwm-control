/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2016 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * power_distribution_stock.c - Crazyflie stock power distribution code
 */
#include "power_distribution.h"

#include "log.h"
#include "param.h"
#include "num.h"

#include "motors.h"

static bool motorSetEnable = false;

uint32_t packedMotors;
uint16_t stamp_tx;			// add declaration for stamp variable
int32_t controlTick = 0;


static struct {
  uint32_t m1;
  uint32_t m2;
  uint32_t m3;
  uint32_t m4;
} motorPower;

static struct {
  uint16_t m1;
  uint16_t m2;
  uint16_t m3;
  uint16_t m4;
} motorPowerSet;

void powerDistributionInit(void)
{
  motorsInit(motorMapDefaultBrushed);
}

bool powerDistributionTest(void)
{
  bool pass = true;

  pass &= motorsTest();

  return pass;
}

#define limitThrust(VAL) limitUint16(VAL)

void powerStop()
{
  motorsSetRatio(MOTOR_M1, 0);
  motorsSetRatio(MOTOR_M2, 0);
  motorsSetRatio(MOTOR_M3, 0);
  motorsSetRatio(MOTOR_M4, 0);
}


void custPowerDistribution(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4, uint32_t tick)
{
    motorPower.m1 = (uint16_t) limitThrust(m1);
    motorPower.m2 = (uint16_t) limitThrust(m2);
    motorPower.m3 = (uint16_t) limitThrust(m3);
    motorPower.m4 = (uint16_t) limitThrust(m4);


    if(motorPower.m1 > 60000){
    	motorPower.m1 = 60000;
    }

    if(motorPower.m2 > 60000){
      motorPower.m2 = 60000;
    }

    if(motorPower.m3 > 60000){
      motorPower.m3 = 60000;
    }

    if(motorPower.m4 > 60000){
      motorPower.m4 = 60000;
    }

    if (motorSetEnable)
    {
      motorsSetRatio(MOTOR_M1, (uint16_t) motorPowerSet.m1);
      motorsSetRatio(MOTOR_M2, (uint16_t) motorPowerSet.m2);
      motorsSetRatio(MOTOR_M3, (uint16_t) motorPowerSet.m3);
      motorsSetRatio(MOTOR_M4, (uint16_t) motorPowerSet.m4);
    }
    else
    {
      motorsSetRatio(MOTOR_M1, (uint16_t) motorPower.m1);
      motorsSetRatio(MOTOR_M2, (uint16_t) motorPower.m2);
      motorsSetRatio(MOTOR_M3, (uint16_t) motorPower.m3);
      motorsSetRatio(MOTOR_M4, (uint16_t) motorPower.m4);
    }

    uint32_t tmp_packedMotors =   ((motorPower.m1 & 0xFF00) >> 8)
  		           + ((motorPower.m2 & 0xFF00))
                   + ((motorPower.m3 & 0xFF00) << 8)
  				   + ((motorPower.m4 & 0xFF00) << 16);

    if (tmp_packedMotors != packedMotors){
    	controlTick = (int32_t) tick;
    }
    packedMotors = tmp_packedMotors;

}

void custPowerDistributionTwo(uint16_t m1, uint16_t m2, uint16_t m3, uint16_t m4, uint16_t stamp)
{
    motorPower.m1 = (uint16_t) limitThrust(m1);
    motorPower.m2 = (uint16_t) limitThrust(m2);
    motorPower.m3 = (uint16_t) limitThrust(m3);
    motorPower.m4 = (uint16_t) limitThrust(m4);
    motorPowerSet.m1 = motorPower.m1;
    motorPowerSet.m2 = motorPower.m2;
    motorPowerSet.m3 = motorPower.m3;
    motorPowerSet.m4 = motorPower.m4;

//    if(motorPower.m1 > 60000){
//    	motorPower.m1 = 60000;
//    }
//
//    if(motorPower.m2 > 60000){
//      motorPower.m2 = 60000;
//    }
//
//    if(motorPower.m3 > 60000){
//      motorPower.m3 = 60000;
//    }
//
//    if(motorPower.m4 > 60000){
//      motorPower.m4 = 60000;
//    }

    if (motorSetEnable)
    {
      motorsSetRatio(MOTOR_M1, (uint16_t) motorPowerSet.m1);
      motorsSetRatio(MOTOR_M2, (uint16_t) motorPowerSet.m2);
      motorsSetRatio(MOTOR_M3, (uint16_t) motorPowerSet.m3);
      motorsSetRatio(MOTOR_M4, (uint16_t) motorPowerSet.m4);
    }
    else
    {
      motorsSetRatio(MOTOR_M1, (uint16_t) motorPower.m1);
      motorsSetRatio(MOTOR_M2, (uint16_t) motorPower.m2);
      motorsSetRatio(MOTOR_M3, (uint16_t) motorPower.m3);
      motorsSetRatio(MOTOR_M4, (uint16_t) motorPower.m4);
    }

    uint32_t tmp_packedMotors =   ((motorPower.m1 & 0xFF00) >> 8)
  		           + ((motorPower.m2 & 0xFF00))
                   + ((motorPower.m3 & 0xFF00) << 8)
  				   + ((motorPower.m4 & 0xFF00) << 16);

    packedMotors = tmp_packedMotors;

    stamp_tx = stamp; // pass the stamp to transmit

}

void powerDistribution(const control_t *control, uint32_t tick)
{

	//custPowerDistribution(control->roll, (uint16_t) control->thrust, control->yaw, control->pitch, tick);

  #ifdef QUAD_FORMATION_X
    int16_t r = control->roll / 2.0f;
    int16_t p = control->pitch / 2.0f;
    motorPower.m1 = limitThrust(control->thrust - r + p + control->yaw);
    motorPower.m2 = limitThrust(control->thrust - r - p - control->yaw);
    motorPower.m3 =  limitThrust(control->thrust + r - p + control->yaw);
    motorPower.m4 =  limitThrust(control->thrust + r + p - control->yaw);
  #else // QUAD_FORMATION_NORMAL
    motorPower.m1 = limitThrust(control->thrust + control->pitch +
                               control->yaw);
    motorPower.m2 = limitThrust(control->thrust - control->roll -
                               control->yaw);
    motorPower.m3 =  limitThrust(control->thrust - control->pitch +
                               control->yaw);
    motorPower.m4 =  limitThrust(control->thrust + control->roll -
                               control->yaw);
  #endif

  if (motorSetEnable)
  {
    motorsSetRatio(MOTOR_M1, motorPowerSet.m1);
    motorsSetRatio(MOTOR_M2, motorPowerSet.m2);
    motorsSetRatio(MOTOR_M3, motorPowerSet.m3);
    motorsSetRatio(MOTOR_M4, motorPowerSet.m4);
  }
  else
  {
    motorsSetRatio(MOTOR_M1, motorPower.m1);
    motorsSetRatio(MOTOR_M2, motorPower.m2);
    motorsSetRatio(MOTOR_M3, motorPower.m3);
    motorsSetRatio(MOTOR_M4, motorPower.m4);
  }

}

LOG_GROUP_START(motorCompact) //ADD STAMP
LOG_ADD(LOG_UINT32, pwms, &packedMotors)
LOG_ADD(LOG_UINT16, packet_id, &stamp_tx) // Lambert add stamp to this log block for packet id
LOG_GROUP_STOP(motorCompact)

//LOG_GROUP_START(tickCount)
//LOG_ADD(LOG_UINT32, boomtick, &packedMotors)
//LOG_GROUP_STOP(tickCount)

PARAM_GROUP_START(motorPowerSet)
PARAM_ADD(PARAM_UINT8, enable, &motorSetEnable)
PARAM_ADD(PARAM_UINT16, m1, &motorPowerSet.m1)
PARAM_ADD(PARAM_UINT16, m2, &motorPowerSet.m2)
PARAM_ADD(PARAM_UINT16, m3, &motorPowerSet.m3)
PARAM_ADD(PARAM_UINT16, m4, &motorPowerSet.m4)
PARAM_GROUP_STOP(motorPowerSet)

//LOG_GROUP_START(motor)
//LOG_ADD(LOG_INT32, m4, &motorPower.m4)
//LOG_ADD(LOG_INT32, m1, &controlTick)
//LOG_ADD(LOG_INT32, m2, &motorPower.m2)
//LOG_ADD(LOG_INT32, m3, &motorPower.m3)
//LOG_GROUP_STOP(motor)
