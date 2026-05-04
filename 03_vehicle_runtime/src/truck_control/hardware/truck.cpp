// Copyright (c) 2021, Stogl Robotics Consulting UG (haftungsbeschränkt)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//
// Authors: Subhas Das, Denis Stogl
//

#include "truck_control/truck.hpp"
#include "Common/Util.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>
#include <algorithm>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

namespace truck_control
{
hardware_interface::CallbackReturn TruckHardware::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (
    hardware_interface::SystemInterface::on_init(info) !=
    hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }
  logger_ = std::make_shared<rclcpp::Logger>(rclcpp::get_logger(
    "controller_manager.resource_manager.hardware_component.system.Truck"));
  clock_ = std::make_shared<rclcpp::Clock>(rclcpp::Clock());

  // BEGIN: This part here is for exemplary purposes - Please do not copy to your production code
  /*hw_start_sec_ = stod(info_.hardware_parameters["example_param_hw_start_duration_sec"]);
  hw_stop_sec_ = stod(info_.hardware_parameters["example_param_hw_stop_duration_sec"]);
  hw_slowdown_ = stod(info_.hardware_parameters["example_param_hw_slowdown"]);
  hw_sensor_change_ = stod(info_.hardware_parameters["example_param_max_sensor_change"]);*/
  // END: This part here is for exemplary purposes - Please do not copy to your production code
  
  // Truck has exactly one GPIO component
  if (info_.gpios.size() != 1)
  {
    RCLCPP_FATAL(
      get_logger(), "Truck has '%ld' GPIO components, '%d' expected.",
      info_.gpios.size(), 1);
    return hardware_interface::CallbackReturn::ERROR;
  }
  // with exactly 3 command interface
  if (info_.gpios[0].command_interfaces.size() != 3)
  {
    RCLCPP_FATAL(
      get_logger(), "GPIO component %s has '%ld' command interfaces, '%d' expected.",
      info_.gpios[0].name.c_str(), info_.gpios[0].command_interfaces.size(), 3);
    return hardware_interface::CallbackReturn::ERROR;
  }
  // and 3 state interfaces
  if (info_.gpios[0].state_interfaces.size() != 3)
  {
    RCLCPP_FATAL(
      get_logger(), "GPIO component %s has '%ld' state interfaces, '%d' expected.",
      info_.gpios[0].name.c_str(), info_.gpios[0].state_interfaces.size(), 3);
    return hardware_interface::CallbackReturn::ERROR;
  }

  _joints.clear();

