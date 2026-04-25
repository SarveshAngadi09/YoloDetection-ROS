cd ~/yolo_ws

colcon build

source install/setup.bash

#!/bin/bash

echo "Running Windows app..."

python.exe "C:\Users\sarve\Downloads\videoserver.py" &

echo "Starting ROS 2..."


ros2 launch ./src/yolo_cam/src/launch.py

