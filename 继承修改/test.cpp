// Command-line user intraface
#define OPENPOSE_FLAGS_DISABLE_DISPLAY
#include <openpose/flags.hpp>
// OpenPose dependencies
#include <openpose/headers.hpp>
#include "CvxText.h"
//中文显示
static int ToWchar(char* &src, wchar_t* &dest, const char *locale = "zh_CN.utf8")
{
    if (src == NULL) {
        dest = NULL;
        return 0;
    }

    // 根据环境变量设置locale
    setlocale(LC_CTYPE, locale);

    // 得到转化为需要的宽字符大小
    int w_size = mbstowcs(NULL, src, 0) + 1;

    // w_size = 0 说明mbstowcs返回值为-1。即在运行过程中遇到了非法字符(很有可能使locale
    // 没有设置正确)
    if (w_size == 0) {
        dest = NULL;
        return -1;
    }

    //wcout << "w_size" << w_size << endl;
    dest = new wchar_t[w_size];
    if (!dest) {
        return -1;
    }

    int ret = mbstowcs(dest, src, strlen(src)+1);
    if (ret <= 0) {
        return -1;
    }
    return 0;
}

// 如果用户需要自己的变量，可以继承op::Datum结构并在其中添加。
// UserDatum 可以直接由openspose包装器使用，因为它继承自op::Datum，只需定义
// WrapperT<std::vector<std::shared_ptr<UserDatum>>> instead of Wrapper
// (or equivalently WrapperT<std::vector<std::shared_ptr<UserDatum>>>)
struct UserDatum : public op::Datum
{
    bool boolThatUserNeedsForSomeReason;

    UserDatum(const bool boolThatUserNeedsForSomeReason_ = false) :
        boolThatUserNeedsForSomeReason{boolThatUserNeedsForSomeReason_}
    {}
};

// W-classes 既可以作为模板实现，也可以作为给定的简单类实现。
// 用户通常知道他将在队列之间移动哪种数据,
// in this case we assume a std::shared_ptr of a std::vector of UserDatum

// 此工作进程只读取并返回目录中的所有JPG文件
class WUserOutput : public op::WorkerConsumer<std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>>>
{
public:
    void initializationOnThread() {}

