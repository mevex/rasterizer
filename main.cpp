#include "main.h"
#include <chrono>

// NOTE(mevex): This routine works only if the mesh has been triangulated
bool LoadObj(Mesh *mesh, const char* filename, const char* basepath = NULL, bool triangulate = true)
{
    printf("Loading %s\n", filename);
    
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    
    std::string warn;
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);
    
    if (!warn.empty()) {
        printf("WARN: %s\n", warn.c_str());
    }
    if (!err.empty()) {
        printf("ERR: %s\n", err.c_str());
    }
    if (!ret) {
        printf("Failed to load/parse .obj.\n");
        return false;
    }
    
    // NOTE(mevex): Get the color indices
    vector<Color> colors;
    for(tinyobj::material_t m : materials)
    {
        Color c(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
        colors.push_back(c);
    }
    
    // NOTE(mevex): Fill the vertices array
    int vertexCount = (int)attrib.vertices.size() / 3;
    for(int i = 0; i < vertexCount; i++)
    {
        p3 vertex(attrib.vertices[3*i], attrib.vertices[3*i + 1], attrib.vertices[3*i + 2]);
        mesh->Add(vertex);
    }
    
    // NOTE(mevex): Fill the triangles array
    int facesCount = (int)shapes[0].mesh.num_face_vertices.size();
    tinyobj::index_t *indexPtr = &shapes[0].mesh.indices[0];
    int *materialIndex = &shapes[0].mesh.material_ids[0];
    
    for(int i = 0; i < facesCount; i++)
    {
        Triangle t = {};
        
        t.a = indexPtr->vertex_index;
        indexPtr++;
        t.b = indexPtr->vertex_index;
        indexPtr++;
        t.c = indexPtr->vertex_index;
        indexPtr++;
        
        t.color = colors[materialIndex[i]];
        
        mesh->Add(t);
    }
    
    mesh->CalculateBoundingSphere();
    
    return true;
}

int ClipSphere(Sphere s, Plane clippingPlane)
{
    f32 distance = Dot(s.center, clippingPlane.normal) + clippingPlane.d;
    
    if(Abs(distance) < s.r)
        return UNKNOWN;
    else if(distance < -s.r)
        return DISCARDED;
    return ACCEPTED;
}

vector<Triangle> ClipTriangles(vector<Triangle> &tris, vector<p3> &vertices, Plane clippingPlane)
{
    size_t trisCount = tris.size();
    vector<Triangle> resultingTris;
    vector<p3> resultingVerts = vertices;
    int vertsIndex = 0;
    
    int accepted = 0;
    int modified = 0;
    int discarded = 0;
    for(int i = 0; i < trisCount; ++i)
    {
        Triangle tri = tris[i];
        p3 a = vertices[tri.a];
        p3 b = vertices[tri.b];
        p3 c = vertices[tri.c];
        
        f32 dA = Dot(a, clippingPlane.normal) + clippingPlane.d;
        f32 dB = Dot(b, clippingPlane.normal) + clippingPlane.d;
        f32 dC = Dot(c, clippingPlane.normal) + clippingPlane.d;
        
        int positives = CountPositives(3, dA, dB, dC);
        if(positives == 3)
        {
            ++accepted;
            resultingTris.push_back(tri);
        }
        else if(positives == 1)
        {
            ++modified;
            
            int newAIndex = tri.a;
            if(dB > ZERO)
            {
                Swap(a, b);
                newAIndex = tri.b;
            }
            else if(dC > ZERO)
            {
                Swap(a, c);
                newAIndex = tri.c;
            }
            
            f32 tNumerator = (-clippingPlane.d - Dot(clippingPlane.normal, a));
            f32 tAB = tNumerator / Dot(clippingPlane.normal, b-a);
            f32 tAC = tNumerator / Dot(clippingPlane.normal, c-a);
            
            p3 newB = Lerp(a, b, tAB);
            p3 newC = Lerp(a, c, tAC);
            
            resultingVerts.push_back(newB);
            int newBIndex = int(resultingVerts.size() - 1);
            resultingVerts.push_back(newC);
            int newCIndex = newBIndex + 1;
            
            Triangle t = {newAIndex, newBIndex, newCIndex, tri.color};
            resultingTris.push_back(t);
        }
        else if(positives == 2)
        {
            ++modified;
            
            int aIndex = tri.a;
            int bIndex = tri.b;
            if(dA < -ZERO)
            {
                Swap(a, c);
                aIndex = tri.c;
            }
            else if(dB < -ZERO)
            {
                Swap(b, c);
                bIndex = tri.c;
            }
            
            dA = Dot(a, clippingPlane.normal) + clippingPlane.d;
            dB = Dot(b, clippingPlane.normal) + clippingPlane.d;
            dC = Dot(c, clippingPlane.normal) + clippingPlane.d;
            
            f32 tNumerator = (-clippingPlane.d - Dot(clippingPlane.normal, c));
            f32 tCA = tNumerator / Dot(clippingPlane.normal, a-c);
            f32 tCB = tNumerator / Dot(clippingPlane.normal, b-c);
            
            p3 aPrime = Lerp(c, a, tCA);
            p3 bPrime = Lerp(c, b, tCB);
            
            resultingVerts.push_back(aPrime);
            int aPrimeIndex = int(resultingVerts.size() - 1);
            resultingVerts.push_back(bPrime);
            int bPrimeIndex = aPrimeIndex + 1;
            
            Triangle t1 = {aIndex, bIndex, aPrimeIndex, tri.color};
            Triangle t2 = {aPrimeIndex, bIndex, bPrimeIndex, tri.color};
            resultingTris.push_back(t1);
            resultingTris.push_back(t2);
        }
        else
        {
            ++discarded;
        }
        
    }
    printf("Accepted:%i Modified:%i Discarded:%i\n", accepted, modified, discarded);
    vertices = resultingVerts;
    return resultingTris;
}

