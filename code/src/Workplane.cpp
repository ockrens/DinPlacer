#include "../include/Workplane.hpp"
#include <cmath>

// Constructor: vult de matrix als een Identity matrix
Matrix4::Matrix4() {
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = 1.0f; m[5] = 1.0f; m[10] = 1.0f; m[15] = 1.0f;
}

// Standaard matrixvermenigvuldiging (rijen x kolommen)
Matrix4 Matrix4::Multiply(Matrix4 a, Matrix4 b) {
    Matrix4 res;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            res.m[r + c * 4] = 
                a.m[r + 0*4] * b.m[0 + c*4] +
                a.m[r + 1*4] * b.m[1 + c*4] +
                a.m[r + 2*4] * b.m[2 + c*4] +
                a.m[r + 3*4] * b.m[3 + c*4];
        }
    }
    return res;
}

Matrix4 BaseObject::GetLocalMatrix() const {
    // 1. Translatie (verplaatsing)
    Matrix4 tMat;
    tMat.m[12] = position.x; tMat.m[13] = position.y; tMat.m[14] = position.z;

    // 2. Rotatie om X-as (Pitch)
    Matrix4 rx;
    rx.m[5] = cos(rotation.x); rx.m[6] = sin(rotation.x);
    rx.m[9] = -sin(rotation.x); rx.m[10] = cos(rotation.x);

    // 3. Rotatie om Y-as (Yaw)
    Matrix4 ry;
    ry.m[0] = cos(rotation.y); ry.m[2] = -sin(rotation.y);
    ry.m[8] = sin(rotation.y); ry.m[10] = cos(rotation.y);

    // 4. Rotatie om Z-as (Roll)
    Matrix4 rz;
    rz.m[0] = cos(rotation.z); rz.m[1] = sin(rotation.z);
    rz.m[4] = -sin(rotation.z); rz.m[5] = cos(rotation.z);

    // Combineer alles: Eerst Translatie, dan de rotaties (Y * X * Z)
    Matrix4 rotationCombined = Matrix4::Multiply(ry, Matrix4::Multiply(rx, rz));
    return Matrix4::Multiply(tMat, rotationCombined);
}

Matrix4 BaseObject::GetWorldMatrix() const {
    if (parent != nullptr) {
        // Recursie: we vermenigvuldigen de wereld-matrix van de ouder met onze lokale matrix
        return Matrix4::Multiply(parent->GetWorldMatrix(), GetLocalMatrix());
    }
    return GetLocalMatrix();
}

void BaseObject::Draw(Mesh mesh, Material mat) const {
    mat.maps[MATERIAL_MAP_ALBEDO].color = color;
    // Teken de mesh met de berekende wereld-matrix
    DrawMesh(mesh, mat, ToRaylib(GetWorldMatrix()));
}

BaseObject::BaseObject(Vector3 pos, Vector3 rot, Color col) : position(pos), rotation(rot), color(col), parent(nullptr) {}

Matrix ToRaylib(Matrix4 m) {
    return { m.m[0], m.m[4], m.m[8], m.m[12], m.m[1], m.m[5], m.m[9], m.m[13],
             m.m[2], m.m[6], m.m[10], m.m[14], m.m[3], m.m[7], m.m[11], m.m[15] };
}