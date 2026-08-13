#pragma once

#ifdef VP_WITH_JHCV

#include <memory>
#include <string>

#include <JHDeepCore.h>

#include "../vp_node.h"

namespace vp_nodes {

// VideoPipe adapter around JHDeepCore::Detector.
// The jhcv_lib project remains an independent submodule; this class only
// translates VideoPipe frame metadata and detection results.
class vp_jhcv_detector_node: public vp_node {
private:
    std::shared_ptr<JHDeepCore::Detector> detector;
    int class_id_offset;

protected:
    virtual std::shared_ptr<vp_objects::vp_meta> handle_frame_meta(
        std::shared_ptr<vp_objects::vp_frame_meta> meta) override;

public:
    vp_jhcv_detector_node(std::string node_name,
                          std::string model_path,
                          std::string label_path = "",
                          int device_id = 0,
                          int class_id_offset = 0,
                          float conf_threshold = 0.25f,
                          float iou_threshold = 0.45f);
    ~vp_jhcv_detector_node();
};

}

#endif  // VP_WITH_JHCV
