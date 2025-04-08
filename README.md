# RHI(Render Hardware Interface)

* Instance 分为DirectX12或者Vulkan
* Adapter 对应显卡或者软件模拟
* Device 逻辑显卡设备，创建资源
* CommandList 记录GPU命令
* CommandQueue 向GPU执行命令
* Memory 显存，配合资源对象
* Buffer/Texture 资源对象，可直接创建并分配显存，也可以直接使用分配好的显存用于重用或频繁申请
* Pipeline 执行光栅化或者计算的流水线，在流水线配置状态和阶段编程（着色器）
* BindSet 记录资源引用给着色器访问资源
* BindSetLayout 布局需要对应到流水线
* Swapchain 窗口交换链，平台相关

### Fence
同步对象。64位无符号整数

作完操作后面标记signal某个值

操作之前等待wait某个值

一系列操作一般是整数累加下去

如GPU之间同步：
``` cpp
uint64 v = 0;
fence.create(v)
// 某处
commandQueue.exeCommand()
commandQueue.signal(++v)
// 另外一处
commandQueue.wait(v)
commandQueue.exeCommand()
```

如CPU等待GPU完成：
``` cpp
uint64 v = 0;
fence.create(v)
commandQueue.exeCommand()
commandQueue.signal(++v)
fence.wait(v)
```

# RenderGraph

* 有向无环图。pass(渲染任务)为图节点，pass读写的资源作为边
* lambda函数和闭包减少代码量
* 自动缩放所有帧纹理大小
* 裁剪无效pass
* 生成可以串并行的pass列表
* 自动推导texture的usage
* 自动转换texture的state
* 重用显存/显存别名

``` cpp
// 后续pass可以访问pass0Out
RenderGraphResource pass0Out;
renderGraph->addPass("pass0", [&](RenderGraphBuilder& builder)
	{
        // （构造函数）
        // 通过builder构建图
        // 节点输入
		builder.read(inTexture);
        // 节点输出
        builder.write(pass0Out);
        return [=](RenderGraphRegistry& registry, CommandList& commandList)
        {
            // （渲染执行函数）
            // registry拿到真实资源
            auto in = registry.getTexture(inTexture);
            // 录制命令
            commandList.beginRenderPass(pass0Out)
            commandList.draw()
        };
    });
```

# 编译

## 推荐使用vcpkg包管理

``` bash
vcpkg install directx-dxc directx-headers glfw3 glm spdlog vulkan-headers vulkan-loader vulkan-validationlayers
```

## linux下的apt包管理

### 依赖库
#### 直接安装
``` bash
sudo apt install cmake libglfw3-wayland libglfw3-dev libspdlog-dev libglm-dev
```
如果需要编译x11窗口环境：
cmake option打开USE_XCB
并且额外安装
``` bash
sudo apt remove libglfw3-wayland
sudo apt install libx11-dev libx11-xcb-dev libxrandr-dev
```
#### 安装DirectXShaderCompiler
https://github.com/microsoft/DirectXShaderCompiler/releases
``` bash
tar -xvf linux_dxc.tar dxc
cd dxc
sudo cp -a ./* /usr/
```

### 编译
``` bash
cmake -B build
cd build && make -j16
```

# vscode开发
安装插件 CMake Tools、Native Debug
