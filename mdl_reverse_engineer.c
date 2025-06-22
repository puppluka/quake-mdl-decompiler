#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> // For va_list, vprintf
#include <errno.h>  // For strerror
#include <math.h>   // For sqrtf

// Define missing common types and structures from Quake's original code
#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum {false, true} qboolean;
typedef unsigned char byte;
#endif

typedef float vec3_t[3];

// --- Dummy definitions for cmdlib.h functions ---
// These are simplified implementations of functions found in cmdlib.c
// necessary for basic file I/O and error handling.

// Error function: Prints an error message and exits the program.
void Error (char *error, ...)
{
	va_list argptr;
	fprintf(stderr, "************ ERROR ************\n");
	va_start (argptr,error);
	vfprintf (stderr, error, argptr);
	va_end (argptr);
	fprintf(stderr, "\n");
	exit (1);
}

// filelength: Returns the size of an open file.
int filelength (FILE *f)
{
	long pos;
	long end;
	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET); // Restore original position
	return (int)end;
}

// SafeOpenRead: Opens a file for binary reading and handles errors.
FILE *SafeOpenRead (char *filename)
{
	FILE	*f;
	f = fopen(filename, "rb");
	if (!f)
		Error ("Error opening %s for read: %s", filename, strerror(errno));
	return f;
}

// SafeOpenWrite: Opens a file for binary writing and handles errors.
FILE *SafeOpenWrite (char *filename)
{
	FILE	*f;
	f = fopen(filename, "wb");
	if (!f)
		Error ("Error opening %s for write: %s", filename, strerror(errno));
	return f;
}

// SafeRead: Reads a specified number of bytes from a file and handles errors.
void SafeRead (FILE *f, void *buffer, int count)
{
    size_t bytes_read = fread (buffer, 1, count, f);
	if ( bytes_read != (size_t)count)
		Error ("File read failure: expected %d bytes, read %zu. At file position %ld.", count, bytes_read, ftell(f));
}

// SafeWrite: Writes a specified number of bytes to a file and handles errors.
void SafeWrite (FILE *f, void *buffer, int count)
{
	if (fwrite (buffer, 1, count, f) != (size_t)count)
		Error ("File write failure: expected %d bytes, wrote %zu", count, fwrite(buffer, 1, count, f));
}

// SaveFile: Saves a buffer to a file.
void SaveFile (char *filename, void *buffer, int count)
{
	FILE	*f;
	f = SafeOpenWrite (filename);
	SafeWrite (f, buffer, count);
	fclose (f);
}

// --- Byte Order Conversion Functions ---
// MDL files are Little-Endian. LBM and .tri files are Big-Endian.
// These functions convert from the file's endianness to the host's endianness
// when reading, and from host to target endianness when writing.
// Assuming host system is Little-Endian (e.g., x86 processors).

// ReadLittleLong: Reads a 4-byte integer from a file and converts to host byte order.
int ReadLittleLong(FILE* f) {
    unsigned char b[4];
    SafeRead(f, b, 4);
    return (int)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

// ReadBigLong: Reads a 4-byte integer from a file and converts from Big-Endian to host byte order.
// This is used for specific fields that are Big-Endian in the MDL, like group counts.
int ReadBigLong(FILE* f) {
    unsigned char b[4];
    SafeRead(f, b, 4);
    return (int)(b[3] | (b[2] << 8) | (b[1] << 16) | (b[0] << 24));
}

// BigLongToHost: Converts a 4-byte integer from Big-Endian to host byte order (in-memory).
// This function is still useful for other Big-Endian values that might be read into a struct directly.
int BigLongToHost(int bigEndianVal) {
    unsigned char b[4];
    memcpy(b, &bigEndianVal, 4);
    return (int)(b[3] | (b[2] << 8) | (b[1] << 16) | (b[0] << 24));
}

// ReadLittleFloat: Reads a 4-byte float from a file and converts to host byte order.
float ReadLittleFloat(FILE* f) {
    union { unsigned char b[4]; float f; } val;
    SafeRead(f, val.b, 4);
    return val.f;
}

// WriteBigShortToBuffer: Writes a 2-byte short to a buffer in Big-Endian format.
void WriteBigShortToBuffer(byte* buffer, unsigned short val) {
    buffer[0] = (byte)((val >> 8) & 0xFF);
    buffer[1] = (byte)(val & 0xFF);
}

// WriteBigLongToBuffer: Writes a 4-byte integer to a buffer in Big-Endian format.
void WriteBigLongToBuffer(byte* buffer, unsigned int val) {
    buffer[0] = (byte)((val >> 24) & 0xFF);
    buffer[1] = (byte)((val >> 16) & 0xFF);
    buffer[2] = (byte)((val >> 8) & 0xFF);
    buffer[3] = (byte)(val & 0xFF);
}

// WriteBigFloat: Writes a 4-byte float to a file in Big-Endian format.
void WriteBigFloat(FILE* f, float val) {
    union { unsigned char b[4]; float f; } in;
    in.f = val;
    
    // Swap bytes for Big-Endian
    unsigned char b_swapped[4];
    b_swapped[0] = in.b[3];
    b_swapped[1] = in.b[2];
    b_swapped[2] = in.b[1];
    b_swapped[3] = in.b[0];

    SafeWrite(f, b_swapped, 4);
}


// --- Math Helper Functions (Simplified from Quake's mathlib.h) ---
void VectorCopy(vec3_t in, vec3_t out) {
    out[0] = in[0]; out[1] = in[1]; out[2] = in[2];
}

void VectorSubtract(vec3_t va, vec3_t vb, vec3_t vc) {
    vc[0] = va[0] - vb[0]; vc[1] = va[1] - vb[1]; vc[2] = va[2] - vb[2];
}

void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross) {
    cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
    cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
    cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

float VectorLength(vec3_t v) {
    return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); // Using sqrtf for float
}

