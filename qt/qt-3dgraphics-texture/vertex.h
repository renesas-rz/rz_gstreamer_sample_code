#ifndef VERTEX_H
#define VERTEX_H

#include <QtGui/QVector3D>
#include <QtGui/QVector2D>

class Vertex
{
private:
    QVector3D mPosition;
    QVector2D mTextureCoord;

public:
    // Constructor and destructor
    Vertex();
    Vertex(const QVector3D& position, const QVector2D& textureCoord);
    ~Vertex();

public:
    //Getter, Setter method
    const QVector3D& getPosition();
    void setPosition(const QVector3D& position);

    const QVector2D& getTextureCoord();
    void setTextureCoord(const QVector2D& textureCoord);

public:
    static int getPositionOffset();
    static int getTextureCoordOffset();

    static const int POSITION_TYPLE_SIZE = 3;
    static const int TEXTURE_COORD_TYPLE_SIZE = 2;

    static int getVertexClassStride();

};

#endif // VERTEX_H
