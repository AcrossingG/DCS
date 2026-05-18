# -*- coding: utf-8 -*-
"""
将 Abaqus ODB 文件导出为 JSON，便于 Qt/C++ 代码读取。

请使用 Abaqus Python 运行，例如：
    abaqus python InvokeAPI.py Sample/CP10_L6_DP1.odb -o Sample/CP10_L6_DP1.json

从 C++ 调用时，可以只传入 ODB 路径：
    abaqus python InvokeAPI.py D:/models/job-1.odb

默认 JSON 输出路径为：
    D:/models/job-1.json

注意：
    1. 本脚本必须由 Abaqus Python 执行，因为 odbAccess 模块只在
       Abaqus Python 环境中可用。
    2. ODB 文件可能很大。如果只需要较小的结果文件，可以使用
       --fields 和 --last-frame 参数。
"""

from __future__ import print_function

import argparse
import json
import os
import sys


try:
    from odbAccess import openOdb
except ImportError:
    openOdb = None


VTK_CELL_TYPES = {
    # 线性实体单元
    "C3D4": 10,       # VTK 四面体
    "C3D5": 14,       # VTK 金字塔
    "C3D6": 13,       # VTK 楔形体
    "C3D8": 12,       # VTK 六面体
    "C3D8R": 12,
    "C3D8I": 12,
    "C3D8H": 12,
    # 二次实体单元
    "C3D10": 24,      # VTK 二次四面体
    "C3D10M": 24,
    "C3D15": 26,      # VTK 二次楔形体
    "C3D20": 25,      # VTK 二次六面体
    "C3D20R": 25,
    # 壳单元
    "S3": 5,          # VTK 三角形
    "S3R": 5,
    "STRI3": 5,
    "S4": 9,          # VTK 四边形
    "S4R": 9,
    "S4R5": 9,
    "S8": 23,         # VTK 二次四边形
    "S8R": 23,
    # 梁单元和桁架单元
    "T2D2": 3,        # VTK 线段
    "T3D2": 3,
    "B31": 3,
    "B32": 21,        # VTK 二次边
}


def vtk_type_for_abaqus(element_type):
    """根据常见 Abaqus 单元类型返回对应的 VTK 单元类型编号。"""
    upper_type = element_type.upper()
    if upper_type in VTK_CELL_TYPES:
        return VTK_CELL_TYPES[upper_type]

    for prefix, vtk_type in VTK_CELL_TYPES.items():
        if upper_type.startswith(prefix):
            return vtk_type

    return 0


def number_or_list(value):
    """将 Abaqus 标量、向量或张量值转换为 JSON 可写入的数据。"""
    if isinstance(value, (int, float)):
        return value

    try:
        return [float(item) for item in value]
    except TypeError:
        return float(value)


def field_value_to_record(field_value):
    record = {
        "label": int(field_value.nodeLabel or field_value.elementLabel),
        "data": number_or_list(field_value.data),
    }

    if field_value.nodeLabel:
        record["labelType"] = "node"
    else:
        record["labelType"] = "element"

    if hasattr(field_value, "integrationPoint") and field_value.integrationPoint:
        record["integrationPoint"] = int(field_value.integrationPoint)

    if hasattr(field_value, "sectionPoint") and field_value.sectionPoint:
        record["sectionPoint"] = str(field_value.sectionPoint)

    return record


def field_location(field_output):
    """根据场变量数据判断其位置类型：节点、单元或混合。"""
    has_node = False
    has_element = False

    for value in field_output.values:
        if value.nodeLabel:
            has_node = True
        if value.elementLabel:
            has_element = True
        if has_node and has_element:
            return "mixed"

    if has_node:
        return "node"
    if has_element:
        return "element"
    return "unknown"


def export_instance(instance):
    nodes = []
    elements = []

    for node in instance.nodes:
        coords = list(node.coordinates)
        while len(coords) < 3:
            coords.append(0.0)

        nodes.append({
            "label": int(node.label),
            "coordinates": [float(coords[0]), float(coords[1]), float(coords[2])],
        })

    for element in instance.elements:
        abaqus_type = str(element.type)
        elements.append({
            "label": int(element.label),
            "abaqusType": abaqus_type,
            "vtkType": vtk_type_for_abaqus(abaqus_type),
            "connectivity": [int(label) for label in element.connectivity],
        })

    return {
        "name": instance.name,
        "nodes": nodes,
        "elements": elements,
    }


