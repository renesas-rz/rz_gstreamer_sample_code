#ifndef PROGRAM_H
#define PROGRAM_H

#include <QtWidgets/QOpenGLWidget>
#include <QtGui/QOpenGLFunctions>
#include <QtCore/QBasicTimer>
#include "figure.h"

class Program : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

private:
    // OpenGL programmable pipeline
    QOpenGLShaderProgram mProgram;

    // Zoom in or out 3D figures when users re-size the window
    QMatrix4x4 mProjection;

    QBasicTimer mTimer;

    Figure* mpCube;
    Figure* mpSphere;
    Figure* mpCone;

public:
    // Constructor and destructor
    Program(QOpenGLWidget* parent = 0);
    ~Program();

protected:
    virtual void initializeGL();
    virtual void resizeGL(int width, int height);
    virtual void paintGL();

    virtual void timerEvent(QTimerEvent *e);

private:
    void initializeShaders();
    void initializeCube();
    void initializeSphere();
    void initializeCone();
};

#endif // PROGRAM_H
