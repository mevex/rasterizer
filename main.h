#ifndef MAIN_H
#define MAIN_H

// NOTE: This is a collection of utility macros and typedefs

#include <cstdint>
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef u8 byte;

#define local_persist static
#define global_variable static
#define shared_function static

#if 0
#define Assert(expression) if(!(expression)) *(int *)0 = 0
#else
#define Assert(expression)
#endif

#define Min(a, b) ((a) < (b)) ? (a) : (b)
#define Max(a, b) ((a) > (b)) ? (a) : (b)
#define Clamp(n, min, max) Max(Min((n), (max)), (min))
#define Abs(a) ((a) < 0 ? -(a) : (a))
#define Sign(a) ((a) <= 0 ? -1 : 1)
#define Lerp(a, b, t) (1.0f-(t))*(a) + (t)*(b)

#include <limits>
#undef INFINITY
#define INFINITY std::numeric_limits<float>::infinity()
#define PI 3.1415926535897932385f
#define ZERO 1e-6f

// NOTE(mevex): Utility Functions

inline f32 DegreesToRadians(f32 degrees)
{
    f32 result = degrees * PI / 180.0f;
    return result;
}

#include <time.h>
inline f32 RandomFloat()
{
    // Returns a random real number in [0,1)
    f32 result = rand() / (RAND_MAX + 1.0f);
    return result;
}

inline f32 RandomFloat(f32 min, f32 max)
{
    // Returns a random real number in [min,max)
    f32 result = min + (max-min)*RandomFloat();
    return result;
}

template <typename T>
void Swap(T &a, T &b)
{
    T c = a;
    a = b;
    b = c;
}

#include <stdarg.h>
int CountPositives(int n, ...)
{
    int result = 0;
    
    va_list args;
    va_start(args, n);
    
    for(int i = 0; i < n; ++i)
    {
        f64 value = va_arg(args, f64);
        if(value > -ZERO)
            ++result;
    }
    va_end(args);
    
    return result;
}

#include <vector>
using std::vector;

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"

#include "v3.h"
#include "v4.h"
#include "mesh.h"
#include "light.h"

class Canvas
{
    public:
    
    i32 width;
    i32 height;
    i32 bytesPerPixel;
    f32 ratio;
    void *memory;
    f32 *zBuffer;
    
    Canvas(i32 w, i32 h, i32 bpp)
    {
        width = w;
        height = h;
        bytesPerPixel = bpp;
        ratio = (f32)w / (f32)h;
        memory = malloc(bpp * w * h);
        
        zBuffer = (f32*)malloc(sizeof(f32) * w * h);
        std::fill(zBuffer, zBuffer + (width*height), INFINITY);
    }
    
    void SetPixel(i32 x, i32 y, f32 red, f32 green, f32 blue)
    {
        if(x < 0 || x >= width ||
           y < 0 || y >= height)
            return;
        
        red = Min(red, 255);
        green = Min(green, 255);
        blue = Min(blue, 255);
        
        u8 r,g,b;
        
        r = (u8)(255.99f * sqrt(red));
        g = (u8)(255.99f * sqrt(green));
        b = (u8)(255.99f * sqrt(blue));
        
        i32* pixel = (i32*)memory;
        pixel += (height-y-1)*width + x;
        *pixel = 255<<24 | b << 16 | g << 8 | r;
    }
    
    void SetPixel(i32 x, i32 y, Color c)
    {
        SetPixel(x, y, c.r, c.g, c.b);
    }
    
    void SetPixel(i32 x, i32 y, Color c, int spp)
    {
        f32 scale = 1.0f / spp;
        c *= scale;
        SetPixel(x, y, c.r, c.g, c.b);
    }
    
    void FillEntireCanvas(Color c = {0,0,0})
    {
        u8 r = (u8)(255.99f * sqrt(c.r));
        u8 g = (u8)(255.99f * sqrt(c.g));
        u8 b = (u8)(255.99f * sqrt(c.b));
        
        u32 *begin = (u32 *)memory;
        u32 *end = begin + (width*height);
        u32 value = (u32)(255<<24 | b << 16 | g << 8 | r);
        std::fill(begin, end, value);
    }
    
    ~Canvas()
    {
        // NOTE(mevex): no need to free the memory since the canvas will be destroyed only when the program closes
    }
};

#include "draw.h"

enum clipping
{
    NEAR,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    
    CLIPPING_PLANES_COUNT
};

enum status
{
    DISCARDED,
    ACCEPTED,
    UNKNOWN
};

class Camera
{
    public:
    
    f32 vpHeight;
    f32 vpWidth;
    
    Canvas &canvas;
    
    m4x4 transform;
    
    Plane clippingPlanes[5];
    
    Camera(p3 pos, v3 lookAt, v3 viewUp, f32 verticalFOV, Canvas &c) : canvas(c)
    {
        //          to       from
        v3 w = Unit(lookAt - pos); // -z
        v3 u = Unit(Cross(w, viewUp)); // x
        v3 v = Cross(u, w); // y
        
        m4x4 rotation = Identity();
        rotation.x = HomogeneousVector(v3(u.x, v.x, w.x));
        rotation.y = HomogeneousVector(v3(u.y, v.y, w.y));
        rotation.z = HomogeneousVector(v3(-u.z, -v.z, -w.z));
        
        m4x4 position = Translation(-pos);
        
        transform = position * rotation;
        
        f32 theta = DegreesToRadians(verticalFOV);
        f32 h = tan(theta/2);
        vpHeight = 2.0f * h;
        vpWidth = vpHeight * canvas.ratio;
        
        clippingPlanes[NEAR].normal = v3(0,0,-1);
        clippingPlanes[NEAR].d = 1;
        
        p3 topLeft = v3(-vpWidth*0.5f, +vpHeight*0.5f, -1);
        p3 topRight = v3(+vpWidth*0.5f, +vpHeight*0.5f, -1);
        p3 bottomLeft = v3(-vpWidth*0.5f, -vpHeight*0.5f, -1);
        p3 bottomRight = v3(+vpWidth*0.5f, -vpHeight*0.5f, -1);
        
        clippingPlanes[LEFT].normal = Unit(Cross(bottomLeft, topLeft));
        clippingPlanes[LEFT].d = 0;
        clippingPlanes[RIGHT].normal = Unit(Cross(topRight, bottomRight));
        clippingPlanes[RIGHT].d = 0;
        clippingPlanes[TOP].normal = Unit(Cross(topLeft, topRight));
        clippingPlanes[TOP].d = 0;
        clippingPlanes[BOTTOM].normal = Unit(Cross(bottomRight, bottomLeft));
        clippingPlanes[BOTTOM].d = 0;
    }
    
    p3 Project(p3 point)
    {
        f32 px = point.x / -point.z;
        f32 py = point.y / -point.z;
        
        f32 u = px / vpWidth + 0.5f;
        f32 v = py / vpHeight + 0.5f;
        
        f32 cx = u * canvas.width;
        f32 cy = v * canvas.height;
        
        return p3(cx, cy, -point.z);
    }
};

#endif //MAIN_H
