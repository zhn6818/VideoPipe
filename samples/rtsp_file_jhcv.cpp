#include "../nodes/vp_rtsp_src_node.h"
#include "../nodes/infers/vp_jhcv_detector_node.h"
#include "../nodes/osd/vp_osd_node.h"
#include "../nodes/vp_file_des_node.h"

#include "../utils/analysis_board/vp_analysis_board.h"

#include <iostream>
#include <string>

/*
* ## rtsp_file_jhcv ##
* 使用 VideoPipe 的 RTSP 源接入一条摄像头流，调用 jhcv_lib 的
* JHDeepCore::Detector 推理，再通过普通 OSD 节点画框并保存为 MP4。
*
* 用法：
*   rtsp_file_jhcv [model.onnx] [label_path] [device_id] [rtsp_url] [output_dir]
*
* jhcv_lib 会根据 model.onnx 自动读取同目录下的 model.yaml；如果模型
* 不需要单独的标签文件，label_path 可以传空字符串 ""。
*/

int main(int argc, char **argv) {
    // wugui_det is the jhcv_lib model currently used by this camera.  The
    // command line argument is kept so another model can be tested without
    // changing the sample source.
    const std::string model_path = argc > 1
        ? argv[1]
        : "/data1/code/jhcv_lib/models/wugui_det/best.onnx";
    const std::string label_path = argc > 2 ? argv[2] : "";
#if defined(VP_WITH_CUDA) || defined(VP_WITH_TRT)
    // JHDeepCore uses a non-negative device_id for CUDA and a negative value
    // for CPU inference.
    constexpr int default_device_id = 0;
#else
    constexpr int default_device_id = -1;
#endif
    const int device_id = argc > 3 ? std::stoi(argv[3]) : default_device_id;
    const std::string rtsp_url = argc > 4
        ? argv[4]
        : "rtsp://admin:Nj666888@10.0.0.34:554/cam/realmonitor?channel=1&subtype=0";
    const std::string output_dir = argc > 5 ? argv[5] : "./output";

    VP_SET_LOG_INCLUDE_CODE_LOCATION(false);
    VP_SET_LOG_INCLUDE_THREAD_ID(false);
    VP_LOGGER_INIT();

    // create nodes
    // RTSP 源：参数 (节点名, 通道, rtsp_url, resize_ratio, decoder, skip_interval)
    auto rtsp_src_0 = std::make_shared<vp_nodes::vp_rtsp_src_node>(
        "rtsp_src_0", 0,
        rtsp_url,
        1, "avdec_h264", 2);
    auto jhcv_detector_0 = std::make_shared<vp_nodes::vp_jhcv_detector_node>(
        "jhcv_detector_0", model_path, label_path, device_id);
    auto osd_0 = std::make_shared<vp_nodes::vp_osd_node>("osd_0");
    // 第5个参数 max_duration_for_single_file=1：每 1 分钟切一个 mp4 文件
    auto file_des_0 = std::make_shared<vp_nodes::vp_file_des_node>(
        "file_des_0", 0, output_dir, "rtsp_jhcv_", 1);

    // construct pipeline
    jhcv_detector_0->attach_to({rtsp_src_0});
    osd_0->attach_to({jhcv_detector_0});
    file_des_0->attach_to({osd_0});

    // for debug purpose
    // 注意：vp_analysis_board 构造时会遍历链路、给每个节点挂它自己的 meta_handled_hooker，
    // 所以帧号 hooker 必须在 board 构造【之后】再挂，否则会被覆盖、帧号画不上。
    vp_utils::vp_analysis_board board({rtsp_src_0});
    // board.display(1, false);

    rtsp_src_0->start();

    std::string wait;
    std::getline(std::cin, wait);
    rtsp_src_0->detach_recursively();
    return 0;
}
