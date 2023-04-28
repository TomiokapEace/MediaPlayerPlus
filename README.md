# Media Player Plus

## 简介

基于QMPlay完成的跨平台全格式多媒体播放器，使用C++编写并支持Qt框架。

该项目在实现上，使用了一些设计模式，例如观察者模式、工厂模式和单例模式等。该项目还包括各种功能模块，采用了多种技术和库，但整个代码结构清晰，易于理解和扩展。

## 涉及框架与技术

1. C++编程语言：使用C++编写并实现其核心逻辑。
   
2. Qt框架：使用Qt作为其UI界面的开发环境，并利用Qt自带的工具和库来加速开发。主要使用了Qt框架中的QCoreApplication和QGuiApplication类以及相关的QObject、QWidget等类。播放器的UI界面使用了Qt框架中的QML语言进行描述，并通过QQuickView类将QML与C++代码进行绑定。
   
3. FFmpeg库：使用FFmpeg库进行视频解码和音频处理。
   
4. PortAudio库：使用PortAudio库将音频输出到各种平台。
   
5. SDL库：如果必要，使用SDL库进行视频渲染。
   
## 目录说明

```
├─lang // 语言翻译文件
└─src  // 源代码
    ├─cmake // 存储构建项目所需的CMake模块
    │  └─Modules    
    ├─gui // 存放应用程序的UI界面相关资源
    │  ├─Android // 存储Android平台相关的资源文件
    │  ├─Colors  // 包含各种颜色值的文件
    │  ├─Icons   // 包含各种图标的文件，按照尺寸细分
    │  │  └─hicolor
    │  ├─macOS // 存储macOS平台相关的资源文件
    │  ├─Ui    // 包括各种ui文件及其转换后生成的ui头文件
    │  ├─Unix  // 存储Unix平台相关的资源文件
    │  └─Windows // 存储Windows平台相关的资源文件
    ├─modules    // 包含使用的各种模块
    │  ├─ALSA    // ALSA音频驱动模块
    │  ├─AudioCD // CD音频播放器模块
    │  ├─AudioFilters  // 音频过滤器模块，包括bs2b过滤器
    │  │  └─bs2b
    │  ├─Chiptune      // 小型计算机/游戏机音乐格式播放器
    │  ├─CUVID         // Nvidia视频解码库CUDA插件
    │  │  └─cuda
    │  ├─DirectX       // DirectX图形API插件
    │  ├─Extensions    // 插件扩展，包括Radio插件
    │  │  └─Radio      
    │  ├─FFmpeg        // 音视频解码器，使用FFmpeg库实现
    │  ├─FileAssociation  // 文件关联处理模块
    │  ├─Inputs        // 输入文件格式支持模块
    │  ├─MediaKit      // BeOS/Haiku平台媒体库模块
    │  ├─Modplug       // 模块音乐播放器插件，使用libmodplug库实现
    │  ├─Notify        // 消息通知模块
    │  ├─OpenSLES      // Android平台OpenSLES音频驱动模块
    │  ├─PipeWire      // PipeWire音频服务器和客户端模块
    │  ├─Playlists     // 播放列表模块
    │  ├─PortAudio     // 跨平台音频API库PortAudio的QMPlay2插件
    │  ├─PulseAudio    // PulseAudio音频驱动模块
    │  ├─QPainter      // QPainter绘图引擎模块
    │  ├─Subtitles     // 字幕处理模块
    │  ├─VideoFilters  // 视频过滤器模块
    │  ├─Visualizations// 可视化效果模块
    │  └─XVideo        // XVideo输出模块
    ├─qmplay2          // QMPlay2应用程序核心代码
    │  ├─opengl        // OpenGL相关代码
    │  │  └─shaders    // OpenGL着色器代码
    │  └─vulkan        // Vulkan相关代码
    │      ├─headers   // Vulkan头文件
    │      └─shaders   // Vulkan着色器SPIR-V代码
    ├─qmvk             // VKontakte音乐下载器
    └─svg              // 存储SVG格式的图标文件
```