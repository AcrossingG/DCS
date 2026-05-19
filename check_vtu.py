# This Python file uses the following encoding: utf-8

import sys
import vtk

if len(sys.argv) < 2:
    print("Usage: python check_vtu.py model.vtu")
    sys.exit(1)

path = sys.argv[1]

reader = vtk.vtkXMLUnstructuredGridReader()
reader.SetFileName(path)
reader.Update()

grid = reader.GetOutput()

print("VTU:", path)
print("Points:", grid.GetNumberOfPoints())
print("Cells:", grid.GetNumberOfCells())

print("\nPointData:")
point_data = grid.GetPointData()
for i in range(point_data.GetNumberOfArrays()):
    arr = point_data.GetArray(i)
    print(
        f"  {arr.GetName()} | tuples={arr.GetNumberOfTuples()} "
        f"components={arr.GetNumberOfComponents()}"
    )

print("\nCellData:")
cell_data = grid.GetCellData()
for i in range(cell_data.GetNumberOfArrays()):
    arr = cell_data.GetArray(i)
    print(
        f"  {arr.GetName()} | tuples={arr.GetNumberOfTuples()} "
        f"components={arr.GetNumberOfComponents()}"
    )

if grid.GetNumberOfPoints() <= 0:
    print("\nERROR: no points")
    sys.exit(2)

if grid.GetNumberOfCells() <= 0:
    print("\nERROR: no cells")
    sys.exit(3)

print("\nOK: VTU can be read by VTK.")
