# Better ConsolePauser

这个程序改进了 `ConsolePauser.exe`，使其能够提供更多的参考信息（如程序运行 CPU 时间、使用内存峰值等）。

## 介绍

### `ConsolePauser.exe` 是什么？

`ConsolePauser.exe` 是 Dev C++ 等众多主流 C++ 便携式 IDE 在运行程序时给程序套的一层“壳”，在程序运行完后显示关于程序运行的一些参考信息（通常是程序运行的墙上时钟时间），随后等待用户按任意键退出。

### 为什么要写这个项目？

原本的 `ConsolePauser.exe` 显示的参考信息并不全面，且缺乏 Oier 们所需的内容，本项目因此而来。

## 使用方法

### 获取程序

下载源码后使用 C++ 编译器编译。

**编译器命令中应至少包含以下选项：**

```
-std=c++11 -lpsapi
```

或者下载 Release 中已经编译好的版本。

### 使用该程序

将程序重命名为 `ConsolePauser.exe`，随后复制到原本的 `ConsolePauser.exe` 所在的位置（通常是 IDE 的安装目录），替换原文件即可。

此时使用 IDE 的“运行”功能，就能正常使用该程序了。
