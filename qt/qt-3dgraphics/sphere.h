#ifndef SPHERE_H
#define SPHERE_H

#include "figure.h"

class Sphere : public Figure
{
private:
    float mRadius;

    // Horizontal lines
    unsigned int mLatitudeBands;

    // Vertical lines
    unsigned int mLongitudeBands;

public:
    // Constructor and deconstructor
    Sphere(const QVector3D& centerVertex, float radius, unsigned int latitudeBands,
           unsigned int longitudeBands, const QVector3D& rotationAxis, float angularSpeed);
    virtual ~Sphere();

public:
    // Getter methods
    unsigned int getLatitudeBands();
    unsigned int getLongitudeBands();
    float getRadius();

protected:
    // Virtual method need to be implemented from Abstract class Figure
    void initializeVertices();
    void initializeIndices();
    void drawElements();
};

#endif // SPHERE_H