  for (const hardware_interface::ComponentInfo & joint : info_.joints)
  {
    RCLCPP_INFO(get_logger(), "Found joint '%s'.", joint.name.c_str());
    
    if(joint.name.find("front_wheel_joint") != std::string::npos) 
    {
      const std::vector<std::string> expected_command_interfaces = {hardware_interface::HW_IF_POSITION};
      const std::vector<std::string> expected_state_interfaces   = {hardware_interface::HW_IF_POSITION};
      
      if(validate_joint(joint, expected_command_interfaces, expected_state_interfaces) == hardware_interface::CallbackReturn::SUCCESS)
      {
        _joints.emplace("steering", Joint(joint.name, expected_command_interfaces, expected_state_interfaces, STEERING_PWM_CHANNEL));
      }
      else
      {
        return hardware_interface::CallbackReturn::ERROR;
      }
    }
    else if(joint.name.find("rear_wheel_joint") != std::string::npos)
    {
      const std::vector<std::string> expected_command_interfaces = {hardware_interface::HW_IF_VELOCITY};
      const std::vector<std::string> expected_state_interfaces   = {hardware_interface::HW_IF_POSITION, hardware_interface::HW_IF_VELOCITY};

      if(validate_joint(joint, expected_command_interfaces, expected_state_interfaces) == hardware_interface::CallbackReturn::SUCCESS)
      {
        _joints.emplace("traction", Joint(joint.name, expected_command_interfaces, expected_state_interfaces, TRACTION_PWM_CHANNEL));
      }
      else
      {
        return hardware_interface::CallbackReturn::ERROR;
      }
    }
    else
    {
      std::vector<std::string> expected_command_interfaces;
      const std::vector<std::string> expected_state_interfaces   = {hardware_interface::HW_IF_POSITION, hardware_interface::HW_IF_VELOCITY};

      int pwm_channel = -1;

      if (joint.name == "theta1_slewing_joint")
      {
        pwm_channel = SLEWING_PWM_CHANNEL;
        expected_command_interfaces.push_back(hardware_interface::HW_IF_VELOCITY);
      }
      else if (joint.name == "theta2_boom_joint")
      {
        pwm_channel = BOOM_PWM_CHANNEL;
        expected_command_interfaces.push_back(hardware_interface::HW_IF_VELOCITY);
      }
      else if (joint.name == "theta3_arm_joint")
      {
        pwm_channel = ARM_PWM_CHANNEL;
        expected_command_interfaces.push_back(hardware_interface::HW_IF_VELOCITY);
      }
      else if (joint.name == "q4_big_telescope")
      {
        pwm_channel = TELESCOPE_PWM_CHANNEL;
        expected_command_interfaces.push_back(hardware_interface::HW_IF_VELOCITY);
      }
      else if (joint.name == "theta8_rotator_joint")
      {
        pwm_channel = ROTATOR_PWM_CHANNEL;
        expected_command_interfaces.push_back(hardware_interface::HW_IF_VELOCITY);
      }
      else if (joint.name == "q8")
      {
        pwm_channel = GRIPPER_PWM_CHANNEL;
        expected_command_interfaces.push_back(hardware_interface::HW_IF_VELOCITY);
      }
      else if (joint.name == "theta6_tip_joint")
      {
        
      }
      else if (joint.name == "theta7_tilt_joint")
      {
        
      }
      else if (joint.name == "q9")
      {
        
      }
      else
      {
        RCLCPP_ERROR(get_logger(), "Unexpected joint '%s'.", joint.name.c_str());
        return hardware_interface::CallbackReturn::ERROR;
      }

      if(validate_joint(joint, (const std::vector<std::string>&)expected_command_interfaces, expected_state_interfaces) == hardware_interface::CallbackReturn::SUCCESS)
      {
        _joints.emplace(joint.name, Joint(joint.name, (const std::vector<std::string>&)expected_command_interfaces, expected_state_interfaces, pwm_channel));

        // set initial value
        for (size_t j = 0; j < joint.state_interfaces.size(); ++j)
        {
          const auto & si = joint.state_interfaces[j];

          if (si.name == "position")
          {
            if (!si.initial_value.empty())
            {
              auto initial_value = std::stod(si.initial_value);

              RCLCPP_INFO(get_logger(), "Set default value '%f' for interface '%s[%ld]'.", initial_value, si.name.c_str(), j);
              _joints.at(joint.name).set_state_interface_value(hardware_interface::HW_IF_POSITION, j, initial_value);
            }
            else
            {
              _joints.at(joint.name).set_state_interface_value(hardware_interface::HW_IF_POSITION, j, 0);  // default fallback
            }
          }
        }
      }
      else
      {
        return hardware_interface::CallbackReturn::ERROR;
      }
    }
  }

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn TruckHardware::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(get_logger(), "Configuring ...please wait...");

#ifndef SIMULATE_HARDWARE
  // Initialize PWM channels
  if (check_apm()) 
  {
    RCLCPP_ERROR(get_logger(), "Failed to initialize navio. APM is running.");
    return hardware_interface::CallbackReturn::ERROR;
  }

  _pwm = new PWM();

  if(!_pwm->init(HORN_PWM_CHANNEL)) 
  {
    RCLCPP_ERROR(get_logger(), "Failed to initialize PWM for horn.");
    return hardware_interface::CallbackReturn::ERROR;
  }

  if(!_pwm->init(GEAR_PWM_CHANNEL)) 
  {
    RCLCPP_ERROR(get_logger(), "Failed to initialize PWM for gearbox.");
    return hardware_interface::CallbackReturn::ERROR;
  }

