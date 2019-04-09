#ifndef CUBE_H
#define CUBE_H

#include "figure.h"

class Cube : public Figure
{
private:
    // The higher mEdgeLength, the bigger the Cube will be
    float mEdgeLength;

public:
    // Constructor and destructor
    Cube(const QVector3D& centerVertex, float edgeLength,
         const QVector3D& rotationAxis, float angularSpeed);
    virtual ~Cube();

public:
    // Getter method
    unsigned int getEdgeLength();

protected:
    // Virtual method need to be implemented from Abstract class Figure
    virtual void initializeVertices();
    virtual void initializeIndices();
    virtual void drawElements();
};

#endif // CUBE_H