def export_field(field_name, field_output, max_values):
    values = field_output.values
    if max_values is not None:
        values = values[:max_values]

    return {
        "name": field_name,
        "description": field_output.description,
        "type": str(field_output.type),
        "location": field_location(field_output),
        "componentLabels": list(field_output.componentLabels),
        "validInvariants": [str(item) for item in field_output.validInvariants],
        "values": [field_value_to_record(value) for value in values],
        "truncated": max_values is not None and len(field_output.values) > max_values,
    }


def selected_frames(frames, last_frame_only):
    if last_frame_only and frames:
        return [(len(frames) - 1, frames[-1])]

    return list(enumerate(frames))


def export_steps(odb, field_names, last_frame_only, max_field_values):
    steps = []

    for step_name, step in odb.steps.items():
        step_data = {
            "name": step_name,
            "description": step.description,
            "domain": str(step.domain),
            "procedure": step.procedure,
            "frames": [],
        }

        for frame_index, frame in selected_frames(step.frames, last_frame_only):
            frame_data = {
                "index": frame_index,
                "frameId": int(frame.frameId),
                "incrementNumber": int(frame.incrementNumber),
                "frameValue": float(frame.frameValue),
                "description": frame.description,
                "fields": [],
            }

            available_names = set(frame.fieldOutputs.keys())
            names_to_export = field_names or sorted(available_names)

            for field_name in names_to_export:
                if field_name not in frame.fieldOutputs:
                    continue
                field_output = frame.fieldOutputs[field_name]
                frame_data["fields"].append(
                    export_field(field_name, field_output, max_field_values)
                )

            step_data["frames"].append(frame_data)

        steps.append(step_data)

    return steps


def export_odb(odb_path, output_path, field_names, last_frame_only, max_field_values):
    if openOdb is None:
        raise RuntimeError(
            "无法导入 odbAccess。请使用 Abaqus Python 运行本脚本。"
        )

    odb = openOdb(path=odb_path, readOnly=True)
    try:
        assembly = odb.rootAssembly
        data = {
            "schemaVersion": 1,
            "source": {
                "odbPath": os.path.abspath(odb_path),
                "name": odb.name,
                "analysisTitle": odb.analysisTitle,
                "description": odb.description,
            },
            "instances": [],
            "steps": export_steps(
                odb,
                field_names=field_names,
                last_frame_only=last_frame_only,
                max_field_values=max_field_values,
            ),
        }

        for instance_name in sorted(assembly.instances.keys()):
            data["instances"].append(export_instance(assembly.instances[instance_name]))

        with open(output_path, "w") as output_file:
            json.dump(data, output_file, indent=2, sort_keys=True)

        return data
    finally:
        odb.close()


def default_output_path(odb_path):
    odb_dir = os.path.dirname(os.path.abspath(odb_path))
    odb_name = os.path.splitext(os.path.basename(odb_path))[0]
    return os.path.join(odb_dir, odb_name + ".json")


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="将 Abaqus ODB 文件导出为 JSON。",
        add_help=False,
    )
    parser.add_argument("-h", "--help", action="help", help="显示帮助信息并退出。")
    parser.add_argument("odb", help="输入 .odb 文件路径。")
    parser.add_argument(
        "-o",
        "--output",
        help="输出 .json 文件路径。默认在 ODB 所在目录生成同名 .json 文件。",
    )
    parser.add_argument(
        "--fields",
        help="需要导出的场变量名称，用英文逗号分隔，例如 U,S,PEEQ。",
    )
    parser.add_argument(
        "--last-frame",
        action="store_true",
        help="每个分析步只导出最后一帧。",
    )
    parser.add_argument(
        "--max-field-values",
        type=int,
        default=None,
        help="限制每个场变量最多导出的数据数量，适合大 ODB 文件快速测试。",
    )
    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)

    odb_path = os.path.abspath(args.odb)
    output_path = os.path.abspath(args.output or default_output_path(odb_path))
    field_names = None

    if args.fields:
        field_names = [
            name.strip()
            for name in args.fields.split(",")
            if name.strip()
        ]

    export_odb(
        odb_path=odb_path,
        output_path=output_path,
        field_names=field_names,
        last_frame_only=args.last_frame,
        max_field_values=args.max_field_values,
    )

    print("已导出 JSON:", output_path)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
