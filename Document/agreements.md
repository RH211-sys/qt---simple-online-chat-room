# 协议 

```C++
#define HISTORY_RECORD "0"			.
#define CHAT_INFO "1"				.
#define PUNPING_INFO "2"			.
#define SERVER_CLOSE "3"			.
#define SERVER_KICK "4"				.
#define PRIVATE_SEND_REQUEST "5"	.
#define SEARCH_SUCCESS "6"	.
#define SEARCH_FAILED "7"	.
#define INTERUPT '\n'		.
#define PRIVATE_MSG "8"		.
#define USER_UPDATE "9"		.
#define USER_INIT '\b'		.
#define USER_UPDATE_OFF '\r'		.

#define FILE_TRANSFER_REQUEST "A"	.
#define FILE_TRANSFER_RESULT "B"	.
#define FILE_TRANSFER_END "C"		.
```





### 宏的重新设计

```c++
/* ==================== 基础协议标志 ==================== */
#define CHAT_INFO "1"             // 聊天消息父标志
#define PUNPING_INFO "2"         // 心跳包
#define SERVER_CLOSE "3"         // 服务器关闭
#define SERVER_KICK "4"          // 服务器踢人
#define FILE_TRANSFER_REQUEST "A" // 文件传输：客户端→服务端
#define FILE_TRANSFER_RESULT "B" // 文件传输：服务端→客户端

/* ==================== 私发回执（保留独立flag） ==================== */
#define SEARCH_SUCCESS "6"        // 私发成功回执
#define SEARCH_FAILED "7"         // 私发失败（目标用户不存在）

/* ==================== CHAT_INFO 子类型 ==================== */
#define CHAT_BROADCAST  "000"  // C→S 广播消息
#define CHAT_PRIVATE_REQ "001"  // C→S 私发请求
#define CHAT_PRIVATE_FWD  "995"  // S→C 私发转发
#define CHAT_HISTORY      "996"  // S→C 历史记录
#define CHAT_USER_OFFLINE "997"  // S→C 用户离线
#define CHAT_USER_INIT    "998"  // S→C 在线用户初始化
#define CHAT_USER_JOIN    "999"  // S→C 新用户加入

/* ==================== FILE_TRANSFER 子类型 ==================== */
// 客户端→服务端
#define FT_SHARED_UPLOAD  "000"  // 共享文件上传
#define FT_PRIVATE_UPLOAD "001"  // 私发文件上传
#define FT_QUERY_FILES    "002"  // 查询文件列表
#define FT_DOWNLOAD_REQ   "003"  // 请求下载文件
// 服务端→客户端
#define FT_SHARED_NOTIFY  "999"  // 共享文件通知
#define FT_PRIVATE_NOTIFY "998"  // 私发文件通知
#define FT_QUERY_SUCCESS  "997"  // 查询文件列表成功
#define FT_QUERY_FAIL     "995"  // 查询文件列表失败
#define FT_SEND_FILE      "994"  // 发送文件数据
#define FT_ACK_SENDER     "993"  // 回应发送源

/* ==================== 通用分隔符 ==================== */
#define INTERUPT "\x1E"               // 字段分隔符（两端统一）
#define FILE_TRANSFER_END "\x03"     // 文件传输结束标记
```







### 文件传输模块

文件协议

客户端发送文件：

共享文件：FILE_TRANSFER_REQUEST + 000 + 文件名称 + INTERUPT + 文件内容大小 + INTERUPT  + 文件内容 + FILE_TRANSFER_END

私发文件：FILE_TRANSFER_REQUEST + 001 + 用户名 + INTERUPT + 文件名 + INTERUPT + 文件内容大小 + INTERUPT  + 文件内容 + FILE_TRANSFER_END

查询文件：FILE_TRANSFER_REQUEST + 002 + FILE_TRANSFER_END

接收文件：FILE_TRANSFER_REQUEST + 003 + 文件名 + FILE_TRANSFER_END



服务器处理

共享文件：FILE_TRANSFER_RESULT + 999+ fileName + FILE_TRANSFER_END

私发文件：FILE_TRANSFER_RESULT + 998+ fileName + FILE_TRANSFER_END

查询文件：

- 成功：FILE_TRANSFER_RESULT + 997+ 消息正文大小 + INTERUPT + 正文{文件名 + INTERUPT + 文件名 + INTERUPT ...}
- 失败：FILE_TRANSFER_RESULT + 995

发送文件：FILE_TRANSFER_RESULT + 994+ 文件名 + INTERUPT + 文件正文大小 + INTERUPT + 文件正文 + FILE_TRANSFER_END

回应发送源：FILE_TRANSFER_RESULT + "993" + FILE_TRANSFER_END



### 消息模块

消息协议

客户端发送消息：

广播消息类型：CHAT_INFO + "000" + 消息正文大小 + INTERUPT + 消息正文

私发消息类型：CHAT_INFO + "001" + 对方名字 + INTERUPT + 消息正文大小 + INTERUPT + 消息正文



服务端消息：

KICK消息类型：SERVER_KICK + 消息正文大小 + INTERUPT + 正文{kick理由}

服务器关闭消息：SERVER_CLOSE + 消息正文大小 + INTERUPT + 正文{close理由}

新用户加入：CHAT_INFO + "999" + 用户名 + INTERUPT

客户端的前在线用户初始化消息：CHAT_INFO + "998" + 消息正文大小 + INTERUPT + 消息正文{用户名+ INTERUPT + 用户名+ INTERUPT ...}

用户离线：CHAT_INFO + "997" + 用户名 + INTERUPT

历史记录消息：CHAT_INFO + "996" + 消息正文大小 + INTERUPT + 正文{用户名 : 消息内容}

私发消息转发：CHAT_INFO + "995" + "发送方名字" + INTERUPT + 消息正文大小 + INTERUPT + 正文{用户名 : 消息内容}



### 心跳包

客户端心跳包：PUNPING

服务端只接收

这个模块应该不会改变

