void VectorNormalize(vec3_t v) {
    float length = VectorLength(v);
    if (length == 0) {
        v[0] = v[1] = v[2] = 0;
        return;
    }
    v[0] /= length; v[1] /= length; v[2] /= length;
}

float DotProduct(vec3_t v1, vec3_t v2) {
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}


// --- Structures from modelgen.h, adapted for reading ---
#define ALIAS_VERSION	6
#define IDPOLYHEADER	(('O'<<24)+('P'<<16)+('D'<<8)+'I') // Little-endian "IDPO"
#define IDTRIHEADER     123322 // Correct magic number for Alias .tri files


typedef enum {ST_SYNC=0, ST_RAND } synctype_t;
typedef enum { ALIAS_SINGLE=0, ALIAS_GROUP } aliasframetype_t;
typedef enum { ALIAS_SKIN_SINGLE=0, ALIAS_SKIN_GROUP } aliasskintype_t; // Original struct uses an anonymous union for type

// MDL Header structure
typedef struct {
	int			ident;
	int			version;
	vec3_t		scale;          // Model scale factor for vertex coordinates
	vec3_t		scale_origin;   // Origin for scaling vertex coordinates
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;       // Number of skins
	int			skinwidth;      // Width of the skin texture
	int			skinheight;     // Height of the skin texture
	int			numverts;       // Number of vertices
	int			numtris;        // Number of triangles
	int			numframes;      // Number of frames
	synctype_t	synctype;
	int			flags;
	float		size;
} mdl_header_t;

// ST vertex structure (for texture coordinates)
typedef struct {
	int		onseam;
	int		s;
	int		t;
} stvert_t;

// Triangle structure (indices to vertices)
typedef struct {
	int					facesfront;
	int					vertindex[3]; // Indices into the vertex list
} dtriangle_t;

// Raw vertex data in MDL frames (byte-packed)
typedef struct {
	byte	v[3]; // X, Y, Z coordinates packed as bytes
	byte	lightnormalindex; // Index into a predefined normal lookup table
} trivertx_t;

// Single animation frame header
// Using __attribute__((packed)) to prevent compiler padding
typedef struct __attribute__((packed)) {
	trivertx_t	bboxmin;
	trivertx_t	bboxmax;
	char		name[16]; // Frame name
} daliasframe_t;

// Animation frame group header
// Using __attribute__((packed)) to prevent compiler padding
typedef struct __attribute__((packed)) {
	int			numframes; // Placeholder value in original modelgen.c (often 0)
	trivertx_t	bboxmin;
	trivertx_t	bboxmax;
} daliasgroup_t;

// Dummy trilib.h structures for output .tri files
// .tri files typically contain the number of triangles followed by vertex data for each triangle.
typedef struct {
	vec3_t	verts[3]; // 3 vertices, each with 3 float coordinates (X, Y, Z)
} triangle_t;

// --- FUNCTION PROTOTYPES ---
// These are declared here so the compiler knows their signatures before they are defined.
void WriteLBMfile (char *filename, byte *data, int width, int height, byte *palette);
void WriteTriFile(char *filename, triangle_t *triangles, int num_triangles);


// --- LBM Structures and Functions from lbmlib.h and lbmlib.c ---
typedef unsigned char	UBYTE;
typedef short			WORD;
typedef unsigned short	UWORD;

typedef enum
{
	ms_none, ms_mask, ms_transcolor, ms_lasso
} mask_t;

typedef enum
{
	cm_none, cm_rle1
} compress_t;

typedef struct
{
	UWORD		w,h;
	WORD		x,y;
	UBYTE		nPlanes;
	UBYTE		masking;
	UBYTE		compression;
	UBYTE		pad1;
	UWORD		transparentColor;
	UBYTE		xAspect,yAspect;
	WORD		pageWidth,pageHeight;
} bmhd_t; // Bitmap Header struct for LBM


