# 通过替换方式修改openpose
## 修改显示界面中文
### 命令行修改字库依赖
sudo ln -s /usr/include/freetype2/freetype/ /usr/include/freetype
sudo ln -s /usr/include/freetype2/ft2build.h /usr/include/
### 替换源码
将CvxText.cpp复制到src/openpose/gui文件夹内
将CvxText.h复制到include/openpose/gui文件夹内
将simhei.ttf复制到3rdparty文件夹内
### 修改cmake
参照CMakeLists1.txt修改src/openpose/gui/CMakeLists.txt
