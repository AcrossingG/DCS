# DCS

工业软件课程实践大作业



本地环境下abaqus.bat路径是 "D:/Abaqus/Commands/abaqus.bat"，出现启动Abaqus Python失败: "Process failed to start: 系统找不到指定的文件。"报错时，请在readodb.cpp文件内修改abaqusProgram变量为当地abaqus.bat路径。



本项目Sample中示例来自https://github.com/Liujie-SYSU/odb2vtk.git，由于odb文件落后于本地环境，通过命令：

&#x09;D:/Abaqus/Commands/abaqus.bat -upgrade -job "D:/Qt set/Project/DCS/Sample/80sub\_coarse\_DP1" -odb "D:/Qt set/Project/DCS/Sample/80sub\_coarse\_DP1.odb"

&#x09;D:/Abaqus/Commands/abaqus.bat -upgrade -job "D:/Qt set/Project/DCS/Sample/CP10\_L6\_DP1\_upgraded" -odb "D:/Qt set/Project/DCS/Sample/CP10\_L6\_DP1.odb"

升级。

