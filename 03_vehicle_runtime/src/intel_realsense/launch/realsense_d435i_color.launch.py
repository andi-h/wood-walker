# Copied and modified from https://github.com/introlab/rtabmap_ros/tree/0.21.10-humble/rtabmap_examples/launch

# Requirements:
#   A realsense D435i
#   Install realsense2 ros2 package (ros-$ROS_DISTRO-realsense2-camera)
# Example:
#   $ ros2 launch rtabmap_examples realsense_d435i_color.launch.py

import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch_ros.actions import Node, SetParameter
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    remappings=[
          ('imu', '/imu/data'),
          ('rgb/image', '/camera/color/image_raw'),
          ('rgb/camera_info', '/camera/color/camera_info'),
          ('depth/image', '/camera/aligned_depth_to_color/image_raw')]

    return LaunchDescription([

        # Launch arguments
        DeclareLaunchArgument(
            'unite_imu_method', default_value='2',
            description='0-None, 1-copy, 2-linear_interpolation. Use unite_imu_method:="1" if imu topics stop being published.'),

        # Make sure IR emitter is enabled
        SetParameter(name='depth_module.emitter_enabled', value=1),
        SetParameter(name='color_qos', value='SENSOR_DATA'),
        SetParameter(name='depth_qos', value='SENSOR_DATA'),
        SetParameter(name='color_info_qos', value='SENSOR_DATA'),
        SetParameter(name='depth_info_qos', value='SENSOR_DATA'),
        SetParameter(name='gyro_qos', value='SENSOR_DATA'),
        SetParameter(name='accel_qos', value='SENSOR_DATA'),

        # Launch camera driver
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([os.path.join(
                get_package_share_directory('realsense2_camera'), 'launch'),
                '/rs_launch.py']),
                launch_arguments={'camera_namespace': '',
                                  'enable_gyro': 'true',
                                  'enable_accel': 'true',
                                  'unite_imu_method': LaunchConfiguration('unite_imu_method'),
                                  'align_depth.enable': 'true',
                                  'enable_sync': 'true',
                                  'color_compressed': 'true',
                                  'depth_compressed': 'true',
                                  'rgb_camera.color_profile': '640x360x6',
                                  'depth_module.depth_profile': '640x360x6',
                                  'depth_module.infra_profile': '640x360x6',
                                  'pointcloud.enable': 'false'}.items(),
        ),
        
        # Compute quaternion of the IMU
        Node(
            package='imu_filter_madgwick', executable='imu_filter_madgwick_node', output='screen',
            parameters=[{'use_mag': False, 
                         'world_frame':'enu', 
                         'publish_tf':False}],
            remappings=[('imu/data_raw', '/camera/imu')]),
    ])
