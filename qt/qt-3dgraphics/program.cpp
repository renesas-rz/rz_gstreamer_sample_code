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

    // Initialize common variables for figures
    const float rotationSpeed = 5.0f;
    const QVector3D rotationAxis = QVector3D{ 1.0f, 1.0f, 0.0f};

    // Initialize variables for cube shape
    const QVector3D cubePosition = QVector3D{ 0.0f, 0.0f, -9.0f };
    const float cubeEdgeLength = 1.0f;

    // Initialize variables for sphere shape
    const QVector3D spherePosition = QVector3D{ -3.5f, 0.0f, -9.0f };
    const float sphereRadius = 1.0f;
    const unsigned int sphereLatBands = 30;
    const unsigned int sphereLongBands = 30;

    // Initialize variables for cone shape
    const QVector3D conePosition = QVector3D{ 3.5f, 0.0f, -9.0f };
    const float coneRadius = 1.0f;
    const float coneHeight = 1.0f;
    const unsigned int coneChords = 20;

    // Initialize cube shape
    mpCube = new Cube(cubePosition, cubeEdgeLength, rotationAxis, rotationSpeed);
    mpCube->initializeComponents();

    // Initialize sphere shape
    mpSphere = new Sphere(spherePosition, sphereRadius, sphereLatBands,
                          sphereLongBands, rotationAxis, rotationSpeed);
    mpSphere->initializeComponents();

    // Initialize cone shape
    mpCone = new Cone(conePosition, coneRadius, coneHeight, coneChords,
                      rotationAxis, rotationSpeed);
    mpCone->initializeComponents();

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Enable back face culling
    glEnable(GL_CULL_FACE);

    // Set basic timer to 40FPS
    mTimer.start(25, this);
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
