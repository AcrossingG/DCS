#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>

#include "readodb.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QHBoxLayout *layout = new QHBoxLayout(ui->centralwidget);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    QPushButton *selODB = new QPushButton(tr("选择 ODB"), ui->centralwidget);
    selODB->setMinimumSize(240, 64);
    QPushButton *selVTK = new QPushButton(tr("选择 VTK"), ui->centralwidget);
    selVTK->setMinimumSize(240, 64);

    layout->addWidget(selODB);
    layout->addWidget(selVTK);

    setMinimumSize(1120, 400);
    resize(640, 240);

    connect(selODB, &QPushButton::clicked, this, &MainWindow::onSelODB);
    connect(selVTK, &QPushButton::clicked, this, &MainWindow::onSelVTK);
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
    outPath = QFileInfo(path).path() + QDir::separator()  + QFileInfo(path).completeBaseName() + QStringLiteral(".vtk");

    statusBar()->showMessage(tr("ODB文件：%1").arg(path), 5000);
    qDebug() << "打开ODB文件：" << path;

    statusBar()->showMessage(tr("正在读取ODB文件"), 5000);
    qDebug() << "读取ODB文件：" << path;
    ReadODB read(odbPath);
    if (!read.isOk()) {
        statusBar()->showMessage(tr("读取失败（或出现未知错误）"), 5000);
        qDebug() << "读取ODB文件失败（或出现未知错误）";
        return;
    }

    statusBar()->showMessage(tr("正在生成VTK文件"), 5000);
    qDebug() << "生成VTK文件：" << outPath;
    //TODO:writevtk write(read.info());
}

void MainWindow::onSelVTK()
{
    const QString path = selectFile(tr("选择VTK文件"));
    if (path.isEmpty())
        return;

    vtkPath = path;
    statusBar()->showMessage(tr("VTK文件：%1").arg(path), 5000);
    qDebug() << "打开VTK文件：" << path;
}