  for(const auto& [id, joint] : _joints)
  {
    int pwm_channel = joint.get_pwm_channel();

    if(pwm_channel >= 0 && !_pwm->init(pwm_channel)) 
    {
      RCLCPP_ERROR(get_logger(), "Failed to initialize PWM for joint %s.", joint.get_name().c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }
  }
#endif

  // reset values always when configuring hardware
  _start_command = 0.0;
  _horn_command = 0.0;
  _gear_command = 1.0;
  _start_state = 0.0;
  _horn_state = 0.0;
  _gear_state = 1.0;
  _enable = false;

  for(auto& [id, joint] : _joints)
  {
    for (auto& interface : *joint.get_command_interfaces()) {
      interface.second = 0;
    }
  }

  RCLCPP_INFO(get_logger(), "Successfully configured!");

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn TruckHardware::on_cleanup(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(get_logger(), "Cleanup ...please wait...");

#ifndef SIMULATE_HARDWARE
  // Uninitalize all PWM channels
  if(!_pwm->deinit(HORN_PWM_CHANNEL)) 
  {
    RCLCPP_ERROR(get_logger(), "Failed to uninitialize PWM for horn.");
    return hardware_interface::CallbackReturn::ERROR;
  }

  if(!_pwm->deinit(GEAR_PWM_CHANNEL)) 
  {
    RCLCPP_ERROR(get_logger(), "Failed to uninitialize PWM for gearbox.");
    return hardware_interface::CallbackReturn::ERROR;
  }

  for(const auto& [id, joint] : _joints)
  {
    int pwm_channel = joint.get_pwm_channel();

    if(pwm_channel >= 0 && _pwm != NULL && !_pwm->deinit(joint.get_pwm_channel())) 
    {
      RCLCPP_ERROR(get_logger(), "Failed to uninitialize PWM for joint %s.", joint.get_name().c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }
  }
#endif

  RCLCPP_INFO(get_logger(), "Successfully cleaned up!");

  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface>
TruckHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;

  // export gpio state interfaces
  for (uint i = 0; i < info_.gpios[0].state_interfaces.size(); i++)
  {
    if(info_.gpios[0].state_interfaces[i].name.find("start") != std::string::npos)
    {
      state_interfaces.emplace_back(hardware_interface::StateInterface(
        info_.gpios[0].name, info_.gpios[0].state_interfaces[i].name, &_start_state));
    }
    else if(info_.gpios[0].state_interfaces[i].name.find("horn") != std::string::npos)
    {
      state_interfaces.emplace_back(hardware_interface::StateInterface(
        info_.gpios[0].name, info_.gpios[0].state_interfaces[i].name, &_horn_state));
    }
    else if(info_.gpios[0].state_interfaces[i].name.find("gear") != std::string::npos)
    {
      state_interfaces.emplace_back(hardware_interface::StateInterface(
        info_.gpios[0].name, info_.gpios[0].state_interfaces[i].name, &_gear_state));
    }
  }

  // export joints state interfaces
  for(auto& [id, joint] : _joints)
  {
    for(auto& interface : *joint.get_state_interfaces())
    {
      state_interfaces.emplace_back(hardware_interface::StateInterface(joint.get_name(), interface.first, &interface.second));
    }
  }

  // export sensors state interfaces TODO
  for (uint i = 0; i < info_.sensors.size(); i++)
  {
    for (uint j = 0; j < info_.sensors[i].state_interfaces.size(); j++)
    {
      //state_interfaces.emplace_back(hardware_interface::StateInterface(
      // info_.sensors[i].name, info_.sensors[i].state_interfaces[j].name, find_joint_by_name(info_.sensors[i].name)->state &hw_sensor_states_[i+j]));
    }
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface>
TruckHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;

  // export gpio command interfaces
  for (uint i = 0; i < info_.gpios[0].command_interfaces.size(); i++)
  {
    if(info_.gpios[0].command_interfaces[i].name.find("start") != std::string::npos)
    {
      command_interfaces.emplace_back(hardware_interface::CommandInterface(
        info_.gpios[0].name, info_.gpios[0].command_interfaces[i].name, &_start_command));
    }
    else if(info_.gpios[0].command_interfaces[i].name.find("horn") != std::string::npos)
    {
      command_interfaces.emplace_back(hardware_interface::CommandInterface(
        info_.gpios[0].name, info_.gpios[0].command_interfaces[i].name, &_horn_command));
    }
    else if(info_.gpios[0].command_interfaces[i].name.find("gear") != std::string::npos)
    {
      command_interfaces.emplace_back(hardware_interface::CommandInterface(
        info_.gpios[0].name, info_.gpios[0].command_interfaces[i].name, &_gear_command));
    }
  }

  // export joints state interfaces
  for(auto& [id, joint] : _joints)
  {
    for(auto& interface : *joint.get_command_interfaces())
    {
      command_interfaces.emplace_back(hardware_interface::CommandInterface(joint.get_name(), interface.first, &interface.second));
    }
  }
  
  return command_interfaces;
}

hardware_interface::CallbackReturn TruckHardware::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(get_logger(), "Activating... please wait...");

#ifndef SIMULATE_HARDWARE
  // Enable PWM channels
  if(enable_pwm(HORN_PWM_CHANNEL) != hardware_interface::CallbackReturn::SUCCESS)
  {
    RCLCPP_ERROR(get_logger(), "Error enabling PWM for horn.");
    return hardware_interface::CallbackReturn::ERROR;
  }


  if(enable_pwm(GEAR_PWM_CHANNEL) != hardware_interface::CallbackReturn::SUCCESS)
  {
    RCLCPP_ERROR(get_logger(), "Error enabling PWM for shift.");
    return hardware_interface::CallbackReturn::ERROR;
  }

  _start_applied_command = 0.0;
  _horn_applied_command = 0.0;
  _gear_applied_command = 2.0;
#endif

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn TruckHardware::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(get_logger(), "Deactivating ...please wait...");
  bool error = false;

#ifndef SIMULATE_HARDWARE
  // Disable all PWM channels
  if(disable_pwm(HORN_PWM_CHANNEL) != hardware_interface::CallbackReturn::SUCCESS)
  {
    RCLCPP_ERROR(get_logger(), "Error disabling PWM for horn.");
    error = true;
  }

  if(disable_pwm(GEAR_PWM_CHANNEL) != hardware_interface::CallbackReturn::SUCCESS)
  {
    RCLCPP_ERROR(get_logger(), "Error disabling PWM for shift.");
    error = true;
  }

  for(const auto& [id, joint] : _joints)
  {
    if (joint.get_pwm_channel() < 0) continue;
    if (id != "steering" && id != "traction") continue;

    if(disable_pwm(joint.get_pwm_channel()) != hardware_interface::CallbackReturn::SUCCESS)
    {
      RCLCPP_ERROR(get_logger(), "Error disabling PWM for joint %s.", joint.get_name().c_str());
      error = true;
    }
  }
#endif

  if(error) { RCLCPP_INFO(get_logger(), "Error while deactivating!"); return hardware_interface::CallbackReturn::ERROR;   }
  else      { RCLCPP_INFO(get_logger(), "Successfully deactivated!"); return hardware_interface::CallbackReturn::SUCCESS; }
}

hardware_interface::return_type TruckHardware::read(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & period)
{
  const double dt = period.seconds();

  auto& steering_joint = _joints.at("steering");
  double position = steering_joint.get_applied_command_interface_values(hardware_interface::HW_IF_POSITION)[0];
  steering_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);

  auto& traction_joint = _joints.at("traction");
  double velocity = traction_joint.get_applied_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0];
  position = traction_joint.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  position = position + velocity * dt;
  traction_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  traction_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  auto& theta1_slewing_joint = _joints.at("theta1_slewing_joint");
  velocity = theta1_slewing_joint.get_applied_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0];
  position = theta1_slewing_joint.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  position = std::clamp(position + velocity * dt, PI-0.8, PI+0.8);
  theta1_slewing_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  theta1_slewing_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  auto& theta2_boom_joint = _joints.at("theta2_boom_joint");
  velocity = theta2_boom_joint.get_applied_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0];
  position = theta2_boom_joint.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  position = std::clamp(position + velocity * dt, 0.0, 1.0);
  theta2_boom_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  theta2_boom_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  auto& theta3_arm_joint = _joints.at("theta3_arm_joint");
  velocity = theta3_arm_joint.get_applied_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0];
  position = theta3_arm_joint.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  position = std::clamp(position + velocity * dt, 0.0, 1.0);
  theta3_arm_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  theta3_arm_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  auto& q4_big_telescope = _joints.at("q4_big_telescope");
  velocity = q4_big_telescope.get_applied_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0];
  position = q4_big_telescope.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  position = std::clamp(position + velocity * dt, 0.0, 0.1);
  q4_big_telescope.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  q4_big_telescope.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  auto& theta6_tip_joint = _joints.at("theta6_tip_joint");
  velocity = 0;
  position = theta6_tip_joint.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  theta6_tip_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  theta6_tip_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  auto& theta7_tilt_joint = _joints.at("theta7_tilt_joint");
  velocity = 0;
  position = theta7_tilt_joint.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  theta7_tilt_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  theta7_tilt_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  auto& theta8_rotator_joint = _joints.at("theta8_rotator_joint");
  velocity = theta8_rotator_joint.get_applied_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0];
  position = theta8_rotator_joint.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  position = std::clamp(position + velocity * dt, -3.0, 3.0);
  theta8_rotator_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  theta8_rotator_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  auto& q8_joint = _joints.at("q8");
  auto& q9_joint = _joints.at("q9");
  velocity = q8_joint.get_applied_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0];
  position = q8_joint.get_state_interface_values(hardware_interface::HW_IF_POSITION)[0];
  position = std::clamp(position + velocity * dt, 0.8, 2.5);
  q8_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  q8_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);
  q9_joint.set_state_interface_value(hardware_interface::HW_IF_POSITION, 0, position);
  q9_joint.set_state_interface_value(hardware_interface::HW_IF_VELOCITY, 0, velocity);

  _start_state = _start_applied_command;
  _horn_state = _horn_applied_command;
  _gear_state = _gear_applied_command;

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type truck_control::TruckHardware::write(
  const rclcpp::Time & time, const rclcpp::Duration & /*period*/)
{
  static double start_time_s;
  static double stop_time_s;
  static double start_drive_back_time_s;
  static bool reverse_mode;
  static bool m_start_command;
  static bool m_enable;
  static double m_traction_command_rad_p_s;

  double traction_velocity_rad_p_s;
  double traction_pwm_period;

  hardware_interface::CallbackReturn start_truck_result;
  hardware_interface::CallbackReturn stop_truck_result;

  const double t = time.seconds();
  const bool enable = _enable;
  const bool start_command = _start_command == 1.0;
  const bool horn_command = _horn_command == 1.0;
  const int gear_command = std::clamp((int)_gear_command, 1, 3);
  const double steering_angle_rad = std::clamp(_joints.at("steering").get_command_interface_values(hardware_interface::HW_IF_POSITION)[0], -25.0, 25.0);
  const double traction_command_rad_p_s = _joints.at("traction").get_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0];
  
  const double slew_velocity = std::clamp(_joints.at("theta1_slewing_joint").get_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0], -0.2, 0.2);
  const double boom_velocity = std::clamp(_joints.at("theta2_boom_joint").get_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0], -0.1, 0.1);
  const double arm_velocity = std::clamp(_joints.at("theta3_arm_joint").get_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0], -0.1, 0.3);
  const double telescope_velocity = std::clamp(_joints.at("q4_big_telescope").get_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0], -0.1, 0.1);
  const double rotator_velocity = std::clamp(_joints.at("theta8_rotator_joint").get_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0], -0.1, 0.1);
  const double gripper_velocity = std::clamp(_joints.at("q8").get_command_interface_values(hardware_interface::HW_IF_VELOCITY)[0], -1.2, 1.2);

  if(!m_start_command && start_command) { start_time_s = t; }
  // else;

  if(m_start_command && !start_command) { stop_time_s = t; }
  // else;

  if(start_command && t - start_time_s >= 3.0) { _enable = true;  }
  if(!start_command && t - stop_time_s >= 0.3) { _enable = false; }
  // else;

  if(start_command && !enable) { _pwm->set_duty_cycle(HORN_PWM_CHANNEL, 1.9); }
  else if(horn_command)        { _pwm->set_duty_cycle(HORN_PWM_CHANNEL, 1.2); }
  else                         { _pwm->set_duty_cycle(HORN_PWM_CHANNEL, 1.5); }

  if(!m_enable && enable) { start_truck_result = start_truck();                               }
  else                    { start_truck_result = hardware_interface::CallbackReturn::SUCCESS; }

  if(start_truck_result != hardware_interface::CallbackReturn::SUCCESS) { return hardware_interface::return_type::ERROR; }
  // else;

  if(m_start_command && !start_command) { disable_pwm(GEAR_PWM_CHANNEL);               }
  else if(m_enable && !enable)          { enable_pwm(GEAR_PWM_CHANNEL);                }
  else if(gear_command == 2)            { _pwm->set_duty_cycle(GEAR_PWM_CHANNEL, 1.5); }
  else if(gear_command == 3)            { _pwm->set_duty_cycle(GEAR_PWM_CHANNEL, 1.9); }
  else                                  { _pwm->set_duty_cycle(GEAR_PWM_CHANNEL, 1.1); }

  if(m_enable && !enable) { stop_truck_result = stop_truck();                                }
  else                    { stop_truck_result = hardware_interface::CallbackReturn::SUCCESS; }

  if(stop_truck_result != hardware_interface::CallbackReturn::SUCCESS) { return hardware_interface::return_type::ERROR; }
  // else;

  if(enable && start_command)
  {
    /* measurement:
     * left limit: PWM: 1.8ms; 26°
     * right limit: PWM: 1.2ms; 24°
     * --> 25° (0.4276 rad) = 0.3ms PWM change
     * --> scaling factor = 0.3 / 0.4276 = 0.7016
     */
    #define STEERING_RAD_TO_PWM ((double) 0.7016)
    #define TRACTION_RAD_P_SEC_TO_PWM ((double) 0.3)

    #define GRIPPER_VELOCITY_TO_PWM ((double) 0.25)

    const double steering_pwm_period = -steering_angle_rad * STEERING_RAD_TO_PWM + 1.5;

    if(!reverse_mode && traction_command_rad_p_s < 0 && m_traction_command_rad_p_s >= 0) { start_drive_back_time_s = t; }
    // else;
    
    if(traction_command_rad_p_s > 0)                                            { reverse_mode = 0; }
    else if(traction_command_rad_p_s < 0 && t - start_drive_back_time_s >= 0.3) { reverse_mode = 1; }
    // else;

    if(traction_command_rad_p_s < 0 && t - start_drive_back_time_s < 0.1) { traction_pwm_period = 1.8; }
    else if(traction_command_rad_p_s < 0 && t - start_drive_back_time_s < 0.3) { traction_pwm_period = 1.5; }
    else                                                                       
    { 
      traction_pwm_period = -traction_command_rad_p_s * TRACTION_RAD_P_SEC_TO_PWM + 1.5; 

      if(traction_pwm_period < 1.4)     { traction_pwm_period = 1.4; }
      else if(traction_pwm_period > 1.65) { traction_pwm_period = 1.65; }
      // else;
    }  // TODO: calculate period

    if(traction_command_rad_p_s < 0 && t - start_drive_back_time_s < 0.3) { traction_velocity_rad_p_s = 0; }
    else  { traction_velocity_rad_p_s = -(traction_pwm_period - 1.5) / TRACTION_RAD_P_SEC_TO_PWM; }

    _pwm->set_duty_cycle(_joints.at("steering").get_pwm_channel(), steering_pwm_period);
    _pwm->set_duty_cycle(_joints.at("traction").get_pwm_channel(), traction_pwm_period);
    _pwm->set_duty_cycle(_joints.at("theta1_slewing_joint").get_pwm_channel(), 1.5 + slew_velocity);
    _pwm->set_duty_cycle(_joints.at("theta2_boom_joint").get_pwm_channel(), 1.5 + boom_velocity);
    _pwm->set_duty_cycle(_joints.at("theta3_arm_joint").get_pwm_channel(), 1.5 + arm_velocity);
    _pwm->set_duty_cycle(_joints.at("q4_big_telescope").get_pwm_channel(), 1.5 + telescope_velocity);
    _pwm->set_duty_cycle(_joints.at("theta8_rotator_joint").get_pwm_channel(), 1.5 + rotator_velocity);
    _pwm->set_duty_cycle(_joints.at("q8").get_pwm_channel(), 1.5 + GRIPPER_VELOCITY_TO_PWM * gripper_velocity);

    // TODO: get value from pwm period; calc traction position
    _joints.at("steering").set_applied_command_interface_value(hardware_interface::HW_IF_POSITION, 0, steering_angle_rad);
    _joints.at("traction").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, traction_velocity_rad_p_s);
    _joints.at("theta1_slewing_joint").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, slew_velocity);
    _joints.at("theta2_boom_joint").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, boom_velocity);
    _joints.at("theta3_arm_joint").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, arm_velocity);
    _joints.at("q4_big_telescope").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, telescope_velocity);
    _joints.at("theta8_rotator_joint").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, rotator_velocity);
    _joints.at("q8").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, gripper_velocity);

    _gear_applied_command = (double)gear_command;
  }
  else
  {
    _pwm->set_duty_cycle(_joints.at("traction").get_pwm_channel(), 1.5);
    _pwm->set_duty_cycle(_joints.at("theta1_slewing_joint").get_pwm_channel(), 1.5);
    _pwm->set_duty_cycle(_joints.at("theta2_boom_joint").get_pwm_channel(), 1.5);
    _pwm->set_duty_cycle(_joints.at("theta3_arm_joint").get_pwm_channel(), 1.5);
    _pwm->set_duty_cycle(_joints.at("q4_big_telescope").get_pwm_channel(), 1.5);
    _pwm->set_duty_cycle(_joints.at("theta8_rotator_joint").get_pwm_channel(), 1.5);
    _pwm->set_duty_cycle(_joints.at("q8").get_pwm_channel(), 1.5);
    
    _joints.at("traction").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, 0);
    _joints.at("theta1_slewing_joint").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, 0);
    _joints.at("theta2_boom_joint").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, 0);
    _joints.at("theta3_arm_joint").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, 0);
    _joints.at("q4_big_telescope").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, 0);
    _joints.at("theta8_rotator_joint").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, 0);
    _joints.at("q8").set_applied_command_interface_value(hardware_interface::HW_IF_VELOCITY, 0, 0);
  }

  _start_applied_command = enable && start_command;
  _horn_applied_command = horn_command && start_command && !enable;

  _pwm->heartbeat();

  m_start_command = start_command;
  m_enable = enable;
  m_traction_command_rad_p_s = traction_command_rad_p_s;

  std::stringstream ss;
  ss << "Writing commands:";

  ss << std::fixed << std::setprecision(2) << std::endl
     << "\t"
     << "start: " << start_command
     << "\t"
     << "enabled: " << enable << std::endl
     << "\t"
     << "traction: " << traction_velocity_rad_p_s
     << "\t"
     << "steering: " << steering_angle_rad << std::endl
     << "\t"
     << "slew: " << slew_velocity
     << "\t"
     << "boom: " << boom_velocity << std::endl
     << "\t"
     << "arm: " << arm_velocity
     << "\t"
     << "telescope: " << telescope_velocity << std::endl
     << "\t"
     << "rotator: " << rotator_velocity
     << "\t"
     << "gripper: " << gripper_velocity << std::endl;

  RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 500, "%s", ss.str().c_str());

  return hardware_interface::return_type::OK;
}

