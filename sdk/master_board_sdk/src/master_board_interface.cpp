#include <math.h>
#include <signal.h>
#include "master_board_sdk/master_board_interface.h"

MasterBoardInterface* MasterBoardInterface::instance = NULL;

MasterBoardInterface::MasterBoardInterface(const std::string &if_name, bool listener_mode)
{
  uint8_t my_mac[6] = {0xa0, 0x1d, 0x48, 0x12, 0xa0, 0xc5}; //take it as an argument?
  uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(this->my_mac_, my_mac, 6);
  memcpy(this->dest_mac_, dest_mac, 6);
  this->if_name_ = if_name;
  this->listener_mode = listener_mode;
  for (int i = 0; i < N_SLAVES; i++)
  {
    motors[2 * i].SetDriver(&motor_drivers[i]);
    motors[2 * i + 1].SetDriver(&motor_drivers[i]);
    motor_drivers[i].SetMotors(&motors[2 * i], &motors[2 * i + 1]);
  }
  instance = this;
}
MasterBoardInterface::MasterBoardInterface(const MasterBoardInterface &to_be_copied) : MasterBoardInterface::MasterBoardInterface(to_be_copied.if_name_, to_be_copied.listener_mode)
{
}

MasterBoardInterface::~MasterBoardInterface()
{
  delete(link_handler_);
}

