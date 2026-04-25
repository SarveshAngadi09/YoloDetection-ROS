
Yolo-Ros
A ROS 2 package that captures a live camera feed from a remote Windows machine, runs YOLOv8 person detection using ONNX Runtime, and publishes the results for downstream use (e.g. a robot simulator or controller).

---

## System Overview

```
Windows Machine                    Ubuntu / WSL2 (ROS 2)
┌─────────────────┐                ┌──────────────────────────────────────┐
│  videoserver.py │ ── UDP/9999 ──►│  cam2image node  ──► /image topic    │
│  (webcam feed)  │                │                                       │
└─────────────────┘                │  image_processor ──► /image_gray     │
                                   │  (YOLOv8 + ORT)  ──► /Threshold      │
                                   │                   (person center x)   │
                                   └──────────────────────────────────────┘
```

---

## Nodes

### 1. `cam2image`
Connects to `videoserver.py` running on Windows and publishes each received frame as a ROS 2 image message.

| | |
|---|---|
| **Publishes** | `/image` (`sensor_msgs/msg/Image`) |
| **Subscribes** | `/flip_image` (`std_msgs/msg/Bool`) |

**Parameters:**

| Parameter | Type | Default | Description |
|---|---|---|---|
| `remote_mode` | bool | `true` | Use remote video server (false = local webcam) |
| `socket_ip` | string | `do ip route` | IP address of the Windows machine |
| `socket_port` | int | `9999` | UDP port of `videoserver.py` |
| `depth` | int | `10` | QoS queue depth |
| `reliability` | string | `reliable` | QoS reliability (`reliable` / `best_effort`) |

---

### 2. `image_processor`
Runs YOLOv8n inference using ONNX Runtime. Filters detections to **persons only** (COCO class 0) and publishes the annotated image plus the horizontal center pixel of each detected person.

| | |
|---|---|
| **Subscribes** | `/image` (`sensor_msgs/msg/Image`) |
| **Publishes** | `/image_gray` — annotated image with bounding boxes |
| | `/Threshold` (`std_msgs/msg/Float64`) — center X pixel of detected person |

**How detection works:**
1. Frame resized to 640×640, normalized to [0, 1], converted BGR→RGB, HWC→CHW
2. ONNX Runtime runs YOLOv8n → output tensor `[1, 84, 8400]`
3. Rows 0–3: bounding box (cx, cy, w, h) in 640×640 pixel space
4. Rows 4–83: class confidence scores (80 COCO classes)
5. Only detections with `class == 0` (person) and `score > 0.45` are kept
6. NMS applied with IoU threshold 0.4
7. `center_x = box.x + box.width / 2` published on `/Threshold`

**Parameters:**

| Parameter | Type | Default | Description |
|---|---|---|---|
| `threshold` | double | `120.0` | Brightness threshold (dynamically updatable at runtime) |

---

## Topic Summary

| Topic | Type | Published by |
|---|---|---|
| `/image` | `sensor_msgs/msg/Image` | `cam2image` |
| `/image_gray` | `sensor_msgs/msg/Image` | `image_processor` |
| `/Threshold` | `std_msgs/msg/Float64` | `image_processor` |
| `/light_status` | `std_msgs/msg/String` | `image_processor` |

---

## Dependencies

- ROS 2 Jazzy
- OpenCV 4.6 (`libopencv-dev`)
- `cv_bridge`, `rclcpp`, `sensor_msgs`, `std_msgs`
- [ONNX Runtime 1.18.1 C++](https://github.com/microsoft/onnxruntime/releases/tag/v1.18.1) — installed at `~/onnxruntime/`

---

## Setup

### 1. Install ONNX Runtime C++ SDK

```bash
cd ~
wget https://github.com/microsoft/onnxruntime/releases/download/v1.18.1/onnxruntime-linux-x64-1.18.1.tgz
tar -xzf onnxruntime-linux-x64-1.18.1.tgz
mv onnxruntime-linux-x64-1.18.1 ~/onnxruntime
```

### 2. Export the YOLOv8n model (opset 11, required for compatibility)

```bash
pip install ultralytics
python3 -c "from ultralytics import YOLO; YOLO('yolov8n.pt').export(format='onnx', opset=11)"
cp yolov8n.onnx /home/sarve/ros2_ws/src/cam2image_vm2ros/models/yolov8n.onnx
```

> **Why opset 11?** OpenCV 4.6's DNN module cannot handle YOLOv8's broadcast `Add` operations. ONNX Runtime handles all opsets correctly, but the model must be re-exported from the original `.pt` weights at a lower opset.

### 3. Start the video server on Windows if using Remote through WSL

```cmd
python videoserver.py
```

### 4. Build the ROS 2 package

```bash
cd ~/yolo_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-select cam2image_vm2ros
source install/setup.bash
```

### 5. Launch

```bash
ros2 launch cam2image_vm2ros launch.py
```

---

## Runtime Parameter Tuning

Update the detection threshold without restarting the node:

```bash
ros2 param set /vision_system threshold 100.0
```

Monitor person center X:

```bash
ros2 topic echo /Threshold
```

View annotated detections:

```bash
ros2 run rqt_image_view rqt_image_view /image_gray
```
