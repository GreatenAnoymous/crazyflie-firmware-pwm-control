/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie Firmware
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
 *
 */
#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

#include "system.h"
#include "log.h"
#include "param.h"

#include "stabilizer.h"

#include "num.h"

#include "sensors.h"
#include "commander.h"
#include "crtp_localization_service.h"
#include "sitaw.h"
#include "controller.h"
#include "power_distribution.h"

#include "estimator_kalman.h"
#include "estimator.h"

static bool isInit;
static bool emergencyStop = false;
static int emergencyStopTimeout = EMERGENCY_STOP_TIMEOUT_DISABLED;

// State variables for the stabilizer
static setpoint_t setpoint;
static sensorData_t sensorData;
static state_t state;
static control_t control;
//static Axis3f gyroAccumulatorSENT;
//static Axis3f accAccumulatorSENT;


static StateEstimatorType estimatorType;
static ControllerType controllerType;


static uint32_t packedImuL;
static uint32_t packedImuA;
static uint32_t packedYPR;

static void stabilizerTask(void* param);

void stabilizerInit(StateEstimatorType estimator)
{
  if(isInit)
    return;

  sensorsInit();
  stateEstimatorInit(estimator);
  controllerInit(ControllerTypeAny);

  powerDistributionInit();
  if (estimator == kalmanEstimator)
  {
    sitAwInit();
  }
  estimatorType = getStateEstimator();
  controllerType = getControllerType();

  xTaskCreate(stabilizerTask, STABILIZER_TASK_NAME,
              STABILIZER_TASK_STACKSIZE, NULL, STABILIZER_TASK_PRI, NULL);

  isInit = true;


}

bool stabilizerTest(void)
{
  bool pass = true;

  pass &= sensorsTest();
  pass &= stateEstimatorTest();
  pass &= controllerTest();
  //pass &= powerDistributionTest();

  return pass;
}

static void checkEmergencyStopTimeout()
{
  if (emergencyStopTimeout >= 0) {
    emergencyStopTimeout -= 1;

    if (emergencyStopTimeout == 0) {
      emergencyStop = true;
    }
  }
}

/* The stabilizer loop runs at 1kHz (stock) or 500Hz (kalman). It is the
 * responsibility of the different functions to run slower by skipping call
 * (ie. returning without modifying the output structure).
 */

