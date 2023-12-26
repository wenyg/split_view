
# SpiltView

Ubuntu 分屏软件，支持快捷键， 1/2, 1/3, 2/3 分屏, 

## BUILD

> 以下仅在 ubuntu20.04 下测试过

安装依赖

```shell
sudo apt-get install git gcc make pkg-config libx11-dev libxtst-dev libxi-dev libgtk-3-dev
```
编译

```shell
mkdir build
cd build
cmake ..
make
```

运行
```shell
./SpiltView
```
> ./SpiltView -d, 以debug模式运行，可以输出一些有用信息方便排查

## 使用说明

CtAl 表示 Ctrl + Alt, 按下对应的快捷键，即可实现对应的分屏

### 左右 1/2 分屏

```text
+----------+----------+
|  CtAl ←  |  CtAl →  |
+----------+----------+
```

### 1/3  分屏

```text
+-----------+-----------+-----------+
| CtAl Num1 | CtAl Num2 | CtAl Num3 |
+-----------+-----------+-----------+
```

### 2/3 分屏

```text
+-----------+-----------+-----------+
| CtAl Num4             |           |
+-----------+-----------+-----------+


+-----------+-----------+-----------+
|           |         CtAl Num6     |
+-----------+-----------+-----------+
```

### 全屏

CtAl + Enter 全屏，再次按下 CtAl + Enter 退出全屏






