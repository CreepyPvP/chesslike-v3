import bpy
import bmesh
from array import array

for obj in bpy.data.objects:
    mesh = obj.to_mesh();
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    meta_data = [len(mesh.vertices), len(bm.faces) * 3];
    array('I', meta_data).tofile(output_file)
    indices = []
    for vert in mesh.vertices:
        vertices = []
        # pos (3 float)
        vertices.append(vert.co.x)
        vertices.append(vert.co.y)
        vertices.append(vert.co.z)
        # normals (3 float)
        vertices.append(vert.normal.x)
        vertices.append(vert.normal.y)
        vertices.append(vert.normal.z)

        # flush vertices to change to writing uints
        array('f', vertices).tofile(output_file)
        vertices = [];

        # bone data (3 uint, 3 float)
        bone_count = len(vert.groups)
        for i in range(0, min(3, bone_count)):
            group = vert.groups[i]
            vertices.append(group.group)
        for i in range(bone_count, 3):
            vertices.append(0)

        array('I', vertices).tofile(output_file)
        vertices = []

        for i in range(0, min(3, bone_count)):
            group = vert.groups[i]
            vertices.append(group.weight)
        for i in range(bone_count, 3):
            vertices.append(0)
        array('f', vertices).tofile(output_file)

    for face in list(bm.faces):
        indices.append(face.verts[0].index)
        indices.append(face.verts[1].index)
        indices.append(face.verts[2].index)
    output_file = open(obj.name + ".mod", 'wb')
    array('I', indices).tofile(output_file)
    output_file.close()