void MasterBoardInterface::GenerateSessionId()
{
  // 0 is the default in the master board, it should not be used
  do
  {
    // number of milliseconds since 01/01/1970 00:00:00, casted in 16 bits
    session_id = (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  } while (session_id == 0);
}

int MasterBoardInterface::Init()
{
  printf("if_name: %s\n", if_name_.c_str());
  ResetPacketLossStats();

  memset(&command_packet, 0, sizeof(command_packet_t)); //todo make it more C++
  memset(&sensor_packet, 0, sizeof(sensor_packet_t));

  memset(&init_packet, 0, sizeof(init_packet_t));
  memset(&ack_packet, 0, sizeof(ack_packet_t));

  timeout = false;
  first_command_sent_ = false;
  init_sent = false;
  ack_received = false;

  session_id = -1; // default is -1, which means not set
  if (!listener_mode)
    GenerateSessionId(); // we don't generate a session id, we will get it from the received packets

  if (if_name_[0] == 'e')
  {
    /*Ethernet*/
    printf("Using Ethernet (%s)\n", if_name_.c_str());
    link_handler_ = new ETHERNET_manager(if_name_, my_mac_, dest_mac_);
    link_handler_->set_recv_callback(this);
    link_handler_->start();
  }
  else if (if_name_[0] == 'w')
  {
    /*WiFi*/
    printf("Using WiFi (%s)\n", if_name_.c_str());
    link_handler_ = new ESPNOW_manager(if_name_, DATARATE_24Mbps, CHANNEL_freq_9, my_mac_, dest_mac_, false); //TODO write setter for espnow specific parametters
    link_handler_->set_recv_callback(this);
    link_handler_->start();
    ((ESPNOW_manager *)link_handler_)->bind_filter();
  }
  else
  {
    return -1;
  }
  signal(SIGINT, KeyboardStop);
  return 0;
}

int MasterBoardInterface::Stop()
{
  printf("Shutting down connection (%s)\n", if_name_.c_str());
  if (link_handler_ != NULL)
    link_handler_->stop();
  return 0;
}

void MasterBoardInterface::KeyboardStop(int signum)
{
  printf("Keyboard Interrupt\n");
  instance->Stop();
  printf("-- End of script --\n");
  delete(instance->link_handler_);
  exit(0);
}

int MasterBoardInterface::SendInit()
{
  if (!init_sent)
  {
    t_last_packet = std::chrono::high_resolution_clock::now();
    init_sent = true;
  }

  if (timeout)
  {
    return -1;
  }

  init_packet.protocol_version = PROTOCOL_VERSION;
  init_packet.session_id = session_id;

  // Current time point
  std::chrono::high_resolution_clock::time_point t_send_packet = std::chrono::high_resolution_clock::now();

  // Assess time duration since the last packet has been received
  std::chrono::duration<double, std::milli> time_span = t_send_packet - t_last_packet;

  // If this duration is greater than the timeout limit duration
  // then the packet is not sent and the connection with the master board is closed
  if (time_span > t_before_shutdown_ack)
  {
    timeout = true;
    Stop();
    return -1; // Return -1 since the command has not been sent.
  }

  link_handler_->send((uint8_t *)&init_packet, sizeof(init_packet_t));
  return 0;
}

int MasterBoardInterface::SendCommand()
{

  if (listener_mode)
    return -1; // we don't allow sending commands in listener mode

  // If SendCommand is not called every N milli-second we shutdown the
  // connexion. This check is performed only from the first time SendCommand
  // is called. See the comment below for more information.

  if (!first_command_sent_)
  {
    t_last_packet = std::chrono::high_resolution_clock::now();
    first_command_sent_ = true;
  }

  // If the MasterBoardInterface has been shutdown due to a timeout we don't
  // want to send another command before the user manually reset the timeout
  // state
  if (timeout)
  {
    return -1; // Return -1 since the command has not been sent.
  }

  command_packet.session_id = session_id;

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
    command_packet.dual_motor_driver_command_packets[i].position_ref[0] = FLOAT_TO_D16QN(motor_drivers[i].motor1->position_ref / (2. * M_PI), UD_QN_POS) - motor_drivers[i].motor1->position_offset;
    command_packet.dual_motor_driver_command_packets[i].position_ref[1] = FLOAT_TO_D16QN(motor_drivers[i].motor2->position_ref / (2. * M_PI), UD_QN_POS) - motor_drivers[i].motor2->position_offset;
    command_packet.dual_motor_driver_command_packets[i].velocity_ref[0] = FLOAT_TO_D16QN(motor_drivers[i].motor1->velocity_ref * 60. / (2. * M_PI * 1000.), UD_QN_VEL);
    command_packet.dual_motor_driver_command_packets[i].velocity_ref[1] = FLOAT_TO_D16QN(motor_drivers[i].motor2->velocity_ref * 60. / (2. * M_PI * 1000.), UD_QN_VEL);
    command_packet.dual_motor_driver_command_packets[i].current_ref[0] = FLOAT_TO_D16QN(motor_drivers[i].motor1->current_ref, UD_QN_IQ);
    command_packet.dual_motor_driver_command_packets[i].current_ref[1] = FLOAT_TO_D16QN(motor_drivers[i].motor2->current_ref, UD_QN_IQ);
    command_packet.dual_motor_driver_command_packets[i].kp[0] = FLOAT_TO_D16QN(2. * M_PI * motor_drivers[i].motor1->kp, UD_QN_ISAT);
    command_packet.dual_motor_driver_command_packets[i].kp[1] = FLOAT_TO_D16QN(2. * M_PI * motor_drivers[i].motor2->kp, UD_QN_ISAT);
    command_packet.dual_motor_driver_command_packets[i].kd[0] = FLOAT_TO_D16QN(2. * M_PI * 1000. / 60. * motor_drivers[i].motor1->kd, UD_QN_ISAT);
    command_packet.dual_motor_driver_command_packets[i].kd[1] = FLOAT_TO_D16QN(2. * M_PI * 1000. / 60. * motor_drivers[i].motor2->kd, UD_QN_ISAT);
  }

  // Current time point
  std::chrono::high_resolution_clock::time_point t_send_packet = std::chrono::high_resolution_clock::now();

  // Assess time duration since the last packet has been received
  std::chrono::duration<double, std::milli> time_span = t_send_packet - t_last_packet;

  // If this duration is greater than the timeout limit duration
  // then the packet is not sent and the connection with the master board is closed
  if (time_span > t_before_shutdown_control)
  {
    timeout = true;
    Stop();
    return -1; // Return -1 since the command has not been sent.
  }
  command_packet.command_index = cmd_packet_index;
  link_handler_->send((uint8_t *)&command_packet, sizeof(command_packet_t));
  cmd_packet_index++;
  nb_cmd_sent++;
  return 0; // Return 0 since the command has been sent.
}

