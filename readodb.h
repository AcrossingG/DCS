#ifndef READODB_H
#define READODB_H

#include <QString>
#include <QVector>
#include <QHash>

class QObject;

class ReadODB
{
public:
    ReadODB();

    ReadODB(QString odbPath);

    bool getDataFromJson(QString path);

    bool isOk();

private:
    bool isFailed = true;
};

#endif // READODB_H
