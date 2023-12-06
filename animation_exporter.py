import bpy
import bmesh
from array import array

# Armature
def read_skeleton(armature):
    # child_id => parent_id
    parents = []
    # name => id
    bones = {}
    id_counter = 1
    root_bone = armature.bones[0]
    bones[root_bone.name] = 0
    parents.append(0)
    queue = [root_bone]
    queue_next = []
    while len(queue) > 0:
        for bone in queue:
            bone_id = bones[bone.name]
            queue_next += bone.children
            for child in bone.children:
                bones[child.name] = id_counter
                parents.append(bone_id)
                id_counter += 1
        queue = queue_next
        queue_next = []
    return {
        "parents": parents, 
        "bones": bones, 
        "bone_count": len(armature.bones)
    }

def export_skeleton(armature):
    sk = read_skeleton(armature)
    data = [sk["bone_count"]]
    data += sk["parents"]
    output_file = open(armature.name + ".sk", 'wb')
    array('I', data).tofile(output_file)
    output_file.close()

def export_model():
    for obj in bpy.data.objects:
        mesh = obj.to_mesh();
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        meta_data = [len(mesh.vertices), len(bm.faces) * 3];
        output_file = open(obj.name + ".mod", 'wb')
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
            array('I', indices).tofile(output_file)
            output_file.close()
