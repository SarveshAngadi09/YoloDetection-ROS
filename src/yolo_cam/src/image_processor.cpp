#include "image_processor.hpp"
#include <fstream>

namespace image_processing_vm2ros
{

ImageProcessor::ImageProcessor(const rclcpp::NodeOptions & options)
: Node("image_processor", options)
{
    sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        "image", 10,
        std::bind(&ImageProcessor::image_callback, this, std::placeholders::_1)
    );

    thresh_ = this->declare_parameter("threshold", 120.0);

    pub_     = this->create_publisher<sensor_msgs::msg::Image>("image_gray", 10);
    pub_T    = this->create_publisher<std_msgs::msg::Float64>("Threshold", 10);
    pub_status_ = this->create_publisher<std_msgs::msg::String>("light_status", 10);

    param_sub_ = this->add_on_set_parameters_callback(
        [this](const std::vector<rclcpp::Parameter> & params)
        -> rcl_interfaces::msg::SetParametersResult
        {
            rcl_interfaces::msg::SetParametersResult result;
            result.successful = true;
            result.reason = "success";
            for (const auto & p : params) {
                if (p.get_name() == "threshold") {
                    thresh_ = p.as_double();
                    RCLCPP_INFO(this->get_logger(), "Threshold updated to %.0f", thresh_);
                }
            }
            return result;
        }
    );

    // Load ONNX model via ONNX Runtime
    std::string model_path = "./src/yolo_cam/models/yolov8n.onnx";
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(4);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    session_ = Ort::Session(env_, model_path.c_str(), session_options);
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::AllocatorWithDefaultOptions allocator;
    for (size_t i = 0; i < session_.GetInputCount(); ++i)
        input_name_strings_.push_back(session_.GetInputNameAllocated(i, allocator).get());
    for (size_t i = 0; i < session_.GetOutputCount(); ++i)
        output_name_strings_.push_back(session_.GetOutputNameAllocated(i, allocator).get());

    RCLCPP_INFO(this->get_logger(), "Loaded YOLOv8 via ONNX Runtime");

    // Load class names
    std::ifstream ifs("models/coco.names");
    std::string line;
    while (std::getline(ifs, line))
        class_names_.push_back(line);
}

void ImageProcessor::image_callback(
    const sensor_msgs::msg::Image::SharedPtr msg)
{
    cv::Mat frame = cv_bridge::toCvCopy(msg, "bgr8")->image;
    if (frame.empty()) {
        RCLCPP_ERROR(this->get_logger(), "Empty frame received");
        return;
    }

    // Preprocess: resize to 640x640, normalize, BGR→RGB, HWC→CHW
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(640, 640));
    cv::Mat float_img;
    resized.convertTo(float_img, CV_32FC3, 1.0 / 255.0);

    cv::Mat channels[3];
    cv::split(float_img, channels);
    std::vector<float> input_data;
    input_data.reserve(3 * 640 * 640);
    for (int c = 2; c >= 0; --c) {   // BGR→RGB
        const float* ptr = channels[c].ptr<float>(0);
        input_data.insert(input_data.end(), ptr, ptr + 640 * 640);
    }

    // Run inference
    std::array<int64_t, 4> input_shape = {1, 3, 640, 640};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info_,
        input_data.data(), input_data.size(),
        input_shape.data(), input_shape.size()
    );

    const char* input_name  = input_name_strings_[0].c_str();
    const char* output_name = output_name_strings_[0].c_str();

    auto output_tensors = session_.Run(
        Ort::RunOptions{nullptr},
        &input_name,  &input_tensor, 1,
        &output_name, 1
    );

    // Output shape: [1, 84, 8400]
    // Rows 0-3: cx, cy, w, h (pixels in 640×640 space)
    // Rows 4-83: class scores
    auto shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
    int num_classes = static_cast<int>(shape[1]) - 4;
    int num_anchors = static_cast<int>(shape[2]);
    const float* data = output_tensors[0].GetTensorMutableData<float>();

    std::vector<int>     class_ids;
    std::vector<float>   confidences;
    std::vector<cv::Rect> boxes;

    for (int a = 0; a < num_anchors; ++a) {
        // Find best class score for this anchor
        float max_score = 0.0f;
        int   best_class = 0;
        for (int c = 0; c < num_classes; ++c) {
            float score = data[(4 + c) * num_anchors + a];
            if (score > max_score) { max_score = score; best_class = c; }
        }
        if (max_score < conf_threshold_ || best_class != 0)  // 0 = person in COCO
            continue;

        float cx = data[0 * num_anchors + a] / 640.0f;
        float cy = data[1 * num_anchors + a] / 640.0f;
        float w  = data[2 * num_anchors + a] / 640.0f;
        float h  = data[3 * num_anchors + a] / 640.0f;

        int left   = static_cast<int>((cx - 0.5f * w) * frame.cols);
        int top    = static_cast<int>((cy - 0.5f * h) * frame.rows);
        int width  = static_cast<int>(w * frame.cols);
        int height = static_cast<int>(h * frame.rows);

        boxes.push_back(cv::Rect(left, top, width, height));
        confidences.push_back(max_score);
        class_ids.push_back(best_class);
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, conf_threshold_, nms_threshold_, indices);

    for (int idx : indices) {
        cv::Rect box = boxes[idx];
        int center_x = box.x + box.width / 2;

        std_msgs::msg::Float64 cx_msg;
        cx_msg.data = static_cast<double>(center_x);
        pub_T->publish(cx_msg);

        RCLCPP_INFO(this->get_logger(), "Person center_x: %d px", center_x);

        cv::rectangle(frame, box, cv::Scalar(0, 255, 0), 2);
        cv::circle(frame, cv::Point(center_x, box.y + box.height / 2),
                   5, cv::Scalar(0, 0, 255), -1);
        cv::putText(frame, "person x=" + std::to_string(center_x), box.tl(),
            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
    }

    pub_->publish(*cv_bridge::CvImage(msg->header, "bgr8", frame).toImageMsg());
    RCLCPP_INFO(this->get_logger(), "Persons detected: %zu", indices.size());
}

} // namespace image_processing_vm2ros
