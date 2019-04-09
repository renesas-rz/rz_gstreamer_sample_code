#include "cone.h"
#include <QtCore/QtMath>

Cone::Cone(const QVector3D& centerVertex, float radius, float height,
           unsigned int chords, const QVector3D& rotationAxis, float angularSpeed,
           const std::vector<QString>& texturePaths)

    : Figure(centerVertex, rotationAxis, angularSpeed,
             QVector3D{ radius, height, radius }, texturePaths)
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
    // Create vertices for cone figure
    // Create head's vertex
    mVertices.push_back({ QVector3D{ 0.0f, 1.0f, 0.0f }, QVector2D { 0.5f, 1.0f } });

    // Create vertices for base circle
    // Push vertex (0, 0, 0) for later drawing using index array
    mVertices.push_back({ QVector3D{ 0.0f, -1.0f, 0.0f }, QVector2D{ 0.5f, 0.5f } });

    qreal angle = 0.0;
    for (unsigned int chord = 0; chord <= mChords; chord++) {
        qreal posX = qCos(angle);
        qreal posZ = qSin(angle);

        mVertices.push_back({ QVector3D{ (float)posX, -1.0f, (float)posZ },
                              QVector2D{ 0.0f, 1.0f * chord / mChords } });

        mVertices.push_back({ QVector3D{ (float)posX, -1.0f, (float)posZ },
                              QVector2D{ ((float)posX + 1.0f) * 0.5f,
                                         ((float)posZ + 1.0f) * 0.5f } });

        // Update value of angle
        angle += (2 * M_PI) / mChords;
    }
}

void Cone::initializeIndices()
{
    // Create the surface for cone shape
    std::vector<GLuint> surfaceIndices;

    // Create the base circle for cone shape
    std::vector<GLuint> baseIndices;

    for (size_t index = 2; index < mVertices.size() - 2; index++) {
        if (index % 2 == 0) {
            surfaceIndices.push_back(0);
            surfaceIndices.push_back((unsigned int)(index));
            surfaceIndices.push_back((unsigned int)(index + 2));
        } else {
            baseIndices.push_back(1);
            baseIndices.push_back((unsigned int)(index));
            baseIndices.push_back((unsigned int)(index + 2));
        }
    }

    mIndices.insert(mIndices.begin(), surfaceIndices.begin(), surfaceIndices.end());
    mIndices.insert(mIndices.end(), baseIndices.begin(), baseIndices.end());
}

void Cone::drawElements()
{
    const unsigned int coneParts = 2U;
    const unsigned int verticesPerTriangle = 3U;

    // Disable cull face feature so that we could see
    // the base circle of Cone shape
    glDisable(GL_CULL_FACE);

    // Bind textures
    for (unsigned int index = 0; index < coneParts; index++) {
        mTextures[index]->bind();
        glDrawElements(GL_TRIANGLES, (GLsizei)mChords * verticesPerTriangle, GL_UNSIGNED_INT,
                       (const GLvoid*)(index * mChords * verticesPerTriangle * sizeof(GLuint)));
    }

    glEnable(GL_CULL_FACE);
}
