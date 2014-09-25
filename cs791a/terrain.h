#ifndef TERRAIN_H
#define TERRAIN_H

#include <QVector>
#include <QString>
#include <utility>

#include "gl.h"
#include "vertex.h"

#include <glm/glm.hpp>

class GDALDataset;

class Engine;

class Terrain {
public:
    Terrain(Engine *eng, const QString& map, GLuint prog);
    ~Terrain();

    void init();
    void tick(float dt);
    void render();

    void applyDataset(const QString& file);

    static QVector<Terrain*> createTerrainFromDEMandMask(Engine *engine, const QString& dem, const QString& mask, Terrain *large_dem = nullptr);

    static std::pair<double,double> getGeoTransformFromDEMs(Terrain *large, Terrain *small);

    QVector<double> getGeot() const {return geot;}
    QVector<Vertex> getGeometry() const {return geometry;}
    QVector<Vertex>& getGeometryRef() {return geometry;}

    GDALDataset* getDataset() const {return dataset;}

    void translate(const glm::vec3& vec);

private:
    void initTerrainFile();
    void initGL(bool genBuffer = true);

    Engine *engine;
    QString map_file;
    GLuint program;

    GLuint vbo;
    GLint loc_mvp, loc_position, loc_texture, loc_heightScalar;
    GLint loc_dataPoint;

    QVector<Vertex> geometry;
    QVector<GLuint> textures;
    QVector<double> geot;

    glm::mat4 model;

    GDALDataset *dataset;
};

#endif // TERRAIN_H