void MasterBoardInterface::callback(uint8_t src_mac[6], uint8_t *data, int len)
{
  if ((listener_mode || (init_sent && !ack_received)) && len == sizeof(ack_packet_t))
  {
    if (listener_mode)
    {
      // ack packets are used to set up the session id in listener mode
      session_id = ((ack_packet_t *)data)->session_id;
    }
    else
    {
      // ensuring that session id is right if in normal mode
      if (((ack_packet_t *)data)->session_id != session_id)
      {
        //printf("Wrong session id in ack msg, got %d instead of %d ignoring packet\n", ((ack_packet_t *)data)->session_id, session_id);
        return; // ignoring the packet
      }
    }

    // reset variables for feedback on packet loss
    ResetPacketLossStats();

    received_packet_mutex.lock();
    memcpy(&ack_packet, data, sizeof(ack_packet_t));
    received_packet_mutex.unlock();

    // parse ack data
    for (int i = 0; i < N_SLAVES; i++)
    {
      motor_drivers[i].is_connected = (ack_packet.spi_connected & (1 << i)) >> i;
    }

    ack_received = true;
  }

  else if ((listener_mode || (init_sent && ack_received)) && len == sizeof(sensor_packet_t))
  {
    // special cases for listener mode
    if (listener_mode)
    {
      // if the interface session id is not set
      if (session_id == -1)
        session_id = ((sensor_packet_t *)data)->session_id; // if we launch the interface in listener mode while the masterboard is running
                                                            // session_id is set to the one of the first sensor packet received

      // if current session id is not 0 and the received session id is 0, the master board has rebooted
      else if (session_id != 0 && ((sensor_packet_t *)data)->session_id == 0)
      {
        session_id = 0;
        ResetPacketLossStats();
      }
    }

    if (((sensor_packet_t *)data)->session_id != session_id)
    {
      //printf("Wrong session id in sensor msg, got %d instead of %d, ignoring packet\n", ((sensor_packet_t *)data)->session_id, session_id);
      return; // ignoring the packet
    }

    // Update time point of the latest received packet
    t_last_packet = std::chrono::high_resolution_clock::now();

    nb_sensors_recv++;

    received_packet_mutex.lock();
    memcpy(&sensor_packet, data, sizeof(sensor_packet_t));
    received_packet_mutex.unlock();

    struct sensor_packet_t *packet_recv = (struct sensor_packet_t *)data;

    if (!first_sensor_received)
    {
      first_sensor_received = true;
      last_sensor_index = packet_recv->sensor_index - 1; //initialisation of last_sensor_index at first reception
      last_cmd_packet_loss = packet_recv->packet_loss;
    }

    //Sensor_loss
    if (packet_recv->sensor_index - last_sensor_index - 1 != 0)
    {
      if ((packet_recv->sensor_index - last_sensor_index - 2) >= MAX_HIST)
      {
        histogram_lost_sensor_packets[MAX_HIST - 1]++; //add all sequence too big at the end of the histogram
      }
      else
      {
        histogram_lost_sensor_packets[packet_recv->sensor_index - last_sensor_index - 2]++; // index 0 -> 1 packet lost !
      }
    }
    
    //Command_loss
    if (packet_recv->packet_loss != last_cmd_packet_loss)
    {
      if ((packet_recv->packet_loss - last_cmd_packet_loss - 1) >= MAX_HIST)
      {
        histogram_lost_cmd_packets[MAX_HIST - 1]++; //add all sequence too big at the end of the histogram
      }
      else
      {
        histogram_lost_cmd_packets[packet_recv->packet_loss - last_cmd_packet_loss - 1]++; // index 0 -> 1 packet lost !
      }
    }

    nb_cmd_lost += uint16_t(packet_recv->packet_loss - last_cmd_packet_loss);
    last_cmd_packet_loss = packet_recv->packet_loss;

    nb_sensors_lost += uint16_t(packet_recv->sensor_index - last_sensor_index - 1);
    nb_sensors_sent += uint16_t(packet_recv->sensor_index - last_sensor_index);
    last_sensor_index = packet_recv->sensor_index;

    last_recv_cmd_index = packet_recv->last_cmd_index;
  }
  else
  {
    // packet not the right size for the situation
  }
}