    void workConsumer(const std::shared_ptr<std::vector<std::shared_ptr<UserDatum>>>& datumsPtr)
    {
        try
        {
            //设置中文格式
            CvxText text("examples/user_code/simhei.ttf"); //指定字体
            cv::Scalar size1{ 20, 0.5, 0.1, 0 }; // (字体大小, 无效的, 字符间距, 无效的 }
            text.setFont(nullptr, &size1, nullptr, 0);
            // 用户在此处显示/保存/其他处理
                // datumPtr->cvOutputData: 带姿势或热图的渲染帧
                // datumPtr->poseKeypoints: 带估计姿势的数组<float> 
            if (datumsPtr != nullptr && !datumsPtr->empty())
            {
                // 在命令行中显示身体、面部和手的最终姿势关键点
                op::log("\nKeypoints:");
                // 获取关键点的每个元素
                const auto& poseKeypoints = datumsPtr->at(0)->poseKeypoints;
                op::log("Person pose keypoints:");
                for (auto person = 0 ; person < poseKeypoints.getSize(0) ; person++)
                {
                    op::log("Person " + std::to_string(person) + " (x, y, score):");
                    for (auto bodyPart = 0 ; bodyPart < poseKeypoints.getSize(1) ; bodyPart++)
                    {
                        std::string valueToPrint;
                        for (auto xyscore = 0 ; xyscore < poseKeypoints.getSize(2) ; xyscore++)
                        {
                            valueToPrint += std::to_string(   poseKeypoints[{person, bodyPart, xyscore}]   ) + " ";
                        }
                        op::log(valueToPrint);
                    }
                }
                op::log(" ");
                // Alternative: just getting std::string equivalent
                op::log("Face keypoints: " + datumsPtr->at(0)->faceKeypoints.toString());
                op::log("Left hand keypoints: " + datumsPtr->at(0)->handKeypoints[0].toString());
                op::log("Right hand keypoints: " + datumsPtr->at(0)->handKeypoints[1].toString());
                // Heatmaps
                const auto& poseHeatMaps = datumsPtr->at(0)->poseHeatMaps;
                if (!poseHeatMaps.empty())
                {
                    op::log("Pose heatmaps size: [" + std::to_string(poseHeatMaps.getSize(0)) + ", "
                            + std::to_string(poseHeatMaps.getSize(1)) + ", "
                            + std::to_string(poseHeatMaps.getSize(2)) + "]");
                    const auto& faceHeatMaps = datumsPtr->at(0)->faceHeatMaps;
                    op::log("Face heatmaps size: [" + std::to_string(faceHeatMaps.getSize(0)) + ", "
                            + std::to_string(faceHeatMaps.getSize(1)) + ", "
                            + std::to_string(faceHeatMaps.getSize(2)) + ", "
                            + std::to_string(faceHeatMaps.getSize(3)) + "]");
                    const auto& handHeatMaps = datumsPtr->at(0)->handHeatMaps;
                    op::log("Left hand heatmaps size: [" + std::to_string(handHeatMaps[0].getSize(0)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(1)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(2)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(3)) + "]");
                    op::log("Right hand heatmaps size: [" + std::to_string(handHeatMaps[1].getSize(0)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(1)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(2)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(3)) + "]");
                }
                std::string s1 = "人数";
                char *data;
                int len = s1.length();
                data = (char *)malloc((len + 1)*sizeof(char));
                s1.copy(data, len, 0);
                data[len] = '\0';
                char* str1 = data;
                wchar_t *w_str1;
                ToWchar(str1,w_str1);
                text.putText(datumsPtr->at(0)->cvOutputData, w_str1, cv::Point(50,50), cv::Scalar(255, 0, 0));
                std::string text = "1";
                cv::putText(datumsPtr->at(0)->cvOutputData, text, cv::Point(50,80),cv::FONT_HERSHEY_COMPLEX, 1,cv::Scalar(255, 0, 0),2,8,0);
                // 显示渲染输出图像
                cv::imshow("User worker GUI", datumsPtr->at(0)->cvOutputData);
                // 显示图像并休眠至少1毫秒（通常休眠5-10毫秒以显示图像）
                const char key = (char)cv::waitKey(1);
                if (key == 27)
                    this->stop();
            }
        }
        catch (const std::exception& e)
        {
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
};

int tutorialApiCpp()
{
    try
    {
        op::log("Starting OpenPose demo...", op::Priority::High);
        const auto timerBegin = std::chrono::high_resolution_clock::now();

        // 记录器等级
        op::check(0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.",
                  __LINE__, __FUNCTION__, __FILE__);
        op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
        op::Profiler::setDefaultX(FLAGS_profile_speed);
        // // For debugging
        // // Print all logging messages
        // op::ConfigureLog::setPriorityThreshold(op::Priority::None);
        // // Print out speed values faster
        // op::Profiler::setDefaultX(100);

        // 将用户定义的配置-gflags应用于程序变量
        // 摄像头尺寸
        const auto cameraSize = op::flagsToPoint(FLAGS_camera_resolution, "-1x-1");
        // 输出尺寸
        const auto outputSize = op::flagsToPoint(FLAGS_output_resolution, "-1x-1");
        // 网络输入尺寸
        const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x368");
        // 人脸模型输入尺寸
        const auto faceNetInputSize = op::flagsToPoint(FLAGS_face_net_resolution, "368x368 (multiples of 16)");
        // 手模型输入尺寸
        const auto handNetInputSize = op::flagsToPoint(FLAGS_hand_net_resolution, "368x368 (multiples of 16)");
        // 产品类型
        op::ProducerType producerType;
        std::string producerString;
        std::tie(producerType, producerString) = op::flagsToProducer(
            FLAGS_image_dir, FLAGS_video, FLAGS_ip_camera, FLAGS_camera, FLAGS_flir_camera, FLAGS_flir_camera_index);
        // 姿态模型
        const auto poseModel = op::flagsToPoseModel(FLAGS_model_pose);
        // JSON格式存储
        if (!FLAGS_write_keypoint.empty())
            op::log("Flag `write_keypoint` is deprecated and will eventually be removed."
                    " Please, use `write_json` instead.", op::Priority::Max);
        // 关键点比例模型
        const auto keypointScaleMode = op::flagsToScaleMode(FLAGS_keypoint_scale);
        // 添加热图
        const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg,
                                                      FLAGS_heatmaps_add_PAFs);
        const auto heatMapScaleMode = op::flagsToHeatMapScaleMode(FLAGS_heatmaps_scale);
        // >1 camera view?
        const auto multipleView = (FLAGS_3d || FLAGS_3d_views > 1 || FLAGS_flir_camera);
        // 启用Google日志记录
        const bool enableGoogleLogging = true;

        // 配置OpenPose
        op::log("Configuring OpenPose...", op::Priority::High);
        op::WrapperT<UserDatum> opWrapperT;

        // Initializing the user custom classes
        // GUI (Display)
        auto wUserOutput = std::make_shared<WUserOutput>();
        // Add custom processing
        const auto workerOutputOnNewThread = true;
        opWrapperT.setWorker(op::WorkerType::Output, wUserOutput, workerOutputOnNewThread);

        //配置Pose，提取并渲染该图像的身体关键点/热图/ PAF(Part Affinity Fields)（pose模块） (使用wrapperstructpose作为默认配置和推荐配置)
        const op::WrapperStructPose wrapperStructPose{
            !FLAGS_body_disable, netInputSize, outputSize, keypointScaleMode, FLAGS_num_gpu, FLAGS_num_gpu_start,
            FLAGS_scale_number, (float)FLAGS_scale_gap, op::flagsToRenderMode(FLAGS_render_pose, multipleView),
            poseModel, !FLAGS_disable_blending, (float)FLAGS_alpha_pose, (float)FLAGS_alpha_heatmap,
            FLAGS_part_to_show, FLAGS_model_folder, heatMapTypes, heatMapScaleMode, FLAGS_part_candidates,
            (float)FLAGS_render_threshold, FLAGS_number_people_max, FLAGS_maximize_positives, FLAGS_fps_max,
            FLAGS_prototxt_path, FLAGS_caffemodel_path, enableGoogleLogging};
        opWrapperT.configure(wrapperStructPose);
        // 配置Face，提取并渲染该图像的面部关键点/热图/ PAF（face模块） (使用WrapperStructFace{} 禁用)
        const op::WrapperStructFace wrapperStructFace{
            FLAGS_face, faceNetInputSize, op::flagsToRenderMode(FLAGS_face_render, multipleView, FLAGS_render_pose),
            (float)FLAGS_face_alpha_pose, (float)FLAGS_face_alpha_heatmap, (float)FLAGS_face_render_threshold};
        opWrapperT.configure(wrapperStructFace);
        // 配置Hand，提取并渲染该图像的手部关键点/热图/ PAF（hand模块） (使用WrapperStructHand{}禁用)
        const op::WrapperStructHand wrapperStructHand{
            FLAGS_hand, handNetInputSize, FLAGS_hand_scale_number, (float)FLAGS_hand_scale_range, FLAGS_hand_tracking,
            op::flagsToRenderMode(FLAGS_hand_render, multipleView, FLAGS_render_pose), (float)FLAGS_hand_alpha_pose,
            (float)FLAGS_hand_alpha_heatmap, (float)FLAGS_hand_render_threshold};
        opWrapperT.configure(wrapperStructHand);
        // 配置额外功能 (使用WrapperStructExtra{}禁用)
        const op::WrapperStructExtra wrapperStructExtra{
            FLAGS_3d, FLAGS_3d_min_views, FLAGS_identification, FLAGS_tracking, FLAGS_ik_threads};
        opWrapperT.configure(wrapperStructExtra);
        // 读取图像/视频/网络摄像头的文件夹（producer模块） (使用默认值禁用任何输入)
        const op::WrapperStructInput wrapperStructInput{
            producerType, producerString, FLAGS_frame_first, FLAGS_frame_step, FLAGS_frame_last,
            FLAGS_process_real_time, FLAGS_frame_flip, FLAGS_frame_rotate, FLAGS_frames_repeat,
            cameraSize, FLAGS_camera_parameter_path, FLAGS_frame_undistort, FLAGS_3d_views};
        opWrapperT.configure(wrapperStructInput);
        // 输出 (注释或使用默认参数禁用任何输出)
        const op::WrapperStructOutput wrapperStructOutput{
            FLAGS_cli_verbose, FLAGS_write_keypoint, op::stringToDataFormat(FLAGS_write_keypoint_format),
            FLAGS_write_json, FLAGS_write_coco_json, FLAGS_write_coco_foot_json, FLAGS_write_coco_json_variant,
            FLAGS_write_images, FLAGS_write_images_format, FLAGS_write_video, FLAGS_write_video_fps,
            FLAGS_write_heatmaps, FLAGS_write_heatmaps_format, FLAGS_write_video_3d, FLAGS_write_video_adam,
            FLAGS_write_bvh, FLAGS_udp_host, FLAGS_udp_port};
        opWrapperT.configure(wrapperStructOutput);
        // No GUI. Equivalent to: opWrapper.configure(op::WrapperStructGui{});
        //  设置为单线程 (用于顺序处理和/或调试和/或减少延迟)
        if (FLAGS_disable_multi_thread)
            opWrapperT.disableMultiThreading();

        // 启动，运行\停止线程，当其他所有线程单元完成后启动该线程。
        op::log("Starting thread(s)...", op::Priority::High);
        opWrapperT.exec();

        // 测量总时间
        const auto now = std::chrono::high_resolution_clock::now();
        const auto totalTimeSec = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(now-timerBegin).count()
                                * 1e-9;
        const auto message = "OpenPose demo successfully finished. Total time: "
                           + std::to_string(totalTimeSec) + " seconds.";
        op::log(message, op::Priority::High);

        // 返回成功消息
        return 0;
    }
    catch (const std::exception& e)
    {
        return -1;
    }
}

int main(int argc, char *argv[])
{
    // Parsing command line flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // Running tutorialApiCpp
    return tutorialApiCpp();
}
