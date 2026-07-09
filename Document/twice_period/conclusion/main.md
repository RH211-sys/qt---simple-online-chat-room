# 项目总结

## 1. 架构设计

### 类职责一览

**服务端：**

| 类                | 所在线程   | 职责                                                         |
| ----------------- | ---------- | ------------------------------------------------------------ |
| `Server`          | 主线程     | 连接管理（accept）、消息路由（根据 flag 分发）、广播（3 个重载）、数据库读写、UI 交互 |
| `Worker`          | cli_thread | 心跳超时检测（`checkTime` 定时器）、socket peek（探测数据首字节 flag）、断线检测 |
| `FileTransformer` | cli_thread | 文件接收存盘（`doReceiveFile`）、文件发送（`doTransfer`）、文件列表查询 |

**客户端：**

| 类                | 所在线程           | 职责                                                         |
| ----------------- | ------------------ | ------------------------------------------------------------ |
| `Client`          | 主线程             | 消息解析（`receiveMsg` 循环消费粘包）、UI 交互、跨线程信号中转 |
| `serverSocket`    | 主线程             | `QTcpSocket` 的子类，无额外逻辑，仅暴露 `setPeerName`        |
| `FilesTransFerer` | fileTransferThread | 共享/私发文件上传（`TransferSharedFile` / `TransferPrivateFile`） |
| `FilesReceiver`   | 主线程             | 文件下载界面、可用文件列表查询、文件数据接收存盘             |

### 类关系



```
Server ──owns──► QMap<QTcpSocket*, QThread*>     client_group
         ──owns──► QMap<QTcpSocket*, QMutex*>     client_mutexs
         ──owns──► QMap<QTcpSocket*, QString>     client_name
         ──owns──► QMap<QString, QTcpSocket*>     name_to_ip
         ──owns──► QSet<QTcpSocket*>              client_firstBag_set
         ──owns──► QSet<QString>                  sharedFiles
         ──owns──► QMap<QTcpSocket*, QSet<QString>> privateFiles

Worker ──ref──► QTcpSocket* client
       ──ref──► QMutex* socketMutex

FileTransformer ──ref──► Server* server
                ──ref──► QTcpSocket* client
                ──ref──► QMutex* socketMutex

Client ──owns──► serverSocket* serverTar
       ──owns──► QThread* fileTransferThread
       ──owns──► FilesTransFerer* TransferWorker
       ──owns──► FilesReceiver* filesReceiver
       ──owns──► QRecursiveMutex* socketMutex
```

------

## 2. 信号/槽连接全图

### 服务端

| 发送者            | 信号                                   | 接收者                              | 槽                  | 类型       | 说明                            |
| ----------------- | -------------------------------------- | ----------------------------------- | ------------------- | ---------- | ------------------------------- |
| `QTcpServer`      | `newConnection`                        | `Server`                            | `handleConnect`     | Direct     | 新连接到达                      |
| `Worker`          | `send_server(QByteArray, QTcpSocket*)` | `Server`                            | `receiveCliMsg`     | **Queued** | 子线程→主线程，数据到达通知     |
| `Worker`          | `cli_exit(QTcpSocket*)`                | `Server`                            | `handleDisConnect`  | **Queued** | 子线程→主线程，心跳超时/断线    |
| `Server`          | `receiveFile(QTcpSocket*)`             | `FileTransformer`                   | `doReceiveFile`     | **Queued** | 主线程→子线程，转发文件请求     |
| `Server`          | `modifyDepot(QString)`                 | `FileTransformer`                   | `modifyDepot`       | **Queued** | 主线程→子线程，修改仓库路径     |
| `FileTransformer` | `addNewSharedFile(...)`                | `Server`                            | `addNewSharedFile`  | **Queued** | 子线程→主线程，共享文件存盘完成 |
| `FileTransformer` | `addNewPrivateFile(...)`               | `Server`                            | `addNewPrivateFile` | **Queued** | 子线程→主线程，私发文件存盘完成 |
| `QTcpSocket`      | `readyRead`                            | `Worker`                            | `read_msg_cli`      | Direct     | 同线程，数据可读                |
| `QTcpSocket`      | `disconnected`                         | `Worker`                            | `doExit`            | Direct     | 同线程，客户端断线              |
| `QTimer`          | `timeout`                              | `Worker`                            | `doExit`            | Direct     | 同线程，心跳超时                |
| `QThread`         | `started`                              | `Worker`                            | `doWork`            | Direct     | 同线程，启动心跳定时器          |
| `QThread`         | `finished`                             | `Worker` / `FileTransformer` / 自身 | `deleteLater`       | Direct     | 同线程，资源回收                |

### 客户端

| 发送者         | 信号                                    | 接收者            | 槽                      | 类型       | 说明                        |
| -------------- | --------------------------------------- | ----------------- | ----------------------- | ---------- | --------------------------- |
| `serverSocket` | `connected`                             | `Client`          | `isConnected`           | Direct     | 同线程，连接成功            |
| `serverSocket` | `readyRead`                             | `Client`          | `receiveMsg`            | Direct     | 同线程，数据可读            |
| `QTimer`       | `timeout`                               | `Client`          | `sendHeartPing`         | Direct     | 同线程，定时心跳            |
| `Client`       | `TransferSharedFile(QString)`           | `FilesTransFerer` | `TransferSharedFile`    | **Queued** | 主线程→子线程，上传共享文件 |
| `Client`       | `TransferPrivateFile(QString, QString)` | `FilesTransFerer` | `TransferPrivateFile`   | **Queued** | 主线程→子线程，上传私发文件 |
| `Client`       | `receiveFile()`                         | `FilesReceiver`   | `bootProcess`           | Direct     | 同线程，打开文件接收窗口    |
| `Client`       | `getFiles()`                            | `FilesReceiver`   | `receiveAvailableFiles` | Direct     | 同线程，解析文件列表        |
| `Client`       | `downloadFile()`                        | `FilesReceiver`   | `downloadFile`          | Direct     | 同线程，解析文件数据        |