static void stabilizerTask(void* param)
{
  uint32_t tick;
  uint32_t lastWakeTime;
  vTaskSetApplicationTaskTag(0, (void*)TASK_STABILIZER_ID_NBR);

  //Wait for the system to be fully started to start stabilization loop
  systemWaitStart();

  // Wait for sensors to be calibrated
  lastWakeTime = xTaskGetTickCount ();
  while(!sensorsAreCalibrated()) {
    vTaskDelayUntil(&lastWakeTime, F2T(RATE_MAIN_LOOP));
  }
  // Initialize tick to something else then 0
  tick = 1;

  while(1) {
    vTaskDelayUntil(&lastWakeTime, F2T(RATE_MAIN_LOOP));

    // allow to update estimator dynamically
//    if (getStateEstimator() != estimatorType) {
//      stateEstimatorInit(estimatorType);
//      estimatorType = getStateEstimator();
//    }
//    // allow to update controller dynamically
//    if (getControllerType() != controllerType) {
//      controllerInit(controllerType);
//      controllerType = getControllerType();
//    }

//    getExtPosition(&state);
   stateEstimator(&state, &sensorData, &control, tick); //This updates the state data

    // This seems to be where the problem is. Motors updated well faster than setpoint
//    commanderGetSetpoint(&setpoint, &state);	//Not needed; PWMs are passed directly in packet handler

    // Below changes the setpoint in specific states
//    sitAwUpdateSetpoint(&setpoint, &sensorData, &state);

   	//if(controllerType == ControllerTypePID){

//   		uint16_t rawThrust = 35000;
//   		setpoint->thrust = rawThrust;
//   	    setpoint->velocity.z = ((float) rawThrust - 32767.f) / 32767.f;
//
//   		controller(&control, &setpoint, &sensorData, &state, tick);
   	//}

    uint16_t x, y, z, ax, ay, az;
    float lin_min = -20;
    float lin_max =  20;

    float ang_min = -360;
    float ang_max =  360;

    // convert from Gs to m/s^2 state.acc.z
    x  = (constrain(sensorData.acc.x * 9.81f, lin_min, lin_max) + 20) * 25.6f;
    y  = (constrain(sensorData.acc.y * 9.81f, lin_min, lin_max) + 20) * 25.6f;
    z  = (constrain(sensorData.acc.z * 9.81f, lin_min, lin_max) + 20) * 25.6f;


    ax = (constrain(sensorData.gyro.x, ang_min, ang_max) + 360) * 1.42f;
    ay = (constrain(sensorData.gyro.y, ang_min, ang_max) + 360) * 1.42f;
    az = (constrain(sensorData.gyro.z, ang_min, ang_max) + 360) * 1.42f;

//    x  = (constrain(accAccumulatorSENT.x * 9.81f, lin_min, lin_max) + 50) * 10.23f;
//	y  = (constrain(accAccumulatorSENT.y * 9.81f, lin_min, lin_max) + 50) * 10.23f;
//	z  = (constrain(accAccumulatorSENT.z * 9.81f, lin_min, lin_max) + 50) * 10.23f;
//
//
//	ax = (constrain(gyroAccumulatorSENT.x, ang_min, ang_max) + 360) * 1.42f;
//	ay = (constrain(gyroAccumulatorSENT.y, ang_min, ang_max) + 360) * 1.42f;
//	az = (constrain(gyroAccumulatorSENT.z, ang_min, ang_max) + 360) * 1.42f;


    packedImuL =   (x & 0b1111111111)
    	        + ((y & 0b1111111111) << 10)
			    + ((z & 0b1111111111) << 20);

	packedImuA =   (ax & 0b1111111111)
			    + ((ay & 0b1111111111) << 10)
			    + ((az & 0b1111111111) << 20);

	float y_max = 180;
	float y_min = -180;
	float pr_min = -90;
	float pr_max =  90;

	// PACKAGE YPR
	uint16_t py, pp, pr;  // holders for packed values

	// grab yaw pitch and roll
//	py = state.attitude.yaw;
//	pp = state.attitude.pitch;
//	pr = state.attitude.roll;
	// Trying constrained version
	py = (constrain(state.attitude.yaw, y_min, y_max) + 180) * 2.84f;
	pp = (constrain(state.attitude.pitch, pr_min, pr_max) + 90) * 2*2.84f;
	pr = (constrain(state.attitude.roll, pr_min, pr_max) + 90) * 2*2.84f;

	// package
	packedYPR =   (py & 0b1111111111)
    	       + ((pp & 0b1111111111) << 10)
    		   + ((pr & 0b1111111111) << 20);



    checkEmergencyStopTimeout();



    if (emergencyStop) {
      powerStop();
    } else {
    	//Dont call this here if you do it in crtp_commander_rpyt
    	//powerDistribution(&control,tick);
    	//custPowerDistribution(3000.0, 0.0, 0.0, 0.0);*/
    }

    tick++;
  }
}

void stabilizerSetEmergencyStop()
{
  emergencyStop = true;
}

void stabilizerResetEmergencyStop()
{
  emergencyStop = false;
}

void stabilizerSetEmergencyStopTimeout(int timeout)
{
  emergencyStop = false;
  emergencyStopTimeout = timeout;
}

PARAM_GROUP_START(stabilizer)
PARAM_ADD(PARAM_UINT8, estimator, &estimatorType)
PARAM_ADD(PARAM_UINT8, controller, &controllerType)
PARAM_GROUP_STOP(stabilizer)

//LOG_GROUP_START(ctrltarget)
//LOG_ADD(LOG_FLOAT, roll, &setpoint.attitude.roll)
//LOG_ADD(LOG_FLOAT, pitch, &setpoint.attitude.pitch)
//LOG_ADD(LOG_FLOAT, yaw, &setpoint.attitudeRate.yaw)
//LOG_ADD(LOG_FLOAT, thrust, &setpoint.thrust)
//LOG_GROUP_STOP(ctrltarget)

