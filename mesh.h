#ifndef MESH_H
#define MESH_H

struct Sphere
{
    p3 center;
    f32 r;
};

struct Plane
{
    // NOTE(mevex): Plane represented by its normal and SIGNED distance from the origin
    v3 normal;
    f32 d;
};

struct Triangle
{
    // NOTE(mevex): a, b and c are indeces of the vertices array;
    //bool discarded;
    int a, b, c;
    Color color;
};

struct Mesh
{
    vector<p3> vertices;
    vector<Triangle> triangles;
    Sphere boundingSphere;
    
    inline void Add(p3 p)
    {
        vertices.push_back(p);
    }
    
    inline void Add(Triangle t)
    {
        triangles.push_back(t);
    }
    
    void CalculateBoundingSphere()
    {
        size_t vCount = vertices.size();
        f32 weight = 1.0f / vCount;
        
        p3 avgP;
        for(int i = 0; i < vCount; ++i)
        {
            avgP += weight * vertices[i];
        }
        
        f32 maxDistance = 0;
        for(int i = 0; i < vCount; ++i)
        {
            f32 distance = Abs((vertices[i] - avgP).LengthSquared());
            if(maxDistance < distance)
                maxDistance = distance;
        }
        
        boundingSphere.center = avgP;
        boundingSphere.r = sqrt(maxDistance);
    }
};

enum axis
{
    X,
    Y,
    Z
};

struct Instance
{
    Mesh *mesh;
    
    // NOTE(mevex): This is the transform of the instance
    f32 scale;
    f32 rotations[3]; // degrees around X - Y - Z
    p3 position;
    
    //bool discarded = false;
    
    Instance()
    {
        scale = 1.0f;
        rotations[X] = 0;
        rotations[Y] = 0;
        rotations[Z] = 0;
        position = p3(0,0,0);
    }
};

#endif //MESH_H
