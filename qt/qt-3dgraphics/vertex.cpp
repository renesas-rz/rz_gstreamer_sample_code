#include "vertex.h"

Vertex::Vertex()
    : mPosition(0, 0, 0), mColor(0, 0, 0)
{
    // Do nothing
}

Vertex::Vertex(const QVector3D& position, const QVector3D& color)
{
    mPosition = position;
    mColor = color;
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

const QVector3D& Vertex::getColor()
{
    return mColor;
}

void Vertex::setColor(const QVector3D& color)
{
    mColor = color;
}

int Vertex::getPositionOffset()
{
    return offsetof(Vertex, mPosition);
}

int Vertex::getColorOffset()
{
    return offsetof(Vertex, mColor);
}

int Vertex::getVertexClassStride()
{
    return sizeof(Vertex);
}
