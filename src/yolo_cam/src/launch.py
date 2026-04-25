from launch import LaunchDescription
from launch_ros.actions import Node
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # 5C: CONFIGURATION - Load parameter file
    config = os.path.join(
        get_package_share_directory('cam2image_vm2ros'),
        'config',
        'waypoint_params.yaml'
    )
    
    ort_lib = os.path.expanduser('~/onnxruntime/lib')
    ld_path = ort_lib + ':' + os.environ.get('LD_LIBRARY_PATH', '')

    return LaunchDescription([
        Node(
            package='cam2image_vm2ros',
            executable='cam2image_node',
            name='vision_system',
            output='screen',
            additional_env={'LD_LIBRARY_PATH': ld_path},
            parameters=[
                {'remote_mode': True},
                {'socket_ip': '172.28.32.1'},
                {'socket_port': 9999}
            ]
        )
    ])
