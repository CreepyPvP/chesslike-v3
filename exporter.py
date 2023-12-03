import bpy
import bmesh
from array import array

for obj in bpy.data.objects:
    if obj.name not in bpy.data.meshes:
        continue
    mesh = bpy.data.meshes[obj.name]
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    meta_data = [len(mesh.vertices), len(bm.faces) * 3];
    vertices = []
    indices = []
    for vert in mesh.vertices:
        # pos
        vertices.append(vert.co.x)
        vertices.append(vert.co.y)
        vertices.append(vert.co.z)
        # normals
        vertices.append(vert.normal.x)
        vertices.append(vert.normal.y)
        vertices.append(vert.normal.z)
    for face in list(bm.faces):
        indices.append(face.verts[0].index)
        indices.append(face.verts[1].index)
        indices.append(face.verts[2].index)
    output_file = open("models/" + obj.name + ".mod", 'wb')
    array('I', meta_data).tofile(output_file)
    array('f', vertices).tofile(output_file)
    array('I', indices).tofile(output_file)
    output_file.close()