vector<v3> CalculateNormals(vector<Triangle> &tris, vector<p3> &vertices)
{
    size_t trisCount = tris.size();
    vector<v3> normals(trisCount);
    for(int i = 0; i < trisCount; i++)
    {
        v3 vBA = vertices[tris[i].b] - vertices[tris[i].a];
        v3 vCA = vertices[tris[i].c] - vertices[tris[i].a];
        
        v3 n = Cross(vBA, vCA);
        normals[i] = n;
    }
    
    return normals;
}

vector<Triangle> CullBackFace(vector<Triangle> &tris, vector<p3> &vertices, vector<v3> &normals)
{
    size_t trisCount = tris.size();
    vector<Triangle> resultingTris;
    vector<v3> remainingNormals;
    
    for(int i = 0; i < trisCount; ++i)
    {
        v3 v = vertices[tris[i].a];
        v3 n = normals[i];
        
        if(Dot(n, v) < 0)
        {
            resultingTris.push_back(tris[i]);
            remainingNormals.push_back(normals[i]);
        }
    }
    
    normals = remainingNormals;
    return resultingTris;
}

void Render(vector<Instance> &instances, vector<Light*> lights, Canvas &canv, Camera &cam)
{
    canv.FillEntireCanvas(Color(0.2f,0.5f,0.7f));
    
    for(auto inst: instances)
    {
        // NOTE(mevex): Apply the inst transform in the following order:
        //              scale -> rotation -> position;
        m4x4 instTransform = Translation(inst.position) * ((ZRotation(inst.rotations[Z]) * YRotation(inst.rotations[Y]) * XRotation(inst.rotations[X])) * Scale(inst.scale));
        m4x4 absoluteTransform = cam.transform * instTransform;
        
        size_t verticesCount = inst.mesh->vertices.size();
        // NOTE(mevex): Apply the absolute transfom
        vector<p3> transformedVertices(verticesCount);
        for(int i = 0; i < verticesCount; ++i)
        {
            p4 homogP = HomogeneousPoint(inst.mesh->vertices[i]);
            transformedVertices[i] = NotHomogeneous(absoluteTransform*homogP);
        }
        Sphere testSphere = inst.mesh->boundingSphere;
        testSphere.center = NotHomogeneous(absoluteTransform * HomogeneousPoint(testSphere.center));
        
        // NOTE(mevex): Clipping
        int clipping = ACCEPTED;
        vector<Plane> unknownPlanes;
        for(auto p : cam.clippingPlanes)
        {
            int result = ClipSphere(testSphere, p);
            
            if(result == DISCARDED)
            {
                clipping = DISCARDED;
                break;
            }
            else if(result == UNKNOWN)
            {
                clipping = UNKNOWN;
                unknownPlanes.push_back(p);
            }
        }
        if(clipping == DISCARDED)
            continue;
        
        vector<v3> normals = CalculateNormals(inst.mesh->triangles, transformedVertices);
        vector<Triangle> newTriangles = CullBackFace(inst.mesh->triangles, transformedVertices, normals);
        
        for(auto p : unknownPlanes)
            newTriangles = ClipTriangles(newTriangles, transformedVertices, p);
        
        // NOTE(mevex): We need to do this in case we add new vertices
        verticesCount = transformedVertices.size();
        // NOTE(mevex): Project each vertex
        vector<p3> projectedVertices(verticesCount);
        for(int i = 0; i < verticesCount; ++i)
            projectedVertices[i] = cam.Project(transformedVertices[i]);
        
        // NOTE(mevex):  Compute lightning and draw each triangle
        int trianglesIndex = 0;
        for(auto t : newTriangles)
        {
            f32 intensityA = 0.0f;
            f32 intensityB = 0.0f;
            f32 intensityC = 0.0f;
            
            for(auto l : lights)
            {
                v3 n = normals[trianglesIndex];
                p3 vertA = transformedVertices[t.a];
                p3 vertB = transformedVertices[t.b];
                p3 vertC = transformedVertices[t.c];
                
                intensityA += l->ComputeLightning(n, vertA);
                intensityB += l->ComputeLightning(n, vertB);
                intensityC += l->ComputeLightning(n, vertC);
                
                Assert(intensityA >= 0.0f && intensityA <= 1.0f);
                Assert(intensityB >= 0.0f && intensityB <= 1.0f);
                Assert(intensityC >= 0.0f && intensityC <= 1.0f);
            }
            
            DrawFilledTriangle(projectedVertices[t.a], projectedVertices[t.b], projectedVertices[t.c], intensityA, intensityB, intensityC, t.color, canv);
            ++trianglesIndex;
        }
        
        printf("Render\n");
    }
}

