# 二次开发的协议汇总

## 一、宏定义

| 宏                      | 值       | 含义                |
| ----------------------- | -------- | ------------------- |
| `FIRST_BAG`             | `"0"`    | 客户端首包          |
| `CHAT_INFO`             | `"1"`    | 聊天消息父标志      |
| `PUNPING_INFO`          | `"2"`    | 心跳包              |
| `SERVER_CLOSE`          | `"3"`    | 服务器关闭          |
| `SERVER_KICK`           | `"4"`    | 服务器踢人          |
| `FILE_TRANSFER_REQUEST` | `"A"`    | 文件传输请求（C→S） |
| `FILE_TRANSFER_RESULT`  | `"B"`    | 文件传输响应（S→C） |
| `SEARCH_SUCCESS`        | `"6"`    | 私发成功回执        |
| `SEARCH_FAILED`         | `"7"`    | 私发失败回执        |
| `INTERUPT`              | `"\x1E"` | 字段分隔符          |
| `FILE_TRANSFER_END`     | `"\x03"` | 文件传输结束标记    |

**CHAT_INFO 子类型：**

| 宏                  | 值      | 含义           |
| ------------------- | ------- | -------------- |
| `CHAT_BROADCAST`    | `"000"` | 广播消息       |
| `CHAT_PRIVATE_REQ`  | `"001"` | 私发请求       |
| `CHAT_PRIVATE_FWD`  | `"995"` | 私发转发       |
| `CHAT_HISTORY`      | `"996"` | 历史记录       |
| `CHAT_USER_OFFLINE` | `"997"` | 用户下线       |
| `CHAT_USER_INIT`    | `"998"` | 在线用户初始化 |
| `CHAT_USER_JOIN`    | `"999"` | 新用户加入     |

**FILE_TRANSFER 子类型：**

| 宏                  | 值      | 方向 | 含义                |
| ------------------- | ------- | ---- | ------------------- |
| `FT_SHARED_UPLOAD`  | `"000"` | C→S  | 共享文件上传        |
| `FT_PRIVATE_UPLOAD` | `"001"` | C→S  | 私发文件上传        |
| `FT_QUERY_FILES`    | `"002"` | C→S  | 查询可下载文件列表  |
| `FT_DOWNLOAD_REQ`   | `"003"` | C→S  | 请求下载文件        |
| `FT_ACK_SENDER`     | `"993"` | S→C  | 上传回执            |
| `FT_SEND_FILE`      | `"994"` | S→C  | 发送文件数据        |
| `FT_QUERY_FAIL`     | `"995"` | S→C  | 查询失败/无可用文件 |
| `FT_QUERY_SUCCESS`  | `"997"` | S→C  | 查询文件列表成功    |
| `FT_PRIVATE_NOTIFY` | `"998"` | S→C  | 私发文件通知        |
| `FT_SHARED_NOTIFY`  | `"999"` | S→C  | 共享文件通知        |

------

## 二、客户端 → 服务端

### 1. 连接/心跳模块

**首包（注册用户名）**

```
FIRST_BAG + {size} + INTERUPT + {name}
```

无 flag，纯文本用户名。服务端收到后在 `client_firstBag_set` 中命中，读取后注册。



**心跳包**【旧协议】

```
PUNPING_INFO
```

即单个字节 `"2"`。客户端每 3s 发送一次。服务端如果10s内检测不到心跳包则判定超时



### 2. 消息模块

**广播消息**

```
CHAT_INFO + CHAT_BROADCAST + {bodySize} + INTERUPT + {body}
```

- `{bodySize}`: 消息正文的字节数（十进制数字字符串）
- `{body}`: UTF-8 编码的消息正文



**私发消息**

```
CHAT_INFO + CHAT_PRIVATE_REQ + {targetName} + INTERUPT + {bodySize} + INTERUPT + {body}
```

- `{targetName}`: 目标用户名
- `{bodySize}`: 消息正文字节数
- `{body}`: UTF-8 编码的消息正文



### 3. 文件传输模块

**共享文件上传**

```
FILE_TRANSFER_REQUEST + FT_SHARED_UPLOAD + {fileName} + INTERUPT + {fileSize} + INTERUPT + [file data] + FILE_TRANSFER_END
```

- `{fileName}`: 文件名
- `{fileSize}`: 文件字节数（十进制数字字符串）
- `[file data]`: 文件二进制数据（64KB 分片）
- `FILE_TRANSFER_END` 标记数据结束



**私发文件上传**

