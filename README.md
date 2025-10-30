# ai_api_c

这是 ai_api_c 项目的中文说明文档（README）。该项目提供了一个用 C 语言实现的轻量级 AI / 大模型 API 客户端示例，方便在 C 项目中快速调用 HTTP 接口与 AI 服务交互（例如 OpenAI / 其他兼容服务）。

## 特性

- 纯 C 语言实现，零依赖或小依赖（可编译为静态或动态库）。
- 简单的 API 封装，便于发送请求、接收响应并解析 JSON（建议配合 cJSON 等库）。
- 示例代码展示请求构造、认证与响应处理。

## 先决条件

- C++ 编译器（g++）
- make 构建工具
- OpenSSL 库（用于SSL/TLS连接）
- 网络访问权限
- Windows平台额外需要：Winsock2库

### 安装依赖

**Ubuntu/Debian系统：**
```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

**CentOS/RHEL系统：**
```bash
sudo yum install gcc-c++ openssl-devel
```

**Windows系统（使用MinGW或MSYS2）：**
```bash
sudo pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl
```

## 快速开始

1. 克隆仓库或下载源代码

   ```bash
   git clone https://github.com/ycm50/ai_api_c.git
   cd ai_api_c
   ```

2. 修改代码中的API密钥

   在`ai-api-c_posix.cpp`和`ai-api-c_windows.cpp`文件中，找到并修改以下行：
   ```cpp
   static const char *API_KEY = "sk-xxxxx";  // 替换为您的实际API密钥
   ```

3. 编译项目

   项目现在使用统一的Makefile，会根据当前操作系统自动选择正确的源文件和编译选项：
   ```bash
   make
   ```

4. 运行程序

   **POSIX平台：**
   ```bash
   ./ai-api
   ```

   **Windows平台：**
   ```bash
   ./ai-api.exe
   ```

## 使用示例（伪代码）

下面给出一个调用 AI 接口的简化示例：

```c
#include <stdio.h>
#include "ai_api.h" // 假设项目提供的头文件

int main() {
    const char *prompt = "请用中文介绍一下 C 语言的优点。";
    ai_client_t *client = ai_client_create(getenv("AI_API_KEY"));
    if (!client) return 1;

    ai_response_t *resp = ai_client_completion(client, prompt);
    if (resp) {
        printf("AI 回答:\n%s\n", resp->text);
        ai_response_free(resp);
    }

    ai_client_free(client);
    return 0;
}
```

请查看仓库中的 examples/ 目录获取真实示例和构建说明。

## 配置说明

- 环境变量：AI_API_KEY 或 OPENAI_API_KEY（根据实现选择）
- 可选配置：API 端点（如果要使用自托管或兼容服务）

## 常见问题（FAQ）

- Q: 如何解析 JSON？
  A: 建议使用 cJSON、jansson 或类似的 C JSON 库来解析响应。

- Q: 是否支持流式响应？
  A: 如果后端支持流式输出，客户端需使用 libcurl 的 easy/multi 接口或原生 socket 来处理分块传输。

## 贡献

欢迎提交 issue 或 PR：

- 提交 bug 报告和复现步骤
- 提交功能请求并说明用例
- 提交代码时请遵循仓库已有的代码风格与测试用例

## 许可证

本仓库的许可证信息请参见项目中的 LICENSE 文件。若没有，请联系仓库维护者添加合适的许可证（例如 MIT）。

## 联系

如有问题，请在 GitHub 仓库中打开 issue，或联系维护者 ycm50。