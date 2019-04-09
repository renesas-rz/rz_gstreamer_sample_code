#include "vertex.h"

Vertex::Vertex()
    : mPosition(0, 0, 0), mTextureCoord(0, 0)
{
    // Do nothing
}

Vertex::Vertex(const QVector3D& position, const QVector2D& textureCoord)
{
    mPosition = position;
    mTextureCoord = textureCoord;
}

Vertex::~Vertex()
{
    // Do nothing
}

const QVector3D& Vertex::getPosition()
{
    return mPosition;
}

void Vertex::setPosition(const QVector3D& position)
{
    mPosition = position;
}

const QVector2D& Vertex::getTextureCoord()
{
    return mTextureCoord;
}

void Vertex::setTextureCoord(const QVector2D& textureCoord)
{
    mTextureCoord = textureCoord;
}

int Vertex::getPositionOffset()
{
    return offsetof(Vertex, mPosition);
}

int Vertex::getTextureCoordOffset()
{
    return offsetof(Vertex, mTextureCoord);
}

int Vertex::getVertexClassStride()
{
    return sizeof(Vertex);
}