```
FILE_TRANSFER_REQUEST + FT_PRIVATE_UPLOAD + {targetName} + INTERUPT + {fileName} + INTERUPT + {fileSize} + INTERUPT + [file data] + FILE_TRANSFER_END
```

- `{targetName}`: 目标用户名
- `{fileName}`: 文件名
- `{fileSize}`: 文件字节数（十进制数字字符串）
- `[file data]`: 文件二进制数据（64KB 分片）



**查询可下载文件列表**

```
FILE_TRANSFER_REQUEST + FT_QUERY_FILES + FILE_TRANSFER_END
```

无 body，总长 5 字节（`"A002\x03"`）。



**请求下载文件**

```
FILE_TRANSFER_REQUEST + FT_DOWNLOAD_REQ + {fileName} + FILE_TRANSFER_END
```

------

## 三、服务端 → 客户端

### 1. 连接/系统模块

**历史聊天记录（连接后立即发送）**

```
CHAT_INFO + CHAT_HISTORY + {bodySize} + INTERUPT + {records}
```

- `{records}`: `{record1}` + INTERUPT + `{record2}` + INTERUPT + ...
- 每条 record: `{time} {senderIP} {userName} {msg}`



**在线用户列表（连接后立即发送）**

```
CHAT_INFO + CHAT_USER_INIT + {bodySize} + INTERUPT + {names}
```

- `{names}`: `{name1}` + INTERUPT + `{name2}` + INTERUPT + ...



### 2. 消息模块

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



### 3. 私信回执模块

**私发成功回执**

```
SEARCH_SUCCESS + INTERUPT
```

（content 为空，仅 flag + 分隔符），content用于拓展预留



**私发失败回执**

```
SEARCH_FAILED + INTERUPT
```



### 4. 系统通知模块

**服务器关闭**

```
SERVER_CLOSE + {bodySize} + INTERUPT + {body}
```

- `{body}`: 关闭原因文本，如 `"服务器已关闭"`



**踢出通知**

```
SERVER_KICK + {bodySize} + INTERUPT + {body}
```

- `{body}`: 踢出原因文本，这里是静态的，可以再后续拓展为管理员自定义理由



### 5. 文件传输模块

**共享文件通知（广播）**

```
FILE_TRANSFER_RESULT + FT_SHARED_NOTIFY + {fileName} + FILE_TRANSFER_END
```

有新共享文件时广播给所有在线客户端。



**私发文件通知**

```
FILE_TRANSFER_RESULT + FT_PRIVATE_NOTIFY + {fileName} + FILE_TRANSFER_END
```

仅发目标用户，告知有私发文件可下载。



**查询文件列表成功**

```
FILE_TRANSFER_RESULT + FT_QUERY_SUCCESS + {bodySize} + INTERUPT + {fileNames}
```

- `{fileNames}`: `{fileName1}` + INTERUPT + `{fileName2}` + INTERUPT + ...
- 包含共享文件 + 该用户的私发文件



**查询文件列表失败（无可用文件）**

```
FILE_TRANSFER_RESULT + FT_QUERY_FAIL
```

即 `"B995"`，4 字节，无 body。



**发送文件数据**

```
FILE_TRANSFER_RESULT + FT_SEND_FILE + {fileName} + INTERUPT + {fileSize} + INTERUPT + [file data] + FILE_TRANSFER_END
```

- `{fileName}`: 文件名
- `{fileSize}`: 文件字节数（十进制数字字符串）
- `[file data]`: 文件二进制数据（64KB 分片）



**上传回执**

```
FILE_TRANSFER_RESULT + FT_ACK_SENDER + {status} + FILE_TRANSFER_END
```

- `{status}`:
  - `"000ok"` — 共享文件上传成功
  - `"001ok"` — 私发文件上传成功
  - `"001fail"` — 私发文件上传失败（目标不在线）

------

## 四、协议设计规则

| 规则              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| **flag 1字节**    | 每条消息首字节为协议族标志（`1`/`2`/`3`/`4`/`A`/`B`/`6`/`7`） |
| **subType 3字节** | 文件传输和聊天消息紧跟 3 字节子类型                          |
| **变长字段分隔**  | 变长字段前用数字标明长度 + `INTERUPT` 分隔，或直接用 `INTERUPT` 分隔后由接收方逐字节读取 |
| **文件边界**      | `FILE_TRANSFER_END` 标记文件正文结束                         |
| **循环消费**      | 客户端 `receiveMsg` 用 `while (bytesAvailable > 0)` 循环处理粘包 |