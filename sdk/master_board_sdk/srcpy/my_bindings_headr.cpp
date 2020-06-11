#include "my_bindings_headr.h"

using namespace boost::python;

// Wrapper to access the adc property, an array of 2 floats (read-only access)) 
boost::python::tuple wrap_adc(MotorDriver const * motDriver) 
{
  boost::python::list a;
  for (int i = 0; i < 2; ++i) 
  {
    a.append(motDriver->adc[i]);
  }
  return boost::python::tuple(a);
}


    BOOST_PYTHON_MODULE(libmaster_board_sdk_pywrap)
    {
        // Bindings for LINK_manager_callback class
        class_<LINK_manager_callback, boost::noncopyable>("LINK_manager_callback", no_init)
            .def("callback", pure_virtual(&LINK_manager_callback::callback)) 
        ;
        // End of bindings for LINK_manager_callback class

        // Bindings for MasterBoardInterface class
        class_<MasterBoardInterface, bases<LINK_manager_callback> >("MasterBoardInterface", init<std::string, optional<bool> >())
            // Methods of MasterBoardInterface class
            .def("Init", &MasterBoardInterface::Init)
            .def("Stop", &MasterBoardInterface::Stop)
            // .def("SetMasterboardTimeoutMS", &MasterBoardInterface::SetMasterboardTimeoutMS) // Not defined in master_board_interface.cpp but declared in master_board_interface.h
            .def("SendCommand", &MasterBoardInterface::SendCommand)
            .def("ParseSensorData", &MasterBoardInterface::ParseSensorData)
            .def("PrintIMU", &MasterBoardInterface::PrintIMU)
            .def("PrintADC", &MasterBoardInterface::PrintADC)
            .def("PrintMotors", &MasterBoardInterface::PrintMotors)
            .def("PrintMotorDrivers", &MasterBoardInterface::PrintMotorDrivers)
            .def("PrintCmdStats", &MasterBoardInterface::PrintCmdStats)
            .def("PrintSensorStats", &MasterBoardInterface::PrintSensorStats)
            .def("ResetTimeout", &MasterBoardInterface::ResetTimeout)
            .def("IsTimeout", &MasterBoardInterface::IsTimeout)
            .def("GetDriver", make_function(&MasterBoardInterface::GetDriver, return_value_policy<boost::python::reference_existing_object>()))
            .def("GetMotor", make_function(&MasterBoardInterface::GetMotor, return_value_policy<boost::python::reference_existing_object>()))
            .def("imu_data_accelerometer", make_function(&MasterBoardInterface::imu_data_accelerometer))
            .def("imu_data_gyroscope", make_function(&MasterBoardInterface::imu_data_gyroscope))
            .def("imu_data_attitude", make_function(&MasterBoardInterface::imu_data_attitude))
            .def("imu_data_linear_acceleration", make_function(&MasterBoardInterface::imu_data_linear_acceleration))

            .def("IsAckMsgReceived", &MasterBoardInterface::IsAckMsgReceived)
            .def("SendInit", &MasterBoardInterface::SendInit)

            .def("ResetPacketLossStats", &MasterBoardInterface::ResetPacketLossStats)

            .def("GetSensorsSent", &MasterBoardInterface::GetSensorsSent)
            .def("GetSensorsLost", &MasterBoardInterface::GetSensorsLost)
            .def("GetCmdSent", &MasterBoardInterface::GetCmdSent)
            .def("GetCmdLost", &MasterBoardInterface::GetCmdLost)
            .def("GetSensorHistogram", &MasterBoardInterface::GetSensorHistogram)
            .def("GetCmdHistogram", &MasterBoardInterface::GetCmdHistogram)
            .def("GetLastRecvCmdIndex", &MasterBoardInterface::GetLastRecvCmdIndex)
            .def("GetCmdPacketIndex", &MasterBoardInterface::GetCmdPacketIndex)
            
            .def("GetSessionId", &MasterBoardInterface::GetSessionId)

        ;
        // End of bindings for MasterBoardInterface class
        
        // Bindings for Motor class
        class_<Motor>("Motor", init<>())
            // Methods of Motor class
            .def("SetCurrentReference", &Motor::SetCurrentReference)
            .def("SetVelocityReference", &Motor::SetVelocityReference)
            .def("SetPositionReference", &Motor::SetPositionReference)
            .def("SetPositionOffset", &Motor::SetPositionOffset)
            // .def("SetKp", &Motor::SetKp) // Not defined in motor.cpp but declared in motor.h
            // .def("SetKd", &Motor::SetKd) // Not defined in motor.cpp but declared in motor.h
            // .def("SetSaturationCurrent", &Motor::SetSaturationCurrent) // Not defined in motor.cpp but declared in motor.h
            .def("SetDriver", &Motor::SetDriver)
            .def("Print", &Motor::Print)
            .def("Enable", &Motor::Enable)
            .def("Disable", &Motor::Disable)
            .def("IsReady", &Motor::IsReady)
            .def("IsEnabled", &Motor::IsEnabled)
            .def("HasIndexBeenDetected", &Motor::HasIndexBeenDetected)
            .def("GetIndexToggleBit", &Motor::GetIndexToggleBit)
            .def("GetPosition", &Motor::GetPosition)
            .def("GetPositionOffset", &Motor::GetPositionOffset)
            .def("GetVelocity", &Motor::GetVelocity)
            .def("GetCurrent", &Motor::GetCurrent)

            // Public properties of Motor class
            .add_property("position", &Motor::get_position, &Motor::set_position)
            .add_property("velocity", &Motor::get_velocity, &Motor::set_velocity)
            .add_property("current", &Motor::get_current, &Motor::set_current)
            .add_property("is_enabled", &Motor::get_is_enabled, &Motor::set_is_enabled)
            .add_property("is_ready", &Motor::get_is_ready, &Motor::set_is_ready)
            .add_property("index_toggle_bit", &Motor::get_index_toggle_bit, &Motor::set_index_toggle_bit)
            .add_property("has_index_been_detected", &Motor::get_has_index_been_detected, &Motor::set_has_index_been_detected)
            .add_property("position_ref", &Motor::get_position_ref, &Motor::set_position_ref)
            .add_property("velocity_ref", &Motor::get_velocity_ref, &Motor::set_velocity_ref)
            .add_property("current_ref", &Motor::get_current_ref, &Motor::set_current_ref)
            .add_property("kp", &Motor::get_kp, &Motor::set_kp)
            .add_property("kd", &Motor::get_kd, &Motor::set_kd)
            .add_property("enable", &Motor::get_enable, &Motor::set_enable)
            .add_property("enable_position_rollover_error", &Motor::get_enable_position_rollover_error, &Motor::set_enable_position_rollover_error)
            .add_property("enable_index_toggle_bit", &Motor::get_enable_index_toggle_bit, &Motor::set_enable_index_toggle_bit)
            .add_property("enable_index_offset_compensation", &Motor::get_enable_index_offset_compensation, &Motor::set_enable_index_offset_compensation)
            .add_property("driver", make_function(&Motor::get_driver, return_value_policy<boost::python::reference_existing_object>()), &Motor::set_driver)
        ;
        // End of bindings for Motor class

        
        // Bindings for MotorDriver class
        class_<MotorDriver>("MotorDriver", init<>())
            // Methods of MotorDriver class
            // .def("EnableMotorDriver", &MotorDriver::EnableMotorDriver) // Not defined in motor_driver.cpp but declared in motor_driver.h
            // .def("DisableMotorDriver", &MotorDriver::DisableMotorDriver) // Not defined in motor_driver.cpp but declared in motor_driver.h
            .def("SetMotors", &MotorDriver::SetMotors)
            .def("Print", &MotorDriver::Print)
            .def("EnablePositionRolloverError", &MotorDriver::EnablePositionRolloverError)
            .def("DisablePositionRolloverError", &MotorDriver::EnablePositionRolloverError)
            .def("SetTimeout", &MotorDriver::SetTimeout)
            .def("IsConnected", &MotorDriver::IsConnected)
            .def("IsEnabled", &MotorDriver::IsEnabled)
            .def("GetErrorCode", &MotorDriver::GetErrorCode)
            .def("Enable", &MotorDriver::Enable)
            .def("Disable", &MotorDriver::Disable)
            
            // Public properties of MotorDriver class
            .add_property("motor1", make_function(&MotorDriver::get_motor1, return_value_policy<boost::python::reference_existing_object>()), &MotorDriver::set_motor1)
            .add_property("motor2", make_function(&MotorDriver::get_motor2, return_value_policy<boost::python::reference_existing_object>()), &MotorDriver::set_motor2)
            .add_property("is_connected", &MotorDriver::get_is_connected, &MotorDriver::set_is_connected)
            .add_property("is_enabled", &MotorDriver::get_is_enabled, &MotorDriver::set_is_enabled)
            .add_property("error_code", &MotorDriver::get_error_code, &MotorDriver::set_error_code)
            .add_property("enable", &MotorDriver::get_enable, &MotorDriver::set_enable)
            .add_property("enable_position_rollover_error", &MotorDriver::get_enable_position_rollover_error, &MotorDriver::set_enable_position_rollover_error)
            .add_property("timeout", &MotorDriver::get_timeout, &MotorDriver::set_timeout)
            .add_property("adc", wrap_adc)
        ;
        // End of bindings for MotorDriver class


    }

