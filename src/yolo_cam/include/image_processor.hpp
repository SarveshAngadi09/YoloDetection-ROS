#pragma once

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "cv_bridge/cv_bridge.hpp"
#include "opencv2/opencv.hpp"
#include "std_msgs/msg/float64.hpp"
#include "std_msgs/msg/string.hpp"
#include <onnxruntime_cxx_api.h>

namespace image_processing_vm2ros
{

class ImageProcessor : public rclcpp::Node
{
public:
    explicit ImageProcessor(const rclcpp::NodeOptions & options);

private:
    void image_callback(const sensor_msgs::msg::Image::SharedPtr msg);
    double thresh_;

    Ort::Env env_{ORT_LOGGING_LEVEL_WARNING, "yolo"};
    Ort::Session session_{nullptr};
    Ort::MemoryInfo memory_info_{nullptr};
    std::vector<std::string> input_name_strings_;
    std::vector<std::string> output_name_strings_;

    std::vector<std::string> class_names_;
    float conf_threshold_ = 0.45f;
    float nms_threshold_  = 0.4f;

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
    rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr pub_T;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_status_;
    rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_sub_;
};

} // namespace image_processing_vm2ros
