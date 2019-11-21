#include <math.h>
#include "master_board_sdk/master_board_interface.h"

MasterBoardInterface::MasterBoardInterface(const std::string &if_name)
{
  uint8_t my_mac[6] = {0xa0, 0x1d, 0x48, 0x12, 0xa0, 0xc5}; //take it as an argument?
  uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(this->my_mac_, my_mac, 6);
  memcpy(this->dest_mac_, dest_mac, 6);
  this->if_name_ = if_name;
  for (int i = 0; i < N_SLAVES; i++)
  {
    motors[2 * i].SetDriver(&motor_drivers[i]);
    motors[2 * i + 1].SetDriver(&motor_drivers[i]);
    motor_drivers[i].SetMotors(&motors[2 * i], &motors[2 * i + 1]);
  }
}

int MasterBoardInterface::Init()
{
  printf("if_name: %s\n", if_name_.c_str());
  //reset the variables
  nb_recv = 0;
  last_sensor_index = 0;
  nb_sensors_sent = 0; //this variable deduce the total number of received sensor packet from sensor index and previous sensor index
  nb_sensors_lost = 0;
  nb_cmd_lost_offset = -1;
  last_cmd_lost = 0;
  memset(&command_packet, 0, sizeof(command_packet_t)); //todo make it more C++
  memset(&sensor_packet, 0, sizeof(sensor_packet_t));
  if (if_name_[0] == 'e')
  {
    /*Ethernet*/
    printf("Using Ethernet (%s)", if_name_.c_str());
    link_handler_ = new ETHERNET_manager(if_name_, my_mac_, dest_mac_);
    link_handler_->set_recv_callback(this);
    link_handler_->start();
  }
  else if (if_name_[0] == 'w')
  {
    /*WiFi*/
    printf("Using WiFi (%s)", if_name_.c_str());
    link_handler_ = new ESPNOW_manager(if_name_, DATARATE_24Mbps, CHANNEL_freq_9, my_mac_, dest_mac_, false); //TODO write setter for espnow specific parametters
    link_handler_->set_recv_callback(this);
    link_handler_->start();
    ((ESPNOW_manager *)link_handler_)->bind_filter();
  }
  else
  {
    return -1;
  }
  return 0;
}

void MasterBoardInterface::SendCommand()
{
  //construct the command packet
  for (int i = 0; i < N_SLAVES; i++)
  {
    uint16_t mode = 0;
    if (motor_drivers[i].enable)
    {
      mode |= UD_COMMAND_MODE_ES;
    }
    if (motor_drivers[i].motor1->enable)
    {
      mode |= UD_COMMAND_MODE_EM1;
    }
    if (motor_drivers[i].motor2->enable)
    {
      mode |= UD_COMMAND_MODE_EM2;
    }
    if (motor_drivers[i].enable_position_rollover_error)
    {
      mode |= UD_COMMAND_MODE_EPRE;
    }
    if (motor_drivers[i].motor1->enable_index_offset_compensation)
    {
      mode |= UD_COMMAND_MODE_EI1OC;
    }
    if (motor_drivers[i].motor2->enable_index_offset_compensation)
    {
      mode |= UD_COMMAND_MODE_EI2OC;
    }
    mode |= UD_COMMAND_MODE_TIMEOUT & motor_drivers[i].timeout;
    command_packet.dual_motor_driver_command_packets[i].mode = mode;
    command_packet.dual_motor_driver_command_packets[i].position_ref[0] = FLOAT_TO_D16QN(motor_drivers[i].motor1->position_ref / (2. * M_PI), UD_QN_POS);
    command_packet.dual_motor_driver_command_packets[i].position_ref[1] = FLOAT_TO_D16QN(motor_drivers[i].motor2->position_ref / (2. * M_PI), UD_QN_POS);
    command_packet.dual_motor_driver_command_packets[i].velocity_ref[0] = FLOAT_TO_D16QN(motor_drivers[i].motor1->velocity_ref * 60. / 1000., UD_QN_VEL);
    command_packet.dual_motor_driver_command_packets[i].velocity_ref[1] = FLOAT_TO_D16QN(motor_drivers[i].motor2->velocity_ref * 60. / 1000., UD_QN_VEL);
    command_packet.dual_motor_driver_command_packets[i].current_ref[0] = FLOAT_TO_D16QN(motor_drivers[i].motor1->current_ref, UD_QN_IQ);
    command_packet.dual_motor_driver_command_packets[i].current_ref[1] = FLOAT_TO_D16QN(motor_drivers[i].motor2->current_ref, UD_QN_IQ);
    command_packet.dual_motor_driver_command_packets[i].kp[0] = FLOAT_TO_D16QN(2. * M_PI * motor_drivers[i].motor1->kp, UD_QN_ISAT);
    command_packet.dual_motor_driver_command_packets[i].kp[1] = FLOAT_TO_D16QN(2. * M_PI *motor_drivers[i].motor2->kp, UD_QN_ISAT);
    command_packet.dual_motor_driver_command_packets[i].kd[0] = FLOAT_TO_D16QN(1000. / 60. * motor_drivers[i].motor1->kd, UD_QN_ISAT);
    command_packet.dual_motor_driver_command_packets[i].kd[1] = FLOAT_TO_D16QN(1000. / 60. * motor_drivers[i].motor2->kd, UD_QN_ISAT);
  }
  link_handler_->send((uint8_t *)&command_packet, sizeof(command_packet_t));
}

