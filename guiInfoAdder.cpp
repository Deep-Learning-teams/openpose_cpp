#include <cstdio> // std::snprintf
#include <limits> // std::numeric_limits
#include <openpose/utilities/fastMath.hpp>
#include <openpose/utilities/openCv.hpp>
#include <openpose/gui/guiInfoAdder.hpp>
#include <openpose/gui/CvxText.h>

namespace op
{
    // Used colors
    const cv::Scalar WHITE_SCALAR{255, 255, 255};

    void updateFps(unsigned long long& lastId, double& fps, unsigned int& fpsCounter,
                   std::queue<std::chrono::high_resolution_clock::time_point>& fpsQueue,
                   const unsigned long long id, const int numberGpus)
    {
        try
        {
            // If only 1 GPU -> update fps every frame.
            // If > 1 GPU:
                // We updated fps every (3*numberGpus) frames. This is due to the variability introduced by
                // using > 1 GPU at the same time.
                // However, we update every frame during the first few frames to have an initial estimator.
            // In any of the previous cases, the fps value is estimated during the last several frames.
            // In this way, a sudden fps drop will be quickly visually identified.
            if (lastId != id)
            {
                lastId = id;
                fpsQueue.emplace(std::chrono::high_resolution_clock::now());
                bool updatePrintedFps = true;
                if (fpsQueue.size() > 5)
                {
                    const auto factor = (numberGpus > 1 ? 25u : 15u);
                    updatePrintedFps = (fpsCounter % factor == 0);
                    // updatePrintedFps = (numberGpus == 1 ? true : fpsCounter % (3*numberGpus) == 0);
                    fpsCounter++;
                    if (fpsQueue.size() > factor)
                        fpsQueue.pop();
                }
                if (updatePrintedFps)
                {
                    const auto timeSec = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(
                        fpsQueue.back()-fpsQueue.front()
                    ).count() * 1e-9;
                    fps = (fpsQueue.size()-1) / (timeSec != 0. ? timeSec : 1.);
                }
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

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

    void addPeopleIds(cv::Mat& cvOutputData, const Array<long long>& poseIds, const Array<float>& poseKeypoints,
                      const int borderMargin)
    {
        try
        {
            if (!poseIds.empty())
            {
                const auto poseKeypointsArea = poseKeypoints.getSize(1)*poseKeypoints.getSize(2);
                const auto isVisible = 0.05f;
                for (auto i = 0u ; i < poseIds.getVolume() ; i++)
                {
                    if (poseIds[i] > -1)
                    {
                        const auto indexMain = i * poseKeypointsArea;
                        const auto indexSecondary = i * poseKeypointsArea + poseKeypoints.getSize(2);
                        if (poseKeypoints[indexMain+2] > isVisible || poseKeypoints[indexSecondary+2] > isVisible)
                        {
                            const auto xA = intRound(poseKeypoints[indexMain]);
                            const auto yA = intRound(poseKeypoints[indexMain+1]);
                            const auto xB = intRound(poseKeypoints[indexSecondary]);
                            const auto yB = intRound(poseKeypoints[indexSecondary+1]);
                            int x;
                            int y;
                            if (poseKeypoints[indexMain+2] > isVisible && poseKeypoints[indexSecondary+2] > isVisible)
                            {
                                const auto keypointRatio = intRound(0.15f * std::sqrt((xA-xB)*(xA-xB) + (yA-yB)*(yA-yB)));
                                x = xA + 3*keypointRatio;
                                y = yA - 3*keypointRatio;
                            }
                            else if (poseKeypoints[indexMain+2] > isVisible)
                            {
                                x = xA + intRound(0.25f*borderMargin);
                                y = yA - intRound(0.25f*borderMargin);
                            }
                            else //if (poseKeypoints[indexSecondary+2] > isVisible)
                            {
                                x = xB + intRound(0.25f*borderMargin);
                                y = yB - intRound(0.5f*borderMargin);
                            }
                            putTextOnCvMat(cvOutputData, std::to_string(poseIds[i]), {x, y}, WHITE_SCALAR, false, cvOutputData.cols);
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    GuiInfoAdder::GuiInfoAdder(const int numberGpus, const bool guiEnabled) :
        mNumberGpus{numberGpus},
        mGuiEnabled{guiEnabled},
        mFpsCounter{0u},
        mLastElementRenderedCounter{std::numeric_limits<int>::max()},
        mLastId{std::numeric_limits<unsigned long long>::max()}
    {
    }

    GuiInfoAdder::~GuiInfoAdder()
    {
    }

    void GuiInfoAdder::addInfo(cv::Mat& cvOutputData, const int numberPeople, const unsigned long long id,
                               const std::string& elementRenderedName, const unsigned long long frameNumber,
                               const Array<long long>& poseIds, const Array<float>& poseKeypoints)
    {
        try
        {
            //设置中文格式
            CvxText text("3rdparty/simhei.ttf"); //指定字体
            cv::Scalar size1{ 20, 0.5, 0.1, 0 }; // (字体大小, 无效的, 字符间距, 无效的 }
            text.setFont(nullptr, &size1, nullptr, 0);
            // Sanity check
            if (cvOutputData.empty())
                error("Wrong input element (empty cvOutputData).", __LINE__, __FUNCTION__, __FILE__);
            // Size
            const auto borderMargin = intRound(fastMax(cvOutputData.cols, cvOutputData.rows) * 0.025);
            // Update fps
            updateFps(mLastId, mFps, mFpsCounter, mFpsQueue, id, mNumberGpus);
            // Fps or s/gpu
            char charArrayAux[15];
            std::snprintf(charArrayAux, 15, "%4.1f fps", mFps);
            // Recording inverse: sec/gpu
            // std::snprintf(charArrayAux, 15, "%4.2f s/gpu", (mFps != 0. ? mNumberGpus/mFps : 0.));
            putTextOnCvMat(cvOutputData, charArrayAux, {intRound(cvOutputData.cols - borderMargin), borderMargin},
                           WHITE_SCALAR, true, cvOutputData.cols);
            // Part to show
            // Allowing some buffer when changing the part to show (if >= 2 GPUs)
            // I.e. one GPU might return a previous part after the other GPU returns the new desired part, it looks
            // like a mini-bug on screen
            // Difference between Titan X (~110 ms) & 1050 Ti (~290ms)
            if (mNumberGpus == 1 || (elementRenderedName != mLastElementRenderedName
                                     && mLastElementRenderedCounter > 4))
            {
                mLastElementRenderedName = elementRenderedName;
                mLastElementRenderedCounter = 0;
            }
            mLastElementRenderedCounter = fastMin(mLastElementRenderedCounter, std::numeric_limits<int>::max() - 5);
            mLastElementRenderedCounter++;
            // Add each person ID
            addPeopleIds(cvOutputData, poseIds, poseKeypoints, borderMargin);
            // OpenPose name as well as help or part to show
            //显示中文
            std::string s1 = "人数: ";
            char *data;
            int len = s1.length();
            data = (char *)malloc((len + 1)*sizeof(char));
            s1.copy(data, len, 0);
            data[len] = '\0';
            char* str1 = data;
            wchar_t *w_str1;
            ToWchar(str1,w_str1);
            text.putText(cvOutputData, w_str1, cv::Point((int)(cvOutputData.cols - borderMargin-100),(int)(cvOutputData.rows - borderMargin)), cv::Scalar(255, 0, 0));
            putTextOnCvMat(cvOutputData,std::to_string(numberPeople),
                           {(int)(cvOutputData.cols - borderMargin-30), (int)(cvOutputData.rows - borderMargin-10)},
                           cv::Scalar(255, 0, 0), true, cvOutputData.cols);
            


            putTextOnCvMat(cvOutputData, "OpenPose - " +
                           (!mLastElementRenderedName.empty() ?
                                mLastElementRenderedName : (mGuiEnabled ? "'h' for help" : "")),
                           {borderMargin, borderMargin}, WHITE_SCALAR, false, cvOutputData.cols);
            // Frame number
            putTextOnCvMat(cvOutputData, "Frame: " + std::to_string(frameNumber),
                           {borderMargin, (int)(cvOutputData.rows - borderMargin)}, WHITE_SCALAR, false, cvOutputData.cols);
            // Number people
           // putTextOnCvMat(cvOutputData, "People: " + std::to_string(numberPeople),
           //                {(int)(cvOutputData.cols - borderMargin), (int)(cvOutputData.rows - borderMargin)},
           //                WHITE_SCALAR, true, cvOutputData.cols);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
}
