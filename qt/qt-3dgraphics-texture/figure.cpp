#include "figure.h"

Figure::Figure(const QVector3D& centerVertex, const QVector3D& rotationAxis,
               float angularSpeed, const QVector3D& scaleAxis,
               const std::vector<QString>& texturePaths)

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

    mTexturePaths = texturePaths;
    mTextures.assign(mTexturePaths.size(), NULL);
}

Figure::~Figure()
{
    // Delete buffers
    mVerticesBuffer.destroy();
    mIndicesBuffer.destroy();

    // Delete textures
    for (size_t index = 0; index < mTextures.size(); index++) {
        if (mTextures[index] != NULL) {
            delete mTextures[index];
            mTextures[index] = NULL;
        }
    }
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

    // Initialize textures
    initializeTextures();

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
    int colorAddr = program.attributeLocation("a_texcoord");
    program.enableAttributeArray(colorAddr);
    program.setAttributeBuffer(colorAddr, GL_FLOAT, Vertex::getTextureCoordOffset(),
                                Vertex::TEXTURE_COORD_TYPLE_SIZE, Vertex::getVertexClassStride());

    // Initialize variable texture in Fragment Stage
    program.setUniformValue("texture", 0);

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

QOpenGLTexture* Figure::createTexture(const QString& texturePath)
{
    // Instantiate object
    QOpenGLTexture* texture = NULL;

    // Load image
    texture = new QOpenGLTexture(QImage(texturePath).mirrored(true, true));

    // Set nearest filtering mode for texture minification
    texture->setMinificationFilter(QOpenGLTexture::Nearest);

    // Set bilinear filtering mode for texture magnification
    texture->setMagnificationFilter(QOpenGLTexture::Linear);

    // Wrap texture coordinates by repeating
    // f.ex. texture coordinate (1.1, 1.2) is same as (0.1, 0.2)
    texture->setWrapMode(QOpenGLTexture::Repeat);

    return texture;
}

void Figure::initializeTextures()
{
    for (size_t index = 0; index < mTextures.size(); index++)
        mTextures[index] = Figure::createTexture(mTexturePaths[index]);
}
