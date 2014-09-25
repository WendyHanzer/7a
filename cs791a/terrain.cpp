#include "gl.h"
#include "engine.h"
#include "terrain.h"
#include "graphics.h"

#include <gdal_priv.h>
#include <cpl_conv.h>
#include <ogr_srs_api.h>
#include <ogr_spatialref.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QDebug>

Terrain::Terrain(Engine *eng, const QString& map, GLuint prog)
    : engine(eng), map_file(map), program(prog)
{
    //init();
}

Terrain::~Terrain()
{
     GDALClose( (GDALDatasetH) dataset );
}

void Terrain::init()
{
    QString color_map = QString::fromStdString(engine->getOptions().color_map);

    textures.push_back(engine->graphics->createTextureFromFile(color_map, GL_TEXTURE_1D));

    initTerrainFile();
    initGL();
}

void Terrain::tick(float dt)
{
    Q_UNUSED(dt);
}

void Terrain::render()
{
    glm::mat4 mvp = engine->graphics->projection * engine->graphics->view * model;

    glUseProgram(program);

    glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

    //glEnableVertexAttribArray(loc_heightScalar);
    glUniform1f(loc_heightScalar, engine->getOptions().height_scalar);

    glEnableVertexAttribArray(loc_position);

    if(program == engine->graphics->getShaderProgram("data"))
        glEnableVertexAttribArray(loc_dataPoint);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer( loc_position,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,position));

    if(program == engine->graphics->getShaderProgram("data"))
        glVertexAttribPointer(loc_dataPoint,
                              1,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(Vertex),
                              (void*)offsetof(Vertex,dataPoint));



    // send texture locations for all textures
    for(int i = 0; i < textures.size(); i++) {
        glUniform1i(loc_texture,i);
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D,textures[i]);
        //glUniform1i(loc_texture,i);
    }

    // draw object
    glDrawArrays(GL_TRIANGLES, 0, geometry.size());//mode, starting index, count

    // disable attribute pointers
    glDisableVertexAttribArray(loc_position);

    if(program == engine->graphics->getShaderProgram("data"))
        glDisableVertexAttribArray(loc_dataPoint);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);
}

void Terrain::initTerrainFile()
{
    auto t = map_file.toLatin1();
    dataset = (GDALDataset*) GDALOpen(t.constData(), GA_ReadOnly);

    if(dataset == nullptr) {
        qDebug() << "Unable to get GDAL Dataset for file: " << map_file;
        exit(1);
    }

    GDALRasterBand *raster = dataset->GetRasterBand(1);

    int width = raster->GetXSize();//terrain_img.getWidth();
    int height = raster->GetYSize();//terrain_img.getHeight();

    int gotMin, gotMax;

    float min = raster->GetMinimum(&gotMin);
    float max = raster->GetMaximum(&gotMax);

    double minMax[2] = {min, max};

    if(!(gotMin && gotMax)) {
        GDALComputeRasterMinMax((GDALRasterBandH) raster, TRUE, minMax);

        min = minMax[0];
        max = minMax[1];
    }

    if(engine->getOptions().verbose)
        qDebug() << "terrain: " << map_file << "x: " << width << " y: " << height << "   min: " << min << " max: " << max;

    int woffset = width / 2;
    int hoffset = height / 2;

    Vertex tempVert;
    tempVert.position[1] = 0;

    float maxOffset = max - min;

    float scale = engine->getOptions().map_scalar;
    float *lineData = (float*) CPLMalloc(sizeof(float) * width);
    float *lineData2 = (float*) CPLMalloc(sizeof(float) * width);

    for(int z = -hoffset; z < height - hoffset-1; z++) {
        raster->RasterIO(GF_Read, 0, z + hoffset, width, 1, lineData, width, 1, GDT_Float32, 0, 0);
        raster->RasterIO(GF_Read, 0, z + hoffset + 1, width, 1, lineData2, width, 1, GDT_Float32, 0, 0);

        for(int x = -woffset; x < width - woffset-1; x++) {
            tempVert.position[0] = x*scale;
            tempVert.position[1] = (lineData[x+woffset]-min) / maxOffset;
            tempVert.position[2] = z*scale;

            geometry.push_back(tempVert);

            tempVert.position[0] = (x+1) * scale;
            tempVert.position[1] = (lineData[x+woffset+1]-min) / maxOffset;
            tempVert.position[2] = z*scale;

            geometry.push_back(tempVert);

            tempVert.position[0] = x*scale;
            tempVert.position[1] = (lineData2[x+woffset]-min) / maxOffset;
            tempVert.position[2] = (z+1) * scale;

            geometry.push_back(tempVert);

            // push bottom row of triangles
            tempVert.position[0] = x*scale;
            tempVert.position[1] = (lineData2[x+woffset]-min) / maxOffset;
            tempVert.position[2] = (z+1) * scale;

            geometry.push_back(tempVert);

            tempVert.position[0] = (x+1) * scale;
            tempVert.position[1] = (lineData2[x+woffset+1]-min) / maxOffset;
            tempVert.position[2] = (z+1) * scale;

            geometry.push_back(tempVert);

            tempVert.position[0] = (x+1) * scale;
            tempVert.position[1] = (lineData[x+woffset+1]-min) / maxOffset;
            tempVert.position[2] = z*scale;

            geometry.push_back(tempVert);

        }
    }

    CPLFree(lineData);
    CPLFree(lineData2);
}

