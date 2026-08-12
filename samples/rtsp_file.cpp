#include "../nodes/vp_rtsp_src_node.h"
#include "../nodes/infers/vp_yunet_face_detector_node.h"
#include "../nodes/infers/vp_sface_feature_encoder_node.h"
#include "../nodes/osd/vp_face_osd_node_v2.h"
#include "../nodes/vp_file_des_node.h"

#include "../objects/vp_frame_meta.h"
#include <opencv2/imgproc.hpp>
#include "../utils/analysis_board/vp_analysis_board.h"

/*
* ## rtsp_file ##
* 基于 1-1-1_sample：仅把源节点从 vp_file_src_node 换成 vp_rtsp_src_node。
* 拉 1 路 RTSP 流做人脸检测，并按每 1 分钟切一个 mp4 存盘（每帧右上角带帧号）。
*/

int main() {
    VP_SET_LOG_INCLUDE_CODE_LOCATION(false);
    VP_SET_LOG_INCLUDE_THREAD_ID(false);
    VP_LOGGER_INIT();

    // create nodes
    // RTSP 源：参数 (节点名, 通道, rtsp_url, resize_ratio, decoder, skip_interval)
    auto rtsp_src_0 = std::make_shared<vp_nodes::vp_rtsp_src_node>(
        "rtsp_src_0", 0,
        "rtsp://admin:Nj666888@10.0.0.34:554/cam/realmonitor?channel=1&subtype=0",
        1, "avdec_h264", 2);
    auto yunet_face_detector_0 = std::make_shared<vp_nodes::vp_yunet_face_detector_node>("yunet_face_detector_0", "./vp_data/models/face/face_detection_yunet_2022mar.onnx");
    auto sface_face_encoder_0 = std::make_shared<vp_nodes::vp_sface_feature_encoder_node>("sface_face_encoder_0", "./vp_data/models/face/face_recognition_sface_2021dec.onnx");
    auto osd_0 = std::make_shared<vp_nodes::vp_face_osd_node_v2>("osd_0");
    // 第5个参数 max_duration_for_single_file=1：每 1 分钟切一个 mp4 文件
    auto file_des_0 = std::make_shared<vp_nodes::vp_file_des_node>("file_des_0", 0, "./output", "rtsp_", 1);

    // construct pipeline
    yunet_face_detector_0->attach_to({rtsp_src_0});
    sface_face_encoder_0->attach_to({yunet_face_detector_0});
    osd_0->attach_to({sface_face_encoder_0});
    file_des_0->attach_to({osd_0});

    // for debug purpose
    // 注意：vp_analysis_board 构造时会遍历链路、给每个节点挂它自己的 meta_handled_hooker，
    // 所以帧号 hooker 必须在 board 构造【之后】再挂，否则会被覆盖、帧号画不上。
    vp_utils::vp_analysis_board board({rtsp_src_0});
    // board.display(1, false);

    // 帧号记录：osd_0 画完人脸框后，把帧序号写到每帧画面右上角（随 osd_frame 存进 mp4）
    // osd_0->set_meta_handled_hooker([](std::string node_name, int queue_size, std::shared_ptr<vp_objects::vp_meta> meta) {
    //     auto fm = std::dynamic_pointer_cast<vp_objects::vp_frame_meta>(meta);
    //     if (!fm || fm->osd_frame.empty()) return;
    //     auto text = "frame: " + std::to_string(fm->frame_index);
    //     int baseline = 0;
    //     auto sz = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseline);
    //     cv::putText(fm->osd_frame, text,
    //                 cv::Point(fm->osd_frame.cols - sz.width - 15, 30),
    //                 cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
    // });

    rtsp_src_0->start();

    std::string wait;
    std::getline(std::cin, wait);
    rtsp_src_0->detach_recursively();
}
