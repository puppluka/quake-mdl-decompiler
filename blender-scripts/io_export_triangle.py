bl_info = {
    "name": "Alias Triangle File - Export Script",
    "author": "Pup Luka (Troubleshooting by Paril)",
    "version": (1, 0),
    "blender": (3, 0, 0), # Targeting Blender 3.0+ for better API consistency
    "location": "File > Export",
    "description": "Exports Blender meshes to official Quake 1/2 .TRI mesh files. Modifiers are NOT applied during export. Adheres to trilib.c object/group structure.",
    "warning": "This script exports only mesh data and assigns default normals/colors. Materials are simplified to a single texture name. Coordinate system conversion is applied (Blender Z-up to Quake Y-up). Modifiers on objects are NOT applied.",
    "doc_url": "",
    "category": "Import-Export",
}

import bpy
import struct
import os
import math
import bmesh # Make sure bmesh is imported for triangulation

# Constants from trilib.c and io_import_triangle.py
Q1_TRI_MAGIC = 0x0001E1BA
FLOAT_START_VALUE = 99999.0
FLOAT_END_VALUE = -99999.0

# Epsilon for float comparisons (though less critical for export)
FLOAT_EPSILON = 0.0001

def write_tri_data(operator, context, filepath,
                   use_selection=True, # apply_modifiers removed
                   coordinate_conversion='BLENDER_TO_QUAKE'):
    """
    Writes selected Blender mesh objects to a Quake 1/2 .tri file.
    Modifiers are NOT applied during this export.
    """
    print(f"Exporting Quake .tri file: {filepath}")

    # Ensure the file has the correct extension
    if not filepath.lower().endswith(".tri"):
        filepath += ".tri"

    try:
        with open(filepath, 'wb') as f:
            # 1. Write Magic Number (4 bytes, Big-Endian)
            f.write(struct.pack('>I', Q1_TRI_MAGIC))
            print(f"Wrote Magic: 0x{Q1_TRI_MAGIC:08X}")

            objects_to_export = []
            if use_selection:
                # Filter for mesh objects in selection
                objects_to_export = [obj for obj in context.selected_objects if obj.type == 'MESH']
            else:
                # Filter for all mesh objects in the scene
                objects_to_export = [obj for obj in context.scene.objects if obj.type == 'MESH']

            if not objects_to_export:
                print("No mesh objects found to export (check selection or 'Export Selected Only' option).")
                # Still write FLOAT_END and empty name for a valid, empty file
                f.write(struct.pack('>f', FLOAT_END_VALUE))
                f.write(b"NoObjectsExported\0")
                operator.report({'WARNING'}, "No mesh objects selected or found for export. Exported an empty .tri file.")
                return {'CANCELLED'}

            exported_count = 0

            for obj in objects_to_export:
                # Ensure the object is visible and a mesh
                if obj.type != 'MESH' or obj.hide_get() or obj.hide_render:
                    print(f"Skipping non-mesh or hidden object: {obj.name} (Type: {obj.type})")
                    continue

                print(f"\nProcessing object: {obj.name}")
                print(f"  Initial vertex count (obj.data): {len(obj.data.vertices)}")
                print(f"  Initial polygon count (obj.data): {len(obj.data.polygons)}")

                # Directly use the original mesh data - modifiers will NOT be applied
                mesh_eval = obj.data
                mesh_created_by_script = False # No temporary mesh created
                print("  Using original mesh data (modifiers NOT applied).")

                # --- DEBUG PRINTS for mesh_eval ---
                print(f"  Type of mesh_eval: {type(mesh_eval)}")
                if mesh_eval:
                    print(f"  mesh_eval has valid data (not None).")
                    print(f"  mesh_eval vertex count: {len(mesh_eval.vertices)}")
                    print(f"  mesh_eval polygon count: {len(mesh_eval.polygons)}")
                    if len(mesh_eval.vertices) == 0 or len(mesh_eval.polygons) == 0:
                        print("  WARNING: mesh_eval contains no vertices or polygons. This is likely the cause of 0 triangles.")
                else:
                    print("  ERROR: mesh_eval is None! Cannot proceed with BMesh conversion.")
                    operator.report({'ERROR'}, f"Mesh data for {obj.name} is None. Cannot export.")
                    continue # Skip to next object
                # --- END DEBUG PRINTS ---

                # We need to triangulate the mesh, as .tri only supports triangles
                # Create a BMesh from the mesh data for robust triangulation
                bm = bmesh.new()
                try:
                    bm.from_mesh(mesh_eval)
                    print(f"  BMesh created from mesh_eval. Verts: {len(bm.verts)}, Faces: {len(bm.faces)}")
                    if len(bm.verts) == 0 or len(bm.faces) == 0:
                        print("  WARNING: BMesh creation resulted in 0 verts/faces, despite mesh_eval having data. This is unexpected and might indicate an issue with the mesh data itself.")
                        operator.report({'WARNING'}, f"BMesh creation for {obj.name} resulted in an empty mesh. Skipping.")
                        bm.free()
                        continue # Skip to next object if BMesh is empty
                except Exception as e:
                    print(f"  ERROR: Could not create BMesh from mesh_eval: {e}")
                    operator.report({'ERROR'}, f"Failed to process mesh for {obj.name}: {e}")
                    bm.free()
                    continue # Skip to next object if this one fails
                
                # Triangulate all faces
                # Check if triangulation is needed (i.e., if there are any N-gons or quads)
                needs_triangulation = False
                for face in bm.faces:
                    if len(face.loops) > 3:
                        needs_triangulation = True
                        break
                
                if needs_triangulation:
                    bmesh.ops.triangulate(bm, faces=bm.faces)
                    print(f"  BMesh triangulated. New Face count: {len(bm.faces)}")
                else:
                    print("  No N-gons or quads found, triangulation skipped.")
                
                # Collect triangulated vertices and faces with their original loop indices
                triangles_data = []
                
                # Get active UV layer
                uv_layer = bm.loops.layers.uv.active
                if not uv_layer:
                    print("  No active UV layer found for this mesh. UVs will be exported as (0.0, 0.0).")
                
                # Get active vertex color layer (if any)
                color_layer = bm.loops.layers.color.active
                if not color_layer:
                    print("  No active Vertex Color layer found for this mesh. Colors will be exported as (1.0, 1.0, 1.0).")

                # Default color (white) if no vertex colors
                default_color = (1.0, 1.0, 1.0) # RGB

                actual_triangles_processed = 0
                for face in bm.faces:
                    if len(face.loops) == 3: # Ensure it's a triangle after triangulation
                        actual_triangles_processed += 1
                        triangle_points = []
                        triangle_normals = []
                        triangle_colors = []
                        triangle_uvs = []

                        for loop in face.loops:
                            # Position
                            pos = loop.vert.co
                            
                            # Normal (per loop for smooth shading, using vertex normal as fallback)
                            normal = loop.vert.normal # Corrected: using loop.vert.normal instead of loop.normal

                            # Color
                            if color_layer:
                                col = loop[color_layer].color
                                triangle_colors.append((col.r, col.g, col.b))
                            else:
                                triangle_colors.append(default_color)

                            # UV
                            uv = (0.0, 0.0)
                            if uv_layer:
                                uv = loop[uv_layer].uv
                            triangle_uvs.append(uv)

                            # Apply coordinate conversion
                            if coordinate_conversion == 'BLENDER_TO_QUAKE':
                                # Blender Z-up (X, Y, Z) to Quake Y-up (X, -Z, Y)
                                pos_converted = (pos.x, -pos.z, pos.y)
                                normal_converted = (normal.x, -normal.z, normal.y) # Normals transform same as positions
                            else: # No conversion (identity)
                                pos_converted = (pos.x, pos.y, pos.z)
                                normal_converted = (normal.x, normal.y, normal.z)
                            
                            triangle_points.append(pos_converted)
                            triangle_normals.append(normal_converted)
                            

                        # Store per-triangle data. The .tri format requires unique (normal, position, color, uv)
                        # for each vertex in each triangle, even if the underlying vertex is shared.
                        alias_points = []
                        for i in range(3):
                            n_val = triangle_normals[i]
                            p_val = triangle_points[i]
                            c_val = triangle_colors[i]
                            uv_val = triangle_uvs[i]
                            
                            # aliaspoint_t: n.x, n.y, n.z, p.x, p.y, p.z, c.x, c.y, c.z, u, v (11 floats)
                            alias_points.append(
                                (n_val[0], n_val[1], n_val[2],
                                 p_val[0], p_val[1], p_val[2],
                                 c_val[0], c_val[1], c_val[2],
                                 uv_val[0], uv_val[1])
                            )
                        triangles_data.append(alias_points)
                
                print(f"  Actual triangles processed (after BMesh face iteration): {actual_triangles_processed}")
                # Free BMesh data
                bm.free()

                # Cleanup temporary mesh data block (not created in this simplified version)
                # This block is now effectively a no-op due to mesh_created_by_script always being False.
                if mesh_created_by_script and mesh_eval:
                    if mesh_eval.users == 0:
                        bpy.data.meshes.remove(mesh_eval)
                        print(f"  Cleaned up temporary mesh data block: {mesh_eval.name}")
                    else:
                        print(f"  WARNING: Temporary mesh data block {mesh_eval.name} still has users ({mesh_eval.users}). Not removing.")

                num_triangles_for_obj = len(triangles_data)
                print(f"  Total triangles collected for writing: {num_triangles_for_obj}")

                # --- Write FLOAT_START marker ---
                f.write(struct.pack('>f', FLOAT_START_VALUE))

                # --- Write Object Name ---
                object_name = obj.name[:254].encode('ascii', errors='replace') + b'\0'
                f.write(object_name)
                print(f"  Object Name written: '{obj.name}'")

                # --- Write Number of Triangles ---
                f.write(struct.pack('>i', num_triangles_for_obj))
                print(f"  Triangle Count written: {num_triangles_for_obj}")

                if num_triangles_for_obj > 0:
                    # --- Determine Texture Name ---
                    texture_name = "default_tex.tga" # Default fallback
                    if obj.material_slots:
                        for slot in obj.material_slots:
                            material = slot.material
                            if material and material.use_nodes:
                                for node in material.node_tree.nodes:
                                    if node.type == 'TEX_IMAGE' and node.image:
                                        tex_base_name = os.path.basename(node.image.filepath)
                                        texture_name = os.path.splitext(tex_base_name)[0].lower()[:8] + ".tga"
                                        break
                                if texture_name != "default_tex.tga":
                                    break
                    
                    texture_name_bytes = texture_name[:254].encode('ascii', errors='replace') + b'\0'
                    f.write(texture_name_bytes)
                    print(f"  Texture Name written: '{texture_name}'")

                    # --- Write Triangle Data ---
                    for triangle_alias_points in triangles_data:
                        for alias_point_data in triangle_alias_points:
                            f.write(struct.pack('>11f', *alias_point_data))
                
                exported_count += 1
                print(f"Successfully exported {num_triangles_for_obj} triangles for object '{obj.name}'.")

            # --- Final FLOAT_END marker and dummy name ---
            f.write(struct.pack('>f', FLOAT_END_VALUE))
            f.write(b"EndOfFile\0")
            print(f"Wrote FLOAT_END marker and EndOfFile name.")

        operator.report({'INFO'}, f"Exported {exported_count} mesh(es) to {filepath}")
        return {'FINISHED'}

    except FileNotFoundError:
        operator.report({'ERROR'}, f"Error: File not found at '{filepath}'")
        return {'CANCELLED'}
    except IOError as e:
        operator.report({'ERROR'}, f"IO Error: {e}")
        return {'CANCELLED'}
    except Exception as e:
        operator.report({'ERROR'}, f"An unexpected error occurred: {e}")
        import traceback
        traceback.print_exc()
        return {'CANCELLED'}


