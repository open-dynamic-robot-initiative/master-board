#ifndef MOTOR_H
#define MOTOR_H
#include "master_board_sdk/motor_driver.h"
class MotorDriver;
class Motor
{
public:
  Motor();
  void SetCurrentReference(double);
  void SetVelocityReference(double);
  void SetPositionReference(double);
  void SetPositionOffset(double);
  void SetKp(double);
  void SetKd(double);
  void SetSaturationCurrent(double);
  void SetDriver(MotorDriver *driver);
  void Print();
  void Enable();
  void Disable();

  bool IsReady();
  bool IsEnabled();
  bool HasIndexBeenDetected();
  bool GetIndexToggleBit();
  double GetPosition();
  double GetVelocity();
  double GetCurrent();
  double GetPositionOffset();

  //state
  double position;       // [rad]
  double velocity;       // [rad/s]
  double current;        // [A]
  double position_offset;// [rad]

  bool is_enabled;
  bool is_ready;
  bool index_toggle_bit;
  bool has_index_been_detected;

  //commands
  double position_ref;  // [rad]
  double velocity_ref;  // [rad/s]
  double current_ref;   // [A]
  double current_sat;   // [A]
  double kp;            // [A/rad]
  double kd;            // [As/rad]


  bool enable;
  bool enable_position_rollover_error;
  bool enable_index_toggle_bit;
  bool enable_index_offset_compensation;

  MotorDriver *driver;

  // Set functions for Python binding with Boost
  void set_position(double val) { this->position = val; };
  void set_velocity(double val) { this->velocity = val; };
  void set_current(double val) { this->current = val; };
  void set_is_enabled(bool val) { this->is_enabled = val; };
  void set_is_ready(bool val) { this->is_ready = val; };
  void set_has_index_been_detected(bool val) { this->has_index_been_detected = val; };
  void set_index_toggle_bit(bool val) { this->index_toggle_bit = val; };
  void set_position_ref(double val) { this->position_ref = val; };
  void set_velocity_ref(double val) { this->velocity_ref = val; };
  void set_current_ref(double val) { this->current_ref = val; };
  void set_current_sat(double val) { this->current_sat = val; };
  void set_kp(double val) { this->kp = val; };
  void set_kd(double val) { this->kd = val; };
  void set_enable(bool val) { this->enable = val; };
  void set_enable_position_rollover_error(bool val) { this->enable_position_rollover_error = val; };
  void set_enable_index_toggle_bit(bool val) { this->enable_index_toggle_bit = val; };
  void set_enable_index_offset_compensation(bool val) { this->enable_index_offset_compensation = val; };
  void set_driver(MotorDriver* new_driver) { this->driver = new_driver; }; // driver is a pointer but in Python we will use the object itself

  // Get functions for Python binding with Boost
  double get_position() { return this->position; };
  double get_velocity() { return this->velocity; };
  double get_current() { return this->current; };
  bool get_is_enabled() { return this->is_enabled; };
  bool get_is_ready() { return this->is_ready; };
  bool get_has_index_been_detected() { return this->has_index_been_detected; };
  bool get_index_toggle_bit() { return this->index_toggle_bit; };
  double get_position_ref() { return this->position_ref; };
  double get_velocity_ref() { return this->velocity_ref; };
  double get_current_ref() { return this->current_ref; };
  double get_current_sat() { return this->current_sat; };
  double get_kp() { return this->kp; };
  double get_kd() { return this->kd; };
  bool get_enable() { return this->enable; };
  bool get_enable_position_rollover_error() { return this->enable_position_rollover_error; };
  bool get_enable_index_toggle_bit() { return this->enable_index_toggle_bit; };
  bool get_enable_index_offset_compensation() { return this->enable_index_offset_compensation; };
  MotorDriver* get_driver() { return (this->driver); }; // driver is a pointer but in Python we will use the object itself
  
};

#endif