void Terrain::initGL(bool genBuffer)
{
    Vertex *geo = geometry.data();

    if(genBuffer)
        glGenBuffers(1, &vbo);


    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(*geo) * geometry.size(), geo, GL_STATIC_DRAW);

    loc_mvp = glGetUniformLocation(program, "mvpMatrix");
    loc_position = glGetAttribLocation(program, "v_position");
    loc_texture = glGetUniformLocation(program, "tex");
    loc_heightScalar = glGetUniformLocation(program, "heightScalar");
    loc_dataPoint = glGetAttribLocation(program, "dataPoint");
}


// static functions
QVector<Terrain*> Terrain::createTerrainFromDEMandMask(Engine *engine, const QString& dem, const QString& mask, Terrain *large_dem)
{
    QVector<Terrain*> terrain_vec(2);

    Terrain *dem_t = new Terrain(engine, dem, engine->graphics->getShaderProgram("gray"));
    Terrain *mask_t = new Terrain(engine, mask, engine->graphics->getShaderProgram("color"));

    auto t = dem.toLatin1();
    auto t2 = mask.toLatin1();
    GDALDataset *dataset = (GDALDataset*) GDALOpen(t.constData(), GA_ReadOnly);
    GDALDataset *dataset_mask = (GDALDataset*) GDALOpen(t2.constData(), GA_ReadOnly);

    if(dataset == nullptr) {
        qDebug() << "Unable to get GDAL Dataset for file: " << dem;
        exit(1);
    }

    if(dataset_mask == nullptr) {
        qDebug() << "Unable to get GDAL Dataset for mask file: " << mask;
        exit(1);
    }

    GDALRasterBand *raster = dataset->GetRasterBand(1);
    GDALRasterBand *raster_mask = dataset_mask->GetRasterBand(1);

    int width = raster->GetXSize();//terrain_img.getWidth();
    int height = raster->GetYSize();//terrain_img.getHeight();
    int width_mask = raster_mask->GetXSize();
    //int height_mask = raster_mask->GetYSize();

    int gotMin, gotMax;
    float large_min, large_max;
    float large_max_offset;

    if(large_dem) {
        GDALRasterBand *large_raster = large_dem->dataset->GetRasterBand(1);
        large_min = large_raster->GetMinimum(&gotMin);
        large_max = large_raster->GetMaximum(&gotMax);

        if(!(gotMin && gotMax)) {
            double minMax[2] = {large_min, large_max};
            GDALComputeRasterMinMax((GDALRasterBandH) large_raster, TRUE, minMax);

            large_min = minMax[0];
            large_max = minMax[1];
        }

        large_max_offset = large_max - large_min;
    }

    float min = raster->GetMinimum(&gotMin);
    float max = raster->GetMaximum(&gotMax);

    double minMax[2] = {min, max};

    if(!(gotMin && gotMax)) {
        GDALComputeRasterMinMax((GDALRasterBandH) raster, TRUE, minMax);

        min = minMax[0];
        max = minMax[1];
    }

    float min_mask = raster_mask->GetMinimum(&gotMin);
    float max_mask = raster_mask->GetMaximum(&gotMax);

    minMax[0] = min_mask;
    minMax[1] = max_mask;

    if(!(gotMin && gotMax)) {
        GDALComputeRasterMinMax((GDALRasterBandH) raster_mask, TRUE, minMax);

        min_mask = minMax[0];
        max_mask = minMax[1];
    }

    if(engine->getOptions().verbose) {
        qDebug() << "terrain: " << dem << "   min: " << min << " max: " << max;
        qDebug() << "terrain mask: " << mask << "   min: " << min_mask << " max: " << max_mask;
    }

    int woffset = width / 2;
    int hoffset = height / 2;

    Vertex tempVert;

    float maxOffset = max - min;
    float maxOffset_mask = max_mask - min_mask;

    float scale = engine->getOptions().map_scalar * (2.5f / 10.0f);
    float *lineData = (float*) CPLMalloc(sizeof(float) * width);
    float *lineData2 = (float*) CPLMalloc(sizeof(float) * width);
    float *lineData_mask = (float*) CPLMalloc(sizeof(float) * width_mask);
    float *lineData2_mask = (float*) CPLMalloc(sizeof(float) * width_mask);

    if(!large_dem) {
        large_max_offset = maxOffset;
        large_min = min;
    }

    for(int z = -hoffset; z < height - hoffset-1; z++) {
        raster->RasterIO(GF_Read, 0, z + hoffset, width, 1, lineData, width, 1, GDT_Float32, 0, 0);
        raster->RasterIO(GF_Read, 0, z + hoffset + 1, width, 1, lineData2, width, 1, GDT_Float32, 0, 0);
        raster_mask->RasterIO(GF_Read, 0, z + hoffset, width_mask, 1, lineData_mask, width_mask, 1, GDT_Float32, 0, 0);
        raster_mask->RasterIO(GF_Read, 0, z + hoffset + 1, width_mask, 1, lineData2_mask, width_mask, 1, GDT_Float32, 0, 0);

        for(int x = -woffset; x < width - woffset-1; x++) {

            if((((lineData_mask[x+woffset]-min_mask) / maxOffset_mask) >= 0.1f) && (((lineData_mask[x+woffset+1]-min_mask) / maxOffset_mask) >= 0.1f)
                    && (((lineData2_mask[x+woffset]-min_mask) / maxOffset_mask) >= 0.1f)) {

                tempVert.position[0] = x*scale;
                tempVert.position[1] = (lineData[x+woffset]-large_min) / large_max_offset;
                tempVert.position[2] = z*scale;
                mask_t->geometry.push_back(tempVert);

                tempVert.position[0] = (x+1) * scale;
                tempVert.position[1] = (lineData[x+woffset+1]-large_min) / large_max_offset;
                tempVert.position[2] = z*scale;
                mask_t->geometry.push_back(tempVert);

                tempVert.position[0] = x*scale;
                tempVert.position[1] = (lineData2[x+woffset]-large_min) / large_max_offset;
                tempVert.position[2] = (z+1) * scale;
                mask_t->geometry.push_back(tempVert);
            }

            else {
                tempVert.position[0] = x*scale;
                tempVert.position[1] = (lineData[x+woffset]-large_min) / large_max_offset;
                tempVert.position[2] = z*scale;
                dem_t->geometry.push_back(tempVert);

                tempVert.position[0] = (x+1) * scale;
                tempVert.position[1] = (lineData[x+woffset+1]-large_min) / large_max_offset;
                tempVert.position[2] = z*scale;
                dem_t->geometry.push_back(tempVert);

                tempVert.position[0] = x*scale;
                tempVert.position[1] = (lineData2[x+woffset]-large_min) / large_max_offset;
                tempVert.position[2] = (z+1) * scale;
                dem_t->geometry.push_back(tempVert);
            }

            if((((lineData2_mask[x+woffset]-min_mask) / maxOffset_mask) >= 0.1f) && (((lineData2_mask[x+woffset+1]-min_mask) / maxOffset_mask) >= 0.1f)
                    && (((lineData_mask[x+woffset+1]-min_mask) / maxOffset_mask) >= 0.1f)) {

                tempVert.position[0] = x*scale;
                tempVert.position[1] = (lineData2[x+woffset]-large_min) / large_max_offset;
                tempVert.position[2] = (z+1) * scale;
                mask_t->geometry.push_back(tempVert);

                tempVert.position[0] = (x+1) * scale;
                tempVert.position[1] = (lineData2[x+woffset+1]-large_min) / large_max_offset;
                tempVert.position[2] = (z+1) * scale;
                mask_t->geometry.push_back(tempVert);

                tempVert.position[0] = (x+1) * scale;
                tempVert.position[1] = (lineData[x+woffset+1]-large_min) / large_max_offset;
                tempVert.position[2] = z*scale;
                mask_t->geometry.push_back(tempVert);
            }

            else {
                tempVert.position[0] = x*scale;
                tempVert.position[1] = (lineData2[x+woffset]-large_min) / large_max_offset;
                tempVert.position[2] = (z+1) * scale;
                dem_t->geometry.push_back(tempVert);

                tempVert.position[0] = (x+1) * scale;
                tempVert.position[1] = (lineData2[x+woffset+1]-large_min) / large_max_offset;
                tempVert.position[2] = (z+1) * scale;
                dem_t->geometry.push_back(tempVert);

                tempVert.position[0] = (x+1) * scale;
                tempVert.position[1] = (lineData[x+woffset+1]-large_min) / large_max_offset;
                tempVert.position[2] = z*scale;
                dem_t->geometry.push_back(tempVert);
            }

        }
    }

    CPLFree(lineData);
    CPLFree(lineData2);
    CPLFree(lineData_mask);
    CPLFree(lineData2_mask);
    //GDALClose((GDALDatasetH) dataset);
    //GDALClose((GDALDatasetH) dataset_mask);

    dem_t->dataset = dataset;
    mask_t->dataset = dataset_mask;

    dem_t->initGL();
    mask_t->initGL();
    mask_t->textures.push_back(engine->graphics->createTextureFromFile(QString::fromStdString(engine->getOptions().color_map),
        GL_TEXTURE_1D));

    terrain_vec[0] = dem_t;
    terrain_vec[1] = mask_t;

    return terrain_vec;
}