hardware_interface::CallbackReturn TruckHardware::validate_joint(
  const hardware_interface::ComponentInfo &joint,
  const std::vector<std::string> &expected_command_interfaces,
  const std::vector<std::string> &expected_state_interfaces)
{
  if (joint.command_interfaces.size() != expected_command_interfaces.size())
  {
    RCLCPP_FATAL(
        get_logger(), "Joint '%s' has %zu command interfaces. %zu expected.",
        joint.name.c_str(), joint.command_interfaces.size(), expected_command_interfaces.size());
    return hardware_interface::CallbackReturn::ERROR;
  }

  for (size_t i = 0; i < expected_command_interfaces.size(); ++i)
  {
    if (joint.command_interfaces[i].name != expected_command_interfaces[i])
    {
      RCLCPP_FATAL(
          get_logger(), "Joint '%s' has command interface '%s'. '%s' expected.",
          joint.name.c_str(), joint.command_interfaces[i].name.c_str(), expected_command_interfaces[i].c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }
  }

  if (joint.state_interfaces.size() != expected_state_interfaces.size())
  {
    RCLCPP_FATAL(
        get_logger(), "Joint '%s' has %zu state interfaces. %zu expected.",
        joint.name.c_str(), joint.state_interfaces.size(), expected_state_interfaces.size());
    return hardware_interface::CallbackReturn::ERROR;
  }

  for (size_t i = 0; i < expected_state_interfaces.size(); ++i)
  {
    if (joint.state_interfaces[i].name != expected_state_interfaces[i])
    {
      RCLCPP_FATAL(
          get_logger(), "Joint '%s' has state interface '%s'. '%s' expected.",
          joint.name.c_str(), joint.state_interfaces[i].name.c_str(), expected_state_interfaces[i].c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }
  }

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn TruckHardware::start_truck()
{
#ifndef SIMULATE_HARDWARE
  // Enable PWM channels
  for(const auto& [id, joint] : _joints)
  {
    int pwm_channel = joint.get_pwm_channel();

    if(pwm_channel >= 0 && enable_pwm(joint.get_pwm_channel()) != hardware_interface::CallbackReturn::SUCCESS)
    {
      RCLCPP_ERROR(get_logger(), "Error enabling PWM for joint %s.", joint.get_name().c_str());
      return hardware_interface::CallbackReturn::ERROR;
    }
  }
#endif

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn TruckHardware::stop_truck()
{
  bool error = false;

#ifndef SIMULATE_HARDWARE
  // Disable PWM channels
  for(const auto& [id, joint] : _joints)
  {
    int pwm_channel = joint.get_pwm_channel();

    if(pwm_channel >= 0 && disable_pwm(joint.get_pwm_channel()) != hardware_interface::CallbackReturn::SUCCESS)
    {
      RCLCPP_ERROR(get_logger(), "Error disabling PWM for joint %s.", joint.get_name().c_str());
      error = true;
    }
  }
#endif

  if(error) { RCLCPP_INFO(get_logger(), "Error while stopping Truck!"); return hardware_interface::CallbackReturn::ERROR;   }
  else      { RCLCPP_INFO(get_logger(), "Successfully stopped Truck!"); return hardware_interface::CallbackReturn::SUCCESS; }
}

hardware_interface::CallbackReturn TruckHardware::enable_pwm(unsigned int channel)
{
  auto start = std::chrono::steady_clock::now();
  while(!_pwm->enable(channel))
  {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    if (elapsed.count() > PWM_TIMEOUT_MS) {
      RCLCPP_ERROR(get_logger(), "Timeout enabling PWM for channel %i.", channel);
      return hardware_interface::CallbackReturn::ERROR;
    }

    usleep(100000);
  }

  if ( !(_pwm->set_period(channel, 50)) ) {
    RCLCPP_ERROR(get_logger(), "PWM: Failed to set period for channel %i.", channel);
    return hardware_interface::CallbackReturn::ERROR;
  }

  if ( !(_pwm->set_duty_cycle(channel, 1.5)) ) {
    RCLCPP_ERROR(get_logger(), "PWM: Failed to set duty cycle for channel %i.", channel);
    return hardware_interface::CallbackReturn::ERROR;
  }
  
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn TruckHardware::disable_pwm(unsigned int channel)
{
  auto start = std::chrono::steady_clock::now();
  while(!_pwm->disable(channel))
  {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

    if (elapsed.count() > PWM_TIMEOUT_MS) {
      RCLCPP_ERROR(get_logger(), "Timeout disabling PWM for channel %i.", channel);
      return hardware_interface::CallbackReturn::ERROR;
    }

    usleep(100000);
  }

  return hardware_interface::CallbackReturn::SUCCESS;
}

}  // namespace truck_control

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  truck_control::TruckHardware, hardware_interface::SystemInterface)
