# dlyhttpd
a lightweight web service

v1.0
基本完成，可以运行，可以解析静态页面
支持GET
支持if_modified_since、keep-alive（没有定时器）

v2.0
修改架构，改用多进程+epoll+协程
增加定时器
增加OPTIONS和HEAD方法
完善缓存