// WriteLBMfile: Writes an LBM image to disk.
// This implementation creates a simple PBM (Packed Bitmap) type LBM,
// which is uncompressed and 8-bit paletted.
void WriteLBMfile (char *filename, byte *data, int width, int height, byte *palette)
{
    byte    *lbm_buffer, *lbmptr;
    unsigned int form_len, bmhd_len, cmap_len, body_len;

    // Allocate enough memory for the LBM file.
    // LBM structure:
    // FORM chunk (8 bytes: "FORM" + 4-byte length + "PBM ")
    // BMHD chunk (8 bytes: "BMHD" + 4-byte length + sizeof(bmhd_t) = 20 bytes + optional 1-byte padding)
    // CMAP chunk (8 bytes: "CMAP" + 4-byte length + 768 bytes palette data + optional 1-byte padding)
    // BODY chunk (8 bytes: "BODY" + 4-byte length + width*height pixel data + optional 1-byte padding)
    // Total max size: 8 + (8+20+1) + (8+768+1) + (8 + 512*512 + 1) // Max skin size is typically 256x256 or 512x256, use 512x512 for safety
    lbm_buffer = lbmptr = (byte *)malloc(8 + (8 + sizeof(bmhd_t) + 1) + (8 + 768 + 1) + (8 + 512*512 + 1)); // Increased size for larger possible skins
    if (!lbm_buffer) {
        Error("Failed to allocate memory for LBM file buffer.");
    }

    // FORM chunk header
    memcpy(lbmptr, "FORM", 4); lbmptr += 4;
    byte *form_len_ptr = lbmptr; lbmptr += 4; // Placeholder for FORM length
    memcpy(lbmptr, "PBM ", 4); lbmptr += 4; // Form type (Packed Bitmap)

    // BMHD (Bitmap Header) chunk
    memcpy(lbmptr, "BMHD", 4); lbmptr += 4;
    byte *bmhd_len_ptr = lbmptr; lbmptr += 4; // Placeholder for BMHD length

    bmhd_t  basebmhd;
    memset (&basebmhd,0,sizeof(basebmhd)); // Clear struct to ensure pad1 is 0
    basebmhd.w = (UWORD)width;
    basebmhd.h = (UWORD)height;
    basebmhd.x = (WORD)0;
    basebmhd.y = (WORD)0;
    basebmhd.nPlanes = (UBYTE)8; // 8 bitplanes for 256 colors
    basebmhd.masking = (UBYTE)ms_none; // No mask
    basebmhd.compression = (UBYTE)cm_none; // No compression
    basebmhd.transparentColor = (UWORD)0; // No transparency
    basebmhd.xAspect = (UBYTE)5; // Default aspect ratio
    basebmhd.yAspect = (UBYTE)6;
    basebmhd.pageWidth = (WORD)width;
    basebmhd.pageHeight = (WORD)height;

    // Write BMHD struct values to buffer with Big-Endian conversion for WORD/UWORD
    WriteBigShortToBuffer(lbmptr, basebmhd.w); lbmptr += 2;
    WriteBigShortToBuffer(lbmptr, basebmhd.h); lbmptr += 2;
    WriteBigShortToBuffer(lbmptr, basebmhd.x); lbmptr += 2;
    WriteBigShortToBuffer(lbmptr, basebmhd.y); lbmptr += 2;
    *lbmptr++ = basebmhd.nPlanes;
    *lbmptr++ = basebmhd.masking;
    *lbmptr++ = basebmhd.compression;
    *lbmptr++ = basebmhd.pad1;
    WriteBigShortToBuffer(lbmptr, basebmhd.transparentColor); lbmptr += 2;
    *lbmptr++ = basebmhd.xAspect;
    *lbmptr++ = basebmhd.yAspect;
    WriteBigShortToBuffer(lbmptr, basebmhd.pageWidth); lbmptr += 2;
    WriteBigShortToBuffer(lbmptr, basebmhd.pageHeight); lbmptr += 2; // Write UWORD for pageHeight

    // Calculate and write BMHD chunk length (data only, not including header itself)
    bmhd_len = (lbmptr - (bmhd_len_ptr + 4));
    WriteBigLongToBuffer(bmhd_len_ptr, bmhd_len);
    if (bmhd_len & 1) *lbmptr++ = 0; // Pad if odd length

    // CMAP (Color Map) chunk
    memcpy(lbmptr, "CMAP", 4); lbmptr += 4;
    byte *cmap_len_ptr = lbmptr; lbmptr += 4; // Placeholder for CMAP length
    memcpy(lbmptr, palette, 768); lbmptr += 768; // Copy palette data (256 colors * 3 bytes RGB)
    cmap_len = (lbmptr - (cmap_len_ptr + 4));
    WriteBigLongToBuffer(cmap_len_ptr, cmap_len);
    if (cmap_len & 1) *lbmptr++ = 0; // Pad if odd length

    // BODY chunk
    memcpy(lbmptr, "BODY", 4); lbmptr += 4;
    byte *body_len_ptr = lbmptr; lbmptr += 4; // Placeholder for BODY length
    memcpy(lbmptr, data, width * height); lbmptr += (width * height); // Copy raw pixel data
    body_len = (lbmptr - (body_len_ptr + 4));
    WriteBigLongToBuffer(body_len_ptr, body_len);
    if (body_len & 1) *lbmptr++ = 0; // Pad if odd length

    // Finalize FORM length (total length of all chunks including their headers)
    form_len = (lbmptr - (form_len_ptr + 4));
    WriteBigLongToBuffer(form_len_ptr, form_len);

    // Write the entire buffered LBM data to output file
    SaveFile (filename, lbm_buffer, lbmptr-lbm_buffer);
    free (lbm_buffer);
}


