# 线程模型和锁策略

```
							  Server（主线程）
                           /        |        \
              cli_thread_A      cli_thread_B      cli_thread_C
              ├─ Worker_A       ├─ Worker_B       ├─ Worker_C
              └─ FileTrans_A    └─ FileTrans_B    └─ FileTrans_C

              ▲ 子线程通过信号 → 主线程槽函数（QueuedConnection）
              │ Worker::send_server  →  Server::receiveCliMsg
              │
              ▼ 主线程通过信号 → 子线程槽函数（QueuedConnection）
                Server::receiveFile  →  FileTransformer::doReceiveFile


                            Client（主线程）
                              │
                              └─ fileTransferThread
                                   └─ FilesTransFerer

              ▲ 主线程信号 → 子线程槽（QueuedConnection）
              │ Client::TransferSharedFile  →  FilesTransFerer::TransferSharedFile
              │ Client::TransferPrivateFile →  FilesTransFerer::TransferPrivateFile
              │
              ■ FilesReceiver 在主线程，与 Client 直连（DirectConnection）
```

**锁策略：**

| 端     | 锁类型                    | 原因                                                         |
| ------ | ------------------------- | ------------------------------------------------------------ |
| 服务端 | `QMutex`（非递归）        | `receiveCliMsg` 持锁只做 socket 读，读完解锁再 emit/广播     |
| 客户端 | `QRecursiveMutex`（递归） | `receiveMsg` 持锁后 emit 信号，槽函数（如 `receiveAvailableFiles`）同步执行又需要加锁 |