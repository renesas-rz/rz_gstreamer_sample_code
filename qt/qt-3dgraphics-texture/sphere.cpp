#include "sphere.h"
#include <QtCore/QtMath>

Sphere::Sphere(const QVector3D& centerVertex, float radius, unsigned int latitudeBands,
               unsigned int longitudeBands, const QVector3D& rotationAxis,
               float angularSpeed, const QString& texturePath)

    :Figure(centerVertex, rotationAxis, angularSpeed,
            QVector3D{radius, radius, radius }, { texturePath })
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
    // Find vertices of the sphere
    for (unsigned int latNumber = 0; latNumber <= mLatitudeBands; latNumber++) {
        qreal theta = latNumber * M_PI / mLatitudeBands;
        qreal sinTheta = qSin(theta);
        qreal cosTheta = qCos(theta);

        for (unsigned int longNumber = 0; longNumber <= mLongitudeBands; longNumber++) {
            qreal phi = longNumber * 2 * M_PI / mLongitudeBands;
            qreal sinPhi = qSin(phi);
            qreal cosPhi = qCos(phi);

            // If you remove -1.0 on variable x,
            // the rotation of the sphere won't be the same with cube and cone
            qreal posX = -1.0f * cosPhi * sinTheta;
            qreal poxY = cosTheta;
            qreal posZ = sinPhi * sinTheta;

            qreal textureX = 1 - (1.0f * longNumber / mLongitudeBands);
            qreal textureY = 1 - (1.0f * latNumber / mLatitudeBands);

            mVertices.push_back({ QVector3D{(float)posX, (float)poxY, (float)posZ},
                                  QVector2D{(float)textureX, (float)textureY} });
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
    mTextures[0]->bind();
    glDrawElements(GL_TRIANGLES, (GLsizei)mIndices.size(), GL_UNSIGNED_INT, 0);
}
