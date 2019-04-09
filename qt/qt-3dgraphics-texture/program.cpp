#include "program.h"
#include <QtGui/QOpenGLShader>
#include <iostream>
#include "cube.h"
#include "sphere.h"
#include "cone.h"

Program::Program(QOpenGLWidget *parent)
    : QOpenGLWidget(parent)
{
    // Set variables to NULL to explicitly show that they are currently
    // not pointing to any objects
    mpCube = NULL;
    mpSphere = NULL;
    mpCone = NULL;
}

Program::~Program()
{
    // Make sure the context is current when deleting the texture
    // and the buffers.
    makeCurrent();
    delete mpCube;
    delete mpSphere;
    delete mpCone;
    doneCurrent();
}

void Program::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0, 0, 0, 1);

    initializeShaders();

    // Initialize figures
    initializeCube();
    initializeCone();
    initializeSphere();

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Enable back face culling
    glEnable(GL_CULL_FACE);

    // Set basic timer to 40FPS
    mTimer.start(25, this);
}

void Program::initializeCube()
{
    // Initialize common variables for cube shape
    const float rotationSpeed = 2.0f;

    const QVector3D rotationAxis = QVector3D{ 1.0f, 1.0f, 0.0f};

    const QVector3D position = QVector3D{ 0.0f, 0.0f, -9.0f };

    const float edgeLength = 1.0f;

    std::vector<QString> texurePaths = {
        ":/cube_face1.jpg", ":/cube_face2.jpg", ":/cube_face3.jpg",
        ":/cube_face4.jpg", ":/cube_face5.jpg", ":/cube_face6.jpg"
    };

    // Initialize cube shape
    mpCube = new Cube(position, edgeLength, rotationAxis, rotationSpeed, texurePaths);
    mpCube->initializeComponents();
}

void Program::initializeSphere()
{
    // Initialize variables for sphere shape
    const float rotationSpeed = 2.0f;

    const QVector3D rotationAxis = QVector3D{ 1.0f, 1.0f, 0.0f};

    const QVector3D position = QVector3D{ -3.5f, 0.0f, -9.0f };

    const float radius = 1.0f;

    const unsigned int latitudeBands = 20U;
    const unsigned int longitudeBands = 20U;

    mpSphere = new Sphere(position, radius, latitudeBands, longitudeBands,
                          rotationAxis, rotationSpeed, ":/sphere.jpg");
    mpSphere->initializeComponents();
}

void Program::initializeCone()
{
    // Initialize variables for cone shape
    const float rotationSpeed = 2.0f;

    const QVector3D rotationAxis = QVector3D{ 1.0f, 1.0f, 0.0f};

    const QVector3D position = QVector3D{ 3.5f, 0.0f, -9.0f };

    const float radius = 1.0f;

    const float height = 1.0f;

    const unsigned int chords = 20U;

    std::vector<QString> texturePaths = { ":/cone_face1.jpeg", ":/cone_face2.jpeg" };

    // Initialize cone shape
    mpCone = new Cone(position, radius, height, chords, rotationAxis,
                      rotationSpeed, texturePaths);
    mpCone->initializeComponents();
}

void Program::resizeGL(int width, int height)
{
    // Calculate aspect ratio
    qreal aspect = qreal(width) / qreal(height ? height : 1);

    // Set near plane to 3.0, far plane to 20.0, field of view 45 degrees
    const qreal zNear = 2.0, zFar = 20.0, fov = 45.0;

    // Reset projection
    mProjection.setToIdentity();

    // Set perspective projection
    mProjection.perspective(fov, aspect, zNear, zFar);
}

void Program::timerEvent(QTimerEvent *)
{
    // Update the rotations of figures
    mpCube->update();
    mpSphere->update();
    mpCone->update();

    // Call paintGL method
    update();
}

void Program::paintGL()
{
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw figures
    mpCube->draw(mProjection, mProgram);
    mpSphere->draw(mProjection, mProgram);
    mpCone->draw(mProjection, mProgram);
}

void Program::initializeShaders()
{
    // Compile vertex shader
    if (!mProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl"))
        close();

    // Compile fragment shader
    if (!mProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl"))
        close();

    // Link shader pipeline
    if (!mProgram.link())
        close();

    // Bind shader pipeline for use
    if (!mProgram.bind())
        close();
}
