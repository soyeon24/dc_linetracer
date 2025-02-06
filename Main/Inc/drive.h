/*
 * drive.h
 */

#ifndef INC_DRIVE_H_
#define INC_DRIVE_H_

void Drive_First();
void state_debug();
void  mark_check();

extern int32_t positionCenter[15];

extern volatile float accel_setting;
extern volatile float deccel_Setting;
extern volatile float target_velocity_setting;
extern volatile float pit_in_line;

void velocity_test();

#endif /* INC_DRIVE_H_ */
