#include "readodb.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QProcess>

ReadODB::ReadODB() {}

ReadODB::ReadODB(QString odbPath)
{
    const QFileInfo odbInfo(odbPath);
    if (!odbInfo.exists() || !odbInfo.isFile()) {
        qDebug() << "ODB文件不存在:" << odbPath;
        return;
    }

    const QString currentDir = QDir::currentPath();
    const QString scriptPath = QDir(currentDir).filePath(QStringLiteral("../../InvokeAPI.py"));
    const QFileInfo scriptInfo(scriptPath);
    if (!scriptInfo.exists() || !scriptInfo.isFile()) {
        qDebug() << "未找到调用API脚本:" << scriptInfo.absoluteFilePath();
        return;
    }

    const QString abaqusProgram =
        QStringLiteral("D:/Abaqus/Commands/abaqus.bat");

    QProcess process;
    process.setProgram(abaqusProgram);
    process.setArguments({
        QStringLiteral("python"),
        scriptPath,
        odbInfo.absoluteFilePath()
    });

    qDebug() << "调用Abaqus API脚本:" << process.program() << process.arguments();

    process.start();
    if (!process.waitForStarted()) {
        qDebug() << "启动Abaqus Python失败:" << process.errorString();
        return;
    }

    if (!process.waitForFinished(-1)) {
        qDebug() << "等待Abaqus Python执行完成失败:" << process.errorString();
        return;
    }

    const QString standardOutput = QString::fromLocal8Bit(process.readAllStandardOutput());
    const QString standardError = QString::fromLocal8Bit(process.readAllStandardError());
    if (!standardOutput.trimmed().isEmpty())
        qDebug().noquote() << standardOutput.trimmed();
    if (!standardError.trimmed().isEmpty())
        qDebug().noquote() << standardError.trimmed();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qDebug() << "Abaqus Python执行失败，退出码:" << process.exitCode();
        return;
    }

    const QFileInfo info(odbInfo.absoluteFilePath());
    const QString jsonPath = info.dir().filePath(info.completeBaseName() + QStringLiteral(".json"));
    if (!QFileInfo::exists(jsonPath)) {
        qDebug() << "JSON文件未生成:" << jsonPath;
        return;
    }
    qDebug() << "JSON文件已生成:" << jsonPath;

    isFailed = false;
}

bool ReadODB::isOk()
{
    return !isFailed;
}
