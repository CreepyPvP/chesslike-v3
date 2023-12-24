# import bpy
# import bmesh
from array import array

def export_all_meshes():
    for mesh in bpy.data.meshes:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        vertices = []
        indices = []
        vertex_count = 0
        for vert in mesh.vertices:
            # pos
            vertices.append(vert.co.x)
            vertices.append(vert.co.y)
            vertices.append(vert.co.z)
            # normals
            vertices.append(vert.normal.x)
            vertices.append(vert.normal.y)
            vertices.append(vert.normal.z)
            
            vertex_count += 1
        for face in list(bm.faces):
            indices.append(face.verts[0].index)
            indices.append(face.verts[1].index)
            indices.append(face.verts[2].index)
        export_mesh(mesh.name, vertices, indices, vertex_count)

def export_mesh(name, vertices, indices, vertex_count):
    output_file = open("assets/" + name + ".mod", 'wb')
    array('I', [vertex_count, len(indices)]).tofile(output_file)
    array('f', vertices).tofile(output_file)
    array('I', indices).tofile(output_file)
    output_file.close()

def export_rigged_mesh(name, vertices, indices, vertex_count):
    output_file = open("assets/" + name + ".mod", 'wb')
    array('I', [vertex_count, len(indices)]).tofile(output_file)
    for i in range(0, vertex_count):
        # 3 f position, 3 f normals
        array('f', vertices[12 * i : i * 12 + 6]).tofile(output_file)
        # 3 i bone ids
        array('i', vertices[12 * i + 6 : i * 12 + 9]).tofile(output_file)
        # 3 f weights
        array('f', vertices[12 * i + 9 : i * 12 + 12]).tofile(output_file)
    array('I', indices).tofile(output_file)
    output_file.close()

def export_cube():
    vertices = []
    indices = []
    
    vertices += [1, 1, 1, 0, 0, 1]
    vertices += [-1, 1, 1, 0, 0, 1]
    vertices += [1, -1, 1, 0, 0, 1]
    vertices += [-1, -1, 1, 0, 0, 1]
    indices += [0, 1, 3, 0, 3, 2]

    vertices += [1, 1, -1, 0, 0, -1]
    vertices += [-1, 1, -1, 0, 0, -1]
    vertices += [1, -1, -1, 0, 0, -1]
    vertices += [-1, -1, -1, 0, 0, -1]
    indices += [7, 5, 4, 6, 7, 4]

    vertices += [1, 1, 1, 1, 0, 0]
    vertices += [1, -1, 1, 1, 0, 0]
    vertices += [1, 1, -1, 1, 0, 0]
    vertices += [1, -1, -1, 1, 0, 0]
    indices += [8, 9, 11, 8, 11, 10]

    vertices += [-1, 1, 1, -1, 0, 0]
    vertices += [-1, -1, 1, -1, 0, 0]
    vertices += [-1, 1, -1, -1, 0, 0]
    vertices += [-1, -1, -1, -1, 0, 0]
    indices += [15, 13, 12, 14, 15, 12]

    vertices += [1, 1, 1, 0, 1, 0]
    vertices += [-1, 1, 1, 0, 1, 0]
    vertices += [1, 1, -1, 0, 1, 0]
    vertices += [-1, 1, -1, 0, 1, 0]
    indices += [19, 17, 16, 18, 19, 16]

    vertices += [1, -1, 1, 0, -1, 0]
    vertices += [-1, -1, 1, 0, -1, 0]
    vertices += [1, -1, -1, 0, -1, 0]
    vertices += [-1, -1, -1, 0, -1, 0]
    indices += [20, 21, 23, 20, 23, 22]

    export_mesh("cube", vertices, indices, 24)

def export_rigged_cube():
    vertices = []
    indices = []
    
    vertices += [1, 1, 1, 0, 0, 1, 1, -1, -1, 1, 0, 0]
    vertices += [-1, 1, 1, 0, 0, 1, 1, -1, -1, 1, 0, 0]
    vertices += [1, -1, 1, 0, 0, 1, 1, -1, -1, 1, 0, 0]
    vertices += [-1, -1, 1, 0, 0, 1, 1, -1, -1, 1, 0, 0]
    indices += [0, 1, 3, 0, 3, 2]

    vertices += [1, 1, -1, 0, 0, -1, 0, -1, -1, 1, 0, 0]
    vertices += [-1, 1, -1, 0, 0, -1, 0, -1, -1, 1, 0, 0]
    vertices += [1, -1, -1, 0, 0, -1, 0, -1, -1, 1, 0, 0]
    vertices += [-1, -1, -1, 0, 0, -1, 0, -1, -1, 1, 0, 0]
    indices += [7, 5, 4, 6, 7, 4]

    vertices += [1, 1, 1, 1, 0, 0, 1, -1, -1, 1, 0, 0]
    vertices += [1, -1, 1, 1, 0, 0, 1, -1, -1, 1, 0, 0]
    vertices += [1, 1, -1, 1, 0, 0, 0, -1, -1, 1, 0, 0]
    vertices += [1, -1, -1, 1, 0, 0, 0, -1, -1, 1, 0, 0]
    indices += [8, 9, 11, 8, 11, 10]

    vertices += [-1, 1, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0]
    vertices += [-1, -1, 1, -1, 0, 0, 1, -1, -1, 1, 0, 0]
    vertices += [-1, 1, -1, -1, 0, 0, 0, -1, -1, 1, 0, 0]
    vertices += [-1, -1, -1, -1, 0, 0, 0, -1, -1, 1, 0, 0]
    indices += [15, 13, 12, 14, 15, 12]

    vertices += [1, 1, 1, 0, 1, 0, 1, -1, -1, 1, 0, 0]
    vertices += [-1, 1, 1, 0, 1, 0, 1, -1, -1, 1, 0, 0]
    vertices += [1, 1, -1, 0, 1, 0, 0, -1, -1, 1, 0, 0]
    vertices += [-1, 1, -1, 0, 1, 0, 0, -1, -1, 1, 0, 0]
    indices += [19, 17, 16, 18, 19, 16]

    vertices += [1, -1, 1, 0, -1, 0, 1, -1, -1, 1, 0, 0]
    vertices += [-1, -1, 1, 0, -1, 0, 1, -1, -1, 1, 0, 0]
    vertices += [1, -1, -1, 0, -1, 0, 0, -1, -1, 1, 0, 0]
    vertices += [-1, -1, -1, 0, -1, 0, 0, -1, -1, 1, 0, 0]
    indices += [20, 21, 23, 20, 23, 22]

    export_rigged_mesh("rigged_cube", vertices, indices, 24)

export_cube()
export_rigged_cube()
