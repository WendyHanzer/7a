#include "engine.h"
#include "mainwindow.h"
#include "graphics.h"

#include <QDebug>
#include <QGLFormat>
#include <QApplication>
#include <QTimer>

#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include <iostream>

#include <boost/program_options.hpp>

Engine::Engine(int argc, char **argv)
    : _argc(argc), _argv(argv)
{
    parseArgs();
    init();
}

Engine::~Engine()
{
    delete window;
}

void Engine::init()
{
#ifdef __APPLE__
    int width = 1024, height = 768;
#else
    int width = 1600, height = 900;
#endif

    GDALAllRegister();
    OGRRegisterAll();

    window = new MainWindow(this);
    window->setWindowTitle("CS791a");
    window->resize(width, height);

    QGLFormat format;
    format.setDoubleBuffer(true);
    format.setDirectRendering(true);
    format.setProfile(QGLFormat::CoreProfile);
    format.setVersion(4,1);

    QGLFormat::setDefaultFormat(format);

    graphics = new Graphics(this);

    window->setCentralWidget(graphics);
    window->show();

    timer.setInterval(1000 / 60);
    timer.setSingleShot(false);
    QObject::connect(&timer, &QTimer::timeout, graphics, &Graphics::paintGL);
    timer.start();
}

void Engine::stop(int exit_code)
{
    QApplication::exit(exit_code);
}

void Engine::parseArgs()
{
    using namespace boost;

    program_options::options_description desc("CS791a Program Options");
    program_options::variables_map vm;

    desc.add_options()
            ("help,h", "Print Help Message")
            ("verbose,v", "Set Program Output to Verbose")
            ("scalar,s", program_options::value<float>(&options.height_scalar)->default_value(2.0f), "Height Scalar Value")
            ("map-scalar,m", program_options::value<float>(&options.map_scalar)->default_value(1.0f), "Map Scalar Value")
            ("texture,t", program_options::value<std::string>(&options.color_map)->default_value("../colorMap.png"), "Texture File")
            ("terrain", program_options::value<std::vector<std::string>>(&options.terrain)->required(), "Terrain File")
            ("wireframe,w", "Only Render Wireframes")
            ("sensitivity", program_options::value<float>(&options.camera_sensitivity)->default_value(0.1f), "Mouse Sensitivity")
            ("speed", program_options::value<float>(&options.camera_speed)->default_value(5.0f), "Camera Speed")
            ("data,d", program_options::value<std::string>(&options.data_directory)->default_value("../DryCreek/isnobaloutput/"), "Data Directory")
            ("shape,a", program_options::value<std::vector<std::string>>(&options.shapes), "Shape Files");

        program_options::positional_options_description pos;
        pos.add("terrain", -1);
        //pos.add("texture", 1);

        program_options::store(program_options::command_line_parser(_argc, _argv).options(desc).positional(pos).run(), vm);

        if(vm.count("help")) {
            std::cout << "CS791a Engine Help\n\n";
            std::cout << desc << std::endl;

            exit(0);
        }

        try {
            program_options::notify(vm);
        }
        catch(program_options::error& e) {
            std::cerr << "Command Line Error: " << e.what() << std::endl;
            exit(1);
        }

        if(options.shapes.empty()) {
            options.shapes.push_back("../DryCreek/streamDCEW/streamDCEW.shp");
            options.shapes.push_back("../DryCreek/boundDCEW/boundDCEW.shp");
        }

        options.verbose = vm.count("verbose");
        options.wireframe = vm.count("wireframe");
}
