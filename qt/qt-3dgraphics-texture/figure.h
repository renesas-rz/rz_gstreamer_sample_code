#ifndef FIGURE_H
#define FIGURE_H

#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLTexture>
#include "vertex.h"
#include <vector>

class Figure: protected QOpenGLFunctions
{
protected:
    // Hold vertices and indices and store them in GPU memory
    QOpenGLBuffer mVerticesBuffer;
    QOpenGLBuffer mIndicesBuffer;

    // Save number of vertices and indices for later transfering to buffer
    std::vector<Vertex> mVertices;
    std::vector<GLuint> mIndices;

    // The center vertex of figure
    QVector3D mCenterVertex;

    // Define an axis that the figure will rotate around
    QVector3D mRotationAxis;

    // Define directions that figure will be scaled
    QVector3D mScaleAxis;

    // How fast that the figure shall rotate
    float mAngularSpeed;

    // Rotation will be calculated via Quaternion
    QQuaternion mRotation;

    // List of textures
    std::vector<QOpenGLTexture*> mTextures;
    std::vector<QString> mTexturePaths;

public:
    // Constructor and deconstructor
    Figure(const QVector3D& centerVertex, const QVector3D& rotationAxis,
           float angularSpeed, const QVector3D& scaleAxis,
           const std::vector<QString>& texturePaths);
    virtual ~Figure();

public:
    // Getter methods
    const QVector3D& getCenterVertex();
    const QVector3D& getRotationAxis();
    const QVector3D& getScaleAxis();
    float getAngularSpeed();

private:
    void initializeTextures();

protected:
    // Methods need to be implemented by derived classes
    virtual void initializeVertices() = 0;
    virtual void initializeIndices() = 0;
    virtual void drawElements() = 0;

public:
    // Methods might be extended by derived classes
    virtual void initializeComponents();
    virtual void draw(QMatrix4x4& projectionMatrix, QOpenGLShaderProgram& program);
    virtual void update();

private:
    static QOpenGLTexture* createTexture(const QString& texturePath);
};

#endif // FIGURE_H
