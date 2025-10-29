# lua-gpiod

一个完整的 libgpiod Lua C 库包装器，为 Linux 系统提供完整的 GPIO 控制功能。

## 概述

本库为 libgpiod 库提供了完整的 Lua 绑定，允许您在 Lua 脚本中控制 Linux 系统上的 GPIO 引脚。它支持所有主要的 gpiod 功能，包括输入/输出操作、事件处理、批量操作和芯片枚举。

## 语言版本

- [English](README.md)
- [中文](README_CN.md)

## 特性

- **完整的 GPIO 控制**：输入/输出操作、值读取/写入
- **事件处理**：上升沿、下降沿和双边沿事件检测
- **批量操作**：高效的多线 GPIO 操作
- **芯片管理**：枚举和管理多个 GPIO 芯片
- **线属性**：访问线名称、消费者、方向、偏置设置
- **精确定时**：内置精确睡眠函数，适用于时序关键应用

## 系统要求

- 支持 GPIO 的 Linux 系统
- 已安装 libgpiod 库
- Lua 5.4（或兼容版本）
- GCC 编译器

## 安装

### 安装依赖

在 Ubuntu/Debian 上：
```bash
sudo apt-get install libgpiod-dev lua5.4-dev build-essential
```

在 Fedora/RHEL 上：
```bash
sudo dnf install libgpiod-devel lua-devel gcc
```

### 编译库

```bash
make
```

### 安装（可选）

```bash
sudo make install
```

## 使用方法

### 基础示例

```lua
local gpiod = require("gpiod")

-- 打开 GPIO 芯片
local chip = gpiod.chip_open("gpiochip0")

-- 获取 GPIO 线
local line = chip:get_line(18)

-- 配置为输出
line:request_output("my_app", 0)

-- 设置高电平
line:set_value(1)

-- 清理资源
line:release()
chip:close()
```

### 输入与事件检测

```lua
local gpiod = require("gpiod")

local chip = gpiod.chip_open("gpiochip0")
local line = chip:get_line(24)

-- 请求上升沿事件
line:request_rising_edge_events("button_monitor")

print("等待按钮按下...")
if line:event_wait(5.0) then  -- 5秒超时
    local event = line:event_read()
    print("按钮按下时间:", event:timestamp())
    print("事件类型:", event:event_type())
end

line:release()
chip:close()
```

### 批量操作

```lua
local gpiod = require("gpiod")

local chip = gpiod.chip_open("gpiochip0")
local lines = chip:get_lines({18, 19, 20, 21})

-- 配置所有线为输出并设置初始值
lines:request_output("led_controller", {0, 1, 0, 1})

-- 同时设置所有值
lines:set_values({1, 1, 1, 1})

-- 读取所有值
local values = lines:get_values()
for i, value in ipairs(values) do
    print("线 " .. (i-1) .. ": " .. value)
end

lines:release()
chip:close()
```

## API 参考

### 模块函数

- `gpiod.chip_open(name_or_number)` - 通过名称或编号打开 GPIO 芯片
- `gpiod.chip_iter()` - 创建芯片迭代器
- `gpiod.sleep(seconds)` - 精确睡眠函数
- `gpiod.version()` - 获取 libgpiod 版本

### 芯片方法

- `chip:get_line(offset)` - 获取单个 GPIO 线
- `chip:get_lines(offsets)` - 获取多个 GPIO 线
- `chip:find_line(name)` - 通过名称查找线
- `chip:get_all_lines()` - 获取所有可用线
- `chip:name()` - 获取芯片名称
- `chip:label()` - 获取芯片标签
- `chip:num_lines()` - 获取线数量
- `chip:close()` - 关闭芯片

### 线方法

#### 配置
- `line:request_input(consumer, [flags])` - 配置为输入
- `line:request_output(consumer, default_val, [flags])` - 配置为输出
- `line:release()` - 释放线

#### 值操作
- `line:get_value()` - 读取当前值
- `line:set_value(value)` - 设置输出值

#### 事件处理
- `line:request_rising_edge_events(consumer)` - 监控上升沿
- `line:request_falling_edge_events(consumer)` - 监控下降沿
- `line:request_both_edges_events(consumer)` - 监控双边沿
- `line:event_wait(timeout)` - 等待事件
- `line:event_read()` - 读取事件
- `line:event_get_fd()` - 获取事件文件描述符

#### 属性
- `line:offset()` - 获取线偏移
- `line:name()` - 获取线名称
- `line:consumer()` - 获取当前消费者
- `line:direction()` - 获取方向（"input"/"output"）
- `line:active_state()` - 获取激活状态（"high"/"low"）
- `line:bias()` - 获取偏置设置
- `line:is_used()` - 检查线是否被使用
- `line:is_open_drain()` - 检查开漏配置
- `line:is_open_source()` - 检查开源配置
- `line:update()` - 更新线信息

### 线批量方法

- `bulk:num_lines()` - 获取线数量
- `bulk:get_line(index)` - 通过索引获取线
- `bulk:request_input(consumer, [flags])` - 配置所有线为输入
- `bulk:request_output(consumer, values, [flags])` - 配置所有线为输出
- `bulk:get_values()` - 读取所有值
- `bulk:set_values(values)` - 设置所有值
- `bulk:release()` - 释放所有线

### 事件方法

- `event:event_type()` - 获取事件类型（"rising_edge"/"falling_edge"）
- `event:timestamp()` - 获取事件时间戳

### 常量

#### 请求标志
- `gpiod.OPEN_DRAIN` - 开漏输出
- `gpiod.OPEN_SOURCE` - 开源输出
- `gpiod.ACTIVE_LOW` - 低电平有效逻辑
- `gpiod.BIAS_DISABLE` - 禁用偏置
- `gpiod.BIAS_PULL_DOWN` - 下拉偏置
- `gpiod.BIAS_PULL_UP` - 上拉偏置

## 测试

### 本地测试

运行示例：
```bash
lua gpiod_example.lua
```

测试模块加载：
```bash
make test
```

### Docker 测试

使用 Docker 构建和测试（推荐用于 CI/CD）：

```bash
# 构建 Docker 镜像
docker build -t lua-gpiod-test .

# 在容器中运行测试
docker run --rm lua-gpiod-test

# 交互式 shell 进行手动测试
docker run --rm -it lua-gpiod-test /bin/bash
```

Docker 环境包含：
- Debian bookworm-slim 基础镜像
- 所有必需的依赖项（libgpiod、Lua 5.4、构建工具）
- 自动构建和基本功能测试
- 模块加载验证的健康检查

## 许可证

本项目采用 MIT 许可证 - 详情请查看 [LICENSE](LICENSE) 文件。

## 项目仓库

本项目托管于：https://github.com/HsOjo/lua-gpiod

## 贡献

欢迎贡献！请随时在 GitHub 仓库提交问题和拉取请求。

## 故障排除

### 权限问题
如果遇到权限错误，您可能需要将用户添加到 `gpio` 组：
```bash
sudo usermod -a -G gpio $USER
```

### 缺少 GPIO 芯片
列出可用的 GPIO 芯片：
```bash
gpiodetect
```

### 找不到库
确保已安装 libgpiod 并且共享库在正确的路径中。
