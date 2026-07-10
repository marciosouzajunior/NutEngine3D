#pragma once

#include "Vec3.h"
#include "MathUtils.h"

#ifdef ARDUINO
#include <math.h>
#else
#include <cmath>
#endif

namespace nut {
namespace math {

#ifdef ARDUINO
inline float nutCos(float value) { return cos(value); }
inline float nutSin(float value) { return sin(value); }
inline float nutTan(float value) { return tan(value); }
#else
inline float nutCos(float value) { return std::cos(value); }
inline float nutSin(float value) { return std::sin(value); }
inline float nutTan(float value) { return std::tan(value); }
#endif

// A 4x4 Matrix class. 
// In 3D graphics, 4x4 matrices are used to combine translation, rotation, and scaling
// into a single mathematical structure. They are also used for perspective projection.
// We use a 1D array of 16 floats to represent the matrix, typically stored in column-major 
// or row-major order. We'll use row-major for simpler visualization in code.
struct Mat4 {
    float m[4][4];

    // Default constructor creates an identity matrix.
    // An identity matrix does nothing when multiplied (like multiplying by 1).
    Mat4() {
        for(int r = 0; r < 4; ++r) {
            for(int c = 0; c < 4; ++c) {
                m[r][c] = (r == c) ? 1.0f : 0.0f;
            }
        }
    }

    // Multiply this matrix by another matrix.
    // Order matters: A * B is not necessarily equal to B * A!
    // In our engine, if we want to Scale, then Rotate, then Translate, 
    // the combined matrix is: Translate * Rotate * Scale
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                result.m[r][c] = 
                    m[r][0] * other.m[0][c] +
                    m[r][1] * other.m[1][c] +
                    m[r][2] * other.m[2][c] +
                    m[r][3] * other.m[3][c];
            }
        }
        return result;
    }

    // Multiply a 3D vector by this matrix.
    // Since it's a 4x4 matrix and a 3D vector, we implicitly add a 'w' component to the vector.
    // For position vectors (points), w = 1.0
    // Returns a Vec3. The w component is used for perspective division (handled later if needed),
    // but here we just return the x, y, z.
    Vec3 operator*(const Vec3& v) const {
        float x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*1.0f;
        float y = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*1.0f;
        float z = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*1.0f;
        float w = m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*1.0f;
        
        // If w is not 1.0 and not 0.0, we divide to get perspective division
        if (w != 1.0f && w != 0.0f) {
            x /= w;
            y /= w;
            z /= w;
        }
        
        return Vec3(x, y, z);
    }

    // Static helper to create a Translation matrix
    static Mat4 makeTranslation(float tx, float ty, float tz) {
        Mat4 result; // Identity
        result.m[0][3] = tx;
        result.m[1][3] = ty;
        result.m[2][3] = tz;
        return result;
    }

    // Static helper to create a Scaling matrix
    static Mat4 makeScale(float sx, float sy, float sz) {
        Mat4 result; // Identity
        result.m[0][0] = sx;
        result.m[1][1] = sy;
        result.m[2][2] = sz;
        return result;
    }

    // Static helpers to create Rotation matrices (in radians)
    static Mat4 makeRotationX(float rad) {
        Mat4 result;
        float c = nutCos(rad);
        float s = nutSin(rad);
        result.m[1][1] = c;
        result.m[1][2] = -s;
        result.m[2][1] = s;
        result.m[2][2] = c;
        return result;
    }

    static Mat4 makeRotationY(float rad) {
        Mat4 result;
        float c = nutCos(rad);
        float s = nutSin(rad);
        result.m[0][0] = c;
        result.m[0][2] = s;
        result.m[2][0] = -s;
        result.m[2][2] = c;
        return result;
    }

    static Mat4 makeRotationZ(float rad) {
        Mat4 result;
        float c = nutCos(rad);
        float s = nutSin(rad);
        result.m[0][0] = c;
        result.m[0][1] = -s;
        result.m[1][0] = s;
        result.m[1][1] = c;
        return result;
    }

    // Create a Perspective Projection matrix
    // fovY: field of view in Y direction (in radians)
    // aspect: width / height of the screen/viewport
    // zNear, zFar: clipping planes
    static Mat4 makePerspective(float fovY, float aspect, float zNear, float zFar) {
        Mat4 result;
        // Start with all zeros
        for(int r=0; r<4; ++r) for(int c=0; c<4; ++c) result.m[r][c] = 0.0f;

        float tanHalfFov = nutTan(fovY / 2.0f);
        float zRange = zNear - zFar;

        result.m[0][0] = 1.0f / (aspect * tanHalfFov);
        result.m[1][1] = 1.0f / tanHalfFov;
        result.m[2][2] = (-zNear - zFar) / zRange;
        result.m[2][3] = (2.0f * zFar * zNear) / zRange;
        result.m[3][2] = 1.0f; // This puts the Z value into the W component for perspective divide
        
        return result;
    }
};

} // namespace math
} // namespace nut
