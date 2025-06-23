## Quake MDL Reverse Engineer

This C program is designed to reverse-engineer Quake 1 `.mdl` model files. It extracts the texture skins into `.lbm` image files and the 3D animation frames into Alias `.tri` format files. This tool is useful for analyzing Quake's model assets, extracting geometry for other 3D applications, or for modding purposes.

### Features

- **MDL Header Parsing**: Reads and validates the MDL file header, extracting essential model information like dimensions, vertex, triangle, and frame counts.
- **Skin Extraction**: Extracts all texture skins embedded within the MDL file and saves them as 256-color `.lbm` (Amiga IFF ILBM) image files, using the hardcoded Quake palette.
- **Frame Geometry Extraction**: Extracts the 3D vertex data for each animation frame (including individual frames and frames within groups) and saves them as Alias `.tri` files. The `.tri` files contain the raw triangle geometry in a format compatible with older 3D modeling tools like Alias PowerAnimator (which was used to create Quake's models).
- **Endianness Handling**: Correctly handles Little-Endian data (MDL format) and converts it to Big-Endian where necessary for output formats like `.lbm` and `.tri`.

### How it Works

The program operates by sequentially reading chunks of data from the input `.mdl` file, interpreting them according to the known Quake MDL file format specification.

1. **MDL Header**: The program first reads the `mdl_header_t` structure. It verifies the magic number (`IDPOLYHEADER`) and version (`ALIAS_VERSION`) to ensure it's a valid Quake MDL file.
2. **Skins**: It then proceeds to read the skin data. Each skin is a raw 8-bit paletted image. The program allocates memory for the skin data, reads the pixel information, and then writes it out as an `.lbm` file. The hardcoded Quake palette is applied during the `.lbm` file creation.
3. **ST Vertices (Texture Coordinates)**: These are read to correctly advance the file pointer. While not directly used for the `.tri` geometry output, they are part of the MDL structure.
4. **Triangles (Indices)**: The program reads the `dtriangle_t` structures, which define the triangles by storing indices to the vertex list. This information is crucial for reconstructing the 3D mesh.
5. **Frames**: This is the most complex part. The MDL format supports both single animation frames and grouped animation frames.
   - **Single Frames**: For `ALIAS_SINGLE` frames, the program reads a `daliasframe_t` structure (containing bounding box and name) followed by `numverts` `trivertx_t` structures (byte-packed vertex coordinates).
   - **Grouped Frames**: For `ALIAS_GROUP` frames, it reads a `daliasgroup_t` structure, which indicates how many sub-frames are in the group. It then reads a series of float intervals (timing information, skipped for geometry extraction) followed by individual `daliasframe_t` headers and their corresponding `trivertx_t` vertex data for each sub-frame.
   - **Vertex Reconstruction**: For each frame, the byte-packed `trivertx_t` coordinates are converted back to floating-point `vec3_t` coordinates using the `scale` and `scale_origin` values found in the MDL header. This reverses the compression applied by Quake's original model compiler.
   - **TRI File Output**: The reconstructed floating-point vertices for each triangle are then written to a `.tri` file. The `.tri` format used here is a simplified Alias format, primarily containing vertex positions. Normals, colors, and UVs are zeroed out as they are not directly stored in this decompressed vertex format in the MDL.

### Building the Program

To compile this program, you will need a C compiler (like GCC).

Bash

```
gcc -o mdl_reverse_engineer mdl_reverse_engineer.c -lm
```

- `gcc`: The C compiler.
- `-o mdl_reverse_engineer`: Specifies the output executable name.
- `mdl_reverse_engineer.c`: The source code file.
- `-lm`: Links against the math library, necessary for `sqrtf` (used in `VectorLength`).

### Usage

The program expects a single command-line argument: the path to the input `.mdl` file.

Bash

```
./mdl_reverse_engineer <input_mdl_file>
```

**Example:**

Bash

```
./mdl_reverse_engineer progs/player.mdl
```

This will produce:

- `player_skin0.lbm`, `player_skin1.lbm`, etc. (one `.lbm` file for each skin)
- `player_frame0.tri`, `player_frame1.tri`, etc. (one `.tri` file for each animation frame or sub-frame within a group)
- For grouped frames (e.g., `flame.mdl`), you might see filenames like `flame_frameX_subY.tri`.

The output files will be created in the same directory where you run the program.

### Output Files Explained

- **`.lbm` files**: These are 256-color uncompressed Amiga IFF ILBM image files. They contain the texture data extracted from the MDL model. You can open these with various image editors that support older formats (e.g., Grafx2).
- **`.tri` files**: These are simplified Alias triangle files. Each file represents a single frame of the 3D model's animation. They contain the floating-point vertex coordinates for each triangle. These files are typically used for importing into 3D modeling software that supports the Alias format, or for custom tools. The format used is a basic representation of `tf_triangle`, and may require specific importers for various 3D software. (Blender Scripts for filetype included)

### Important Notes

- **Quake Palette**: The program uses a hardcoded Quake 1 palette to correctly display the `.lbm` skins. This palette is standard for Quake assets.
- **Alias .tri Format**: The `.tri` format generated is a straightforward dump of vertex positions. It does not include normals, colors, or UVs, as these are not directly reconstructable in a simple manner from the raw byte-packed MDL vertex data without additional information (like original `st_verts` or normal generation logic). For basic mesh extraction, it's sufficient.
- **Error Handling**: The program includes basic error handling for file operations and invalid MDL headers, reporting issues to `stderr` and exiting.
- **Memory Management**: Simple `malloc` and `free` are used for memory allocation. Large models might require significant memory, though typically Quake models are relatively small.
- **Debug Output**: The program includes `printf` statements with "DEBUG" prefixes to show sizes of structures and file pointer positions during execution. This can be helpful for understanding the parsing process or for debugging issues with specific MDL files.

### Limitations

- **Basic .tri output**: The `.tri` files only contain vertex positions. More advanced mesh information is not reconstructed.
- **Hardcoded Palette**: The Quake palette is hardcoded. While standard for Quake, it's not dynamic.
- **Simple Skin/Group Handling**: Skin and frame groups are handled by simply iterating and extracting each individual element. More sophisticated handling of skin types (e.g., `ALIAS_SKIN_GROUP`) or frame timing (intervals) is omitted for simplicity of geometry extraction.
- **No UV Unpacking**: The `stvert_t` (texture coordinates) are read to advance the file pointer but are not utilized in the `.tri` output because the `.tri` format as implemented here does not contain UV information per vertex, nor does the direct reverse of the packed `trivertx_t` give UVs. If you need UVs for your 3D application, you'd need to extend the `.tri` writing logic or use a different output format.

This tool provides a solid foundation for understanding and extracting assets from Quake 1 MDL files.
