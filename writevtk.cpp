#include "writevtk.h"

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
#include <QTextStream>

WriteVTK::WriteVTK() {}

WriteVTK::WriteVTK(QString jsonPath)
{
    if (!getDataFromJson(jsonPath)) {
        qDebug() << "读取JSON文件中数据失败";
        return;
    }
    qDebug() << "读取JSON文件中数据成功";

    isFailed = false;
}

bool WriteVTK::getDataFromJson(QString path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "打开JSON文件失败:" << path << file.errorString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "解析JSON文件失败:" << parseError.errorString()
            << "位置:" << parseError.offset;
        return false;
    }

    if (!document.isObject()) {
        qDebug() << "JSON根节点不是对象";
        return false;
    }

    nodes.clear();
    elements.clear();
    steps.clear();
    nodeNumToIndex.clear();
    QHash<int, int> elementNumToIndex;

    const QJsonObject root = document.object();
    const QJsonArray instancesArray = root.value(QStringLiteral("instances")).toArray();
    if (instancesArray.isEmpty()) {
        qDebug() << "JSON中没有instances数据";
        return false;
    }

    const QJsonObject instanceObject = instancesArray.first().toObject();
    const QJsonArray nodesArray = instanceObject.value(QStringLiteral("nodes")).toArray();
    const QJsonArray elementsArray = instanceObject.value(QStringLiteral("elements")).toArray();

    nodes.reserve(nodesArray.size());
    for (const QJsonValue &nodeValue : nodesArray) {
        const QJsonObject nodeObject = nodeValue.toObject();
        const QJsonArray coordinates = nodeObject.value(QStringLiteral("coordinates")).toArray();
        if (coordinates.size() < 3) {
            qDebug() << "节点坐标数量不足";
            return false;
        }

        Node node;
        node.label = nodeObject.value(QStringLiteral("label")).toInt();
        node.x = coordinates.at(0).toDouble();
        node.y = coordinates.at(1).toDouble();
        node.z = coordinates.at(2).toDouble();

        nodeNumToIndex.insert(node.label, nodes.size());
        nodes.append(node);
    }

    elements.reserve(elementsArray.size());
    for (const QJsonValue &elementValue : elementsArray) {
        const QJsonObject elementObject = elementValue.toObject();
        const QJsonArray connectivity = elementObject.value(QStringLiteral("connectivity")).toArray();

        Element element;
        element.label = elementObject.value(QStringLiteral("label")).toInt();
        element.abaqusType = elementObject.value(QStringLiteral("abaqusType")).toString();
        element.vtkType = elementObject.value(QStringLiteral("vtkType")).toInt();
        element.nodeLabels.reserve(connectivity.size());

        for (const QJsonValue &labelValue : connectivity)
            element.nodeLabels.append(labelValue.toInt());

        elementNumToIndex.insert(element.label, elements.size());
        elements.append(element);
    }

    const QJsonArray stepsArray = root.value(QStringLiteral("steps")).toArray();
    steps.reserve(stepsArray.size());
    for (const QJsonValue &stepValue : stepsArray) {
        const QJsonObject stepObject = stepValue.toObject();

        StepData step;
        step.name = stepObject.value(QStringLiteral("name")).toString();

        const QJsonArray framesArray = stepObject.value(QStringLiteral("frames")).toArray();
        step.frames.reserve(framesArray.size());
        for (const QJsonValue &frameValue : framesArray) {
            const QJsonObject frameObject = frameValue.toObject();

            FrameData frame;
            frame.time = frameObject.value(QStringLiteral("frameValue")).toDouble();

            const QJsonArray fieldsArray = frameObject.value(QStringLiteral("fields")).toArray();
            for (const QJsonValue &fieldValue : fieldsArray) {
                const QJsonObject fieldObject = fieldValue.toObject();
                const QString location = fieldObject.value(QStringLiteral("location")).toString();
                const QJsonArray componentLabels =
                    fieldObject.value(QStringLiteral("componentLabels")).toArray();
                const QJsonArray valuesArray = fieldObject.value(QStringLiteral("values")).toArray();

                FieldData field;
                field.name = fieldObject.value(QStringLiteral("name")).toString();
                field.components = componentLabels.isEmpty() ? 1 : componentLabels.size();

                if (field.components == 1 && !valuesArray.isEmpty()) {
                    const QJsonObject firstValueObject = valuesArray.first().toObject();
                    const QJsonValue firstDataValue =
                        firstValueObject.value(QStringLiteral("data"));
                    if (firstDataValue.isArray())
                        field.components = firstDataValue.toArray().size();
                }

                const bool isElementData = location == QStringLiteral("element");
                const bool isNodeData = location == QStringLiteral("node");
                if (isElementData) {
                    field.data.fill(0.0, elements.size() * field.components);
                } else if (isNodeData) {
                    field.data.fill(0.0, nodes.size() * field.components);
                }

                for (const QJsonValue &valueItem : valuesArray) {
                    const QJsonObject valueObject = valueItem.toObject();
                    const int label = valueObject.value(QStringLiteral("label")).toInt();
                    const QJsonValue dataValue = valueObject.value(QStringLiteral("data"));

                    QVector<double> components;
                    if (dataValue.isArray()) {
                        const QJsonArray dataArray = dataValue.toArray();
                        for (const QJsonValue &componentValue : dataArray)
                            components.append(componentValue.toDouble());
                    } else {
                        components.append(dataValue.toDouble());
                    }

                    while (components.size() < field.components)
                        components.append(0.0);

                    if (isElementData) {
                        const int elementIndex = elementNumToIndex.value(label, -1);
                        if (elementIndex < 0)
                            continue;
                        const int dataIndex = elementIndex * field.components;
                        for (int i = 0; i < field.components; ++i)
                            field.data[dataIndex + i] = components.at(i);
                    } else if (isNodeData) {
                        const int nodeIndex = nodeNumToIndex.value(label, -1);
                        if (nodeIndex < 0)
                            continue;
                        const int dataIndex = nodeIndex * field.components;
                        for (int i = 0; i < field.components; ++i)
                            field.data[dataIndex + i] = components.at(i);
                    } else {
                        for (int i = 0; i < field.components; ++i)
                            field.data.append(components.at(i));
                    }
                }

                if (isElementData) {
                    frame.cellData.append(field);
                } else {
                    frame.pointData.append(field);
                }
            }

            step.frames.append(frame);
        }

        steps.append(step);
    }

    qDebug() << "JSON数据读取完成:"
             << "节点" << nodes.size()
             << "单元" << elements.size()
             << "分析步" << steps.size();
    return true;
}

