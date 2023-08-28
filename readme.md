# voice broadcast

基于 winsock IPv6 传输的语音聊天程序，支持 Windows 8.1 及以上版本（Windows Audio Core）  
还没画 UI，音频流合并、控制协议还没写，仅在局域网测试过  
一个 WIN32API 和 Socket 的练习 Demo

### 用法

1. Capture 模式：音频捕获服务端，可以选择任何一个设备
2. Render 模式：音频呈现客户端，仅可选择 Render 设备

**注意**：测试的时候我是在一台电脑上测试的，所以 Render 和 Capture 端绑定的 Socket 端口不一样
