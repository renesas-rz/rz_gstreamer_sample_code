#include "sphere.h"
#include <QtCore/QtMath>

Sphere::Sphere(const QVector3D& centerVertex, float radius,
               unsigned int latitudeBands, unsigned int longitudeBands,
               const QVector3D& rotationAxis, float angularSpeed)
    : Figure(centerVertex, rotationAxis, angularSpeed, QVector3D{
             radius, radius, radius })
{
    mRadius = radius;
    mLatitudeBands = latitudeBands;
    mLongitudeBands = longitudeBands;
}

Sphere::~Sphere()
{
    // Do nothing
}

unsigned int Sphere::getLatitudeBands()
{
    return mLatitudeBands;
}

unsigned int Sphere::getLongitudeBands()
{
    return mLongitudeBands;
}

float Sphere::getRadius()
{
    return mRadius;
}

void Sphere::initializeVertices()
{
    // Create 3 colors and assign them to vertices
    QVector3D colors[3];
    colors[0] = QVector3D{ 0.0f, 0.0f, 1.0f };
    colors[1] = QVector3D{ 0.0f, 1.0f, 0.0f };
    colors[2] = QVector3D{ 1.0f, 0.0f, 0.0f };

    const unsigned int colorsArrSize = sizeof(colors) / sizeof(QVector3D);
    unsigned int colorIndex = 0;

    // Find vertices of the sphere
    for (unsigned int latNumber = 0; latNumber <= mLatitudeBands; latNumber++) {
        qreal theta = latNumber * M_PI / mLatitudeBands;
        qreal sinTheta = qSin(theta);
        qreal cosTheta = qCos(theta);

        // Change color for every 4 latitude bands
        colorIndex = (latNumber / 4) % colorsArrSize;
        for (unsigned int longNumber = 0; longNumber <= mLongitudeBands; longNumber++) {
            qreal phi = longNumber * 2 * M_PI / mLongitudeBands;
            qreal sinPhi = qSin(phi);
            qreal cosPhi = qCos(phi);

            // If you remove -1.0 on variable x,
            // the rotation of the sphere won't be the same with cube and cone
            qreal x = -1.0f * cosPhi * sinTheta;
            qreal y = cosTheta;
            qreal z = sinPhi * sinTheta;

            mVertices.push_back({ QVector3D{(float)x, (float)y, (float)z},
                                  colors[colorIndex] });
        }
    }
}

void Sphere::initializeIndices()
{
    // Create indices for later drawing
    for (unsigned int latNumber = 0; latNumber < mLatitudeBands; latNumber++) 
        for (unsigned int longNumber = 0; longNumber < mLongitudeBands; longNumber++) {
            unsigned int first = (latNumber * (mLongitudeBands + 1)) + longNumber;
            unsigned int second = first + mLongitudeBands + 1;

            mIndices.push_back(first);
            mIndices.push_back(second);
            mIndices.push_back(first + 1);

            mIndices.push_back(second);
            mIndices.push_back(second + 1);
            mIndices.push_back(first + 1);
        }
}

void Sphere::drawElements()
{
    glDrawElements(GL_TRIANGLES, (GLsizei)mIndices.size(), GL_UNSIGNED_INT, 0);
}
