#include "readodb.h"

#include <QFile>
#include <QDebug>

ReadODB::ReadODB() {}

ReadODB::ReadODB(QString odbPath)
{
    QFile file(odbPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "打开ODB文件失败";
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd())
    {
        text.append(in.readLine());
    }
    if (text.isEmpty()) {
        qDebug() << "获取文本失败";
        return;
    }

    for (int i = 0; i < text.size(); i++){
        qDebug() << text[i] << endl;
    }

    isFaile = false;
}

bool ReadODB::isOk()
{
    return !isFaile;
}
