#ifndef V4_H
#define V4_H

union v4
{
    f32 e[4];
    struct
    {
        f32 x, y, z, w;
    };
};

typedef v4 p4;

inline v4 operator +(v4& v, v4& w)
{
    return {v.e[0] + w.e[0], v.e[1] + w.e[1], v.e[2] + w.e[2], v.e[3] + w.e[3]};
}

inline v4 operator -(v4& v, v4& w)
{
    return {v.e[0] - w.e[0], v.e[1] - w.e[1], v.e[2] - w.e[2], v.e[3] - w.e[3]};
}

inline v4 operator -(v4& v)
{
    return {-v.e[0], -v.e[1], -v.e[2], -v.e[3]};
}

inline v4 HomogeneousPoint(p3 v)
{
    return {v.e[0], v.e[1], v.e[2], 1};
}

inline v4 HomogeneousVector(v3 v)
{
    return {v.e[0], v.e[1], v.e[2], 0};
}

inline v3 NotHomogeneous(v4 v)
{
    return {v.e[0], v.e[1], v.e[2]};
}

union m4x4
{
    // NOTE(mevex): rappresentation [ROWS] [COLUMNS]
    f32 e[4][4];
    struct
    {
        v4 x, y, z, w;
    };
};

inline m4x4 operator *(m4x4 &a, m4x4 &b)
{
    m4x4 result = {};
    
    for(int r = 0; r < 4; r++)
    {
        for(int c = 0; c < 4; c++)
        {
            for(int i = 0; i < 4; i++)
            {
                result.e[r][c] += a.e[r][i] * b.e[i][c];
            }
        }
    }
    
    return result;
}

inline v4 operator *(m4x4 &a, v4 &b)
{
    v4 result = {};
    
    for(int r = 0; r < 4; r++)
    {
        for(int c = 0; c < 4; c++)
        {
            result.e[r] += a.e[r][c] * b.e[c];
        }
    }
    
    return result;
}

inline m4x4 operator *(v4 &a, v4 &b)
{
    m4x4 result = {};
    
    for(int r = 0; r < 4; r++)
    {
        for(int c = 0; c < 4; c++)
        {
            result.e[r][c] += a.e[r] * b.e[c];
        }
    }
    
    return result;
}

inline m4x4 Identity()
{
    return
    {{
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        }};
}

inline m4x4 XRotation(f32 c, f32 s)
{
    if(c != 0 || s != 0)
        return
    {{
            {1, 0, 0, 0},
            {0, c,-s, 0},
            {0, s, c, 0},
            {0, 0, 0, 1},
        }};
    else
        return Identity();
}

inline m4x4 XRotation(f32 angle)
{
    f32 a = DegreesToRadians(angle);
    f32 c = cos(a);
    f32 s = sin(a);
    
    return XRotation(c, s);
}

inline m4x4 YRotation(f32 c, f32 s)
{
    if(c != 0 || s != 0)
        return
    {{
            { c, 0, s, 0},
            { 0, 1, 0, 0},
            {-s, 0, c, 0},
            { 0, 0, 0, 1},
        }};
    else
        return Identity();
}

inline m4x4 YRotation(f32 angle)
{
    f32 a = DegreesToRadians(angle);
    f32 c = cos(a);
    f32 s = sin(a);
    
    return YRotation(c, s);
}

inline m4x4 ZRotation(f32 c, f32 s)
{
    if(c != 0 || s != 0)
        return
    {{
            {c,-s, 0, 0},
            {s, c, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        }};
    else
        return Identity();
}

inline m4x4 ZRotation(f32 angle)
{
    f32 a = DegreesToRadians(angle);
    f32 c = cos(a);
    f32 s = sin(a);
    
    return ZRotation(c, s);
}

inline m4x4 Translation(p3 p)
{
    return
    {{
            { 1, 0, 0, p.x},
            { 0, 1, 0, p.y},
            { 0, 0, 1, p.z},
            { 0, 0, 0, 1},
        }};
}

inline m4x4 Scale(f32 s)
{
    return
    {{
            {s, 0, 0, 0},
            {0, s, 0, 0},
            {0, 0, s, 0},
            {0, 0, 0, 1},
        }};
}

#endif //V4_H
