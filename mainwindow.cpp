#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>

#include "readodb.h"
#include "writevtk.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QHBoxLayout *layout = new QHBoxLayout(ui->centralwidget);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    QPushButton *selODB = new QPushButton(tr("选择ODB文件"), ui->centralwidget);
    selODB->setMinimumSize(240, 64);
    QPushButton *outVTK = new QPushButton(tr("生成VTK文件"), ui->centralwidget);
    outVTK->setMinimumSize(240, 64);

    layout->addWidget(selODB);
    layout->addWidget(outVTK);

    setMinimumSize(1120, 400);
    resize(640, 240);

    connect(selODB, &QPushButton::clicked, this, &MainWindow::onSelODB);
    connect(outVTK, &QPushButton::clicked, this, &MainWindow::onOutVTK);
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
        QString(),
        tr("所有文件 (*.*)"));
}

void MainWindow::onSelODB()
{
    const QString path = selectFile(tr("选择ODB文件"));
    if (path.isEmpty())
        return;

    odbPath = path;
    name = QFileInfo(path).fileName();
    if (!name.endsWith(QStringLiteral(".odb"), Qt::CaseInsensitive)) {
        statusBar()->showMessage(tr("请选择正确的文件"), 5000);
        qDebug() << "未知文件：" << path;
    }
    outPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".vtu");


    statusBar()->showMessage(tr("正在读取ODB文件"), 5000);
    qDebug() << "读取ODB文件：" << path;
    ReadODB read(odbPath);
    if (!read.isOk()) {
        statusBar()->showMessage(tr("读取失败（或出现未知错误）"), 5000);
        qDebug() << "读取ODB文件失败（或出现未知错误）";
        return;
    }
    jsonPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".json");
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
    statusBar()->showMessage(tr("生成VTK文件成功"), 5000);
    qDebug() << "生成VTK文件成功：" << outPath;
}
