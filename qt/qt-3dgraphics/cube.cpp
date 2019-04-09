#include "cube.h"

Cube::Cube(const QVector3D& centerVertex, float edgeLength,
           const QVector3D& rotationAxis, float angularSpeed)

    : Figure(centerVertex, rotationAxis, angularSpeed, QVector3D{
             edgeLength, edgeLength, edgeLength })
{
    mEdgeLength = edgeLength;
}

Cube::~Cube()
{
    // Do nothing
}

unsigned int Cube::getEdgeLength()
{
    return mEdgeLength;
}

void Cube::initializeVertices()
{
    // Create vertices for cube
    Vertex verticesArr[] = {
        // Red face 1
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector3D{ 1.0f, 0.0f, 0.0f }}, // v0
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector3D{ 1.0f, 0.0f, 0.0f }}, // v1
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector3D{ 1.0f, 0.0f, 0.0f }}, // v2
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector3D{ 1.0f, 0.0f, 0.0f }}, // v3

        // Green face 2
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector3D{ 0.0f, 1.0f, 0.0f }}, // v4
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector3D{ 0.0f, 1.0f, 0.0f }}, // v5
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector3D{ 0.0f, 1.0f, 0.0f }}, // v6
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector3D{ 0.0f, 1.0f, 0.0f }}, // v7

        // Blue face 3
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector3D{ 0.0f, 0.0f, 1.0f }}, // v8
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector3D{ 0.0f, 0.0f, 1.0f }}, // v9
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector3D{ 0.0f, 0.0f, 1.0f }}, // v10
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector3D{ 0.0f, 0.0f, 1.0f }}, // v11

        // Yellow face 4
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector3D{ 1.0f, 1.0f, 0.0f }}, // v12
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector3D{ 1.0f, 1.0f, 0.0f }}, // v13
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector3D{ 1.0f, 1.0f, 0.0f }}, // v14
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector3D{ 1.0f, 1.0f, 0.0f }}, // v15

        // Pink face 5
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector3D{ 1.0f, 0.0f, 1.0f }}, // v16
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector3D{ 1.0f, 0.0f, 1.0f }}, // v17
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector3D{ 1.0f, 0.0f, 1.0f }}, // v18
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector3D{ 1.0f, 0.0f, 1.0f }}, // v19

        // Light blue face 6
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector3D{ 0.0f, 1.0f, 1.0f }}, // v20
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector3D{ 0.0f, 1.0f, 1.0f }}, // v21
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector3D{ 0.0f, 1.0f, 1.0f }}, // v22
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector3D{ 0.0f, 1.0f, 1.0f }}  // v23
    };

    for (size_t index = 0; index < sizeof(verticesArr) / sizeof(Vertex); index++) {
        mVertices.push_back(verticesArr[index]);
    }
}

void Cube::initializeIndices()
{
    // Indices for drawing cube faces using triangle strips.
    GLuint indicesArr[] = {
        0,  1,  2,  3,  3,      // Red face 1
        4,  4,  5,  6,  7,  7,  // Green face 2
        8,  8,  9, 10, 11, 11,  // Blue face 3
        12, 12, 13, 14, 15, 15, // Yellow face 4
        16, 16, 17, 18, 19, 19, // Pink face 5
        20, 20, 21, 22, 23      // Light blue face 6
    };

    for (size_t index = 0; index < sizeof(indicesArr) / sizeof(GLuint); index++)
        mIndices.push_back(indicesArr[index]);
}

void Cube::drawElements()
{
    glDrawElements(GL_TRIANGLE_STRIP, (GLsizei)mIndices.size(), GL_UNSIGNED_INT, 0);
}
