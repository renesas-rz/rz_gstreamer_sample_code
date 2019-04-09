#include "cone.h"
#include <QtCore/QtMath>

Cone::Cone(const QVector3D& centerVertex, float radius, float height,
           unsigned int chords, const QVector3D& rotationAxis, float angularSpeed)

    : Figure(centerVertex, rotationAxis, angularSpeed, QVector3D{
             radius, height, radius })
{
    mChords = chords;
    mRadius = radius;
    mHeight = height;
}

Cone::~Cone()
{
    // Do nothing
}

unsigned int Cone::getChords()
{
    return mChords;
}

float Cone::getRadius()
{
    return mRadius;
}

float Cone::getHeight()
{
    return mHeight;
}

void Cone::initializeVertices()
{
    // Create top and base color
    QVector3D topColor(0.0f, 1.0f, 1.0f);
    QVector3D baseColor(1.0f, 0.0f, 1.0f);

    // Create vertices for cone figure
    // Create head's vertex
    mVertices.push_back({ QVector3D{ 0.0f, 1.0f, 0.0f }, topColor });

    qreal angle = 0.0;
    for (unsigned int chord = 0; chord <= mChords; chord++) {
        qreal x = qCos(angle);
        qreal z = qSin(angle);
        mVertices.push_back({ QVector3D{ (float)x, -1.0f, (float)z }, baseColor });

        // Update value of angle
        angle += (2 * M_PI) / mChords;
    }

    // Push vertex (0, 0, 0) for later drawing using index array
    mVertices.push_back({ QVector3D{ 0.0f, -1.0f, 0.0f }, baseColor });
}

void Cone::initializeIndices()
{
    // Create indices for cone shape
    for (size_t index = 1; index < mVertices.size() - 2; index++) {
        mIndices.push_back(0);
        mIndices.push_back((unsigned int)index);
        mIndices.push_back((unsigned int)index + 1);

        mIndices.push_back((unsigned int)mVertices.size() - 1);
        mIndices.push_back((unsigned int)index);
        mIndices.push_back((unsigned int)index + 1);
    }
}

void Cone::drawElements()
{
    // Disable cull face feature so that we could see
    // the base circle of Cone shape
    glDisable(GL_CULL_FACE);
    glDrawElements(GL_TRIANGLES, (GLsizei)mIndices.size(), GL_UNSIGNED_INT, 0);
    glEnable(GL_CULL_FACE);
}
