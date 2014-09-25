#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Engine;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Engine *eng);
    ~MainWindow();

protected slots:
    void keyPressEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    Engine *engine;
    int previousX, previousY;
};

#endif // MAINWINDOW_H
