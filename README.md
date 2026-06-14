# QtChat - 简易局域网网络聊天室

基于 C/S 架构的局域网聊天室，使用 Qt6 开发，覆盖 9 项 Qt 核心技术栈。

发行版本中

v1.0.0是客户端

v1.0是服务端

## 项目结构

```
Qt_SimpleChatRoom/
├── Server/                    # 服务端
│   ├── main.cpp
│   ├── CMakeLists.txt
│   ├── Server/
│   │   ├── server.h           # 服务端主类
│   │   ├── server.cpp
│   │   └── server.ui
│   └── Worker/
│       ├── worker.h           # 工作线程类（心跳检测+数据接收）
│       └── worker.cpp
├── Client/                    # 客户端
│   ├── main.cpp
│   ├── CMakeLists.txt
│   ├── Client/
│   │   ├── client.h           # 客户端主类
│   │   ├── client.cpp
│   │   └── client.ui
│   └── serverSocket/
│       ├── serverSocket.h     # QTcpSocket 子类（暴露 setPeerName）|暂时没什么用
│       └── serverSocket.cpp
└── Document/                  # 开发文档
    ├── requireDoc.md          # 需求文档
    ├── AI_temp.md
    ├── AI_chat_record.md
    └── project_log.md
```

## 功能

- **多客户端并发**：每个客户端独立线程处理，互不阻塞
- **实时聊天**：消息存入 SQLite 数据库后广播给所有在线用户
- **历史消息**：新连接时自动推送最近 N 条聊天记录（默认五条）
- **心跳检测**：客户端每 3s 发送心跳，服务端 10s 无响应自动踢出
- **随机昵称**：同名用户自动附加 #四位随机数 区分
- **Enter 快捷发送**：Enter 发送，Shift+Enter 换行
- **连接状态**：QPainter 绘制红/黄/绿连接状态指示灯
- **服务器关闭广播**：关闭服务器时通知所有客户端

## 通信协议

消息首字节为类型标识：

| 标识 | 含义 |
|------|------|
| `0` | 历史消息（批量） |
| `1` | 聊天消息 |
| `2` | 心跳包 |
| `3` | 服务器关闭 |

首个数据包为客户端昵称。

## Qt 技术栈

| 技术 | 应用 |
|------|------|
| Signals & Slots | 全局事件通信 |
| Random Numbers | `QRandomGenerator` 生成昵称后缀 |
| Events | `eventFilter` 拦截键盘事件 |
| Multithreading | Worker + `moveToThread()`，每客户端一线程 |
| Database (SQLite + MVD) | `QSqlTableModel` + `QTableView` |
| Socket (TCP) | `QTcpServer` + `QTcpSocket` |
| Basic Widgets | `QListWidget`、`QTextEdit`、`QLineEdit`、`QPushButton` 等 |
| QTimer | 心跳定时器 |
| QPainter | 连接状态指示灯绘制 |

## 构建

### 依赖

- Qt 6（Core / Gui / Widgets / Network / Sql）
- CMake 3.31+
- MinGW (Windows) 或 MSVC

### 编译

```bash
# 服务端
cd Server
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 客户端
cd Client
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 打包（Windows）

```bash
windeployqt --no-translations path/to/project_client.exe
```

## 运行

1. 启动服务端 `project_server.exe`
2. 客户端填写服务器 IP 和端口（默认 12345），点击连接
3. 两台电脑需在同一局域网，服务器端需放行端口 12345（Windows 防火墙）