# Blender boilerplate for registration
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportTriOfficial(Operator, ExportHelper):
    """Export to official Quake .TRI File"""
    bl_idname = "export_scene.quake_tri_official"
    bl_label = "Export Quake (.tri)"

    filename_ext = ".tri"

    filter_glob: StringProperty(
        default="*.tri",
        options={'HIDDEN'},
        maxlen=255,
    )

    # Export options - apply_modifiers removed
    use_selection: BoolProperty(
        name="Export Selected Only",
        description="Export only selected mesh objects.",
        default=True,
    )
    coordinate_conversion: EnumProperty(
        name="Coordinate Conversion",
        description="Convert coordinates from Blender's Z-up to Quake's Y-up.",
        items=[
            ('NONE', 'None', 'No coordinate conversion applied.'),
            ('BLENDER_TO_QUAKE', 'Blender to Quake (X, -Z, Y)', 'Convert Z-up (Blender) to Y-up (Quake): (X, -Z, Y).'),
        ],
        default='BLENDER_TO_QUAKE',
    )


    def execute(self, context):
        return write_tri_data(
            self, # Pass 'self' (the operator instance) to write_tri_data
            context,
            self.filepath,
            # apply_modifiers=self.apply_modifiers, # Removed
            use_selection=self.use_selection,
            coordinate_conversion=self.coordinate_conversion
        )

    def draw(self, context):
        layout = self.layout
        # layout.row().prop(self, "apply_modifiers") # Removed
        row = layout.row()
        row.prop(self, "use_selection")
        row = layout.row()
        row.prop(self, "coordinate_conversion")

def menu_func_export(self, context):
    self.layout.operator(ExportTriOfficial.bl_idname, text="Quake (.tri)")


def register():
    bpy.utils.register_class(ExportTriOfficial)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
    bpy.utils.unregister_class(ExportTriOfficial)


if __name__ == "__main__":
    register()