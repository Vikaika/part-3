#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QFileDialog>
#include <QFile>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QList>
#include <QDataStream>
#include <QAction>
#include <QMenuBar>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QVBoxLayout>


//перечисление форм (фигур)
enum Shape {None, Rectangle, Triangle, Ellipse, Line, Move, Delete, Connect };

//структура описывающая фигуру
struct Figure {
    Shape shape;
    QRect rect;

    //сравнение фигур
    bool operator==(const Figure &other) const {
        return shape == other.shape && rect == other.rect;
    }
};

//перегрузка потокового ввода/вывода для структуры Figure
QDataStream &operator<<(QDataStream &out, const Figure &figure);
QDataStream &operator>>(QDataStream &in, Figure &figure);

//производный класс
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr); //конструктор с указателем на родительский виджет
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override; //обработка нажатия мыши
    void mouseMoveEvent(QMouseEvent *event) override; //обработка перемещения мыши
    void mouseReleaseEvent(QMouseEvent *event) override; //обработка отпускания мыши
    void keyPressEvent(QKeyEvent *event) override; //обработка нажатий клавиш на клавиатуре

private slots:
    //слоты для режима рисования фигур
    void set_rectangle();
    void set_triangle();
    void set_ellipse();
    void set_move();
    void set_delete();
    void set_connect();
    void save_to_file();
    void load_from_file();
    //слот для очистки всех фигур
    void clear_all();

private:
    Shape current_shape;
    QPoint startPoint, endPoint;
    QPoint last_pos; //последняя позиция мыши
    QPoint connect_startPoint;
    QMap<int, QSet<int>> graph; //граф связей между фигурами

    QVector<Figure> figures; //хранение фигур созданных пользователем
    QVector<QPair<QPoint, QPoint>> connections; //пары точек для соединения
    Figure *moving_figure;

    bool connecting;
    void moveConnectedFigures(int index, const QPoint &delta); //перемещение связных фигур
    void updateGraph(); //обновление графа связей
    void addFigure(const Figure &figure);
};

#endif // MAINWINDOW_H
