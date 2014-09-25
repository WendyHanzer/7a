#include "shape.h"
#include "engine.h"
#include "vertex.h"
#include "terrain.h"
#include "graphics.h"

#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QDebug>

Shape::Shape(Engine *eng, const QString &shape_file, Terrain *large_dem)
    : engine(eng)
{
    auto t = shape_file.toLatin1();
    OGRDataSource* ds = OGRSFDriverRegistrar::Open( t.constData(), FALSE );

    // Do a check
    if(ds == nullptr)
    {
        qDebug() << "Unable to open shape file: " << shape_file;
        engine->stop(1);
    }
    // Now lets grab the first layer
    OGRLayer *layer = ds->GetLayer(0);

    // Grab the spatial reference and create a coordinate transform function
    //OGRSpatialReference *sr = layer->GetSpatialRef();

    // Get Geography Coordinate System
    //OGRSpatialReference *geog = sr->CloneGeogCS();

    // Now to create coordinate transform function
    //OGRCoordinateTransformation *poTransform = OGRCreateCoordinateTransformation( sr, geog );


    // Taking from http://www.compsci.wm.edu/SciClone/documentation/software/geo/gdal-1.9.0/html/ogr/ogr_apitut.html
    OGRFeature *poFeature;

    float xOffset = large_dem->getDataset()->GetRasterXSize() / 2;
    float zOffset = large_dem->getDataset()->GetRasterYSize() / 2;
    auto geot = large_dem->getGeot();

    Vertex tempVert;

    GDALRasterBand *raster = large_dem->getDataset()->GetRasterBand(1);

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

    float maxOffset = max - min;

    QVector<QVector<float>> pixels(height, QVector<float>(width));

    for(int i = 0; i < height; i++) {
        raster->RasterIO(GF_Read, 0, i, width, 1, pixels[i].data(), width, 1, GDT_Float32, 0, 0);
    }

    layer->ResetReading();
    while( (poFeature = layer->GetNextFeature()) != nullptr )
    {
        //OGRFeatureDefn *poFDefn = layer->GetLayerDefn();

        OGRGeometry *poGeometry;

        if(shape_file.contains("bound")) {
            poGeometry = poFeature->GetGeometryRef()->Boundary();

            color[0] = 1;
            color[1] = 0;
            color[2] = 0;
            color[3] = 1;
        }

        else {
            poGeometry = poFeature->GetGeometryRef();

            color[0] = 0;
            color[1] = 0;
            color[2] = 1;
            color[3] = 1;
        }

        if( poGeometry != nullptr
            && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
        {
            //OGRPoint *poPoint = (OGRPoint *) poGeometry;

            //printf( "%.3f,%3.f\n", poPoint->getX(), poPoint->getY() );
        }
        else if (poGeometry != nullptr
            && wkbFlatten(poGeometry->getGeometryType()) == wkbLineString)
        {
            OGRLineString* ls = (OGRLineString*)poGeometry;
            for(int i = 0; i < ls->getNumPoints(); i++ )
            {
                OGRPoint p;
                ls->getPoint(i,&p);

                // This function can transform a larget set of points.....
                float x = p.getX() - geot[0];
                float z = p.getY() - geot[3];

                tempVert.position[0] = (x / 10) - xOffset;
                //tempVert.position[1] = 1.f;
                tempVert.position[2] = (-z / 10) - zOffset;


                tempVert.position[1] = ((pixels[(-z / 10)][(x / 10)] - min) / maxOffset) + 0.04;
                qDebug() << -z << ' ' << x;
                //qDebug() << tempVert.position[0] << ' ' << tempVert.position[1] << ' ' << tempVert.position[2];

                points.push_back(tempVert);

                //qDebug() << x << ' ' << -z;
                //poTransform->Transform (1, &x, &y);
                //qDebug() << p.getX() << " " << p.getY()  << "Transformed!: " << x << " " << y;
            }
        }
        OGRFeature::DestroyFeature( poFeature );
    }

    OGRDataSource::DestroyDataSource( ds );

    init();
}

void Shape::init()
{
    initGL();
}

void Shape::initGL()
{
    program = engine->graphics->getShaderProgram("shape");
    Vertex *geo = points.data();

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(*geo) * points.size(), geo, GL_STATIC_DRAW);

    loc_mvp = glGetUniformLocation(program, "mvpMatrix");
    loc_position = glGetAttribLocation(program, "v_position");
    loc_heightScalar = glGetUniformLocation(program, "heightScalar");
    loc_color = glGetUniformLocation(program, "lineColor");

    //qDebug() << loc_mvp << ' ' << loc_position << ' ' << loc_heightScalar << ' ' << loc_color;
}

void Shape::tick(float dt)
{
    Q_UNUSED(dt);
}

void Shape::render()
{
    glm::mat4 mvp = engine->graphics->projection * engine->graphics->view * model;

    glUseProgram(program);

    glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, glm::value_ptr(mvp));

    //glEnableVertexAttribArray(loc_heightScalar);
    glUniform1f(loc_heightScalar, engine->getOptions().height_scalar);
    glUniform4fv(loc_color, 1, color);

    glEnableVertexAttribArray(loc_position);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer( loc_position,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           sizeof(Vertex),
                           (void*)offsetof(Vertex,position));


    // draw object
    glDrawArrays(GL_POINTS, 0, points.size());//mode, starting index, count

    // disable attribute pointers
    glDisableVertexAttribArray(loc_position);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glUseProgram(0);
}
