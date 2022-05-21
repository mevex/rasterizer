#ifndef CANVAS_H
#define CANVAS_H

#include "v3.h"

#include <vector>
using std::vector;


vector<int> Interpolate(int i0, int d0, int i1, int d1)
{
    vector<int> results;
    
    if(i0 == i1)
    {
        results.push_back(d0);
        return results;
    }
    
    f32 a = f32(d1-d0) / f32(i1-i0);
    f32 d = f32(d0);
    for(int i = i0; i <= i1; i++)
    {
        results.push_back(int(d));
        d += a;
    }
    
    return results;
}


vector<f32> Interpolate(int i0, f32 d0, int i1, f32 d1)
{
    vector<f32> results;
    
    if(i0 == i1)
    {
        results.push_back(d0);
        return results;
    }
    
    f32 a = (d1-d0) / (i1-i0);
    f32 d = d0;
    for(int i = i0; i <= i1; i++)
    {
        results.push_back(d);
        d += a;
    }
    
    return results;
}

void DrawLine(int x0, int y0, int x1, int y1, Color c, Canvas &canvas)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    
    if(Abs(dy) > Abs(dx))
    {
        // NOTE(mevex): The line is vertical-ish
        if(dy < 0)
        {
            Swap(x0, x1);
            Swap(y0, y1);
        }
        
        vector<int> xs = Interpolate(y0, x0, y1, x1);
        for(int y = y0; y <= y1; y++)
            canvas.SetPixel(int(xs[y-y0]), y, c);
    }
    else
    {
        // NOTE(mevex): The line is horizontal-ish
        if(dx < 0)
        {
            Swap(x0, x1);
            Swap(y0, y1);
        }
        
        vector<int> ys = Interpolate(x0, y0, x1, y1);
        for(int x = x0; x <= x1; x++)
            canvas.SetPixel(x, int(ys[x-x0]), c);
    }
}

inline void DrawLine(p3 p0, p3 p1, Color c, Canvas &canvas)
{
    int x0 = int(p0.x + 0.5f);
    int y0 = int(p0.y + 0.5f);
    int x1 = int(p1.x + 0.5f);
    int y1 = int(p1.y + 0.5f);
    
    DrawLine(x0, y0, x1, y1, c, canvas);
}

void DrawFilledTriangle(int x0, int y0, f32 z0, int x1, int y1, f32 z1, int x2, int y2, f32 z2, f32 i0, f32 i1, f32 i2, Color c, Canvas &canvas)
{
    // NOTE(mevex): compute the x coordinates of the edges
    vector<int> x01 = Interpolate(y0, x0, y1, x1);
    vector<f32> z01 = Interpolate(y0, z0, y1, z1);
    vector<f32> i01 = Interpolate(y0, i0, y1, i1);
    
    vector<int> x12 = Interpolate(y1, x1, y2, x2);
    vector<f32> z12 = Interpolate(y1, z1, y2, z2);
    vector<f32> i12 = Interpolate(y1, i1, y2, i2);
    
    vector<int> x02 = Interpolate(y0, x0, y2, x2);
    vector<f32> z02 = Interpolate(y0, z0, y2, z2);
    vector<f32> i02 = Interpolate(y0, i0, y2, i2);
    
    // NOTE(mevex): Concatenate the short sides
    x01.pop_back();
    vector<int> x012(x01);
    x012.insert(x012.end(), x12.begin(), x12.end());
    
    z01.pop_back();
    vector<f32> z012(z01);
    z012.insert(z012.end(), z12.begin(), z12.end());
    
    i01.pop_back();
    vector<f32> i012(i01);
    i012.insert(i012.end(), i12.begin(), i12.end());
    
    // NOTE(mevex): Determine which is left and which is right
    vector<int> *xLeft;
    vector<int> *xRight;
    vector<f32> *zLeft;
    vector<f32> *zRight;
    vector<f32> *iLeft;
    vector<f32> *iRight;
    
    int mid = int(x02.size() / 2);
    if(x012[mid] < x02[mid])
    {
        xLeft = &x012;
        xRight = &x02;
        zLeft = &z012;
        zRight = &z02;
        iLeft = &i012;
        iRight = &i02;
    }
    else
    {
        xLeft = &x02;
        xRight = &x012;
        zLeft = &z02;
        zRight = &z012;
        iLeft = &i02;
        iRight = &i012;
    }
    
    // NOTE(mevex): Draw the horizontal segments
    for(int y = y0; y <= y2; y++)
    {
        int xL = xLeft->at(y - y0);
        int xR = xRight->at(y - y0);
        f32 iL = iLeft->at(y - y0);
        f32 iR = iRight->at(y - y0);
        f32 zL = zLeft->at(y - y0);
        f32 zR = zRight->at(y - y0);
        f32 *zBufferLocation = canvas.zBuffer + y*canvas.width + xL;
        
        vector<f32> iSegment = Interpolate(xL, iL, xR, iR);
        vector<f32> zSegment = Interpolate(xL, zL, xR, zR);
        for(int x = xL; x <= xR; x++)
        {
            //Color shade = c * hSegment[x - xL];
            f32 z = zSegment[x - xL];
            f32 i = iSegment[x - xL];
            if(z < *zBufferLocation)
            {
                canvas.SetPixel(x, y, c*i);
                *zBufferLocation = z;
            }
            ++zBufferLocation;
        }
    }
}

inline void DrawFilledTriangle(p3 p0, p3 p1, p3 p2, f32 i0, f32 i1, f32 i2, Color c, Canvas &canvas)
{
    // NOTE(mevex): Sort the points so that y0 <= y1 <= y2
    if(p0.y > p1.y)
    {
        Swap(p0, p1);
        Swap(i0, i1);
    }
    if(p0.y > p2.y)
    {
        Swap(p0, p2);
        Swap(i0, i2);
    }
    if(p1.y > p2.y) 
    {
        Swap(p1, p2);
        Swap(i1, i2);
    }
    
    int x0 = int(p0.x + 0.5f);
    int x1 = int(p1.x + 0.5f);
    int x2 = int(p2.x + 0.5f);
    int y0 = int(p0.y + 0.5f);
    int y1 = int(p1.y + 0.5f);
    int y2 = int(p2.y + 0.5f);
    f32 z1 = p1.z;
    f32 z0 = p0.z;
    f32 z2 = p2.z;
    
    DrawFilledTriangle(x0, y0, z0, x1, y1, z1, x2, y2, z2, i0, i1, i2, c, canvas);
}

inline void DrawWireframeTriangle(p3 p0, p3 p1, p3 p2, Color c, Canvas &canvas)
{
    DrawLine(p0, p1, c, canvas);
    DrawLine(p1, p2, c, canvas);
    DrawLine(p2, p0, c, canvas);
}

#endif //CANVAS_H