### 系统

- 客户端 `getFiles`/`downloadFile` 与 `FilesReceiver` 是 DirectConnection（双方都在主线程），这就是为什么客户端要用 `QRecursiveMutex`——`receiveMsg` 持锁期间 emit 这些信号，槽同步执行又尝试加锁。

------

## 4. 关键 Bug 与修复

| #    | 问题                                      | 根因                                                         | 修复                                                    |
| ---- | ----------------------------------------- | ------------------------------------------------------------ | ------------------------------------------------------- |
| 1    | 点击刷新后客户端 socket 不可读            | 主线程和 cli_thread 同时对同一 `QTcpSocket` 执行 `read()`/`peek()`/`write()`，QIODevice 内部状态损坏 | 每 socket 加 `QMutex`，所有读写操作加锁保护             |
| 2    | 加锁后服务端 write 返回成功但客户端收不到 | `QTcpSocket::write()` 将数据放入 Qt 内部写缓冲，子线程没有事件循环驱动 flush | 所有 `write()` 后紧跟 `flush()`（共 8 处）              |
| 3    | `doTransfer` 获取锁时卡死                 | `doReceiveFile` 持锁后调用 `doTransfer`，后者再次 `QMutexLocker` 同一把非递归锁 → 自死锁 | 删除 `doTransfer` 内部的 `QMutexLocker`（调用方已持锁） |
| 4    | `QFileInfo` 信号无法跨线程传递            | `QFileInfo` 未注册元类型，QueuedConnection 无法序列化参数    | 改参数为QString表示路径                                 |
| 5    | `FT_QUERY_FAIL` 分支吞字节                | `serverTar->read(1)` 消费了下一条消息的 flag                 | 删除该行                                                |
| 6    | 首包用户名依赖 `readAll()`                | TCP 分片时可能截断                                           | 改为 `{size} + INTERUPT + {name}` 定长协议              |

------

## 6. 构建信息

| 项         | 值                                                           |
| ---------- | ------------------------------------------------------------ |
| Qt 版本    | 6.5.3 (mingw_64)                                             |
| C++ 标准   | C++17                                                        |
| 构建系统   | CMake 3.31+                                                  |
| 编译器     | MinGW (GCC) / MSVC 双兼容                                    |
| IDE        | CLion + Qt Creator                                           |
| 第三方依赖 | Qt6（Core / Gui / Widgets / Network / Sql）                  |
| 数据库     | SQLite（通过 `QSqlDatabase` + `QSqlTableModel`）             |
| 部署       | CMake 自动复制 Qt DLL + platforms/qwindows + sqldrivers/qsqlite |

------

## 7. 已知问题/局限

1. **单文件大小限制** — `fileSize` 用 `QString::toInt()` 解析（32 位 int），单个文件不能超过 ~2GB。
2. **服务端无并发连接上限** — `newConnection` 无节流，恶意客户端可消耗完线程资源。
3. **缺少安全性检查** — 服务端和客户端都没有做输入的安全性检查，如下载路径没有检查路径是否合法

------

## 8. 后续优化方向

| 方向                         | 说明                                                         |
| ---------------------------- | ------------------------------------------------------------ |
| 线程池替代 thread-per-client | 当前每个客户端一个 `QThread`，大规模客户端并发时线程开销大   |
| 加密传输                     | 当前明文 TCP，可加 TLS（`QSslSocket`）                       |
| 日志模块统一                 | 将 `writeLog` 改为信号机制，全项目统一走主线程               |
| 高性能文件传输               | 文件传输采用Worker池，实现更快的传输<br />实现更大的文件传输 |
| 服务端数据库优化             | 先在服务器缓存消息数据，当服务器关闭后或定时自动写入数据库(redis思想)，启动后导入数据 |
| 服务端自定义端口号           | 不用静态的12345，可以输入端口号自己定                        |
| 行为记录模块                 | 客户端和服务端都能记录行为<br />如设置的下载路径，端口号，名字等能记录到文件中，下载启动后自动填充 |
| 服务端循环开关               | 可以关闭和开启服务器，不用通过启动和关闭进程来开关服务器     |
| 服务端的文件管理模块         | 能够管理客户端上传的文件，删除/重命名等                      |
| 路径安全检测                 | 确认目标路径/文件存在，否则响应错误信息                      |

------

## 9. 关键设计决策

| 决策            | 选择                                         | 原因                                                         |
| --------------- | -------------------------------------------- | ------------------------------------------------------------ |
| 线程模型        | thread-per-client                            | 历史原因，初次开发主要是为了实现功能，未用线程池优化，一个客户端一个线程，预留更多的worker拓展，可拓展性强 |
| socket 保护方案 | `QMutex` 加锁                                | 改动最小，不破坏现有架构。加锁点集中在读写操作socket         |
| 服务端锁类型    | `QMutex`（非递归）                           | 精心设计锁范围：持锁只做 socket 读写，emit/广播前解锁，避免递归需求 |
| 客户端锁类型    | `QRecursiveMutex`（递归）                    | `receiveMsg` 持锁期间 emit 信号 → 槽同步执行 → 又需要读 socket，必须可重入 |
| 心跳机制        | 客户端定时 `write("2")`，服务端 reset 定时器 | 简单双向超时检测。防止超时                                   |
| 重构协议格式    | 1字节 flag + 3字节 subType + 变长 body       | 紧凑、易解析、可扩展。flag 决定协议族，subType 决定具体操作  |