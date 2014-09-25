#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "gl.h"

#include <QGLWidget>
#include <QMap>
#include <QVector>
#include <QString>

#include <glm/glm.hpp>

class Engine;
class Terrain;
class Shape;
class Camera;

class Graphics : public QGLWidget
{
    Q_OBJECT
public:
    explicit Graphics(Engine *eng);
    ~Graphics();

    GLuint getShaderProgram(const QString& name) const;
    GLuint createTextureFromFile(const QString& file, GLenum target = GL_TEXTURE_2D);

    glm::mat4 view, projection;
    Camera *camera;
signals:

public slots:
    void paintGL();

protected:
    void initializeGL();
    void resizeGL(int width, int height);

private:
    void initTerrain();
    void initShapes();
    void updateView();
    void updateCamera();
    GLuint loadShader(const QString& shaderFile, GLenum shaderType);
    GLuint createShaderProgram(const QString& name, const QVector<GLuint>& shader_data);

    Engine *engine;

    QMap<QString, GLuint> programs;
    QMap<QString, QVector<GLuint>> shaders;
    QVector<Terrain*> terrain_vec;
    QVector<Shape*> shape_vec;

};

#endif // GRAPHICS_H