void MasterBoardInterface::ParseSensorData()
{
  received_packet_mutex.lock();

  /*Read IMU data*/
  for (int i = 0; i < 3; i++)
  {
    imu_data.accelerometer[i] = 9.80665 * D16QN_TO_FLOAT(sensor_packet.imu.accelerometer[i], IMU_QN_ACC);
    imu_data.gyroscope[i] = D16QN_TO_FLOAT(sensor_packet.imu.gyroscope[i], IMU_QN_GYR);
    imu_data.attitude[i] = D16QN_TO_FLOAT(sensor_packet.imu.attitude[i], IMU_QN_EF);
    imu_data.linear_acceleration[i] = D16QN_TO_FLOAT(sensor_packet.imu.linear_acceleration[i], IMU_QN_ACC);
  }

  //Read Motor Driver Data
  for (int i = 0; i < N_SLAVES; i++)
  {
    //dual motor driver
    motor_drivers[i].is_enabled = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_SE;
    motor_drivers[i].error_code = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_ERROR;

    //acd
    motor_drivers[i].adc[0] = D16QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].adc[0], UD_QN_ADC);
    motor_drivers[i].adc[1] = D16QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].adc[1], UD_QN_ADC);

    //motor 1
    motor_drivers[i].motor1->position = motor_drivers[i].motor1->position_offset + 2. * M_PI * D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].position[0], UD_QN_POS);
    motor_drivers[i].motor1->velocity = 2. * M_PI * 1000. / 60. * D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].velocity[0], UD_QN_VEL);
    motor_drivers[i].motor1->current = D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].current[0], UD_QN_IQ);
    motor_drivers[i].motor1->is_enabled = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_M1E;
    motor_drivers[i].motor1->is_ready = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_M1R;
    motor_drivers[i].motor1->has_index_been_detected = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_IDX1D;
    motor_drivers[i].motor1->index_toggle_bit = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_IDX1T;

    //motor 2
    motor_drivers[i].motor2->position = motor_drivers[i].motor2->position_offset + 2. * M_PI * D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].position[1], UD_QN_POS);
    motor_drivers[i].motor2->velocity = 2. * M_PI * 1000. / 60. * D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].velocity[1], UD_QN_VEL);
    motor_drivers[i].motor2->current = D32QN_TO_FLOAT(sensor_packet.dual_motor_driver_sensor_packets[i].current[1], UD_QN_IQ);
    motor_drivers[i].motor2->is_enabled = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_M2E;
    motor_drivers[i].motor2->is_ready = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_M2R;
    motor_drivers[i].motor2->has_index_been_detected = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_IDX2D;
    motor_drivers[i].motor2->index_toggle_bit = sensor_packet.dual_motor_driver_sensor_packets[i].status & UD_SENSOR_STATUS_IDX2T;
  }
  /*Stat on Packet loss*/

  received_packet_mutex.unlock();
}

void MasterBoardInterface::PrintIMU()
{
  printf("IMU:% 6.3f % 6.3f % 6.3f - % 6.3f % 6.3f % 6.3f - % 6.3f % 6.3f % 6.3f - % 6.3f % 6.3f % 6.3f\n",
         imu_data.accelerometer[0],
         imu_data.accelerometer[1],
         imu_data.accelerometer[2],
         imu_data.gyroscope[0],
         imu_data.gyroscope[1],
         imu_data.gyroscope[2],
         imu_data.attitude[0],
         imu_data.attitude[1],
         imu_data.attitude[2],
         imu_data.linear_acceleration[0],
         imu_data.linear_acceleration[1],
         imu_data.linear_acceleration[2]);
}

void MasterBoardInterface::PrintADC()
{
  for (int i = 0; i < N_SLAVES; i++)
  {
    if (!motor_drivers[i].is_connected)
      continue;

    printf("ADC %2.2d -> %6.3f % 6.3f\n",
           i, motor_drivers[i].adc[0], motor_drivers[i].adc[1]);
  }
}

void MasterBoardInterface::PrintMotors()
{
  for (int i = 0; i < N_SLAVES; i++)
  {
    if (!motor_drivers[i].is_connected)
      continue;

    printf("Motor % 2.2d -> ", 2 *i);
    motors[2 * i].Print();
    printf("Motor % 2.2d -> ", 2 * i + 1);
    motors[2 * i + 1].Print();
  }
}

