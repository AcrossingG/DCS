#ifndef WRITEVTK_H
#define WRITEVTK_H

#include <QString>
#include <QVector>
#include <QHash>

class WriteVTK
{
public:
    WriteVTK();

    WriteVTK(QString jasonPath);

    bool getDataFromJson(QString path);

    void outVTK(QString outPath);

    bool isOk();

private:
    bool isFailed = true;

    struct Node {
        int label;        // Abaqus 节点编号
        double x, y, z;
    };

    struct Element {
        int label;                  // Abaqus 单元编号
        QString abaqusType;      // C3D8, C3D4, S4...
        int vtkType;                // VTK_HEXAHEDRON = 12 等
        QVector<int> nodeLabels; // Abaqus 原始节点编号
    };

    QHash<int, int> nodeNumToIndex;

    struct FieldData {
        QString name;        // U, S, PE, PEEQ...
        int components;          // 1, 3, 6, 9
        QVector<double> data;
    };

    struct FrameData {
        double time;
        QVector<FieldData> pointData;
        QVector<FieldData> cellData;
    };

    struct StepData {
        QString name;
        QVector<FrameData> frames;
    };

    QVector<Node> nodes;
    QVector<Element> elements;
    QVector<StepData> steps;
};

#endif // WRITEVTK_H
