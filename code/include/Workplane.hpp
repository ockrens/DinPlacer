#ifndef WORKPLANE_HPP
#define WORKPLANE_HPP
#include <vector>
#include <string>
#include "raylib.h"

// 1. De Wiskunde-motor voor 4x4 matrices
struct Matrix4 {
    float m[16];
    Matrix4(); // Maakt een 'Identity' matrix (1.0 op de diagonaal)
    static Matrix4 Multiply(Matrix4 a, Matrix4 b);
};

// 2. De Basis voor elk 3D object met hiërarchie
class BaseObject {
public:
    Vector3 position = { 0, 0, 0 };
    Vector3 rotation = { 0, 0, 0 }; // x=pitch, y=yaw, z=roll
    Color color = WHITE;
    BaseObject* parent = nullptr; // Verwijzing naar het ouder-object

    BaseObject(Vector3 pos = {0,0,0}, Vector3 rot = {0,0,0}, Color col = WHITE);
    
    // Berekent de matrix van het object zelf
    Matrix4 GetLocalMatrix() const;
    
    // Berekent de matrix inclusief die van alle ouders (Ouder * Kind)
    Matrix4 GetWorldMatrix() const;
    
    // Tekent het object met de juiste transformatie
    void Draw(Mesh mesh, Material mat) const;
};

// Hulpmiddel om onze Matrix4 om te zetten naar Raylib's Matrix formaat
Matrix ToRaylib(Matrix4 m);

#endif