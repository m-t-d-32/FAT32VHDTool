# FAT32虚拟硬盘工具

这是一个用QT编写的FAT32文件系统的虚拟硬盘工具，可以新建、打开FAT32虚拟硬盘，可以放入、导出、删除文件，新建文件夹或者以树状形式预览文件系统中的文件。




### 编译和运行

可以在Windows或者Linux上编译和运行，请安装最新版本的Qt和c++编译器（Windows上安装mingw、Linux上安装Clang或者G++）。如果Linux上提示报错：spinBox->setStepType不存在，请删除提示的一行即可正确编译。



### 功能支持

- [x] pwd
- [x] cd
- [x] ls
- [x] ls -l
- [ ] touch xxx
- [ ] ed xxx
- [ ] cat xxx
- [x] rm xxx
- [x] mkdir YYY
- [x] rmdir YYY
- [ ] cp XX YY
- [ ] cp -R XX YY
- [x] import ZZ
- [x] export XX
- [x] 长文件名
- [x] 二进制文件正确操作
- [x] tree