#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

// 链接必要的库
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

static const char *HOST      = "api.deepseek.com";
static const int   PORT      = 443;                // HTTPS默认端口
static const char *API_KEY   = "sk-xxxxx";
static const char *PATH      = "/v1/chat/completions";
const std::string global_prompt = "你要尽可能简短的回答我的问题";
const float temperature = 0.7f;

/* 简单的 URL 编码：只处理双引号、换行、反斜杠等关键字符 */
static std::string escape_json(const std::string &s)
{
    std::string r;
    r.reserve(s.size() * 2);
    for (unsigned char c : s) {
        switch (c) {
        case '"': r += "\\""";
        case '\\': r += "\\\\";
        case '\b': r += "\\b";
        case '\f': r += "\\f";
        case '\n': r += "\\n";
        case '\r': r += "\\r";
        case '\t': r += "\\t";
        default:
            if (c < 0x20) { char buf[8]; std::snprintf(buf, sizeof(buf), "\\u%04x", c); r += buf; }
            else r += c;
        }
    }
    return r;
}

// 输入：含 chunked 的 HTTP body  （不含头）
// 输出：去掉 chunk 元数据后的纯 payload
static std::string decode_chunked(const std::string &in)
{
    std::string out;
    out.reserve(in.size());
    std::istringstream sin(in);
    for (;;)
    {
        std::string line;
        if (!std::getline(sin, line)) break;   // 读长度行
        // 去掉可能的 \r
        if (!line.empty() && line[line.size()-1] == '\r')
            line.erase(line.size()-1);

        int len = 0;
        if (sscanf(line.c_str(), "%x", &len) != 1) break; // 16 进制长度
        if (!len) break;                                    // 最后 0 块

        // 读 len 个字节
        std::string chunk(len, '\0');
        sin.read(&chunk[0], len);
        if (!sin) break;        // 数据不够
        out += chunk;

        // 跳过块结尾的 CRLF
        sin.ignore(2);          // \r\n
        // 修复：确保流指针在正确的位置
        if (sin.peek() == EOF) break;
    }
    return out;
}

// Windows错误处理函数
void print_windows_error(const char* function_name)
{
    int error_code = WSAGetLastError();
    char* error_msg = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&error_msg,
        0,
        NULL
    );
    std::cerr << function_name << " failed: " << error_msg << std::endl;
    LocalFree(error_msg);
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    /* 初始化Winsock */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    /* 初始化OpenSSL库 */
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) { std::cerr << "SSL_CTX_new failed\n"; WSACleanup(); return 1; }

    /* 1. 解析域名 */
    struct addrinfo hints, *result, *ptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[6];
    sprintf(port_str, "%d", PORT);
    int iResult = getaddrinfo(HOST, port_str, &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << std::endl;
        SSL_CTX_free(ctx);
        WSACleanup();
        return 1;
    }

    /* 2. 创建 TCP 套接字并连接 */
    SOCKET fd = INVALID_SOCKET;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (fd == INVALID_SOCKET) {
            print_windows_error("socket");
            continue;
        }

        /* 3. 连接 */
        iResult = connect(fd, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(fd);
            fd = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result); // 释放地址信息

    if (fd == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server\n";
        SSL_CTX_free(ctx);
        WSACleanup();
        return 1;
    }

    /* 4. 创建并配置SSL对象 */
    SSL *ssl = SSL_new(ctx);
    if (!ssl) { std::cerr << "SSL_new failed\n"; closesocket(fd); SSL_CTX_free(ctx); WSACleanup(); return 1; }
    SSL_set_fd(ssl, fd);
    if (SSL_connect(ssl) <= 0) {
        std::cerr << "SSL_connect failed\n"; 
        SSL_free(ssl);
        closesocket(fd);
        SSL_CTX_free(ctx);
        WSACleanup();
        return 1;
    }

    std::string prompt;
    std::cout << "请输入问题（空行退出）：";
    while (std::getline(std::cin, prompt) && !prompt.empty())
    {
        std::string user_content = escape_json(global_prompt + "\\n用户问题：" + prompt);

        char body[4096];
        int  body_len = std::snprintf(body, sizeof(body),
            "{\"model\":\"deepseek-chat\","\
            "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"\
            "\"temperature\":%.2f}", user_content.c_str(), temperature);

        char header[1024];
        int  header_len = std::snprintf(header, sizeof(header),
            "POST %s HTTP/1.1\r\n"\
            "Host: %s\r\n"\
            "Authorization: Bearer %s\r\n"\
            "Content-Type: application/json\r\n"\
            "Content-Length: %d\r\n"\
            "Connection: close\r\n"\
            "\r\n", PATH, HOST, API_KEY, body_len);

        /* 5. 使用SSL发送 */
        std::string req(header, header_len);
        req.append(body, body_len);
        if (SSL_write(ssl, req.data(), req.size()) <= 0) {
            std::cerr << "SSL_write failed\n"; 
            break;
        }

        /* 6. 使用SSL接收 */
        std::string full;
        char recv_buf[4096];
        int  n;
        while ((n = SSL_read(ssl, recv_buf, sizeof(recv_buf)-1)) > 0) {
            recv_buf[n] = '\0';
            full += recv_buf;          // 先全部拼起来
        }
        if (full.empty()) { std::cerr << "SSL_read failed\n"; break; }

        /* 简单去掉 HTTP 头，只保留 body */
        size_t body_start = full.find("\r\n\r\n");
        if (body_start == std::string::npos) { std::cerr << "bad response\n"; break; }
        body_start += 4;

        std::string chunked_body = full.substr(body_start);
        std::string json = decode_chunked(chunked_body);

        if (!json.empty() && json[0] == '{') {
            std::cout << "==== JSON 体 ====\n" << json << "\n";
        } else {
            std::cout << "==== 原始回包 ====\n" << full << "\n";
        }
        std::cout << "请输入问题（空行退出）：";
    }

    /* 清理SSL资源 */
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    closesocket(fd);
    WSACleanup();
    return 0;
}