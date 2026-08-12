#include "../nodes/vp_file_src_node.h"
#include "../nodes/infers/vp_yunet_face_detector_node.h"
#include "../nodes/infers/vp_sface_feature_encoder_node.h"
#include "../nodes/osd/vp_face_osd_node_v2.h"
#include "../nodes/vp_file_des_node.h"
#include "../nodes/vp_rtmp_des_node.h"

#include "../objects/vp_frame_meta.h"
#include <opencv2/imgproc.hpp>
#include "../utils/analysis_board/vp_analysis_board.h"

/*
* ## 1-1-1 sample ##
* 1 video input, 1 infer task, and 1 output (save to local mp4 file under ./output).
*/

int main() {
    VP_SET_LOG_INCLUDE_CODE_LOCATION(false);
    VP_SET_LOG_INCLUDE_THREAD_ID(false);
    VP_LOGGER_INIT();

    // create nodes
    auto file_src_0 = std::make_shared<vp_nodes::vp_file_src_node>("file_src_0", 0, "./vp_data/test_video/face.mp4", 0.6);
    auto yunet_face_detector_0 = std::make_shared<vp_nodes::vp_yunet_face_detector_node>("yunet_face_detector_0", "./vp_data/models/face/face_detection_yunet_2022mar.onnx");
    auto sface_face_encoder_0 = std::make_shared<vp_nodes::vp_sface_feature_encoder_node>("sface_face_encoder_0", "./vp_data/models/face/face_recognition_sface_2021dec.onnx");
    auto osd_0 = std::make_shared<vp_nodes::vp_face_osd_node_v2>("osd_0");
    // 第5个参数 max_duration_for_single_file=1：每 1 分钟切一个 mp4 文件
    auto file_des_0 = std::make_shared<vp_nodes::vp_file_des_node>("file_des_0", 0, "./output", "face_", 1);

    // construct pipeline
    yunet_face_detector_0->attach_to({file_src_0});
    sface_face_encoder_0->attach_to({yunet_face_detector_0});
    osd_0->attach_to({sface_face_encoder_0});
    file_des_0->attach_to({osd_0});

    // for debug purpose
    // 注意：vp_analysis_board 构造时会遍历整条链路、给每个节点挂它自己的 meta_handled_hooker，
    // 所以帧号 hooker 必须在 board 构造【之后】再挂，否则会被覆盖、导致帧号画不上。
    vp_utils::vp_analysis_board board({file_src_0});
    // board.display(1, false);

    // 帧号记录：osd_0 画完人脸框后，把帧序号写到每帧画面右上角（随 osd_frame 存进 mp4）
    osd_0->set_meta_handled_hooker([](std::string node_name, int queue_size, std::shared_ptr<vp_objects::vp_meta> meta) {
        auto fm = std::dynamic_pointer_cast<vp_objects::vp_frame_meta>(meta);
        if (!fm || fm->osd_frame.empty()) return;
        auto text = "frame: " + std::to_string(fm->frame_index);
        int baseline = 0;
        // 右上角，用 getTextSize 做右对齐，留 15px 右边距
        auto sz = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseline);
        cv::putText(fm->osd_frame, text,
                    cv::Point(fm->osd_frame.cols - sz.width - 15, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
    });

    file_src_0->start();

    std::string wait;
    std::getline(std::cin, wait);
    file_src_0->detach_recursively();
}
