
<div align="center">

# 💡 Lux-GI

**Hybrid GI solution, based on DDGI ( include  Ray-Tracing and SDF-Tracing )**

The project is a Hybrid GI solution, partly inspired by Lumen and DDGI. The main core idea is to provide a complete GI solution (including Indirect-Light, Infinite-Bounce, Emissive-Lighting, Glossy-Reflection, Shadow, AO) that can run on both Raytracing-supported and non-raytracing hardware. Due to the shortage of time, there are still some performance issues in this project, but as a throwaway, I believe it can provide some new ideas to most of the people who are interested in GI.

</div>



![Overview](./images/overview.png)

## Showcase

[bilibili](https://www.bilibili.com/video/BV1NM411y7sv/)

[youtuebe](https://www.youtube.com/watch?v=RfRbWnsdwx0)

## Document

[English-Version](./English.md)

[中文版](./Chinese.md)


## Main Features
* Hybrid rendering and Global Illumination
  * using Raytracing and SDF-Tracing to make DDGI in realtime
  * Surface Cache for SDF Material representation
  * Deferred-Shading
  * SDF/Ray tracing reflection
  * SDF/Ray tracing soft shadow
  * SVGF denoiser
  * Standard PBR with GGX and roughness/metalness
* ECS IoC Container
  * Data Driven ECS architecture
  * Auto Injection ECS System(based on entt)
* RHI Interface 
  * Vulkan 1.3 
  * SPIR-V reflection

## Compile
* Windows
  * 1. Click gen_build_win.bat
  * 2. Choose Game as main project
  * 3. Building with RelWithDebInfo
* Linux
  * Not support now
* Mac
  * Not support now

## Controls in the `Game` app

* WSAD, QE - movement
* Mouse + Right Button - rotate the camera
* Shift - move faster


## Acknowledgments

This project is made with lots of awesome open-source code, they are

* [RTXGI](https://github.com/NVIDIAGameWorks/RTXGI), the fully raytracing DDGI implementation.

* [cereal](https://github.com/USCiLab/cereal), A C++11 library for serialization.

* [entt](https://github.com/skypjack/entt), high performance ECS library

* [glad](https://github.com/Dav1dde/glad), an vulkan/opengl function loader

* [glfw](https://github.com/glfw/glfw), open source, multi-platform library for OpenGL, OpenGL ES and Vulkan application development

* [imgui](https://github.com/ocornut/imgui), is a bloat-free graphical user interface library for C++

* [libacc](https://github.com/nmoehrle/libacc), About
Acceleration structure library (BVH-, KD-Tree) with basic parallel construction

* [hybrid-rendering](https://github.com/diharaw/hybrid-rendering), provide raytracing and denoise.

* [spriv-cross](https://github.com/KhronosGroup/SPIRV-Cross), SPIRV-Cross is a tool designed for parsing and converting SPIR-V to other shader languages.

* [tracer](https://github.com/mensinda/tracer.git), c++ crash tracer.

* [glm](https://github.com/g-truc/glm), OpenGL Mathematics (GLM) is a header only C++ mathematics library for graphics software.

* [spdlog](https://github.com/gabime/spdlog), Very fast, header-only/compiled, C++ logging library

* [stb](https://github.com/nothings/stb), single-file public domain (or MIT licensed) libraries for C/C++

* [flax-engine](https://github.com/FlaxEngine/FlaxEngine), open source game engine. provide the idea how to generate and trace sdf.

Also Paper Reference

* [Lumen-SIGGRAPH-2022](https://advances.realtimerendering.com/s2022/SIGGRAPH2022-Advances-Lumen-Wright%20et%20al.pdf)

* [DDGI-Website](https://morgan3d.github.io/articles/2019-04-01-ddgi/)

* [DDGI-Paper](https://jcgt.org/published/0008/02/01/)

* [Surface Cache](https://docs.unrealengine.com/5.0/en-US/lumen-technical-details-in-unreal-engine/)

* [Blue noise and Sobol sequence](https://belcour.github.io/blog/slides/2019-sampling-bluenoise/index.html)

* [Importance Sampling](https://learnopengl.com/PBR/IBL/Specular-IBL)

* [Soft-Shadow based on ray tracing method](https://blog.demofox.org/2020/05/16/using-blue-noise-for-raytraced-soft-shadows/) 

* [Gaussian-blur](http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/)

* [Bilateral_filter](https://en.wikipedia.org/wiki/Bilateral_filter)

* [Spatiotemporal Variance-Guided Filtering](https://research.nvidia.com/publication/2017-07_spatiotemporal-variance-guided-filtering-real-time-reconstruction-path-traced) 

* [Atrous](https://jo.dreggn.org/home/2010_atrous.pdf) 