int main()
{
    Canvas canvas(1280, 720, 4);
    Camera cam(p3(3,1,5), p3(0,0,-5), v3(0,1,0), 60.0f, canvas);
    
    Mesh fox;
    Mesh sphere;
    
    Instance instance;
    instance.mesh = &fox;
    instance.scale = 1.f;
    instance.rotations[Y] = 90;
    //instance.rotations[X] = 20;
    instance.position = p3(0,0,-5);
    
    LoadObj(&fox, "../models/fox.obj", "../models/");
    LoadObj(&sphere, "../models/sphere.obj", "../models/");
    vector<Instance> scene;
    for(int i = 0; i < 10; ++i)
    {
        Instance inst;
        
        if(i % 2)
        {
            inst.mesh = &fox;
            
            inst.scale = 3*(rand()/RAND_MAX) + 0.5f;
            //inst.scale = (rand()%2) + 0.5f;
            
            if(i  != 9)
            {
                for(int j = 0; j < (int)(rand()%3); ++j)
                    inst.rotations[(int)(rand()%3)] = (f32)(rand() % 180) - 90;
            }
            else
            {
                for(int j = 0; j < (int)(rand()%3); ++j)
                {
                    rand();
                    rand();
                }
                inst.rotations[Y] = 90;
            }
        }
        else
        {
            inst.mesh = &sphere;
            
            inst.scale = 3;
            rand();
            
            for(int j = 0; j < (int)(rand()%3); ++j)
                inst.rotations[(int)(rand()%3)] = (f32)(rand() % 180) - 90;
            
            //p3((f32)((rand() % 14) - 7), (f32)((rand() % 10) - 5), (f32)((-(rand() % 15)) - 5));
        }
        
        inst.position = p3(f32(-15 + (6*(i%5))), f32(-3 + (6*(i%2))), -10);
        scene.push_back(inst);
    }
    
    PointLight l1(p3(10,20,50), 0.8f);
    AmbientLight l2(0.20f);
    vector<Light*> lights;
    lights.push_back(&l1);
    lights.push_back(&l2);
    
    // NOTE(mevex): Timer start
    printf("Rendering starts\n");
    auto timerStart = std::chrono::high_resolution_clock::now();
    
#if 1
    Render(scene, lights, canvas, cam);
#else
    vector<Instance> test;
    test.push_back(instance);
    Render(test, lights, canvas, cam);
#endif
    
    // NOTE(mevex): Time finish
    auto timerFinish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timerFinish - timerStart);
    
    // NOTE(mevex): Pixel order: AABBGGRR
    auto res = stbi_write_png("../renders/render.png", canvas.width, canvas.height, canvas.bytesPerPixel, canvas.memory, 0);
    
    printf("\nRendering time: %ims", (int)(duration.count()));
    getchar();
    return 0;
}