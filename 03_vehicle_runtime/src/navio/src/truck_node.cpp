#include "rclcpp/rclcpp.hpp"
#include "rclcpp/clock.hpp"
#include "navio_interfaces/msg/adc.hpp"
#include "navio_interfaces/msg/pwm.hpp"
#include "sensor_msgs/msg/fluid_pressure.hpp"
#include "sensor_msgs/msg/temperature.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/nav_sat_fix.hpp"
#include "sensor_msgs/msg/nav_sat_status.hpp"
#include <cstdio>
#include <memory>
#include <chrono>
#include <functional>

#include "Common/Util.h"
#include "Navio2/ADC_Navio2.h"

#include "Common/MS5611.h"

#include "Common/MPU9250.h"
#include "Navio2/LSM9DS1.h"

#include "Common/Ublox.h"

#include "Navio2/RGBled.h"

#include "Navio2/PWM.h"


using namespace std::chrono_literals;
using std::placeholders::_1;

class TruckNode : public rclcpp::Node
{
  public:
    TruckNode() : Node("truck_node")
    {
      if (check_apm()) 
      {
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to initialize Truck Node. APM is running.");
        return;
      }
      
      // ADC
      _adc = new ADC_Navio2();
      _adc->initialize();

      // Barometer
      _barometer = new MS5611();
      _barometer->initialize();

      // IMU
      _imu = new MPU9250(); //LSM9DS1
      _imu->initialize();

      // GPS
      _gps = new Ublox();

      if (!_gps->configureSolutionRate(1000))
      {
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "GPS: Failed to set rate.");
      }
      
      // LED
      /*_led = new RGBled();
      if (!_led->initialize())
      {
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to initialize LED.");
      }*/

      // PWM
      // PWM Example is inactive because it is controlled by ros2_control
      /*#define PWM_OUTPUT 0

      _pwm = new PWM();

      if(!_pwm->init(PWM_OUTPUT)) {
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to initialize PWM.");
      }
      else
      {
        auto start = std::chrono::steady_clock::now();
        while(!_pwm->enable(PWM_OUTPUT))
        {
          auto now = std::chrono::steady_clock::now();
          auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

          if (elapsed.count() > pwm_timeout_ms) {
            RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Timeout enabling PWM.");
            break;
          }

          usleep(100000);
        }
        
        if ( !(_pwm->set_period(PWM_OUTPUT, 50)) ) {
          RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "PWM: Failed to set period.");
        }

        _pwm_subscription = this->create_subscription<navio_interfaces::msg::PWM>("pwm", 10, std::bind(&TruckNode::pwm_callback, this, _1));
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Subscribed to topic 'pwm'.");
      }*/

      _adc_msg = navio_interfaces::msg::ADC();

      _adc_pub = this->create_publisher<navio_interfaces::msg::ADC>("navio/adc", 10);
      _baro_pressure_pub = this->create_publisher<sensor_msgs::msg::FluidPressure>("navio/barometer/pressure", 10);
      _baro_temp_pub     = this->create_publisher<sensor_msgs::msg::Temperature>("navio/barometer/temperature", 10);
      _imu_pub = this->create_publisher<sensor_msgs::msg::Imu>("navio/imu/data_raw", 10);
      _gps_fix_pub = this->create_publisher<sensor_msgs::msg::NavSatFix>("navio/gps/fix", 10);

      _adc_timer       = this->create_wall_timer(1s, std::bind(&TruckNode::adc_timer_callback, this));
      _barometer_timer = this->create_wall_timer(1s, std::bind(&TruckNode::barometer_timer_callback, this));
      _imu_timer       = this->create_wall_timer(1s, std::bind(&TruckNode::imu_timer_callback, this));
      _gps_timer       = this->create_wall_timer(1s, std::bind(&TruckNode::gps_timer_callback, this));
      //_led_timer       = this->create_wall_timer(1s, std::bind(&TruckNode::led_timer_callback, this));
      //_pwm_timer       = this->create_wall_timer(50ms, std::bind(&TruckNode::pwm_timer_callback, this));

      RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Truck Node has been initialized.");
    }

  private:
    rclcpp::Subscription<navio_interfaces::msg::PWM>::SharedPtr _pwm_subscription;

    void adc_timer_callback()
    {
      // Read from ADC
      _adc_msg.board_voltage         = _adc->read(0) / 1000.0;
      _adc_msg.servo_rail_voltage    = _adc->read(1) / 1000.0 * 0.284; // see https://community.emlid.com/t/what-was-the-answer-to-18v-on-the-servo-rail/16017/18
      _adc_msg.power_module_voltage  = _adc->read(2) / 1000.0 * 11.3;
      int power_module_current_digit = _adc->read(3);
      _adc_msg.adc2_voltage          = _adc->read(4) / 1000.0;
      _adc_msg.adc3_voltage          = _adc->read(5) / 1000.0;

      if(power_module_current_digit == 0) { _adc_msg.power_module_current = 0;                                            }
      else                                { _adc_msg.power_module_current = 1.1495 + 0.1239 * power_module_current_digit; }

      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "Board Voltage: %7.2fV", _adc_msg.board_voltage);
      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "Servo Rail Voltage: %7.2fV", _adc_msg.servo_rail_voltage);
      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "ADC2 Voltage: %7.2fV", _adc_msg.adc2_voltage);
      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "ADC3 Voltage: %7.2fV", _adc_msg.adc3_voltage);
      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "Power Module Voltage: %7.2fV", _adc_msg.power_module_voltage);
      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "Power Module Current: %7.2fA", _adc_msg.power_module_current);

      // Publish
      _adc_pub->publish(_adc_msg);
    }

    void barometer_timer_callback()
    {
      // Read from Barometer
      _barometer->refreshPressure();
      usleep(10000); // Waiting for pressure data ready
      _barometer->readPressure();

      _barometer->refreshTemperature();
      usleep(10000); // Waiting for temperature data ready
      _barometer->readTemperature();

      _barometer->calculatePressureAndTemperature();

      float pressure = _barometer->getPressure();
      const double pressure_pa = static_cast<double>(pressure) * 100.0;
      float temperature = _barometer->getTemperature();

      auto stamp = this->get_clock()->now();

      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "Barometer: %fmBar, %f°C", pressure, temperature);

      _baro_pressure_msg.header.stamp = stamp;
      _baro_pressure_msg.header.frame_id = "";
      _baro_pressure_msg.fluid_pressure = pressure_pa;
      _baro_pressure_msg.variance = 0.0;
      _baro_pressure_pub->publish(_baro_pressure_msg);

      _baro_temp_msg.header.stamp = stamp;
      _baro_temp_msg.header.frame_id = "";
      _baro_temp_msg.temperature = static_cast<double>(temperature);
      _baro_temp_msg.variance = 0.0;
      _baro_temp_pub->publish(_baro_temp_msg);
    }

    void imu_timer_callback()
    {  
      constexpr double deg_to_rad = 3.14159265358979323846 / 180.0;

      float ax, ay, az;
      float gx, gy, gz;
      float mx, my, mz;

      _imu->update();
      _imu->read_accelerometer(&ax, &ay, &az);
      _imu->read_gyroscope(&gx, &gy, &gz);
      _imu->read_magnetometer(&mx, &my, &mz);

      auto stamp = this->get_clock()->now();

      _imu_msg.header.stamp = stamp;
      _imu_msg.header.frame_id = "";

      _imu_msg.linear_acceleration.x = static_cast<double>(ax);
      _imu_msg.linear_acceleration.y = static_cast<double>(ay);
      _imu_msg.linear_acceleration.z = static_cast<double>(az);

      _imu_msg.angular_velocity.x = static_cast<double>(gx) * deg_to_rad;
      _imu_msg.angular_velocity.y = static_cast<double>(gy) * deg_to_rad;
      _imu_msg.angular_velocity.z = static_cast<double>(gz) * deg_to_rad;
      
      _imu_msg.orientation_covariance[0] = -1.0;

      _imu_pub->publish(_imu_msg);
      
      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "Acc: %+7.3f %+7.3f %+7.3f", ax, ay, az);
      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "Gyr: %+8.3f %+8.3f %+8.3f", gx, gy, gz);
      RCLCPP_INFO_THROTTLE(rclcpp::get_logger("rclcpp"), *get_clock(), 500, "Mag: %+7.3f %+7.3f %+7.3f", mx, my, mz);
    }

    void gps_timer_callback()
    {
      std::vector<double> pos_data;

      if (_gps->decodeSingleMessage(Ublox::NAV_POSLLH, pos_data) == 1)
      {
        // [0]=iTOW (ms), [1]=lon (1e-7 deg), [2]=lat (1e-7 deg),
        // [3]=height ellipsoid (mm), [4]=hMSL (mm),
        // [5]=hAcc (mm), [6]=vAcc (mm)

        const double lon_deg = pos_data[1] / 10000000.0;
        const double lat_deg = pos_data[2] / 10000000.0;
        const double alt_m   = pos_data[4] / 1000.0;   // using hMSL as altitude
        const double hacc_m  = pos_data[5] / 1000.0;
        const double vacc_m  = pos_data[6] / 1000.0;

        auto stamp = this->get_clock()->now();

        _gps_fix_msg.header.stamp = stamp;
        _gps_fix_msg.header.frame_id = "";

        _gps_fix_msg.status.service = sensor_msgs::msg::NavSatStatus::SERVICE_GPS;

        _gps_fix_msg.latitude = lat_deg;
        _gps_fix_msg.longitude = lon_deg;
        _gps_fix_msg.altitude = alt_m;

        // covariance: diag(hacc^2, hacc^2, vacc^2)
        _gps_fix_msg.position_covariance_type = sensor_msgs::msg::NavSatFix::COVARIANCE_TYPE_DIAGONAL_KNOWN;
        _gps_fix_msg.position_covariance[0] = hacc_m * hacc_m;
        _gps_fix_msg.position_covariance[4] = hacc_m * hacc_m;
        _gps_fix_msg.position_covariance[8] = vacc_m * vacc_m;

        _gps_fix_pub->publish(_gps_fix_msg);

        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "GPS Millisecond Time of Week: %.0lf s", pos_data[0]/1000);
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Longitude: %lf", lon_deg);
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Latitude: %lf", lat_deg);
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Height above mean sea level: %.3lf m", alt_m);
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Horizontal Accuracy Estimate: %.3lf m", hacc_m);
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Vertical Accuracy Estimate: %.3lf m", vacc_m);
      } 
      else {
        RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "GPS: Message not captured");
      }

      if (_gps->decodeSingleMessage(Ublox::NAV_STATUS, pos_data) == 1)
      {
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Current GPS status:");
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "gpsFixOk: %d", ((int)pos_data[1] & 0x01));

        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "gps Fix status: ");
        switch((int)pos_data[0]){
          case 0x00:
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "no fix");
            _gps_fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_NO_FIX;
            break;

          case 0x01:
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "dead reckoning only");
            _gps_fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_NO_FIX;
            break;

          case 0x02:
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "2D-fix");
            _gps_fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_FIX;
            break;

          case 0x03:
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "3D-fix");
            _gps_fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_FIX;
            break;

          case 0x04:
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "GPS + dead reckoning combined");
            _gps_fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_FIX;
            break;

          case 0x05:
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Time only fix");
            _gps_fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_NO_FIX;
            break;

          default:
            RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Reserved value. Current state unknown");
            _gps_fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_NO_FIX;
            break;
        }
      } 
      else 
      {
        RCLCPP_WARN(rclcpp::get_logger("rclcpp"), "GPS: Status Message not captured");
        _gps_fix_msg.status.status = sensor_msgs::msg::NavSatStatus::STATUS_NO_FIX;
      }
    }

    /*void led_timer_callback()
    {
      _led->setColor(Colors::Green);
      printf("LED is green\n");
      sleep(0.2);

      _led->setColor(Colors::Cyan);
      printf("LED is cyan\n");
    }

    void pwm_timer_callback()
    {
      _pwm->set_duty_cycle(PWM_OUTPUT, 0.8);
      _pwm->heartbeat();
    }

    void pwm_callback(const navio_interfaces::msg::PWM & msg) const
    {
      RCLCPP_INFO(this->get_logger(), "Main cylinder setpoint: '%f'", msg.main_cylinder);
      _pwm->set_duty_cycle(PWM_OUTPUT, msg.main_cylinder);
    }*/

    rclcpp::TimerBase::SharedPtr _adc_timer;
    rclcpp::TimerBase::SharedPtr _barometer_timer;
    rclcpp::TimerBase::SharedPtr _imu_timer;
    rclcpp::TimerBase::SharedPtr _gps_timer;
    //rclcpp::TimerBase::SharedPtr _led_timer;
    //rclcpp::TimerBase::SharedPtr _pwm_timer;
    rclcpp::Publisher<navio_interfaces::msg::ADC>::SharedPtr _adc_pub;
    rclcpp::Publisher<sensor_msgs::msg::FluidPressure>::SharedPtr _baro_pressure_pub;
    rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr _baro_temp_pub;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr _imu_pub;
    rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr _gps_fix_pub;

    ADC_Navio2* _adc;
    MS5611* _barometer;
    InertialSensor* _imu;
    Ublox* _gps;
    RGBled* _led;
    //PWM* _pwm;
    navio_interfaces::msg::ADC _adc_msg;
    sensor_msgs::msg::FluidPressure _baro_pressure_msg;
    sensor_msgs::msg::Temperature _baro_temp_msg;
    sensor_msgs::msg::Imu _imu_msg;
    sensor_msgs::msg::NavSatFix _gps_fix_msg;

    int pwm_timeout_ms = 2000;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TruckNode>());
  rclcpp::shutdown();
  return 0;
}
