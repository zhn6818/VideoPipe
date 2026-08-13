#ifdef VP_WITH_JHCV

#include "vp_jhcv_detector_node.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "../../objects/vp_frame_meta.h"
#include "../../objects/vp_frame_target.h"

namespace vp_nodes {

vp_jhcv_detector_node::vp_jhcv_detector_node(
    std::string node_name,
    std::string model_path,
    std::string label_path,
    int device_id,
    int class_id_offset,
    float conf_threshold,
    float iou_threshold)
    : vp_node(std::move(node_name)),
      detector(std::make_shared<JHDeepCore::Detector>(
          model_path, label_path, device_id, "", conf_threshold, iou_threshold)),
      class_id_offset(class_id_offset) {
    initialized();
}

vp_jhcv_detector_node::~vp_jhcv_detector_node() {
    deinitialized();
}

std::shared_ptr<vp_objects::vp_meta> vp_jhcv_detector_node::handle_frame_meta(
    std::shared_ptr<vp_objects::vp_frame_meta> meta) {
    std::vector<cv::Mat> images{meta->frame};
    std::vector<JHDeepCore::DetectionResult> results;
    detector->process(images, results);

    if (results.empty()) {
        return meta;
    }

    for (const auto &detection : results.front().detections) {
        const int left = std::max(0, detection.bbox.x);
        const int top = std::max(0, detection.bbox.y);
        const int right = std::min(meta->frame.cols,
                                   detection.bbox.x + detection.bbox.width);
        const int bottom = std::min(meta->frame.rows,
                                    detection.bbox.y + detection.bbox.height);
        const int width = right - left;
        const int height = bottom - top;
        if (width <= 0 || height <= 0) {
            continue;
        }

        meta->targets.push_back(std::make_shared<vp_objects::vp_frame_target>(
            left,
            top,
            width,
            height,
            detection.class_id + class_id_offset,
            detection.confidence,
            meta->frame_index,
            meta->channel_index,
            detection.class_name));
    }

    return meta;
}

}

#endif  // VP_WITH_JHCV
