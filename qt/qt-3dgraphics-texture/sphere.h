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
    // Constructor and destructor
    Sphere(const QVector3D& centerVertex, float radius, unsigned int latitudeBands,
           unsigned int longitudeBands, const QVector3D& rotationAxis,
           float angularSpeed, const QString& texturePath);

    virtual ~Sphere();

public:
    // Getter methods
    unsigned int getLatitudeBands();
    unsigned int getLongitudeBands();
    float getRadius();

protected:
    // Virtual method need to be implemented drivered from Abstract class figure.
    virtual void initializeVertices();
    virtual void initializeIndices();
    virtual void drawElements();
};

#endif // SPHERE_H
