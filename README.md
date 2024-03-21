# ACG Course Assignments

本仓库归档了清华大学交叉信息研究院2023年秋《高等计算机图形学》的3次课程小作业。

我的课程大作业见：https://github.com/CodingEric/rendertoy2

## assignment1: Geometry
* 证明贝塞尔曲线落在凸包内部。
* 曲面细分。
    * Bonus: 我使用了半边数据结构进行优化。
* QEM减面。
    * Bonus: 我给出了全网少见的当矩阵不可逆时QEM算法的正确实现。

## assignment2: Rendering
* 几何变换，实现若干变换矩阵。
* 光栅化算法，绘制2D图形。
* 光线追踪，写GLSL代码实现一次弹跳光线追踪。需要支持镜面和漫射表面。

## assignment3: Simulation
使用高性能图形计算的`taichi`库实现一个简单的物质点法仿真程序。要求实现刚体、流体和弹性体。