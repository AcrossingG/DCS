#include "readodb.h"

#include <QDebug>

ReadODB::ReadODB() {}

ReadODB::ReadODB(QString odbPath)
{


    isFailed = false;
}

bool ReadODB::isOk()
{
    return !isFailed;
}
