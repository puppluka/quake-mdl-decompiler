bl_info = {
    "name": "Alias Triangle File - Import Script",
    "author": "Pup Luka (Troubleshooting by Paril)",
    "version": (2, 2),
    "blender": (2, 80, 0),
    "location": "File > Import",
    "description": "Imports official Quake 1/2 .TRI mesh files, correctly parsing the trilib.c object/group structure including vertex data padding.",
    "warning": "This script expects a magic number (0x0001E1BA). It strictly adheres to the Alias/trilib.c file format for all expected data. It may not work with simplified or other non-standard .tri files.",
    "doc_url": "",
    "category": "Import-Export",
}

import bpy
import struct
import os
import math

# This has been changed to match the '00 01 E1 BA' sequence when interpreted as Big-Endian.
# The original listed magic was 123322 (0x0001EBA2). [VERY VERY WRONG] ~luka
Q1_TRI_MAGIC = 0x0001E1BA

# Define the FLOAT_START and FLOAT_END markers (Big-Endian float representation)
FLOAT_START_VALUE = 99999.0
FLOAT_END_VALUE = -99999.0

# Epsilon for float comparisons to handle minor precision differences
FLOAT_EPSILON = 0.0001


def read_tri_data(context, filepath):
    """
    Reads a Quake 1/2 .tri file and imports its objects into Blender.
    This script adheres to the Quake trilib.c parsing logic, but with a flexible magic number.
    """
    print(f"Importing Quake .tri file: {filepath}")

    try:
        with open(filepath, 'rb') as f:
            # 1. Read Magic Number (4 bytes, Big-Endian)
            magic_bytes = f.read(4)
            if len(magic_bytes) < 4:
                raise IOError(f"File '{filepath}' is too short to contain a magic number.")

            # Unpack the magic number as a Big-Endian unsigned integer
            magic = struct.unpack('>I', magic_bytes)[0]

            # Check if the read magic number matches our expected (potentially non-standard) value
            if magic != Q1_TRI_MAGIC:
                print(f"DEBUG: Read Magic: 0x{magic:08X} (Decimal: {magic}). Expected 0x{Q1_TRI_MAGIC:08X} (Decimal: {Q1_TRI_MAGIC}).")
                raise ValueError(f"Magic number mismatch. Expected 0x{Q1_TRI_MAGIC:08X}, got 0x{magic:08X}.")
            else:
                print(f"Read Magic: 0x{magic:08X} - OK")

            # Keep reading object chunks until End-Of-File
            while True:
                current_pos = f.tell()
                header_marker_bytes = f.read(4)

                if not header_marker_bytes: # Reached EOF cleanly
                    print("Reached end of file.")
                    break
                
                if len(header_marker_bytes) < 4:
                    print(f"Warning: Partial read ({len(header_marker_bytes)} bytes) at end of file. Possible truncation.")
                    break

                # Unpack the header marker as a Big-Endian float
                header_marker = struct.unpack('>f', header_marker_bytes)[0]

                # Check for the FLOAT_END marker
                if math.isclose(header_marker, FLOAT_END_VALUE, rel_tol=FLOAT_EPSILON):
                    print(f"Read FLOAT_END marker at pos {current_pos}.")
                    # After FLOAT_END, the trilib parser reads another object name (a redundancy).
                    name_bytes = bytearray()
                    while True:
                        byte = f.read(1)
                        if not byte or byte == b'\0':
                            break
                        name_bytes.extend(byte)
                    obj_end_name = name_bytes.decode('ascii', errors='ignore')
                    print(f"  Object End Name: '{obj_end_name}'")
                    # Continue the loop to look for the next FLOAT_START
                    continue
                
                # Check for the FLOAT_START marker (beginning of a new object)
                elif math.isclose(header_marker, FLOAT_START_VALUE, rel_tol=FLOAT_EPSILON):
                    print(f"Read FLOAT_START marker at pos {current_pos}. Beginning a new object.")

                    # Read Object Name (null-terminated string)
                    name_bytes = bytearray()
                    while True:
                        byte = f.read(1)
                        if not byte or byte == b'\0':
                            break
                        name_bytes.extend(byte)
                    
                    object_name_from_file = name_bytes.decode('ascii', errors='ignore')
                    print(f"  Object Name: '{object_name_from_file}'")

                    # Read Number of Triangles for this object (4 bytes, Big-Endian signed int)
                    num_triangles_bytes = f.read(4)
                    if len(num_triangles_bytes) < 4:
                        raise IOError("File is too short to contain triangle count.")
                    
                    num_triangles = struct.unpack('>i', num_triangles_bytes)[0]
                    print(f"  Triangle Count: {num_triangles}")

                    if num_triangles < 0 or num_triangles > 200000: # Sanity check for extremely large models
                        raise ValueError(f"Suspicious number of triangles ({num_triangles}). File may be corrupted.")
                    
                    if num_triangles > 0:
                        # Read Texture Name (null-terminated string) if triangles exist
                        tex_name_bytes = bytearray()
                        while True:
                            byte = f.read(1)
                            if not byte or byte == b'\0':
                                break
                            tex_name_bytes.extend(byte)
                        
                        texture_name_from_file = tex_name_bytes.decode('ascii', errors='ignore')
                        print(f"  Texture Name: '{texture_name_from_file}'")

                        # --- Read Triangle Data ---
                        # Each triangle in the .tri file is stored as 3 'aliaspoint_t' structures.
                        # An 'aliaspoint_t' is comprised of:
                        # - vector n (normal): 3 floats
                        # - vector p (point/position): 3 floats
                        # - vector c (color): 3 floats
                        # - float u, v (texture coordinates): 2 floats
                        # Total: 11 floats per aliaspoint_t = 44 bytes per vertex.
                        # Total: 3 vertices * 44 bytes/vertex = 132 bytes per triangle.

                        verts = []
                        faces = []
                        for i in range(num_triangles):
                            # For each of the 3 vertices in the current triangle
                            for v_idx_in_tri in range(3):
                                # Read 11 Big-Endian floats (44 bytes) for the aliaspoint_t structure
                                alias_point_data_bytes = f.read(44)
                                if len(alias_point_data_bytes) < 44:
                                    raise IOError(f"Unexpected EOF while reading aliaspoint_t for triangle {i+1}, vertex {v_idx_in_tri+1}.")
                                
                                # Unpack all 11 floats from the bytes
                                # n.x, n.y, n.z, p.x, p.y, p.z, c.x, c.y, c.z, u, v
                                all_floats = struct.unpack('>11f', alias_point_data_bytes)
                                
                                # The vertex position (p) is the 4th, 5th, and 6th float (index 3, 4, 5)
                                px, py, pz = all_floats[3], all_floats[4], all_floats[5]
                                
                                verts.append((px, py, pz))
                            
                            # Define the face using the last 3 vertices added to the list.
                            # Blender faces use indices relative to the mesh's vertex list.
                            v0_idx = len(verts) - 3
                            v1_idx = len(verts) - 2
                            v2_idx = len(verts) - 1
                            faces.append((v0_idx, v1_idx, v2_idx))

                        # Create the mesh and object in Blender
                        mesh_name = object_name_from_file + "_mesh"
                        obj_name = object_name_from_file
                        mesh = bpy.data.meshes.new(mesh_name)
                        obj = bpy.data.objects.new(obj_name, mesh)
                        
                        # Link the object to the active collection and make it active for context
                        bpy.context.collection.objects.link(obj)
                        bpy.context.view_layer.objects.active = obj
                        obj.select_set(True)

                        if verts and faces:
                            # Populate the mesh with vertex and face data
                            mesh.from_pydata(verts, [], faces)
                            # Update the mesh to apply changes and recalculate normals implicitly
                            mesh.update()
                            print(f"Successfully imported {len(faces)} triangles for object '{object_name_from_file}'.")
                        else:
                            print(f"Warning: No geometry data read for object '{object_name_from_file}'. Created an empty object.")
                    else:
                        print(f"Object '{object_name_from_file}' has 0 triangles. Skipping geometry import.")

                # If the header marker is neither FLOAT_START nor FLOAT_END, it's an unexpected format.
                else:
                    raise ValueError(f"Unexpected header pattern at pos {current_pos}: {header_marker} (raw: {header_marker_bytes.hex()}). File may be corrupted or not a valid .tri file.")

    except FileNotFoundError:
        print(f"Error: File not found at '{filepath}'")
        return {'CANCELLED'}
    except IOError as e:
        print(f"IO Error: {e}")
        return {'CANCELLED'}
    except ValueError as e:
        print(f"Data Error: {e}")
        return {'CANCELLED'}
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return {'CANCELLED'}

    return {'FINISHED'}


