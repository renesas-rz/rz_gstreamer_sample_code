#ifndef CONE_H
#define CONE_H

#include "figure.h"

class Cone : public Figure
{
private:
    // The higher mChords, the more "curve" the base circle will be
    unsigned int mChords;

    // The radius of the base circle
    float mRadius;

    // The height of the cone shape
    float mHeight;

public:
    // Constructor and destructor
    Cone(const QVector3D& centerVertex, float radius, float height, unsigned int chords,
         const QVector3D& rotationAxis, float angularSpeed);
    virtual ~Cone();

public:
    // Getter methods
    unsigned int getChords();
    float getRadius();
    float getHeight();

protected:
    // Virtual method need to be implemented from Abstract class Figure
    virtual void initializeVertices();
    virtual void initializeIndices();
    virtual void drawElements();
};

#endif // CONE_H