void MasterBoardInterface::callback(uint8_t src_mac[6], uint8_t *data, int len)
{
  if (len != sizeof(sensor_packet_t))
  {
    printf("received a %d long packet\n", len);
    return;
  }

  nb_recv++;
  sensor_packet_mutex.lock();
  memcpy(&sensor_packet, data, sizeof(sensor_packet_t));
  sensor_packet_mutex.unlock();
}

void MasterBoardInterface::ParseSensorData()
{
  sensor_packet_mutex.lock();
  /*Read IMU data*/
  for (int i = 0; i < 3; i++)
  {
    imu_data.accelerometer[i] = D16QN_TO_FLOAT(sensor_packet.imu.accelerometer[i], IMU_QN_ACC);
    imu_data.gyroscope[i] = D16QN_TO_FLOAT(sensor_packet.imu.gyroscope[i], IMU_QN_GYR);
    imu_data.attitude[i] = D16QN_TO_FLOAT(sensor_packet.imu.attitude[i], IMU_QN_EF);
  }

  //Read Motor Driver Data
  for (int i = 0; i < N_SLAVES; i++)
  {
    //dual motor driver
    motor_drivers[i].is_enabled = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_SE;
    motor_drivers[i].error_code = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_ERROR;

    //motor 1
    motor_drivers[i].motor1->position = 2. * M_PI * D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].position[0], UD_QN_POS);
    motor_drivers[i].motor1->velocity = 1000. / 60. * D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].velocity[0], UD_QN_VEL);
    motor_drivers[i].motor1->current = D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].current[0], UD_QN_IQ);
    motor_drivers[i].motor1->is_enabled = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_M1E;
    motor_drivers[i].motor1->is_ready = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_M1R;
    motor_drivers[i].motor1->has_index_been_detected = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_IDX1D;
    motor_drivers[i].motor1->index_toggle_bit = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_IDX1T;

    //motor 2
    motor_drivers[i].motor2->position = 2. * M_PI * D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].position[1], UD_QN_POS);
    motor_drivers[i].motor2->velocity = 1000. / 60. * D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].velocity[1], UD_QN_VEL);
    motor_drivers[i].motor2->current = D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].current[1], UD_QN_IQ);
    motor_drivers[i].motor2->is_enabled = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_M2E;
    motor_drivers[i].motor2->is_ready = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_M2R;
    motor_drivers[i].motor2->has_index_been_detected = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_IDX2D;
    motor_drivers[i].motor2->index_toggle_bit = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_IDX2T;
  }
  /*Stat on Packet loss*/

  sensor_packet_mutex.unlock();
}

void MasterBoardInterface::PrintIMU()
{
  printf("IMU:% 6.3f % 6.3f % 6.3f % 6.3f % 6.3f % 6.3f % 6.3f % 6.3f % 6.3f\n",
         imu_data.accelerometer[0],
         imu_data.accelerometer[1],
         imu_data.accelerometer[2],
         imu_data.gyroscope[0],
         imu_data.gyroscope[1],
         imu_data.gyroscope[2],
         imu_data.attitude[0],
         imu_data.attitude[1],
         imu_data.attitude[2]);
}
void MasterBoardInterface::PrintMotors()
{
  for (int i = 0; i < (2 * N_SLAVES); i++)
  {
    printf("Motor % 2.2d -> ", i);
    motors[i].Print();
  }
}
void MasterBoardInterface::PrintMotorDrivers()
{
  for (int i = 0; i < N_SLAVES; i++)
  {
    printf("Motor Driver % 2.2d -> ", i);
    motor_drivers[i].Print();
  }
}