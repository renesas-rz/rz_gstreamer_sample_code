#ifndef VERTEX_H
#define VERTEX_H

#include <QtGui/QVector3D>

class Vertex
{
private:
    QVector3D mPosition;
    QVector3D mColor;

public:
    // Constructor and deconstructor
    Vertex();
    Vertex(const QVector3D& position, const QVector3D& color);
    ~Vertex();

public:
    // Getter/Setter methods
    const QVector3D& getPosition();
    void setPosition(const QVector3D& position);

    // x = RED, y = GREEN, z = BLUE
    const QVector3D& getColor();
    void setColor(const QVector3D& color);

public:
    static int getPositionOffset();
    static int getColorOffset();

    static const int POSITION_TYPLE_SIZE = 3;
    static const int COLOR_TYPLE_SIZE = 3;

    static int getVertexClassStride();

};

#endif // VERTEX_H
