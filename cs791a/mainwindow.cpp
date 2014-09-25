#include "mainwindow.h"
#include "engine.h"
#include "graphics.h"
#include "camera.h"

#include <QDebug>
#include <QKeyEvent>
#include <QApplication>

MainWindow::MainWindow(Engine *eng)
    : QMainWindow(), engine(eng)
{
    setMouseTracking(true);
}

MainWindow::~MainWindow()
{

}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
        case Qt::Key_Escape:
            engine->stop(0);
        break;

        case Qt::Key_W:
            engine->graphics->camera->moveForward();
        break;

        case Qt::Key_A:
            engine->graphics->camera->moveLeft();
        break;

        case Qt::Key_S:
            engine->graphics->camera->moveBackward();
        break;

        case Qt::Key_D:
            engine->graphics->camera->moveRight();
        break;

        case Qt::Key_R:
            engine->graphics->camera->moveUp();
        break;

        case Qt::Key_F:
            engine->graphics->camera->moveDown();
        break;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    engine->graphics->camera->rotate(event->x() - previousX, event->y() - previousY);

    previousX = event->x();
    previousY = event->y();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    previousX = event->x();
    previousY = event->y();
    setCursor(Qt::BlankCursor);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
}
