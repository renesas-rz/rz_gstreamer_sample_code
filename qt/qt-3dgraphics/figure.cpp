#include "figure.h"

Figure::Figure(const QVector3D& centerVertex, const QVector3D& rotationAxis,
               float angularSpeed, const QVector3D& scaleAxis)

    : mIndicesBuffer(QOpenGLBuffer::IndexBuffer)
{
    initializeOpenGLFunctions();

    // Generate 2 VBOs
    mVerticesBuffer.create();
    mIndicesBuffer.create();

    mCenterVertex = centerVertex;
    mRotationAxis = rotationAxis;
    mAngularSpeed = angularSpeed;

    mScaleAxis = scaleAxis;
}

Figure::~Figure()
{
    mVerticesBuffer.destroy();
    mIndicesBuffer.destroy();
}

const std::vector<Vertex>& Figure::getVertices()
{
    return mVertices;
}

const std::vector<GLuint>& Figure::getIndices()
{
    return mIndices;
}

const QVector3D& Figure::getCenterVertex()
{
    return mCenterVertex;
}

const QVector3D& Figure::getRotationAxis()
{
    return mRotationAxis;
}

const QVector3D& Figure::getScaleAxis()
{
    return mScaleAxis;
}

float Figure::getAngularSpeed()
{
    return mAngularSpeed;
}

void Figure::initializeComponents()
{
    // Initialize vertices
    initializeVertices();

    // Initialize indices
    initializeIndices();

    // Bind vertices and indices to VBOs
    mVerticesBuffer.bind();
    mVerticesBuffer.allocate(&mVertices[0], (int)mVertices.size() * sizeof(Vertex));

    mIndicesBuffer.bind();
    mIndicesBuffer.allocate(&mIndices[0], (int)mIndices.size() * sizeof(GLuint));
}

void Figure::draw(QMatrix4x4& projectionMatrix, QOpenGLShaderProgram& program)
{
    // Tell OpenGL which VBOs to use
    mVerticesBuffer.bind();
    mIndicesBuffer.bind();

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int positionAddr = program.attributeLocation("a_position");
    program.enableAttributeArray(positionAddr);
    program.setAttributeBuffer(positionAddr, GL_FLOAT, Vertex::getPositionOffset(),
                                Vertex::POSITION_TYPLE_SIZE, Vertex::getVertexClassStride());

    // Tell OpenGL programmable pipeline how to locate color data
    int colorAddr = program.attributeLocation("a_color");
    program.enableAttributeArray(colorAddr);
    program.setAttributeBuffer(colorAddr, GL_FLOAT, Vertex::getColorOffset(),
                                Vertex::COLOR_TYPLE_SIZE, Vertex::getVertexClassStride());

    // Pass mvp matrix to OpenGL programmable pipeline
    QMatrix4x4 modelMatrix;
    modelMatrix.translate(mCenterVertex);
    modelMatrix.rotate(mRotation);
    modelMatrix.scale(mScaleAxis);

    program.setUniformValue("mvp_matrix", projectionMatrix * modelMatrix);

    // Draw cube geometry using indices from VBO 1
    drawElements();
}

void Figure::update()
{
    // Update transformation of figure
    mRotation = QQuaternion::fromAxisAndAngle(mRotationAxis, mAngularSpeed) * mRotation;
}
