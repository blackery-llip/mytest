Fefer：https://yellowmax.blog.csdn.net/article/details/80718831?spm=1001.2101.3001.6650.8&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-8-80718831-blog-114541939.235%5Ev43%5Epc_blog_bottom_relevance_base5&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromBaidu%7ERate-8-80718831-blog-114541939.235%5Ev43%5Epc_blog_bottom_relevance_base5&utm_relevant_index=14
# 1 camera 硬件
## 1.1 MIPI-CSI 是什么？
MIPI-CSI 全称：Mobile Industry Processor Interface - Camera Serial Interface
作用：是一种高速串行总线标准，用于摄像头模块向主控芯片（SoC、CPU等）发送图像数据。
组成结构：CSI-1、CSI-2、CSI-3： 其中目前使用最广泛的是 CSI-2。
传输协议：基于 D-PHY 或 C-PHY（物理层标准）

数据以帧/行格式传输（图像像素）

应用场景：
| 芯片	 | 摄像头接口 | 常用标准 |
| ----------- | ----------- |----------- |
| 手机 SoC | TMIPI-CSI2 | RAW10/12 |
| STM32MP1 | DCMI/MIP | YUV422/RAW |
| NVIDIA | Jetson | MIPI-CSI2	支持 ISP 流 |
    	

## 1.2 ISP 是什么？
ISP 全称：Image Signal Processor（图像信号处理器）
作用：是摄像头图像数据从原始传感器格式（如 RAW）转换为**可视图像（如 YUV/JPEG）**的处理单元。
核心处理功能包括：
| 阶段		 | 功能 | 
| ----------- | ----------- |
| 黑电平校正 | 消除图像偏移 | 
| 去噪 |  降低暗光图像噪声 |
| 白平衡  | 调整 RGB 颜色一致性 | 
| CFA 解码 | 将 Bayer RAW 解码成 RGB |
| 色彩校正 | 基于相机配置表 |

## 1.3 MIPI-CSI 与 ISP 的关系图
<img width="350" alt="image" src="https://github.com/user-attachments/assets/4c8d4e2c-b721-45d3-bebe-b80534b28e7a" />

## 1.3 实际工程中怎么理解？
| 组件	 | 举例 | 作用说明|
| ----------- | ----------- |----------- |
| 摄像头模组 | OV5640、IMX219 等| 输出 RAW10/RAW12 图像 |
| MIPI-CSI |  硬件接口 |传输图像流 |
| ISP	SoC  | 内部/外部模块 | 图像修复、美化、转格式 |
| 应用层 | 采集/AI 推理系统 |处理 YUV / JPEG / RGB 图像 |


## 1.4 如果没有 ISP 会怎样？
得到的是RAW 裸数据，图像看起来是灰蒙蒙的马赛克。
需要软件手动执行复杂的图像处理流程（非常消耗性能）。
许多嵌入式平台（如树莓派、RK3399、Jetson）都内置 ISP，处理能力强。

## 1.5 总结
<img width="758" alt="image" src="https://github.com/user-attachments/assets/209882b9-8c6e-404b-a140-4aa5de187b79" />


# 2 omap3isp
omap3isp = TI OMAP3 SoC 上集成的 ISP 硬件的 Linux 驱动，用于实现从 MIPI/CCDC 图像输入到 YUV/RGB 输出的图像信号处理流程。

芯片平台： Texas Instruments OMAP3（如 BeagleBoard、N900）
内核驱动名： omap3isp
驱动路径： drivers/media/platform/ti/omap3isp/
框架类型： V4L2 subdev（通过 Media Controller 框架连接 sensor → ISP → output）

<img width="634" alt="image" src="https://github.com/user-attachments/assets/d9b26463-f709-4a97-99dc-325e11d2be60" />

模块组成详解：
- CCDC：摄像头控制器，接收 RAW 图像（来自 MIPI 或平行接口）
- H3A：自动对焦（AF）、自动曝光（AE）、自动白平衡（AWB）
- Previewer：	去马赛克、色彩校正、伽马等
- Resizer：	图像缩放输出
- AEWB/AFF：	用于图像分析