void Terrain::applyDataset(const QString& file)
{
    auto t = file.toLatin1();
    GDALDataset *dataset_data = (GDALDataset*) GDALOpen(t.constData(), GA_ReadOnly);
    GDALDataset *dataset_mask = dataset;//(GDALDataset*) GDALOpen(map_file.toLatin1()constData(), GA_ReadOnly);

    if(dataset == nullptr) {
        qDebug() << "Unable to get GDAL Dataset for data file: " << file;
        exit(1);
    }

    if(dataset_mask == nullptr) {
        qDebug() << "Unable to get GDAL Dataset for mask file: " << map_file;
        exit(1);
    }

    GDALRasterBand *raster = dataset_data->GetRasterBand(1);
    GDALRasterBand *raster_mask = dataset_mask->GetRasterBand(1);

    int width = raster->GetXSize();//terrain_img.getWidth();
    int height = raster->GetYSize();//terrain_img.getHeight();
    int width_mask = raster_mask->GetXSize();
    //int height_mask = raster_mask->GetYSize();

    int gotMin, gotMax;

    float min = raster->GetMinimum(&gotMin);
    float max = raster->GetMaximum(&gotMax);

    double minMax[2] = {min, max};

    if(!(gotMin && gotMax)) {
        GDALComputeRasterMinMax((GDALRasterBandH) raster, TRUE, minMax);

        min = minMax[0];
        max = minMax[1];
    }

    float min_mask = raster_mask->GetMinimum(&gotMin);
    float max_mask = raster_mask->GetMaximum(&gotMax);

    minMax[0] = min_mask;
    minMax[1] = max_mask;

    if(!(gotMin && gotMax)) {
        GDALComputeRasterMinMax((GDALRasterBandH) raster_mask, TRUE, minMax);

        min_mask = minMax[0];
        max_mask = minMax[1];
    }

    if(engine->getOptions().verbose) {
        qDebug() << "terrain mask: " << map_file << "   min: " << min << " max: " << max;
        qDebug() << "terrain data: " << file << "   min: " << min_mask << " max: " << max_mask;
    }

    int woffset = width / 2;
    int hoffset = height / 2;

    float maxOffset = max - min;
    float maxOffset_mask = max_mask - min_mask;

    float *lineData = (float*) CPLMalloc(sizeof(float) * width);
    float *lineData2 = (float*) CPLMalloc(sizeof(float) * width);
    float *lineData_mask = (float*) CPLMalloc(sizeof(float) * width_mask);
    float *lineData2_mask = (float*) CPLMalloc(sizeof(float) * width_mask);

    int i = 0;
    for(int z = -hoffset; z < height - hoffset-1; z++) {
        raster->RasterIO(GF_Read, 0, z + hoffset, width, 1, lineData, width, 1, GDT_Float32, 0, 0);
        raster->RasterIO(GF_Read, 0, z + hoffset + 1, width, 1, lineData2, width, 1, GDT_Float32, 0, 0);
        raster_mask->RasterIO(GF_Read, 0, z + hoffset, width_mask, 1, lineData_mask, width_mask, 1, GDT_Float32, 0, 0);
        raster_mask->RasterIO(GF_Read, 0, z + hoffset + 1, width_mask, 1, lineData2_mask, width_mask, 1, GDT_Float32, 0, 0);

        for(int x = -woffset; x < width - woffset-1; x++) {

            if((((lineData_mask[x+woffset]-min_mask) / maxOffset_mask) >= 0.1f) && (((lineData_mask[x+woffset+1]-min_mask) / maxOffset_mask) >= 0.1f)
                    && (((lineData2_mask[x+woffset]-min_mask) / maxOffset_mask) >= 0.1f)) {


                geometry[i].dataPoint = (lineData[x+woffset]-min) / maxOffset;
                i++;
                geometry[i].dataPoint = (lineData[x+woffset+1]-min) / maxOffset;
                i++;
                geometry[i].dataPoint = (lineData2[x+woffset]-min) / maxOffset;
                i++;
            }


            if((((lineData2_mask[x+woffset]-min_mask) / maxOffset_mask) >= 0.1f) && (((lineData2_mask[x+woffset+1]-min_mask) / maxOffset_mask) >= 0.1f)
                    && (((lineData_mask[x+woffset+1]-min_mask) / maxOffset_mask) >= 0.1f)) {

                geometry[i].dataPoint = (lineData2[x+woffset]-min) / maxOffset;
                i++;
                geometry[i].dataPoint = (lineData2[x+woffset+1]-min) / maxOffset;
                i++;
                geometry[i].dataPoint = (lineData[x+woffset+1]-min) / maxOffset;
                i++;
            }

        }
    }

    CPLFree(lineData);
    CPLFree(lineData2);
    CPLFree(lineData_mask);
    CPLFree(lineData2_mask);
    //GDALClose((GDALDatasetH) dataset);
    //GDALClose((GDALDatasetH) dataset_mask);
    program = engine->graphics->getShaderProgram("data");
    initGL(false);
}

std::pair<double,double> Terrain::getGeoTransformFromDEMs(Terrain *large, Terrain *small)
{
    QString proj = large->dataset->GetProjectionRef();

    OGRSpatialReference sr1;
    auto t = proj.toLatin1();
    char *test = t.data();
    sr1.importFromWkt(&test);

    proj = small->dataset->GetProjectionRef();

    t = proj.toLatin1();
    OGRSpatialReference sr2;
    test = t.data();
    sr2.importFromWkt(&test);

    OGRCoordinateTransformation* poTransform = OGRCreateCoordinateTransformation( &sr2, &sr1 );
    large->geot.resize(6);
    small->geot.resize(6);
    large->dataset->GetGeoTransform(large->geot.data());
    small->dataset->GetGeoTransform(small->geot.data());
    //
    double x = small->geot[0];
    double y = small->geot[3];
    // DCEWsqrext.tif upperleft hand corner is convert to tl2p5_dem.tif coordinate system.
    poTransform->Transform (1, &x, &y);

    return std::pair<double,double>(x,y);
}

void Terrain::translate(const glm::vec3& vec)
{
    model = glm::translate(model, vec);
}