# Blender boilerplate for registration
from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty
from bpy.types import Operator


class ImportTriOfficial(Operator, ImportHelper):
    """Import an official Quake .TRI File"""
    bl_idname = "import_scene.quake_tri_official"
    bl_label = "Import Quake (.tri)"

    filename_ext = ".tri"

    # Define a filter for the file browser to show only .tri files
    filter_glob: StringProperty(
        default="*.tri",
        options={'HIDDEN'}, # Hidden means it's applied automatically, not selectable by user
        maxlen=255,
    )

    def execute(self, context):
        """
        Main execution method called by Blender when the import operator is run.
        """
        # Call our custom function to read and import the .tri data
        return read_tri_data(context, self.filepath)


def menu_func_import(self, context):
    """
    Adds the import option to Blender's File > Import menu.
    """
    self.layout.operator(ImportTriOfficial.bl_idname, text="Quake (.tri)")


def register():
    """
    Registers Blender classes and adds the import menu entry.
    This function is called by Blender when the add-on is enabled.
    """
    bpy.utils.register_class(ImportTriOfficial)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    """
    Unregisters Blender classes and removes the import menu entry.
    This function is called by Blender when the add-on is disabled.
    """
    bpy.utils.unregister_class(ImportTriOfficial)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)


if __name__ == "__main__":
    # This block is executed when the script is run directly (e.g., for testing within Blender's text editor)
    # It registers the add-on.
    register()
    # To test loading a specific file directly in a Blender script.
    # Could call from Blender Text:
    # import_file_path = "C:\\path\\to\\your\\base.tri" # Replace with actual path
    # bpy.ops.import_scene.quake_tri_official(filepath=import_file_path)

