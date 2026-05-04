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

#ifndef __TRUCK_HPP
#define __TRUCK_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/clock.hpp"
#include "rclcpp/logger.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp_lifecycle/state.hpp"

#include "Navio2/PWM.h"

//#define SIMULATE_HARDWARE 1
#define PWM_TIMEOUT_MS 2000

#define STEERING_PWM_CHANNEL 0
#define TRACTION_PWM_CHANNEL 1
#define HORN_PWM_CHANNEL 2
#define GEAR_PWM_CHANNEL 3
#define SLEWING_PWM_CHANNEL 4
#define BOOM_PWM_CHANNEL 5
#define ARM_PWM_CHANNEL 6
#define TELESCOPE_PWM_CHANNEL 7
#define ROTATOR_PWM_CHANNEL 8
#define GRIPPER_PWM_CHANNEL 9

#define PI 3.14159265358979323846

namespace truck_control
{
  class Joint
  {
    public:
      Joint(const std::string & name, const std::vector<std::string> &expected_command_interfaces, const std::vector<std::string> &expected_state_interfaces, int pwm_channel) : name(name), pwm_channel(pwm_channel) 
      {
        for(auto interface : expected_command_interfaces)
        {
          command_interfaces.insert({ interface, 0 });
        }

        for(auto interface : expected_command_interfaces)
        {
          applied_command_values.insert({ interface, 0 });
        }

        for(auto interface : expected_state_interfaces)
        {
          state_interfaces.insert({ interface, 0 });
        }
      }

      std::string get_name() const {
        return name;
      }

      int get_pwm_channel() const {
        return pwm_channel;
      }

      std::multimap<std::string, double>* get_state_interfaces() {
        return &state_interfaces;
      }

      std::multimap<std::string, double>* get_command_interfaces() {
        return &command_interfaces;
      }

      /* 
      * param interface can be:
      *    hardware_interface::HW_IF_POSITION
      *    hardware_interface::HW_IF_VELOCITY
      *    hardware_interface::HW_IF_ACCELERATION
      *    hardware_interface::HW_IF_EFFORT
      */
      std::vector<double> get_state_interface_values(const std::string& interface) {
        std::vector<double> results;
        for (const auto& [key, value] : state_interfaces) {
          if (key == interface) {
              results.push_back(value);
          }
        }
        return results;
      }

      void set_state_interface_value(const std::string& interface, int index, double value)
      {
        if (index < 0) throw std::out_of_range("negative index");

        auto range = state_interfaces.equal_range(interface);
        const auto count = std::distance(range.first, range.second);
        if (count == 0) throw std::out_of_range("interface not found: " + interface);
        if (index >= count) throw std::out_of_range("index out of range for: " + interface);

        auto it = range.first;
        std::advance(it, index);
        it->second = value;
      }

      std::vector<double> get_command_interface_values(const std::string& interface) {
        std::vector<double> results;
        for (const auto& [key, value] : command_interfaces) {
          if (key == interface) {
              results.push_back(value);
          }
        }
        return results;
      }

      void set_command_interface_value(const std::string& interface, int index, double value) 
      {
        auto filtered_interfaces = command_interfaces.equal_range(interface);
        auto it = std::next(filtered_interfaces.first, index);
        it->second = value;
      }

      std::vector<double> get_applied_command_interface_values(const std::string& interface) {
        std::vector<double> results;
        for (const auto& [key, value] : applied_command_values) {
          if (key == interface) {
              results.push_back(value);
          }
        }
        return results;
      }

      void set_applied_command_interface_value(const std::string& interface, int index, double value) 
      {
        auto filtered_interfaces = applied_command_values.equal_range(interface);
        auto it = std::next(filtered_interfaces.first, index);
        it->second = value;
      }

    private:
      std::string name;
      int pwm_channel;
      std::multimap<std::string, double> state_interfaces;
      std::multimap<std::string, double> command_interfaces;
      std::multimap<std::string, double> applied_command_values;
  };

  class TruckHardware : public hardware_interface::SystemInterface
  {
  public:
    RCLCPP_SHARED_PTR_DEFINITIONS(TruckHardware);

    hardware_interface::CallbackReturn on_init(
      const hardware_interface::HardwareInfo & info) override;

    hardware_interface::CallbackReturn on_configure(
      const rclcpp_lifecycle::State & previous_state) override;

    hardware_interface::CallbackReturn on_cleanup(
      const rclcpp_lifecycle::State & previous_state) override;

    std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

    std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

    hardware_interface::CallbackReturn on_activate(
      const rclcpp_lifecycle::State & previous_state) override;

    hardware_interface::CallbackReturn on_deactivate(
      const rclcpp_lifecycle::State & previous_state) override;

    hardware_interface::return_type read(
      const rclcpp::Time & time, const rclcpp::Duration & period) override;

    hardware_interface::return_type write(
      const rclcpp::Time & time, const rclcpp::Duration & period) override;

    /// Get the logger of the SystemInterface.
    /**
     * \return logger of the SystemInterface.
     */
    rclcpp::Logger get_logger() const { return *logger_; }

    /// Get the clock of the SystemInterface.
    /**
     * \return clock of the SystemInterface.
     */
    rclcpp::Clock::SharedPtr get_clock() const { return clock_; }

  private:
    // Parameters for the RRBot simulation
    double hw_start_sec_;
    double hw_stop_sec_;
    double hw_slowdown_;
    double hw_sensor_change_;

    double _start_command; // 0 = OFF, 1 = ON
    double _horn_command; // 0 = OFF, 1 = ON
    double _gear_command; // 1, 2 or 3

    double _start_applied_command; // 0 = OFF, 1 = ON
    double _horn_applied_command; // 0 = OFF, 1 = ON
    double _gear_applied_command; // 1, 2 or 3

    double _start_state; // 0 = OFF, 1 = ON
    double _horn_state; // 0 = OFF, 1 = ON
    double _gear_state; // 1, 2 or 3

    bool _enable;
    PWM* _pwm;

    // Objects for logging
    std::shared_ptr<rclcpp::Logger> logger_;
    rclcpp::Clock::SharedPtr clock_;

    std::map<std::string, Joint> _joints;

    hardware_interface::CallbackReturn validate_joint(
      const hardware_interface::ComponentInfo &joint,
      const std::vector<std::string> &expected_command_interfaces,
      const std::vector<std::string> &expected_state_interfaces);
    
    hardware_interface::CallbackReturn start_truck();
    hardware_interface::CallbackReturn stop_truck();
    hardware_interface::CallbackReturn enable_pwm(unsigned int channel);
    hardware_interface::CallbackReturn disable_pwm(unsigned int channel);
  };

}  // namespace truck_control

#endif  // __TRUCK_HPP
