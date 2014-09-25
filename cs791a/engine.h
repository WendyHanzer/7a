#ifndef ENGINE_H
#define ENGINE_H

#include <vector>
#include <string>

#include <QTimer>

class MainWindow;
class Graphics;

struct Options {
    bool verbose;
    std::string color_map;
    std::vector<std::string> terrain;
    std::vector<std::string> shapes;
    float height_scalar;
    float map_scalar;
    bool wireframe;
    float camera_sensitivity;
    float camera_speed;
    std::string data_directory;
};

class Engine
{
public:
    Engine(int argc, char **argv);
    ~Engine();

    void init();
    void stop(int exit_code = 0);

    const Options& getOptions() const {return options;}

    Graphics *graphics;
private:
    void parseArgs();

    MainWindow *window;

    Options options;

    int _argc;
    char **_argv;

    QTimer timer;
};

#endif // ENGINE_H
