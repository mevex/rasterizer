#ifndef LIGHT_H
#define LIGHT_H

#include "v3.h"
#include "main.h"

class Light
{
    public:
    
    virtual f32 ComputeLightning(v3 normal, p3 hitPoint) = 0;
};

struct PointLight : public Light
{
    p3 position;
    f32 intensity;
    
    PointLight(p3 p, f32 i) : position(p), intensity(i) {}
    
    f32 ComputeLightning(v3 normal, p3 hitPoint)
    {
        f32 finalIntensity = 0;
        
        v3 light = position - hitPoint;
        f32 nDotL = Dot(normal, light);
        if(nDotL > 0)
            finalIntensity = intensity * nDotL / (normal.Length() * light.Length());
        
        return finalIntensity;
    }
};

struct AmbientLight : public Light
{
    f32 intensity;
    
    AmbientLight(f32 i) : intensity(i) {}
    
    f32 ComputeLightning(v3 normal, p3 hitPoint)
    {
        return intensity;
    }
};

#endif //LIGHT_H
