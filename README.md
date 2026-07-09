# QtChat — 网络会议室

基于 C/S 架构的网络会议室，使用 Qt6 开发，支持多客户端并发、实时聊天、文件传输（共享/私发）。

---

### 效果图

**总体：**

![总体效果](https://raw.giteeusercontent.com/RH211-sys/qt---simple-online-chat-room/raw/master/Document/twice_period/images/image-20260709220108769.png?metadata=eyJyIjoibWFzdGVyIiwiZnAiOiJEb2N1bWVudC90d2ljZV9wZXJpb2QvaW1hZ2VzL2ltYWdlLTIwMjYwNzA5MjIwMTA4NzY5LnBuZyIsInVpZCI6MTYyNTExNzksInBpZCI6NDg0Nzc2MDYsInN0byI6ImdpdC1zaGFyZGluZy1zdG8tMTB0LTAzMCIsInJwIjoicmVwb3MvOGEvMGYvOGEwZmMxYzdlZmE1YjE3ZTVjOWFhNGEwY2VmMzllNWM3YmU1ZjFiZDNhNmI1MjAyMDhkZWIxZjAzYWFjNTZkMS5naXQiLCJpc3AiOnRydWUsImV4cGlyZV9hdCI6MTc4MzYwODAwMH0&signature=8Tsgkz624IuOZYKbyBs8aqRO6rPXvLJ2innfXo8Z03M)

> 白色的是客户端，黑色的是服务端

**客户端的下载模块**

![下载模块](https://raw.giteeusercontent.com/RH211-sys/qt---simple-online-chat-room/raw/master/Document/twice_period/images/image-20260709220256935.png?metadata=eyJyIjoibWFzdGVyIiwiZnAiOiJEb2N1bWVudC90d2ljZV9wZXJpb2QvaW1hZ2VzL2ltYWdlLTIwMjYwNzA5MjIwMjU2OTM1LnBuZyIsInVpZCI6MTYyNTExNzksInBpZCI6NDg0Nzc2MDYsInN0byI6ImdpdC1zaGFyZGluZy1zdG8tMTB0LTAzMCIsInJwIjoicmVwb3MvOGEvMGYvOGEwZmMxYzdlZmE1YjE3ZTVjOWFhNGEwY2VmMzllNWM3YmU1ZjFiZDNhNmI1MjAyMDhkZWIxZjAzYWFjNTZkMS5naXQiLCJpc3AiOnRydWUsImV4cGlyZV9hdCI6MTc4MzYwODAwMH0&signature=h6_mTQ2ycErqmEz-pylvCYXPpoUbQUp2nzXTke7Leh0)

> 左边是前一张图接收共享文件的，右边是前一张图接收共享文件和私发文件的
> 即使客户端退出了，它留下的共享文件仍然可以给新来的用户下载，私发文件仍然可以给目标用户(只要它还存在)

### 注意事项

1. 下载模块没有安全路径检测，用户需要自行检查自己的下载路径是否正确(有"浏览按钮")
2. 服务器的默认下载路径为父目录的fileDepot下面(即../fileDepot/)，用户需要事先创建这个目录或自己重新选择目录
3. 默认只能用局域网，如果需要跨局域网的话需要自行安装内网穿透工具

## 功能

### 消息模块
- **实时聊天**：消息存入 SQLite 数据库后广播给所有在线用户
- **私发消息**：点对点私聊，支持成功/失败回执
- **历史消息**：新连接时自动推送最近 N 条聊天记录（默认 5 条，可配置上限）
- **在线用户列表**：实时显示当前在线用户，上下线自动更新

### 文件传输模块
- **共享文件上传**：客户端上传文件到服务端仓库，广播通知所有在线用户
- **私发文件上传**：客户端上传文件，仅目标用户可下载
- **文件列表查询**：客户端刷新可下载文件列表（共享 + 该用户的私发）
- **文件下载**：选择列表中文件，下载到本地指定目录
- **服务端仓库路径可配**：服务端可自由设置文件存放路径
- **文件名前缀区分**：服务端用 `[share]` / `[private]` 前缀区分共享和私发文件，防止重名冲突
- **在线用户列表**：实时显示当前在线用户，上下线自动更新

### 系统模块
- **多客户端并发**：每个客户端独立 `QThread`，互不阻塞
- **心跳检测**：客户端每 3s 发送心跳，服务端 10s 无心跳判定超时自动踢出
- **随机昵称**：同名用户自动附加 `#四位随机数` 区分
- **服务器踢人**：服务端可按用户名踢出指定客户端
- **服务器关闭广播**：关闭服务器时通知所有客户端，并自动下线
- **Enter 快捷发送**：Enter 发送，Shift+Enter 换行
- **连接状态指示灯**：`QPainter` 绘制红（断开）/ 黄（连接中）/ 绿（已连接）

---

## 项目结构

```
Qt_SimpleChatRoom/
├── README.md
│
├── Document/
│   ├── start_period/
│   │   ├── requireDoc.md           # 需求文档
│   │   ├── project_note.md         # 项目笔记
│   │   ├── AI_temp.md
│   │   └── AI_chat_record.md
│   └── twice_period/
│       ├── agreements.md           # 协议设计文档
│       ├── FileTransfer_note.md    # 文件传输草稿
│       ├── conclusion/
│       │   └── agr_conclu.md       # 二次开发协议汇总
│       └── project_log/
│           ├── day1.md             # 内网穿透测试、踢人、消息上限
│           ├── day2.md             # 消息私发、粘包修复
│           ├── day3.md             # 文件传输框架搭建
│           ├── day4.md             # 协议重构、在线用户列表
│           ├── day5.md             # 跨线程 socket 损坏修复（锁+flush）
│           └── day6.md             # Bug 修复收尾、细节优化
│
├── Server/
│   ├── CMakeLists.txt
│   ├── main.cpp                    # 服务端入口
│   ├── chatInfo.db                 # SQLite 聊天记录数据库
│   ├── Server/
│   │   ├── server.h                # Server 类声明
│   │   ├── server.cpp              # 连接管理 / 消息分发 / 广播 / 数据库
│   │   └── server.ui               # 服务端界面
│   └── Worker/
│       ├── worker.h                # Worker 类声明
│       ├── worker.cpp              # 心跳检测 / socket peek
│       ├── fileTransformer.h       # FileTransformer 类声明
│       └── fileTransformer.cpp     # 文件收发（接收存盘 / 发送下载）
│
├── Client/
│   ├── CMakeLists.txt
│   ├── main.cpp                    # 客户端入口
│   ├── Client/
│   │   ├── client.h                # Client 类声明
│   │   ├── client.cpp              # 消息解析 / 跨线程信号中转 / UI 交互
│   │   └── client.ui               # 客户端主界面
│   ├── serverSocket/
│   │   ├── serverSocket.h          # QTcpSocket 子类，暴露 setPeerName
│   │   └── serverSocket.cpp
│   └── worker/
│       ├── file_transfer_module/
│       │   ├── FilesTransFerer.h    # 文件上传（共享/私发）
│       │   └── FilesTransFerer.cpp
│       └── files_receive_module/
│           ├── filesreceiver.h      # 文件下载界面 + 文件列表查询
│           ├── filesreceiver.cpp
│           └── filesreceiver.ui
│
└── fileDepot/                       # 服务端文件仓库（运行时目录）
```

---

## 通信协议

### 协议格式规则

| 规则 | 说明 |
|---|---|
| flag 1 字节 | 每条消息首字节为协议族标识 |
| subType 3 字节 | 文件传输和聊天消息紧跟 3 字节子类型 |
| 变长字段 | `{size}` + `INTERUPT` + `{body}`，或直接用 `INTERUPT` 分隔后由接收方逐字节读取 |
| 文件边界 | `FILE_TRANSFER_END` 标记文件数据结束 |
| 循环消费 | 客户端 `receiveMsg` 用 `while (bytesAvailable > 0)` 循环处理粘包 |

### 宏定义速查

| 宏 | 值 | 含义 |
|---|---|---|
| `FIRST_BAG` | `"0"` | 首包（注册用户名） |
| `CHAT_INFO` | `"1"` | 聊天消息父标志 |
| `PUNPING_INFO` | `"2"` | 心跳包 |
| `SERVER_CLOSE` | `"3"` | 服务器关闭 |
| `SERVER_KICK` | `"4"` | 服务器踢人 |
| `FILE_TRANSFER_REQUEST` | `"A"` | 文件传输请求（C→S） |
| `FILE_TRANSFER_RESULT` | `"B"` | 文件传输响应（S→C） |
| `SEARCH_SUCCESS` | `"6"` | 私发成功回执 |
| `SEARCH_FAILED` | `"7"` | 私发失败回执 |
| `INTERUPT` | `"\x1E"` | 字段分隔符 |
| `FILE_TRANSFER_END` | `"\x03"` | 文件传输结束标记 |

**CHAT_INFO 子类型：**

| 宏 | 值 | 方向 | 含义 |
|---|---|---|---|
| `CHAT_BROADCAST` | `"000"` | 双向 | 广播消息 |
| `CHAT_PRIVATE_REQ` | `"001"` | C→S | 私发请求 |
| `CHAT_PRIVATE_FWD` | `"995"` | S→C | 私发转发 |
| `CHAT_HISTORY` | `"996"` | S→C | 历史记录 |
| `CHAT_USER_OFFLINE` | `"997"` | S→C | 用户下线通知 |
| `CHAT_USER_INIT` | `"998"` | S→C | 在线用户列表初始化 |
| `CHAT_USER_JOIN` | `"999"` | S→C | 新用户加入通知 |

**FILE_TRANSFER 子类型：**

| 宏 | 值 | 方向 | 含义 |
|---|---|---|---|
| `FT_SHARED_UPLOAD` | `"000"` | C→S | 共享文件上传 |
| `FT_PRIVATE_UPLOAD` | `"001"` | C→S | 私发文件上传 |
| `FT_QUERY_FILES` | `"002"` | C→S | 查询可下载文件列表 |
| `FT_DOWNLOAD_REQ` | `"003"` | C→S | 请求下载文件 |
| `FT_ACK_SENDER` | `"993"` | S→C | 上传回执 |
| `FT_SEND_FILE` | `"994"` | S→C | 发送文件数据 |
| `FT_QUERY_FAIL` | `"995"` | S→C | 查询失败 / 无可用文件 |
| `FT_QUERY_SUCCESS` | `"997"` | S→C | 查询文件列表成功 |
| `FT_PRIVATE_NOTIFY` | `"998"` | S→C | 私发文件通知 |
| `FT_SHARED_NOTIFY` | `"999"` | S→C | 共享文件通知 |

### 客户端 → 服务端

**首包（注册用户名）**
```
FIRST_BAG + {nameSize} + INTERUPT + {name}
```
专属 flag `"0"` 隔离，避免尺寸数字与心跳等 flag 冲突。

**心跳包**
```
PUNPING_INFO
```
即单字节 `"2"`。客户端每 3s 发送一次，服务端 10s 无心跳则判定超时踢出。

**广播消息**
```
CHAT_INFO + CHAT_BROADCAST + {bodySize} + INTERUPT + {body}
```

**私发消息**
```
CHAT_INFO + CHAT_PRIVATE_REQ + {targetName} + INTERUPT + {bodySize} + INTERUPT + {body}
```

**共享文件上传**
```
FILE_TRANSFER_REQUEST + FT_SHARED_UPLOAD + {fileName} + INTERUPT + {fileSize} + INTERUPT + [file data] + FILE_TRANSFER_END
```
服务端存盘时加 `[share]` 前缀。

**私发文件上传**
```
FILE_TRANSFER_REQUEST + FT_PRIVATE_UPLOAD + {targetName} + INTERUPT + {fileName} + INTERUPT + {fileSize} + INTERUPT + [file data] + FILE_TRANSFER_END
```
服务端存盘时加 `[private]` 前缀。

**查询可下载文件列表**
```
FILE_TRANSFER_REQUEST + FT_QUERY_FILES + FILE_TRANSFER_END
```
总长 5 字节（`"A002\x03"`）。

**请求下载文件**
```
FILE_TRANSFER_REQUEST + FT_DOWNLOAD_REQ + {fileName} + FILE_TRANSFER_END
```

### 服务端 → 客户端

**历史聊天记录（连接后立即发送）**
```
CHAT_INFO + CHAT_HISTORY + {bodySize} + INTERUPT + {records}
```
- `{records}`: 每条记录格式 `{time}  {senderIP}  {userName}  {msg}`，以 INTERUPT 分隔

**在线用户列表（连接后立即发送）**
```
CHAT_INFO + CHAT_USER_INIT + {bodySize} + INTERUPT + {name1} + INTERUPT + {name2} + INTERUPT + ...
```

**广播消息（转发）**
```
CHAT_INFO + CHAT_BROADCAST + {bodySize} + INTERUPT + {userName + " :" + msg}
```

**私发消息转发**
```
CHAT_INFO + CHAT_PRIVATE_FWD + {senderName} + INTERUPT + {bodySize} + INTERUPT + {body}
```

**新用户加入通知**
```
CHAT_INFO + CHAT_USER_JOIN + {name} + INTERUPT
```

**用户下线通知**
```
CHAT_INFO + CHAT_USER_OFFLINE + {name} + INTERUPT
```

**私发成功回执**
```
SEARCH_SUCCESS + INTERUPT
```
（content 为空，预留扩展）

**私发失败回执**
```
SEARCH_FAILED + INTERUPT
```

**服务器关闭**
```
SERVER_CLOSE + {bodySize} + INTERUPT + {body}
```

**被踢出通知**
```
SERVER_KICK + {bodySize} + INTERUPT + {body}
```
当前为静态消息，预留为管理员自定义理由。

**共享文件通知（广播所有在线用户）**
```
FILE_TRANSFER_RESULT + FT_SHARED_NOTIFY + {fileName} + FILE_TRANSFER_END
```

**私发文件通知（仅目标用户）**
```
FILE_TRANSFER_RESULT + FT_PRIVATE_NOTIFY + {fileName} + FILE_TRANSFER_END
```

**查询文件列表成功**
```
FILE_TRANSFER_RESULT + FT_QUERY_SUCCESS + {bodySize} + INTERUPT + {fileName1} + INTERUPT + {fileName2} + INTERUPT + ...
```
包含共享文件 + 该用户的私发文件。

**查询文件列表失败（无可用文件）**
```
FILE_TRANSFER_RESULT + FT_QUERY_FAIL
```
即 4 字节 `"B995"`，无 body。

**发送文件数据**
```
FILE_TRANSFER_RESULT + FT_SEND_FILE + {fileName} + INTERUPT + {fileSize} + INTERUPT + [file data] + FILE_TRANSFER_END
```
- 文件数据 64KB 分片发送

**上传回执**
```
FILE_TRANSFER_RESULT + FT_ACK_SENDER + {status} + FILE_TRANSFER_END
```
- `{status}` 取值：
  - `"000ok"` — 共享文件上传成功
  - `"001ok"` — 私发文件上传成功
  - `"001fail"` — 私发文件上传失败（目标用户不在线）

---

## 架构设计

### 线程模型

```
Server（主线程）
├── cli_thread_A ─── Worker_A ─── 心跳检测、socket peek
│                    FileTransformer_A ─── 文件收发
├── cli_thread_B ─── Worker_B / FileTransformer_B
└── cli_thread_C ─── Worker_C / FileTransformer_C

Client（主线程）
└── fileTransferThread ─── FilesTransFerer ─── 文件上传
    FilesReceiver ─── 主线程，与 Client 直连
```

### 跨线程通信

| 方向 | 信号 → 槽 | 连接类型 |
|---|---|---|
| 子线程 → 主线程 | `Worker::send_server` → `Server::receiveCliMsg` | QueuedConnection |
| 主线程 → 子线程 | `Server::receiveFile` → `FileTransformer::doReceiveFile` | QueuedConnection |
| 主线程 → 子线程（客户端） | `Client::TransferSharedFile` → `FilesTransFerer::TransferSharedFile` | QueuedConnection |
| 主线程内（客户端） | `Client::getFiles` → `FilesReceiver::receiveAvailableFiles` | DirectConnection |

### 锁策略

| 端 | 锁类型 | 原因 |
|---|---|---|
| 服务端 | `QMutex`（非递归） | 持锁仅做 socket 读写，读完立即解锁再 emit/广播，不需要递归 |
| 客户端 | `QRecursiveMutex`（递归） | `receiveMsg` 持锁期间 emit 信号（DirectConnection），槽同步执行又需加锁，必须可重入 |

### 服务端数据流

```
QTcpServer::newConnection
  → Server::handleConnect
    → 创建 QThread + Worker + FileTransformer + 每 socket 的 QMutex
    → 移入子线程，started → Worker::doWork（启动心跳定时器）
    → 发送历史记录 + 在线用户列表

QTcpSocket::readyRead（子线程）
  → Worker::read_msg_cli
    → 加锁 peek(1) → 获取 flag → 解锁
    → emit send_server(flag, socket)
      → Server::receiveCliMsg（主线程）
        → 根据 flag 分发：
          FIRST_BAG              → 加锁消费 flag，解析 size+name，解锁后注册广播
          PUNPING_INFO           → 加锁 read(1) 消费心跳，重置定时器
          CHAT_INFO              → 加锁 read flag+subType+body，解锁后 broadcast/tellPointedUser
          FILE_TRANSFER_REQUEST  → emit receiveFile(socket)
            → FileTransformer::doReceiveFile（子线程）
              加锁 → 读协议 → 文件操作 → write + flush → 解锁 → emit 通知主线程
```

---

## 构建

### 环境

| 项 | 值 |
|---|---|
| Qt | 6.5.3 (mingw_64) |
| C++ 标准 | C++17 |
| 构建系统 | CMake 3.31+ |
| 编译器 | MinGW (GCC) / MSVC 双兼容 |
| 依赖 | 仅 Qt6（Core / Gui / Widgets / Network / Sql） |
| 数据库 | SQLite（QSqlTableModel + QTableView） |

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
windeployqt --no-translations path/to/project_server.exe
windeployqt --no-translations path/to/project_client.exe
```

---

## Qt 技术栈

| 技术 | 应用 |
|---|---|
| Signals & Slots | 跨线程通信（QueuedConnection）、UI 事件处理 |
| Multithreading | `QThread` + `moveToThread()`，每客户端一线程 |
| Socket (TCP) | `QTcpServer` + `QTcpSocket`，完整应用层协议 |
| Database (SQLite + MVD) | `QSqlTableModel` + `QTableView` 聊天记录持久化 |
| QMutex / QRecursiveMutex | 跨线程 socket 访问保护 |
| QTimer | 心跳定时器（客户端 3s 发送，服务端 10s 超时踢出） |
| QPainter | 连接状态指示灯绘制 |
| Events | `eventFilter` 拦截 Enter / Shift+Enter 键盘事件 |
| Model/View | `QStringListModel` + `QListView` 在线用户列表 |
| QFileDialog | 文件浏览选择（上传 / 下载路径选择） |
| QRandomGenerator | 同名用户昵称随机后缀 |
| QFile / QFileInfo | 文件读写、分片传输 |

---

## 关键设计决策

| 决策 | 选择 | 原因 |
|---|---|---|
| 线程模型 | thread-per-client | 简单直观，每个客户端独立线程，适合小规模（<100）并发 |
| socket 保护 | `QMutex` + `flush()` | 改动最小，不破坏现有架构；flush 解决子线程无事件循环驱动写缓冲的问题 |
| 服务端锁类型 | `QMutex`（非递归） | 精心控制锁范围：持锁仅做 socket 读写，emit/广播前解锁 |
| 客户端锁类型 | `QRecursiveMutex` | `receiveMsg` 持锁 emit → DirectConnection 槽同步执行 → 又需加锁，必须可重入 |
| 心跳 | 单向通知 + 超时踢出 | 客户端定时写 `"2"`，服务端收到重置定时器，超时即断线，减少网络流量 |
| 协议格式 | 1B flag + 3B subType + 变长 body | 紧凑可扩展，flag 决定协议族，subType 决定具体操作 |
| 首包设计 | 专属 flag `"0"` + 长度前缀 | 避免首包尺寸数字与 flag 冲突（如 2 字节名 '2' 被误判为心跳） |
| 文件存储 | 服务端统一仓库，`[share]`/`[private]` 前缀 | 集中管理，共享文件存一份，下载时按需发送；前缀防止共享和私发文件重名 |
| 跨线程文件信号 | 参数用 `QString` 传路径 | 避免 `QFileInfo` 元类型注册问题，`QString` 在 Qt 中天然支持 QueuedConnection |

---

## 关键 Bug 修复记录

| # | 问题 | 根因 | 修复 |
|---|---|---|---|
| 1 | 点击刷新后 socket 不可读 | 主线程和子线程同时读写同一 `QTcpSocket`，`QIODevice` 内部状态损坏 | 每 socket 加 `QMutex` 保护所有读写操作 |
| 2 | 加锁后 write 返回成功但客户端收不到数据 | `QTcpSocket::write()` 仅写入 Qt 内部缓冲，子线程无事件循环不自动 flush | 所有 `write()` 后加 `flush()` |
| 3 | `doTransfer` 死锁 | `doReceiveFile` 持锁调用 `doTransfer`，后者再次 `QMutexLocker` 同一非递归锁 | 删除 `doTransfer` 内部锁（调用方已持锁） |
| 4 | 文件上传信号不触发 | `QFileInfo` 未注册元类型，QueuedConnection 无法序列化参数 | 信号参数改为 `QString`（传 `absoluteFilePath()`），Qt 原生支持 |
| 5 | `QMutex::Recursive` 编译失败 | Qt6 移除了 `QMutex::Recursive` 枚举 | 改用 `QRecursiveMutex`（客户端 6 处） |
| 6 | 首包被误判为心跳 | 2 字节名字（如 "AA"）首包尺寸前缀首字节 `'2'` = `PUNPING_INFO`，被心跳分支消费 | 首包加专属 flag `FIRST_BAG "0"` + 长度前缀 |
| 7 | 下载文件路径错误 | `QFileDialog::getExistingDirectory` 返回路径无末尾 `/`，拼接后跳父目录 | 拼接时加 `"/"` |

---

## 已知问题

1. **`FileTransformer` 跨线程操作 UI** — `server->writeLog()` 在子线程直接调用 `ui->logInfo->addItem()`，属于 Qt 禁止操作（未定义行为）。应改为 emit 信号到主线程写日志。
2. **文件大小 32 位限制** — `fileSize` 用 `int` 解析，单文件上限约 2GB。
3. **服务端无并发上限** — 无连接节流，恶意客户端可耗尽线程资源。
4. **`fileTransformer.h` 中 `downloadFilePath` 声明为 `const QString`** — `modifyDepot` 对 const 成员赋值实际无效。
5. **`worker.h` 中 `punpingTime` 为普通 int** — 非 `inline constexpr`，多 include 可能重定义。
6. **私发文件目标不在线时文件被丢弃** — 无离线缓存机制。
7. **客户端的下载文件和服务端的下载文件没有进行下载路径的安全检测措施**，需要用户自己防范
8. **私信文件窃取：**服务器未关闭时，客户端可能通过不断上下线刷取到对应的名字+随机数，获取它所有的private文件
9. **使用的是TCP明文传输，未加密：**传输有窃听风险

---

##  后续优化方向

| 方向                         | 说明                                                         |
| ---------------------------- | ------------------------------------------------------------ |
| 线程池替代 thread-per-client | 当前每个客户端一个 `QThread`，大规模客户端并发时线程开销大   |
| 加密传输                     | 当前明文 TCP，可加 TLS（`QSslSocket`）                       |
| 日志模块统一                 | 将 `writeLog` 改为信号机制，全项目统一走主线程               |
| 高性能文件传输               | 文件传输采用Worker池，实现更快的传输，<br />实现更大的文件传输 |
| 服务端数据库优化             | 先在服务器缓存消息数据，当服务器关闭后或定时自动写入数据库(redis思想)，启动后导入数据 |
| 服务端自定义端口号           | 不用静态的12345，可以输入端口号自己定                        |
| 行为记录模块                 | 客户端和服务端都能记录行为<br />如设置的下载路径，端口号，名字等能记录到文件中，下载启动后自动填充 |
| 服务端循环开关               | 可以关闭和开启服务器，不用通过启动和关闭进程来开关服务器     |
| 服务端的文件管理模块         | 能够管理客户端上传的文件，删除/重命名等                      |
| 路径安全检测                 | 确认目标路径/文件存在，否则响应错误信息                      |