LOG_GROUP_START(compactImu)
LOG_ADD(LOG_UINT32, l_xyz, &packedImuL) // removing linear accels from packets TODO NOL
LOG_ADD(LOG_UINT32, a_xyz, &packedImuA)
LOG_GROUP_STOP(compactImu)

// going back to floats for Euler angles
LOG_GROUP_START(stabilizer)						// TODO NOL make these ints, and don't include thrust
//LOG_ADD(LOG_UINT32, pypr, &packedYPR)
LOG_ADD(LOG_FLOAT, yaw, &state.attitude.yaw)
LOG_ADD(LOG_FLOAT, pitch, &state.attitude.pitch)
LOG_ADD(LOG_FLOAT, roll, &state.attitude.roll)
LOG_GROUP_STOP(stabilizer)
//
//LOG_GROUP_START(stabilizer)						// TODO NOL make these ints, and don't include thrust
//LOG_ADD(LOG_FLOAT, roll, &state.attitude.roll)
//LOG_ADD(LOG_FLOAT, pitch, &state.attitude.pitch)
//LOG_ADD(LOG_FLOAT, yaw, &state.attitude.yaw)
//LOG_ADD(LOG_UINT16, thrust, &control.thrust)
//LOG_GROUP_STOP(stabilizer)

//LOG_GROUP_START(acc)
//LOG_ADD(LOG_FLOAT, x, &sensorData.acc.x)
//LOG_ADD(LOG_FLOAT, y, &sensorData.acc.y)
//LOG_ADD(LOG_FLOAT, z, &sensorData.acc.z)
//LOG_GROUP_STOP(acc)

#ifdef LOG_SEC_IMU
LOG_GROUP_START(accSec)
LOG_ADD(LOG_FLOAT, x, &sensorData.accSec.x)
LOG_ADD(LOG_FLOAT, y, &sensorData.accSec.y)
LOG_ADD(LOG_FLOAT, z, &sensorData.accSec.z)
LOG_GROUP_STOP(accSec)
#endif

//LOG_GROUP_START(baro)
//LOG_ADD(LOG_FLOAT, asl, &sensorData.baro.asl)
//LOG_ADD(LOG_FLOAT, temp, &sensorData.baro.temperature)
//LOG_ADD(LOG_FLOAT, pressure, &sensorData.baro.pressure)
//LOG_GROUP_STOP(baro)
//
//LOG_GROUP_START(gyro)
//LOG_ADD(LOG_FLOAT, x, &sensorData.gyro.x)
//LOG_ADD(LOG_FLOAT, y, &sensorData.gyro.y)
//LOG_ADD(LOG_FLOAT, z, &sensorData.gyro.z)
//LOG_GROUP_STOP(gyro)

#ifdef LOG_SEC_IMU
LOG_GROUP_START(gyroSec)
LOG_ADD(LOG_FLOAT, x, &sensorData.gyroSec.x)
LOG_ADD(LOG_FLOAT, y, &sensorData.gyroSec.y)
LOG_ADD(LOG_FLOAT, z, &sensorData.gyroSec.z)
LOG_GROUP_STOP(gyroSec)
#endif

//LOG_GROUP_START(mag)
//LOG_ADD(LOG_FLOAT, x, &sensorData.mag.x)
//LOG_ADD(LOG_FLOAT, y, &sensorData.mag.y)
//LOG_ADD(LOG_FLOAT, z, &sensorData.mag.z)
//LOG_GROUP_STOP(mag)
//
//LOG_GROUP_START(controller)
//LOG_ADD(LOG_INT16, ctr_yaw, &control.yaw)
//LOG_GROUP_STOP(controller)

//LOG_GROUP_START(stateEstimate)
//LOG_ADD(LOG_FLOAT, x, &state.position.x)
//LOG_ADD(LOG_FLOAT, y, &state.position.y)
//LOG_ADD(LOG_FLOAT, z, &state.position.z)
//LOG_GROUP_STOP(stateEstimate)
