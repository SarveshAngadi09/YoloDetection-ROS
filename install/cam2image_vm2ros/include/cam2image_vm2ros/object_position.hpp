#pragma once

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "cv_bridge/cv_bridge.hpp"
#include "opencv2/opencv.hpp"
#include "std_msgs/msg/float64.hpp"


namespace object_position_vm2ros
{

class ObjectPosition : public rclcpp::Node
{
public:
    explicit ObjectPosition(const rclcpp::NodeOptions & options);

private:
    void image_r(const sensor_msgs::msg::Image::SharedPtr msg);

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
    rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr pub_T;
};

} // namespace image_processing_vm2ros
