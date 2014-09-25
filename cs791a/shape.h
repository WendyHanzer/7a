#ifndef SHAPE_H
#define SHAPE_H

#include <QString>
#include <QVector>

#include "gl.h"

#include <glm/glm.hpp>

class Engine;
struct Vertex;
class Terrain;

class Shape {
public:
    Shape(Engine *eng, const QString& shape_file, Terrain *large_dem);

    void init();
    void initGL();
    void tick(float dt);
    void render();

private:
    Engine *engine;

    GLuint program;
    GLuint vbo;
    GLint loc_mvp, loc_position, loc_color, loc_heightScalar;

    GLfloat color[4];

    glm::mat4 model;

    QVector<Vertex> points;
};

#endif // SHAPE_H
