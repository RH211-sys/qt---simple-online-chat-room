# 项目文件目录结构

```
Qt_SimpleChatRoom/
│
├── README.md
│
├── Document/
│   ├── start_period/
│   │   ├── requireDoc.md
│   │   ├── project_note.md
│   │   ├── AI_temp.md
│   │   └── AI_chat_record.md
│   └── twice_period/
│       ├── agreements.md
│       ├── FileTransfer_note.md
│       ├── conclusion/
│       │   └── agr_conclu.md
│       └── project_log/
│           ├── day1.md ~ day6.md
│
├── Server/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── chatInfo.db
│   ├── Server/
│   │   ├── server.h
│   │   ├── server.cpp
│   │   └── server.ui
│   └── Worker/
│       ├── worker.h
│       ├── worker.cpp
│       ├── fileTransformer.h
│       └── fileTransformer.cpp
│
├── Client/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── Client/
│   │   ├── client.h
│   │   ├── client.cpp
│   │   └── client.ui
│   ├── serverSocket/
│   │   ├── serverSocket.h
│   │   └── serverSocket.cpp
│   └── worker/
│       ├── file_transfer_module/
│       │   ├── FilesTransFerer.h
│       │   └── FilesTransFerer.cpp
│       └── files_receive_module/
│           ├── filesreceiver.h
│           ├── filesreceiver.cpp
│           └── filesreceiver.ui
│
└── fileDepot/          # 服务端文件仓库
```

