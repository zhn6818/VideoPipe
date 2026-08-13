#include "../nodes/vp_file_src_node.h"
#include "../nodes/infers/vp_jhcv_detector_node.h"
#include "../nodes/osd/vp_osd_node.h"
#include "../nodes/vp_file_des_node.h"

#include <iostream>
#include <string>

// Usage:
//   jhcv_detector_sample <model.onnx> [video] [labels.txt] [device_id]
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <model.onnx> [video] [labels.txt] [device_id]\n";
        return 1;
    }

    const std::string model_path = argv[1];
    const std::string video_path = argc > 2
        ? argv[2]
        : "./vp_data/test_video/vehicle_stop.mp4";
    const std::string label_path = argc > 3 ? argv[3] : "";
    const int device_id = argc > 4 ? std::stoi(argv[4]) : 0;

    VP_SET_LOG_LEVEL(vp_utils::vp_log_level::INFO);
    VP_LOGGER_INIT();

    auto file_src = std::make_shared<vp_nodes::vp_file_src_node>(
        "file_src", 0, video_path, 1.0f, false);
    auto detector = std::make_shared<vp_nodes::vp_jhcv_detector_node>(
        "jhcv_detector", model_path, label_path, device_id);
    auto osd = std::make_shared<vp_nodes::vp_osd_node>("osd");
    auto file_des = std::make_shared<vp_nodes::vp_file_des_node>(
        "file_des", 0, "./output", "jhcv_", 1);

    detector->attach_to({file_src});
    osd->attach_to({detector});
    file_des->attach_to({osd});

    file_src->start();

    std::string wait;
    std::getline(std::cin, wait);
    file_src->detach_recursively();
    return 0;
}