// WriteTriFile: Writes a .tri file in the Alias format compatible with modelgen.
void WriteTriFile(char *filename, triangle_t *triangles, int num_triangles) {
    FILE *f = SafeOpenWrite(filename);

    // 1. Write the Magic Number (Big-Endian)
    unsigned char magic_bytes[4];
    WriteBigLongToBuffer(magic_bytes, IDTRIHEADER);
    SafeWrite(f, magic_bytes, 4);

    // 2. Write the FLOAT_START marker (Big-Endian float)
    WriteBigFloat(f, 99999.0f);

    // 3. Write a dummy object name (null-terminated string)
    char obj_name[] = "exported_object";
    SafeWrite(f, obj_name, sizeof(obj_name));

    // 4. Write the number of triangles (Big-Endian integer)
    unsigned char count_bytes[4];
    WriteBigLongToBuffer(count_bytes, num_triangles);
    SafeWrite(f, count_bytes, 4);

    // 5. Write a dummy texture name (null-terminated string)
    char tex_name[] = "default_skin";
    SafeWrite(f, tex_name, sizeof(tex_name));
    
    // 6. Write the triangle data in the full tf_triangle format.
    // The on-disk structure (aliaspoint_t) includes normals, colors, and UVs,
    // which we can zero out as we only have the vertex positions.
    for (int i = 0; i < num_triangles; i++) {
        for (int j = 0; j < 3; j++) { // Loop through 3 vertices per triangle
            // For each vertex, write the full "aliaspoint_t" structure.
            // All values must be Big-Endian floats.

            // vector n (normal) - 3 floats
            WriteBigFloat(f, 0.0f); WriteBigFloat(f, 0.0f); WriteBigFloat(f, 0.0f);
            
            // vector p (point) - 3 floats
            WriteBigFloat(f, triangles[i].verts[j][0]);
            WriteBigFloat(f, triangles[i].verts[j][1]);
            WriteBigFloat(f, triangles[i].verts[j][2]);

            // vector c (color) - 3 floats
            WriteBigFloat(f, 0.0f); WriteBigFloat(f, 0.0f); WriteBigFloat(f, 0.0f);

            // float u, v (texture coordinates) - 2 floats
            WriteBigFloat(f, 0.0f);
            WriteBigFloat(f, 0.0f);
        }
    }

    // 7. Write the FLOAT_END marker (Big-Endian float)
    WriteBigFloat(f, -99999.0f);
    
    // 8. Write the dummy object name again, as expected by the trilib reader.
    SafeWrite(f, obj_name, sizeof(obj_name));
    
    fclose(f);
}