void WriteVTK::outVTK(QString outPath)
{
    isFailed = true;

    if (nodes.isEmpty() || elements.isEmpty()) {
        qDebug() << "No mesh data to write.";
        return;
    }

    const FrameData *frame = nullptr;
    if (!steps.isEmpty()) {
        for (int stepIndex = steps.size() - 1; stepIndex >= 0 && frame == nullptr; --stepIndex) {
            const StepData &step = steps.at(stepIndex);
            if (!step.frames.isEmpty())
                frame = &step.frames.last();
        }
    }

    const QFileInfo outInfo(outPath);
    QDir outDir = outInfo.dir();
    if (!outDir.exists() && !outDir.mkpath(QStringLiteral("."))) {
        qDebug() << "Failed to create output directory:" << outDir.absolutePath();
        return;
    }

    QFile file(outInfo.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qDebug() << "Failed to open VTU file for writing:" << outInfo.absoluteFilePath()
                 << file.errorString();
        return;
    }

    QTextStream stream(&file);
    stream.setRealNumberNotation(QTextStream::ScientificNotation);
    stream.setRealNumberPrecision(16);

    QVector<int> connectivity;
    QVector<int> offsets;
    QVector<int> types;
    connectivity.reserve(elements.size() * 8);
    offsets.reserve(elements.size());
    types.reserve(elements.size());

    int offset = 0;
    for (const Element &element : elements) {
        for (int label : element.nodeLabels) {
            const int nodeIndex = nodeNumToIndex.value(label, -1);
            if (nodeIndex < 0) {
                qDebug() << "Element references missing node label:" << element.label << label;
                return;
            }
            connectivity.append(nodeIndex);
            ++offset;
        }
        offsets.append(offset);
        types.append(element.vtkType);
    }

    auto writeField = [&stream](const FieldData &field) {
        const int components = qMax(1, field.components);
        stream << "        <DataArray type=\"Float64\" Name=\"" << field.name
               << "\" NumberOfComponents=\"" << components
               << "\" format=\"ascii\">\n          ";
        for (int i = 0; i < field.data.size(); ++i) {
            stream << field.data.at(i);
            if (i + 1 < field.data.size())
                stream << ' ';
        }
        stream << "\n        </DataArray>\n";
    };

    stream << "<?xml version=\"1.0\"?>\n";
    stream << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    stream << "  <UnstructuredGrid>\n";
    stream << "    <Piece NumberOfPoints=\"" << nodes.size()
           << "\" NumberOfCells=\"" << elements.size() << "\">\n";

    stream << "      <Points>\n";
    stream << "        <DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"ascii\">\n          ";
    for (int i = 0; i < nodes.size(); ++i) {
        const Node &node = nodes.at(i);
        stream << node.x << ' ' << node.y << ' ' << node.z;
        if (i + 1 < nodes.size())
            stream << ' ';
    }
    stream << "\n        </DataArray>\n";
    stream << "      </Points>\n";

    stream << "      <Cells>\n";
    stream << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n          ";
    for (int i = 0; i < connectivity.size(); ++i) {
        stream << connectivity.at(i);
        if (i + 1 < connectivity.size())
            stream << ' ';
    }
    stream << "\n        </DataArray>\n";

    stream << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n          ";
    for (int i = 0; i < offsets.size(); ++i) {
        stream << offsets.at(i);
        if (i + 1 < offsets.size())
            stream << ' ';
    }
    stream << "\n        </DataArray>\n";

    stream << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n          ";
    for (int i = 0; i < types.size(); ++i) {
        stream << types.at(i);
        if (i + 1 < types.size())
            stream << ' ';
    }
    stream << "\n        </DataArray>\n";
    stream << "      </Cells>\n";

    stream << "      <PointData>\n";
    if (frame != nullptr) {
        for (const FieldData &field : frame->pointData)
            writeField(field);
    }
    stream << "      </PointData>\n";

    stream << "      <CellData>\n";
    if (frame != nullptr) {
        for (const FieldData &field : frame->cellData)
            writeField(field);
    }
    stream << "      </CellData>\n";

    stream << "    </Piece>\n";
    stream << "  </UnstructuredGrid>\n";
    stream << "</VTKFile>\n";

    file.close();
    if (file.error() != QFile::NoError) {
        qDebug() << "Failed while writing VTU file:" << file.errorString();
        return;
    }

    qDebug() << "VTU file written:" << outInfo.absoluteFilePath()
             << "points:" << nodes.size()
             << "cells:" << elements.size()
             << "hasFrameData:" << (frame != nullptr);
    isFailed = false;
}

bool WriteVTK::isOk()
{
    return !isFailed;
}
