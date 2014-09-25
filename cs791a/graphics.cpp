#include "graphics.h"
#include "engine.h"
#include "camera.h"
#include "terrain.h"
#include "shape.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QImage>

Graphics::Graphics(Engine *eng)
    : QGLWidget(), engine(eng)

{
    camera = new Camera(engine);
}

Graphics::~Graphics()
{
    for(Terrain *t : terrain_vec) {
        delete t;
    }

    for(Shape *s : shape_vec) {
        delete s;
    }

    delete camera;
}

GLuint Graphics::getShaderProgram(const QString &name) const
{
    return programs[name];
}

GLuint Graphics::createTextureFromFile(const QString &file, GLenum target)
{
    QImage image(file);

    GLuint texId;

    glGenTextures(1, &texId);

    if(engine->getOptions().verbose)
        qDebug() << "Texture ID:" << texId;

    glBindTexture(target, texId);
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if(target == GL_TEXTURE_2D) {
        glTexImage2D(target, 0, GL_RGBA, image.width(), image.height(),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, (void*) image.bits());
    }

    else {
        glTexImage1D(target, 0, GL_RGBA, image.width(), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, (void*) image.bits());
    }

    return texId;
}

void Graphics::initializeGL()
{
#ifndef __APPLE__
    GLenum status = glewInit();
    if(status != GLEW_OK) {
        qDebug() << "Unable to initialize glew";
        engine->stop(1);
    }
#endif
    if(engine->getOptions().verbose)
        qDebug() << (char*)glGetString(GL_VERSION);

    glClearColor(0.0f,0.2f,0.2f,1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(engine->getOptions().wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    updateView();

    QVector<GLuint> shader_data(2);

    shader_data[0] = loadShader("../shaders/colorvert.vs", GL_VERTEX_SHADER);
    shader_data[1] = loadShader("../shaders/colorfrag.fs", GL_FRAGMENT_SHADER);
    createShaderProgram("color", shader_data);

    shader_data[0] = loadShader("../shaders/grayvert.vs", GL_VERTEX_SHADER);
    shader_data[1] = loadShader("../shaders/grayfrag.fs", GL_FRAGMENT_SHADER);
    createShaderProgram("gray", shader_data);

    shader_data[0] = loadShader("../shaders/datavert.vs", GL_VERTEX_SHADER);
    shader_data[1] = loadShader("../shaders/datafrag.fs", GL_FRAGMENT_SHADER);
    createShaderProgram("data", shader_data);

    shader_data[0] = loadShader("../shaders/shapevert.vs", GL_VERTEX_SHADER);
    shader_data[1] = loadShader("../shaders/shapefrag.fs", GL_FRAGMENT_SHADER);
    createShaderProgram("shape", shader_data);

    initTerrain();
    initShapes();

}

void Graphics::resizeGL(int width, int height)
{
    qDebug() << "Resized:" << width << height;
    glViewport(0, 0, width, height);
    projection = glm::perspective(45.0f, float(width) / float(height),
                                  0.01f, 5000.0f);
}

void Graphics::initTerrain()
{
    const auto& terrain_files = engine->getOptions().terrain;

    for(auto& s : terrain_files)
        qDebug() << s.c_str();

    if(terrain_files.size() == 1) {
        terrain_vec.push_back(new Terrain(engine, QString::fromStdString(terrain_files[0]), programs["color"]));
        terrain_vec[0]->init();
    }

    else if(terrain_files.size() == 2) {
        terrain_vec = Terrain::createTerrainFromDEMandMask(engine, QString::fromStdString(terrain_files[0]), QString::fromStdString(terrain_files[1]));
    }

    else {
        Terrain *large = new Terrain(engine, QString::fromStdString(terrain_files[2]), programs["gray"]);
        large->init();

        terrain_vec = Terrain::createTerrainFromDEMandMask(engine, QString::fromStdString(terrain_files[0]), QString::fromStdString(terrain_files[1]), large);
        terrain_vec.push_back(large);

        Terrain *small = terrain_vec[0], *mask = terrain_vec[1];
        auto offsets = Terrain::getGeoTransformFromDEMs(large, small);
        auto geot = large->getGeot();

        double xoffset = offsets.first - geot[0];
        double zoffset = offsets.second - geot[3];

        double xOrrigOffset = large->getGeometryRef()[0].position[0] - small->getGeometryRef()[0].position[0];
        double zOrrigOffset = large->getGeometryRef()[0].position[2] - small->getGeometryRef()[0].position[2];

        float scale = engine->getOptions().map_scalar;
        float resolution = geot[1];

        small->translate(glm::vec3(xOrrigOffset, 0, zOrrigOffset));
        small->translate(glm::vec3((xoffset * scale / resolution), 0.01f * engine->getOptions().height_scalar, (-zoffset * scale / resolution)));

        mask->translate(glm::vec3(xOrrigOffset, 0, zOrrigOffset));
        mask->translate(glm::vec3((xoffset * scale / resolution), 0.01f * engine->getOptions().height_scalar, (-zoffset * scale / resolution)));
    }
}

void Graphics::initShapes()
{
    if(terrain_vec.size() > 2)
       for(const std::string& shape_file : engine->getOptions().shapes) {
           shape_vec.push_back(new Shape(engine, QString::fromStdString(shape_file), terrain_vec[2]));
       }
}

void Graphics::paintGL()
{
    updateCamera();
    updateView();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for(Terrain *t : terrain_vec) {
        t->render();
    }

    for(Shape *s : shape_vec) {
        s->render();
    }

    swapBuffers();
}

void Graphics::updateView()
{
    view = camera->getView();
}

void Graphics::updateCamera()
{
    camera->update();
}

GLuint Graphics::loadShader(const QString &shaderFile, GLenum shaderType)
{
    QFile file(shaderFile);
    if(!file.exists()) {
        qDebug() << "Unable to open shader file:" << shaderFile;
        engine->stop(1);
    }

    file.open(QFile::ReadOnly);

    QTextStream fin(&file);
    QString shader_contents = fin.readAll();
    auto byteArr = shader_contents.toUtf8();
    const char *shaderStr = byteArr.constData();

    file.close();

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderStr, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(!status) {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, NULL, buffer);
        QString shaderTypeStr = (shaderType == GL_VERTEX_SHADER) ? "Vertex Shader" : "Fragment Shader";
        qDebug() << "Failed to compile" << shaderTypeStr << "loaded from" << shaderFile;
        qDebug() << "Compile error:" << buffer;
        engine->stop(1);
    }

    return shader;

}

GLuint Graphics::createShaderProgram(const QString &name, const QVector<GLuint> &shader_data)
{
    GLuint program = glCreateProgram();
    for(auto shader : shader_data) {
       glAttachShader(program, shader);
    }

    glLinkProgram(program);

    GLint shader_status;
    glGetProgramiv(program, GL_LINK_STATUS, &shader_status);
    if(!shader_status) {
       qDebug() << "Unable to create shader program!";
       engine->stop(1);
    }

    programs[name] = program;
    shaders[name] = shader_data;

    if(engine->getOptions().verbose)
       qDebug() << "Created GL Program: " << name << ' ' << program;

    return program;
}
