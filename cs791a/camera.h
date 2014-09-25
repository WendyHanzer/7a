#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Engine;

class Camera
{
public:
    Camera(Engine *eng);

    void moveForward();
    void moveBackward();
    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();

    void rotate(float x, float y);
    void update();

    glm::mat4 getView() const;

private:
    Engine *engine;
    glm::vec3 pos, orientation ,up;
    float rotateX, rotateY, speed, sensitivity;
};

#endif // CAMERA_H