// --- Main Program Logic ---
int main (int argc, char **argv)
{
	// Hardcoded Quake Palette (256 colors, RGB)
    // Values provided by the user in hexadecimal 0xRRGGBB format.
    byte loaded_palette[768] = {
        0x00,0x00,0x00, 0x0f,0x0f,0x0f, 0x1f,0x1f,0x1f, 0x2f,0x2f,0x2f, 0x3f,0x3f,0x3f, 0x4b,0x4b,0x4b, 0x5b,0x5b,0x5b, 0x6b,0x6b,0x6b,
        0x7b,0x7b,0x7b, 0x8b,0x8b,0x8b, 0x9b,0x9b,0x9b, 0xab,0xab,0xab, 0xbb,0xbb,0xbb, 0xcb,0xcb,0xcb, 0xdb,0xdb,0xdb, 0xeb,0xeb,0xeb,
        0x0f,0x0b,0x07, 0x17,0x0f,0x0b, 0x1f,0x17,0x0b, 0x27,0x1b,0x0f, 0x2f,0x23,0x13, 0x37,0x2b,0x17, 0x3f,0x2f,0x17, 0x4b,0x37,0x1b,
        0x53,0x3b,0x1b, 0x5b,0x43,0x1f, 0x63,0x4b,0x1f, 0x6b,0x53,0x1f, 0x73,0x57,0x1f, 0x7b,0x5f,0x23, 0x83,0x67,0x23, 0x8f,0x6f,0x23,
        0x0b,0x0b,0x0f, 0x13,0x13,0x1b, 0x1b,0x1b,0x27, 0x27,0x27,0x33, 0x2f,0x2f,0x3f, 0x37,0x37,0x4b, 0x3f,0x3f,0x57, 0x47,0x47,0x67,
        0x4f,0x4f,0x73, 0x5b,0x5b,0x7f, 0x63,0x63,0x8b, 0x6b,0x6b,0x97, 0x73,0x73,0xa3, 0x7b,0x7b,0xaf, 0x83,0x83,0xbb, 0x8b,0x8b,0xcb,
        0x00,0x00,0x00, 0x07,0x07,0x00, 0x0b,0x0b,0x00, 0x13,0x13,0x00, 0x1b,0x1b,0x00, 0x23,0x23,0x00, 0x2b,0x2b,0x07, 0x2f,0x2f,0x07,
        0x37,0x37,0x07, 0x3f,0x3f,0x07, 0x47,0x47,0x07, 0x4b,0x4b,0x0b, 0x53,0x53,0x0b, 0x5b,0x5b,0x0b, 0x63,0x63,0x0b, 0x6b,0x6b,0x0f,
        0x07,0x00,0x00, 0x0f,0x00,0x00, 0x17,0x00,0x00, 0x1f,0x00,0x00, 0x27,0x00,0x00, 0x2f,0x00,0x00, 0x37,0x00,0x00, 0x3f,0x00,0x00,
        0x47,0x00,0x00, 0x4f,0x00,0x00, 0x57,0x00,0x00, 0x5f,0x00,0x00, 0x67,0x00,0x00, 0x6f,0x00,0x00, 0x77,0x00,0x00, 0x7f,0x00,0x00,
        0x13,0x13,0x00, 0x1b,0x1b,0x00, 0x23,0x23,0x00, 0x2f,0x2b,0x00, 0x37,0x2f,0x00, 0x43,0x37,0x00, 0x4b,0x3b,0x07, 0x57,0x43,0x07,
        0x5f,0x47,0x07, 0x6b,0x4b,0x0b, 0x77,0x53,0x0f, 0x83,0x57,0x13, 0x8b,0x5b,0x13, 0x97,0x5f,0x1b, 0xa3,0x63,0x1f, 0xaf,0x67,0x23,
        0x23,0x13,0x07, 0x2f,0x17,0x0b, 0x3b,0x1f,0x0f, 0x4b,0x23,0x13, 0x57,0x2b,0x17, 0x63,0x2f,0x1f, 0x73,0x37,0x23, 0x7f,0x3b,0x2b,
        0x8f,0x43,0x33, 0x9f,0x4f,0x33, 0xaf,0x63,0x2f, 0xbf,0x77,0x2f, 0xcf,0x8f,0x2b, 0xdf,0xab,0x27, 0xef,0xcb,0x1f, 0xff,0xf3,0x1b,
        0x0b,0x07,0x00, 0x1b,0x13,0x00, 0x2b,0x23,0x0f, 0x37,0x2b,0x13, 0x47,0x33,0x1b, 0x53,0x37,0x23, 0x63,0x3f,0x2b, 0x6f,0x47,0x33,
        0x7f,0x53,0x3f, 0x8b,0x5f,0x47, 0x9b,0x6b,0x53, 0xa7,0x7b,0x5f, 0xb7,0x87,0x6b, 0xc3,0x93,0x7b, 0xd3,0xa3,0x8b, 0xe3,0xb3,0x97,
        0xab,0x8b,0xa3, 0x9f,0x7f,0x97, 0x93,0x73,0x87, 0x8b,0x67,0x7b, 0x7f,0x5b,0x6f, 0x77,0x53,0x63, 0x6b,0x4b,0x57, 0x5f,0x3f,0x4b,
        0x57,0x37,0x43, 0x4b,0x2f,0x37, 0x43,0x27,0x2f, 0x37,0x1f,0x23, 0x2b,0x17,0x1b, 0x23,0x13,0x13, 0x17,0x0b,0x0b, 0x0f,0x07,0x07,
        0xbb,0x73,0x9f, 0xaf,0x6b,0x8f, 0xa3,0x5f,0x83, 0x97,0x57,0x77, 0x8b,0x4f,0x6b, 0x7f,0x4b,0x5f, 0x73,0x43,0x53, 0x6b,0x3b,0x4b,
        0x5f,0x33,0x3f, 0x53,0x2b,0x37, 0x47,0x23,0x2b, 0x3b,0x1f,0x23, 0x2f,0x17,0x1b, 0x23,0x13,0x13, 0x17,0x0b,0x0b, 0x0f,0x07,0x07,
        0xdb,0xc3,0xbb, 0xcb,0xb3,0xa7, 0xbf,0xa3,0x9b, 0xaf,0x97,0x8b, 0xa3,0x87,0x7b, 0x97,0x7b,0x6f, 0x87,0x6f,0x5f, 0x7b,0x63,0x53,
        0x6b,0x57,0x47, 0x5f,0x4b,0x3b, 0x53,0x3f,0x33, 0x43,0x33,0x27, 0x37,0x2b,0x1f, 0x27,0x1f,0x17, 0x1b,0x13,0x0f, 0x0f,0x0b,0x07,
        0x6f,0x83,0x7b, 0x67,0x7b,0x6f, 0x5f,0x73,0x67, 0x57,0x6b,0x5f, 0x4f,0x63,0x57, 0x47,0x5b,0x4f, 0x3f,0x53,0x47, 0x37,0x4b,0x3f,
        0x2f,0x43,0x37, 0x2b,0x3b,0x2f, 0x23,0x33,0x27, 0x1f,0x2b,0x1f, 0x17,0x23,0x17, 0x0f,0x1b,0x13, 0x0b,0x13,0x0b, 0x07,0x0b,0x07,
        0xff,0xf3,0x1b, 0xef,0xdf,0x17, 0xdb,0xcb,0x13, 0xcb,0xb7,0x0f, 0xbb,0xa7,0x0f, 0xab,0x97,0x0b, 0x9b,0x83,0x07, 0x8b,0x73,0x07,
        0x7b,0x63,0x07, 0x6b,0x53,0x00, 0x5b,0x47,0x00, 0x4b,0x37,0x00, 0x3b,0x2b,0x00, 0x2b,0x1f,0x00, 0x1b,0x0f,0x00, 0x0b,0x07,0x00,
        0x00,0x00,0xff, 0x0b,0x0b,0xef, 0x13,0x13,0xdf, 0x1b,0x1b,0xcf, 0x23,0x23,0xbf, 0x2b,0x2b,0xaf, 0x2f,0x2f,0x9f, 0x2f,0x2f,0x8f,
        0x2f,0x2f,0x7f, 0x2f,0x2f,0x6f, 0x2f,0x2f,0x5f, 0x2b,0x2b,0x4f, 0x23,0x23,0x3f, 0x1b,0x1b,0x2f, 0x13,0x13,0x1f, 0x0b,0x0b,0x0f,
        0x2b,0x00,0x00, 0x3b,0x00,0x00, 0x4b,0x07,0x00, 0x5f,0x07,0x00, 0x6f,0x0f,0x00, 0x7f,0x17,0x07, 0x93,0x1f,0x07, 0xa3,0x27,0x0b,
        0xb7,0x33,0x0f, 0xc3,0x4b,0x1b, 0xcf,0x63,0x2b, 0xdb,0x7f,0x3b, 0xe3,0x97,0x4f, 0xe7,0xab,0x5f, 0xef,0xbf,0x77, 0xf7,0xd3,0x8b,
        0xa7,0x7b,0x3b, 0xb7,0x9b,0x37, 0xc7,0xc3,0x37, 0xe7,0xe3,0x57, 0x7f,0xbf,0xff, 0xab,0xe7,0xff, 0xd7,0xff,0xff, 0x67,0x00,0x00,
        0x8b,0x00,0x00, 0xb3,0x00,0x00, 0xd7,0x00,0x00, 0xff,0x00,0x00, 0xff,0xf3,0x93, 0xff,0xf7,0xc7, 0xff,0xff,0xff, 0x9f,0x5b,0x53
    };

	if (argc != 2) // Now expects only the MDL file as input
	{
		fprintf(stderr, "Usage: %s <input_mdl_file>\n", argv[0]);
		return 1;
	}

    char *input_mdl_filename = argv[1];
    char out_filename_base[1024];
    
    // Determine output base filename (e.g., "model" from "model.mdl")
    char *dot_pos = strrchr(input_mdl_filename, '.');
    if (dot_pos) {
        strncpy(out_filename_base, input_mdl_filename, dot_pos - input_mdl_filename);
        out_filename_base[dot_pos - input_mdl_filename] = '\0';
    } else {
        strcpy(out_filename_base, input_mdl_filename);
    }

	FILE *mdl_file = SafeOpenRead(input_mdl_filename);
	printf("Reading MDL file: %s\n", input_mdl_filename);

    // Print struct sizes for debugging padding issues
    printf("DEBUG: sizeof(trivertx_t): %zu\n", sizeof(trivertx_t));
    printf("DEBUG: sizeof(daliasframe_t): %zu\n", sizeof(daliasframe_t));
    printf("DEBUG: sizeof(daliasgroup_t): %zu\n", sizeof(daliasgroup_t));


	mdl_header_t header;

	// Read MDL header and convert fields from little-endian to host byte order
	header.ident = ReadLittleLong(mdl_file);
	header.version = ReadLittleLong(mdl_file);

    // Validate MDL header
	if (header.ident != IDPOLYHEADER || header.version != ALIAS_VERSION) {
		Error("Invalid MDL file: Header ID (0x%X) or Version (%d) mismatch. Expected IDPO (0x%X) and version %d.",
              header.ident, header.version, IDPOLYHEADER, ALIAS_VERSION);
	}

    // Read remaining header fields
    for (int i = 0; i < 3; i++) header.scale[i] = ReadLittleFloat(mdl_file);
    for (int i = 0; i < 3; i++) header.scale_origin[i] = ReadLittleFloat(mdl_file);
    header.boundingradius = ReadLittleFloat(mdl_file);
    for (int i = 0; i < 3; i++) header.eyeposition[i] = ReadLittleFloat(mdl_file);
    header.numskins = ReadLittleLong(mdl_file);
    header.skinwidth = ReadLittleLong(mdl_file);
    header.skinheight = ReadLittleLong(mdl_file);
    header.numverts = ReadLittleLong(mdl_file);
    header.numtris = ReadLittleLong(mdl_file);
    header.numframes = ReadLittleLong(mdl_file);
    header.synctype = (synctype_t)ReadLittleLong(mdl_file);
    header.flags = ReadLittleLong(mdl_file);
    header.size = ReadLittleFloat(mdl_file);

    printf("MDL Header Info:\n");
    printf("  Version: %d\n", header.version);
    printf("  Skins: %d (%dx%d)\n", header.numskins, header.skinwidth, header.skinheight);
    printf("  Vertices: %d\n", header.numverts);
    printf("  Triangles: %d\n", header.numtris);
    printf("  Frames: %d\n", header.numframes);
    printf("  Scale: (%.4f, %.4f, %.4f)\n", header.scale[0], header.scale[1], header.scale[2]);
    printf("  Scale Origin: (%.4f, %.4f, %.4f)\n", header.scale_origin[0], header.scale_origin[1], header.scale_origin[2]);


    // --- Extract Skins ---
    printf("\nExtracting Skins...\n");
    for (int i = 0; i < header.numskins; i++) {
        (void)ReadLittleLong(mdl_file); // Read the type (ALIAS_SINGLE or ALIAS_SKIN_GROUP), cast to void to suppress unused warning
        // For simplicity in reverse engineering, we treat skin groups as sequential skins
        // and just read their raw pixel data. The skin_type_int indicates if it was part
        // of a group, but the data structure for pixel data is the same.

        byte *skin_data = (byte *)malloc(header.skinwidth * header.skinheight);
        if (!skin_data) {
            Error("Failed to allocate memory for skin data.");
        }
        SafeRead(mdl_file, skin_data, header.skinwidth * header.skinheight);

        char skin_filename[1024]; // Increased buffer size to 1024
        sprintf(skin_filename, "%s_skin%d.lbm", out_filename_base, i);
        printf("  Saving skin %d to %s (%dx%d pixels)\n", i, skin_filename, header.skinwidth, header.skinheight);
        // Use the loaded Quake palette here
        WriteLBMfile(skin_filename, skin_data, header.skinwidth, header.skinheight, loaded_palette);
        free(skin_data);
    }

    // --- Read ST Vertices (Texture Coordinates) ---
    // These are read to advance the file pointer past this section.
    // They are not directly needed for the .tri geometry output but describe
    // how vertices map to the 2D texture.
    printf("\nReading ST Vertices...\n");
    stvert_t *st_verts = (stvert_t *)malloc(header.numverts * sizeof(stvert_t));
    if (!st_verts) {
        Error("Failed to allocate memory for ST vertices.");
    }
    for (int i = 0; i < header.numverts; i++) {
        st_verts[i].onseam = ReadLittleLong(mdl_file);
        st_verts[i].s = ReadLittleLong(mdl_file);
        st_verts[i].t = ReadLittleLong(mdl_file);
    }

    // --- Read Triangles (Vertex Indices) ---
    // These define the triangles by referencing indices into the vertex list.
    // They are crucial for reconstructing the 3D geometry of each frame.
    printf("Reading Triangle Indices...\n");
    dtriangle_t *triangles_indices = (dtriangle_t *)malloc(header.numtris * sizeof(dtriangle_t));
    if (!triangles_indices) {
        Error("Failed to allocate memory for triangle indices.");
    }
    for (int i = 0; i < header.numtris; i++) {
        triangles_indices[i].facesfront = ReadLittleLong(mdl_file);
        triangles_indices[i].vertindex[0] = ReadLittleLong(mdl_file);
        triangles_indices[i].vertindex[1] = ReadLittleLong(mdl_file);
        triangles_indices[i].vertindex[2] = ReadLittleLong(mdl_file);
    }

    // --- Extract Frames ---
    // Iterate through all frames, including single frames and frames within groups.
    printf("\nExtracting Frames...\n");
    long current_file_pos = ftell(mdl_file);
    printf("  Initial file position for frame reading: %ld\n", current_file_pos);

    for (int i = 0; i < header.numframes; ) { // 'i' is incremented inside the loop based on frame type
        int frame_type_int = ReadLittleLong(mdl_file); // Read the frame type (ALIAS_SINGLE or ALIAS_GROUP)
        printf("  Frame entry %d: Type %d (File pos after reading type: %ld)\n", i, frame_type_int, ftell(mdl_file));

        if (frame_type_int == ALIAS_SINGLE) {
            daliasframe_t frame_info;
            SafeRead(mdl_file, &frame_info, sizeof(frame_info));
            printf("    Single frame '%s' info read (File pos after reading daliasframe_t: %ld)\n", frame_info.name, ftell(mdl_file));

            trivertx_t *frame_verts_raw = (trivertx_t *)malloc(header.numverts * sizeof(trivertx_t));
            if (!frame_verts_raw) {
                Error("Failed to allocate memory for frame vertices.");
            }
            SafeRead(mdl_file, frame_verts_raw, header.numverts * sizeof(trivertx_t));
            printf("    Single frame vertex data read (File pos after reading vertices: %ld)\n", ftell(mdl_file));


            // Prepare memory for reconstructed triangle vertices (float X,Y,Z)
            triangle_t *current_frame_triangles = (triangle_t *)malloc(header.numtris * sizeof(triangle_t));
            if (!current_frame_triangles) {
                Error("Failed to allocate memory for current frame triangles.");
            }

            // Reconstruct float vertices from byte-packed data using scale and origin from MDL header
            for (int t = 0; t < header.numtris; t++) {
                for (int v_idx = 0; v_idx < 3; v_idx++) { // Loop 3 times for each vertex of the triangle
                    int vert_index = triangles_indices[t].vertindex[v_idx]; // Get the actual vertex index
                    trivertx_t raw_vert = frame_verts_raw[vert_index]; // Get the byte-packed vertex data

                    // Reverse the scaling and translation applied by modelgen.c:
                    // original_float_v = (byte_v * header.scale[k]) + header.scale_origin[k]
                    current_frame_triangles[t].verts[v_idx][0] = (float)raw_vert.v[0] * header.scale[0] + header.scale_origin[0];
                    current_frame_triangles[t].verts[v_idx][1] = (float)raw_vert.v[1] * header.scale[1] + header.scale_origin[1];
                    current_frame_triangles[t].verts[v_idx][2] = (float)raw_vert.v[2] * header.scale[2] + header.scale_origin[2];
                }
            }

            char frame_filename[1024]; // Increased buffer size to 1024
            sprintf(frame_filename, "%s_frame%d.tri", out_filename_base, i);
            printf("  Saving frame %d ('%s') to %s\n", i, frame_info.name, frame_filename);
            WriteTriFile(frame_filename, current_frame_triangles, header.numtris);

            free(frame_verts_raw);
            free(current_frame_triangles);
            i++; // Move to the next frame in the MDL file
        } else if (frame_type_int == ALIAS_GROUP) {
            daliasgroup_t group_info_raw; // Read the raw bytes into this struct
            SafeRead(mdl_file, &group_info_raw, sizeof(group_info_raw));
            printf("    Frame group struct read (File pos after reading daliasgroup_t: %ld)\n", ftell(mdl_file));

            // The actual number of frames for the group is in group_info_raw.numframes.
            // Based on flame.mdl's behavior, it appears to be Little-Endian despite modelgen.c.
            int actual_group_numframes = group_info_raw.numframes; // No BigLongToHost here

            printf("    Frame group info read. Contains %d sub-frames (from struct member conversion).\n", actual_group_numframes);
            printf("    File pos after processing group header: %ld\n", ftell(mdl_file));


            // Sanity check for numframes to prevent large erroneous reads
            if (actual_group_numframes <= 0 || actual_group_numframes > 10000) { // Arbitrary but large upper bound
                Error("Suspicious number of sub-frames (%d) detected in frame group. File might be corrupted or an unsupported format (expected 1 to 10000).", actual_group_numframes);
            }

            // Skip interval data for group frames (timing information, not geometry)
            printf("    Reading %d interval floats (File pos before intervals: %ld)\n", actual_group_numframes, ftell(mdl_file));
            for (int j = 0; j < actual_group_numframes; j++) {
                (void)ReadLittleFloat(mdl_file); // cast to void to suppress unused warning
            }
            printf("    Finished reading intervals (File pos after intervals: %ld)\n", ftell(mdl_file));

            // Process each individual frame within the group
            for (int j = 0; j < actual_group_numframes; j++) {
                daliasframe_t frame_info; // Each group frame still has a daliasframe_t header
                SafeRead(mdl_file, &frame_info, sizeof(frame_info));
                printf("      Sub-frame %d ('%s') info read (File pos after sub-frame info: %ld)\n", j, frame_info.name, ftell(mdl_file));

                trivertx_t *frame_verts_raw = (trivertx_t *)malloc(header.numverts * sizeof(trivertx_t));
                if (!frame_verts_raw) {
                    Error("Failed to allocate memory for frame vertices in group.");
                }
                SafeRead(mdl_file, frame_verts_raw, header.numverts * sizeof(trivertx_t));
                printf("      Sub-frame %d vertex data read (File pos after sub-frame vertices: %ld)\n", j, ftell(mdl_file));

                triangle_t *current_frame_triangles = (triangle_t *)malloc(header.numtris * sizeof(triangle_t));
                if (!current_frame_triangles) {
                    Error("Failed to allocate memory for current frame triangles.");
                }

                // Reconstruct float vertices for group frame
                for (int t = 0; t < header.numtris; t++) {
                    for (int v_idx = 0; v_idx < 3; v_idx++) {
                        int vert_index = triangles_indices[t].vertindex[v_idx];
                        trivertx_t raw_vert = frame_verts_raw[vert_index];

                        current_frame_triangles[t].verts[v_idx][0] = (float)raw_vert.v[0] * header.scale[0] + header.scale_origin[0];
                        current_frame_triangles[t].verts[v_idx][1] = (float)raw_vert.v[1] * header.scale[1] + header.scale_origin[1];
                        current_frame_triangles[t].verts[v_idx][2] = (float)raw_vert.v[2] * header.scale[2] + header.scale_origin[2];
                    }
                }

                char frame_filename[1024]; // Increased buffer size to 1024
                // Naming convention for group frames: base_frame<group_start_idx>_sub<sub_frame_idx>.tri
                sprintf(frame_filename, "%s_frame%d_sub%d.tri", out_filename_base, i, j);
                printf("  Saving group frame %d (sub-frame %d '%s') to %s\n", i, j, frame_info.name, frame_filename);
                WriteTriFile(frame_filename, current_frame_triangles, header.numtris);

                free(frame_verts_raw);
                free(current_frame_triangles);
            }
            i += (1 + actual_group_numframes); // Advance 'i' past group header and all frames within the group
        } else {
            Error("Unknown frame type encountered: %d. File may be corrupted or an unsupported format.", frame_type_int);
        }
    }

    // Clean up allocated memory
    free(st_verts);
    free(triangles_indices);
	fclose(mdl_file);

	printf("\nMDL reverse engineering complete. Check the output directory for .lbm and .tri files.\n");

	return 0;
}
