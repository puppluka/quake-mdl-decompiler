The `.tri` file format is a binary structure designed to store 3D model geometry along with per-vertex normals, colors, and UV coordinates, organized into objects or groups. It is primarily associated with older 3D modeling tools like Alias and is used in engines like Quake.

Here is a detailed and granular breakdown of the `.tri` file structure from start to finish, based on the provided `trilib.c` and `trilib.h` source code:

### 1. File Header

* **Magic Number (4 bytes, Big-Endian Integer):**
    * The file begins with a 4-byte integer representing a magic number.
    * This value is `123322` (or `0x0001E1BA` in hexadecimal).
    * It serves as an identifier to confirm the file is a valid `.tri` format.
    * The integer is stored in **Big-Endian** byte order (most significant byte first).

### 2. Object/Group Blocks (Repeated Structure)

The file then proceeds with a sequence of object or group blocks. Each block begins with a `FLOAT_START` marker and is delineated by a `FLOAT_END` marker. These blocks can be nested, indicating hierarchical grouping.

#### a. Start Marker (`FLOAT_START`)

* **`FLOAT_START` Value (4 bytes, Big-Endian Float):**
    * A floating-point value of `99999.0` marks the beginning of an object or group of objects.
    * This float is stored in **Big-Endian** byte order.

#### b. Object/Group Name

* **Name String (Null-terminated ASCII):**
    * Immediately following the `FLOAT_START` marker is a null-terminated ASCII string representing the name of the object or group.
    * The maximum length for this name is typically 255 characters plus the null terminator.

#### c. Triangle Count

* **Number of Triangles (4 bytes, Big-Endian Integer):**
    * A 4-byte integer specifies the total number of triangles contained within the current object block.
    * This integer is stored in **Big-Endian** byte order.
    * If this count is `0`, it indicates the start of a group (rather than an object with geometry), and no texture name will follow immediately.

#### d. Texture Name (Conditional)

* **Texture Name String (Null-terminated ASCII):**
    * If the "Number of Triangles" for the current object is greater than `0`, a null-terminated ASCII string follows.
    * This string specifies the texture name associated with the triangles in this object.
    * Typically, the texture name is truncated to 8 characters (plus extension) and converted to lowercase (e.g., `texname.tga`).

#### e. Triangle Data (Repeated for `Triangle Count`)

If the "Number of Triangles" is greater than `0`, the detailed triangle data follows. Each triangle consists of three "alias points."

* **Triangle Structure (`tf_triangle`):** Each triangle is defined by 3 `aliaspoint_t` structures, making a total of 33 floating-point values per triangle.
    * **Alias Point (`aliaspoint_t` - 11 floats per point, Big-Endian):** Each of the three points in a triangle is described by 11 floating-point values. All these floats are stored in **Big-Endian** byte order on disk.
        * **Normal (3 floats: N.x, N.y, N.z):**
            * Represents the X, Y, and Z components of the normal vector for this specific corner of the triangle. This allows for smooth shading.
        * **Point (3 floats: P.x, P.y, P.z):**
            * Represents the X, Y, and Z coordinates of the vertex position in 3D space.
        * **Color (3 floats: C.r, C.g, C.b):**
            * Represents the Red, Green, and Blue components of the vertex color, typically in a 0.0 to 1.0 range.
        * **UV Coordinates (2 floats: U, V):**
            * Represents the U and V texture coordinates for this specific corner, mapping to a point on a 2D texture.

### 3. File Footer

* **`FLOAT_END` Value (4 bytes, Big-Endian Float):**
    * A floating-point value of `-99999.0` marks the end of an object or group, or the end of the entire file.
    * This float is stored in **Big-Endian** byte order.

* **End-of-File Name (Null-terminated ASCII):**
    * A null-terminated ASCII string follows the `FLOAT_END` marker. When it signifies the absolute end of the file, this name is typically "EndOfFile".

### Summary of Data Types and Endianness:

* **Magic Number:** 4-byte Integer, Big-Endian.
* **`FLOAT_START`/`FLOAT_END` Markers:** 4-byte Floats, Big-Endian.
* **Object/Group Name:** Null-terminated ASCII string.
* **Triangle Count:** 4-byte Integer, Big-Endian.
* **Texture Name:** Null-terminated ASCII string.
* **All Floats within Triangle Data (`aliaspoint_t` components):** 4-byte Floats, Big-Endian.

This structure allows for a comprehensive representation of polygonal models, including not just geometry but also the necessary data for rendering (normals, colors, UVs) for each vertex of every triangle.