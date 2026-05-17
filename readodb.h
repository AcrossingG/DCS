#ifndef READODB_H
#define READODB_H

#include <QString>
#include <QVector>

class ReadODB
{
public:
    ReadODB();

    ReadODB(QString odbPath);

    bool isOk();

private:
    bool isFaile = true;
    QVector<QString> text;
};

#endif // READODB_H
