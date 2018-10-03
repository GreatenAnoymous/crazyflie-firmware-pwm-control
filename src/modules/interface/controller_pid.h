
#ifndef __CONTROLLER_PID_H__
#define __CONTROLLER_PID_H__

#include "stabilizer_types.h"

void controllerPidInit(void);
bool controllerPidTest(void);
void controllerPid(control_t *control, setpoint_t *setpoint,
                                         const sensorData_t *sensors,
                                         const state_t *state,
                                         const uint32_t tick);

#endif //__CONTROLLER_PID_H__