void MasterBoardInterface::PrintMotorDrivers()
{
  for (int i = 0; i < N_SLAVES; i++)
  {
    if (!motor_drivers[i].is_connected)
      continue;

    printf("Motor Driver % 2.2d -> ", i);
    motor_drivers[i].Print();
  }
}

void MasterBoardInterface::ResetTimeout()
{
  printf("Resetting f_name: %s\n", if_name_.c_str());
  timeout = false;                                           // Resetting timeout variable to be able to send packets again
  t_last_packet = std::chrono::high_resolution_clock::now(); // Resetting time of last packet
  Init();
}

bool MasterBoardInterface::IsTimeout()
{
  // Return true if the MasterBoardInterface has been shut down due to timeout
  return timeout;
}

bool MasterBoardInterface::IsAckMsgReceived()
{
  return ack_received;
}

int MasterBoardInterface::GetSessionId()
{
  return session_id;
}

void MasterBoardInterface::set_motors(Motor input_motors[])
{
  for (int i = 0; i < (2 * N_SLAVES); i++)
  {
    printf("Motor % 2.2d -> ", i);
    (this->motors)[i] = input_motors[i];
  }
}

void MasterBoardInterface::set_motor_drivers(MotorDriver input_motor_drivers[])
{
  for (int i = 0; i < N_SLAVES; i++)
  {
    printf("Motor Driver % 2.2d -> ", i);
    (this->motor_drivers)[i] = input_motor_drivers[i];
  }
}

void MasterBoardInterface::PrintSensorStats()
{
  printf("Sensors lost : %u, sent : %u, loss ratio : %.02f\n", nb_sensors_lost, nb_sensors_sent, nb_sensors_sent != 0 ? 100. * nb_sensors_lost / nb_sensors_sent : 0);

  printf("Histogram sensor : ");
  for (int i = 0; i < MAX_HIST; i++)
  {
    printf("%d ", histogram_lost_sensor_packets[i]);
  }
  printf("\n");
}

void MasterBoardInterface::PrintCmdStats()
{
  if (!listener_mode)
  {
    printf("Commands lost : %u, sent : %u, loss ratio : %.02f\n", nb_cmd_lost, nb_cmd_sent, nb_cmd_sent != 0 ? 100. * nb_cmd_lost / nb_cmd_sent : 0);
  }
  else
  {
    printf("Commands lost : %u\n", nb_cmd_lost);
  }

  printf("Histogram command : ");
  for (int i = 0; i < MAX_HIST; i++)
  {
    printf("%d ", histogram_lost_cmd_packets[i]);
  }
  printf("\n");
}

uint32_t MasterBoardInterface::GetSensorsSent() { return nb_sensors_sent; }

uint32_t MasterBoardInterface::GetSensorsLost() { return nb_sensors_lost; }

uint32_t MasterBoardInterface::GetCmdSent() { return nb_cmd_sent; }

uint32_t MasterBoardInterface::GetCmdLost() { return nb_cmd_lost; }

uint16_t MasterBoardInterface::GetLastRecvCmdIndex() { return last_recv_cmd_index; }

uint16_t MasterBoardInterface::GetCmdPacketIndex() { return cmd_packet_index; }

int MasterBoardInterface::GetSensorHistogram(int index)
{
  if (index >= MAX_HIST)
  {
    return -1; //prevents user from being out of range
  }
  return histogram_lost_sensor_packets[index];
}

int MasterBoardInterface::GetCmdHistogram(int index)
{
  if (index >= MAX_HIST)
  {
    return -1; //prevents user from being out of range
  }
  return histogram_lost_cmd_packets[index];
}

void MasterBoardInterface::ResetPacketLossStats()
{
  //reset the variables
  first_sensor_received = false;

  nb_sensors_recv = 0;
  last_sensor_index = 0;
  nb_sensors_sent = 0;
  nb_sensors_lost = 0;
  memset(histogram_lost_sensor_packets, 0, MAX_HIST * sizeof(int));

  nb_cmd_sent = 0;
  nb_cmd_lost = 0;
  memset(histogram_lost_cmd_packets, 0, MAX_HIST * sizeof(int));
}
