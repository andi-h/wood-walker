# Requirements:
#   A realsense D435i
#   Install realsense2 ros2 package (ros-$ROS_DISTRO-realsense2-camera)
# Example:
#   $ ros2 launch rtabmap_examples realsense_d435i_color.launch.py

import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch_ros.actions import Node, SetParameter
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch.actions import TimerAction

def launch_setup(context, *args, **kwargs):
    # Directories
    pkg_nav2_bringup = get_package_share_directory(
        'nav2_bringup')
    
    pkg_rtabmap_launch = get_package_share_directory(
        'rtabmap_launch')
        
    nav2_params_file = PathJoinSubstitution(
        [FindPackageShare('rtabmap_launch'), 'params', 'nav2_params.yaml']
    )

    # Paths
    nav2_launch = PathJoinSubstitution(
        [pkg_nav2_bringup, 'launch', 'navigation_launch.py'])

    rtabmap_launch = PathJoinSubstitution(
        [pkg_rtabmap_launch, 'launch', 'rtabmap.launch.py'])

    rviz_launch = PathJoinSubstitution(
        [pkg_nav2_bringup, 'launch', 'rviz_launch.py'])

    config_rviz = os.path.join(
        get_package_share_directory('rtabmap_launch'), 'launch', 'config', 'nav2.rviz'
    )
    
    # Includes
    nav2 = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([nav2_launch]),
        launch_arguments=[
            ('use_sim_time', 'false'),
            ('params_file', nav2_params_file)
        ]
    )
    
    rtabmap = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([rtabmap_launch]),
        launch_arguments=[
            ('compressed', 'false'),
            ('rgbd_sync', 'true'),
            ('rgb_topic', '/camera/color/image_raw'),
            ('depth_topic', '/camera/aligned_depth_to_color/image_raw'),
            ('camera_info_topic', '/camera/color/camera_info'),
            ('imu_topic', '/imu/data'),
            ('approx_sync', 'false'),
            ('frame_id', 'truck_origin'),
            ('subscribe_depth', 'false'),
            ('subscribe_odom_info', 'false'),
            ('wait_imu_to_init', 'true'),
            ('rviz', 'false'),
            ('sync_queue_size', '50'),
            ('topic_queue_size', '50'),
            ('qos', '1'),
            ('qos_camera_info', '2'),
            ('qos_image', '2'),
            ('qos_imu', '2'),
            ('qos_odom', '2'),
            ('ground_truth_frame_id', ''),
            ('rtabmap_args', '--delete_db_on_start'),
            ('localization', 'false')
        ]
    )
    rviz = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([rviz_launch]),
        launch_arguments={
            'rviz_config': config_rviz,
        }.items(),
    )

    return [
        # Nodes to launch
        nav2,
        rtabmap,
        rviz
    ]

def generate_launch_description():
    remappings=[
        ('imu', '/imu/data')
    ]

    return LaunchDescription([
        # Launch arguments
        DeclareLaunchArgument(
            'unite_imu_method', default_value='2',
            description='0-None, 1-copy, 2-linear_interpolation. Use unite_imu_method:="1" if imu topics stop being published.'),

        OpaqueFunction(function=launch_setup)
    ])
