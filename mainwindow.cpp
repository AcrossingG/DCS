#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include "readodb.h"
#include "writevtk.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QVBoxLayout *layout = new QVBoxLayout(ui->centralwidget);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    QPushButton *selODB = new QPushButton(tr("选择ODB文件"), ui->centralwidget);
    selODB->setMinimumSize(240, 64);
    QPushButton *selJSON = new QPushButton(tr("选择JSON文件"), ui->centralwidget);
    selJSON->setMinimumSize(240, 64);
    QPushButton *outVTK = new QPushButton(tr("生成VTK文件"), ui->centralwidget);
    outVTK->setMinimumSize(240, 64);
    QPushButton *checkVTK = new QPushButton(tr("验证VTK文件"), ui->centralwidget);
    outVTK->setMinimumSize(240, 64);

    layout->addWidget(selODB);
    layout->addWidget(selJSON);
    layout->addWidget(outVTK);
    layout->addWidget(checkVTK);

    setMinimumSize(1120, 400);
    resize(640, 240);

    connect(selODB, &QPushButton::clicked, this, &MainWindow::onSelODB);
    connect(selJSON, &QPushButton::clicked, this, &MainWindow::onSelJSON);
    connect(outVTK, &QPushButton::clicked, this, &MainWindow::onOutVTK);
    connect(checkVTK, &QPushButton::clicked, this, &MainWindow::onCheckVTK);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::selectFile(const QString &title)
{
    return QFileDialog::getOpenFileName(
        this,
        title,
        "D:/Qt set/Project/DCS/Sample",
        tr("所有文件 (*.*)"));
}

void MainWindow::onSelODB()
{
    const QString path = selectFile(tr("选择ODB文件"));
    if (path.isEmpty()) {
        return;
    }

    odbPath = path;
    name = QFileInfo(path).fileName();
    if (!name.endsWith(QStringLiteral(".odb"), Qt::CaseInsensitive)) {
        statusBar()->showMessage(tr("请选择正确的文件"), 5000);
        qDebug() << "未知文件：" << path;
        return;
    }
    outPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".vtu");


    statusBar()->showMessage(tr("正在读取ODB文件：%1").arg(odbPath), 5000);
    qDebug() << "读取ODB文件：" << path;
    ReadODB read(odbPath);
    if (!read.isOk()) {
        statusBar()->showMessage(tr("读取失败（或出现未知错误）"), 5000);
        qDebug() << "读取ODB文件失败（或出现未知错误）";
        return;
    }
    jsonPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".json");
}

void MainWindow::onSelJSON()
{
    const QString path = selectFile(tr("选择JSON文件"));
    if (path.isEmpty()) {
        return;
    }

    jsonPath = path;
    odbPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".odb");
    outPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".vtu");

    if (!QFileInfo(path).fileName().endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        statusBar()->showMessage(tr("请选择正确的文件"), 5000);
        qDebug() << "未知文件：" << path;
        return;
    }

    statusBar()->showMessage(tr("已选择JSON文件：%1").arg(QFileInfo(path).fileName()), 5000);
    qDebug() << "选择JSON文件" << jsonPath;
}

void MainWindow::onOutVTK()
{
    WriteVTK write(jsonPath);
    if (!write.isOk()) {
        statusBar()->showMessage(tr("获取数据失败"), 5000);
        qDebug() << "获取ODB数据失败";
        return;
    }

    statusBar()->showMessage(tr("正在生成VTK文件"), 5000);
    qDebug() << "生成VTK文件：" << outPath;
    write.outVTK(outPath);
    if (!write.isOk()) {
        statusBar()->showMessage(tr("生成VTK文件失败"), 5000);
        qDebug() << "生成VTK文件失败";
        return;
    }
    statusBar()->showMessage(tr("生成VTK文件成功：%1").arg(outPath), 5000);
    qDebug() << "生成VTK文件成功：" << outPath;
}

void MainWindow::onCheckVTK()
{
    const QString path = selectFile(tr("验证VTK文件"));
    if (path.isEmpty()) {
        return;
    }

    outPath = path;
    odbPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".odb");
    jsonPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".json");

    QDir dir(QFileInfo(path).path());
    dir.cdUp();
    const QString checkPath = dir.filePath("check_vtu.py");
    if (!QFileInfo::exists(checkPath)) {
        statusBar()->showMessage(tr("未找到VTK验证脚本"), 5000);
        qDebug() << "check_vtu.py not found:" << checkPath;
        return;
    }

    QProcess process;
    process.setProgram(QStringLiteral("D:/python/python 3.8.9/python.exe"));
    process.setArguments({checkPath, outPath});

    statusBar()->showMessage(tr("正在验证VTK文件"), 5000);
    qDebug() << "Checking VTU:" << process.program() << process.arguments();

    process.start();
    if (!process.waitForStarted()) {
        statusBar()->showMessage(tr("启动VTK验证脚本失败"), 5000);
        qDebug() << "Failed to start check_vtu.py:" << process.errorString();
        return;
    }

    if (!process.waitForFinished(-1)) {
        statusBar()->showMessage(tr("等待VTK验证脚本失败"), 5000);
        qDebug() << "Failed while waiting for check_vtu.py:" << process.errorString();
        return;
    }

    const QString standardOutput = QString::fromLocal8Bit(process.readAllStandardOutput());
    const QString standardError = QString::fromLocal8Bit(process.readAllStandardError());
    if (!standardOutput.trimmed().isEmpty())
        qDebug().noquote() << standardOutput.trimmed();
    if (!standardError.trimmed().isEmpty())
        qDebug().noquote() << standardError.trimmed();

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        statusBar()->showMessage(tr("VTK文件验证成功"), 5000);
        qDebug() << "VTK check succeeded:" << outPath;
    } else {
        statusBar()->showMessage(tr("VTK文件验证失败"), 5000);
        qDebug() << "VTK check failed. exitCode:" << process.exitCode();
    }

}
