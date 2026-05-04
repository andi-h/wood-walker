import rclpy
from rclpy.node import Node

from navio_py.navio.adc import ADC
from navio_py.navio.util import check_apm

from navio_interfaces.msg import ADC as ADC_Interface

class AdcPublisher(Node):
    def __init__(self):
        check_apm()
        self._adc = ADC()

        super().__init__('adc_publisher')

        self._publisher = self.create_publisher(ADC_Interface, 'topic', 10)
        self._message = ADC_Interface()

        timer_period_s = 0.1
        self._timer = self.create_timer(timer_period_s, self.timer_callback)

        self.get_logger().info('ADC Node has been initialized.')

    def timer_callback(self):
        # Read from ADC
        self._message.board_voltage        = self._adc.read(0) / 1000.0
        self._message.servo_rail_voltage   = self._adc.read(1) / 1000.0
        self._message.power_module_voltage = self._adc.read(2) / 1000.0 * 11.3
        power_module_current_digit         = self._adc.read(3)
        self._message.adc2_voltage         = self._adc.read(4) / 1000.0
        self._message.adc3_voltage         = self._adc.read(5) / 1000.0

        if(power_module_current_digit == 0):
            self._message.power_module_current = 0
        else:
            self._message.power_module_current = 1.1495 + 0.1239 * power_module_current_digit
            
        # Publish
        self._publisher.publish(self._message)

def main(args=None):
    rclpy.init(args=args)

    try:
        adc_publisher = AdcPublisher()
        rclpy.spin(adc_publisher)
    except KeyboardInterrupt:
        pass # Gracefully handle exit on interruption
    finally:
        adc_publisher.destroy_node()
        rclpy.try_shutdown()

if __name__ == '__main__':
    main()