#include "mainwindow.h"

#include <QDataStream>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent), current_shape(None), moving_figure(nullptr), connecting(false)
{
    //создание панели инструментов
    QToolBar *tool_bar = addToolBar("Shapes");
    //настройка стиля панели инструментов
    tool_bar->setStyleSheet("QToolButton { border-radius: 10px; padding: 10px; }"
                           "QToolButton:pressed { background-color: #0078d7; border: 1px solid #005299; }");
    //добавление действий на панель инструментов
    QAction *rectangle_act = tool_bar->addAction("Rectangle");
    QAction *triangle_act = tool_bar->addAction("Triangle");
    QAction *ellipse_act = tool_bar->addAction("Ellipse");
    QAction *move_act = tool_bar->addAction("Move");
    QAction *delete_act = tool_bar->addAction("Delete");
    QAction *connect_act = tool_bar->addAction("Connect");
    QAction *clear_action = tool_bar->addAction("Clear All");

    //подключение сигналов действий к слотам класса MainWindow
    connect(rectangle_act, &QAction::triggered, this, &MainWindow::set_rectangle);
    connect(triangle_act, &QAction::triggered, this, &MainWindow::set_triangle);
    connect(ellipse_act, &QAction::triggered, this, &MainWindow::set_ellipse);
    connect(move_act, &QAction::triggered, this, &MainWindow::set_move);
    connect(delete_act, &QAction::triggered, this, &MainWindow::set_delete);
    connect(connect_act, &QAction::triggered, this, &MainWindow::set_connect);
    connect(clear_action, &QAction::triggered, this, &MainWindow::clear_all);

    //создание меню файла и добавление действий сохранения и загрузки
    QMenuBar *menu_bar = new QMenuBar(this); //создание строки меню
    //создается строка меню с пунктом "File", в который добавляются действия "Save" и "Load"
    QMenu *file_menu = menu_bar->addMenu("File");
    file_menu->addAction("Save", this, &MainWindow::save_to_file);
    file_menu->addAction("Load", this, &MainWindow::load_from_file);
    setMenuBar(menu_bar);

    resize(800, 600); //начальный размера окна
}

MainWindow::~MainWindow() {}

//обработка событий рисования
void MainWindow::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    //отрисовка фигур
    //перебираем все фигуры
    for (const Figure &figure : figures){
        switch (figure.shape){
        case Rectangle:
            //рис прямоугольник
            painter.drawRect(figure.rect);
            break;
        case Triangle: {
            //рис треугольник на основе границ прямоугольника
            QPoint points[3] = {
                QPoint(figure.rect.left(), figure.rect.bottom()),
                QPoint(figure.rect.right(), figure.rect.bottom()),
                QPoint(figure.rect.center().x(), figure.rect.top())
            };
            painter.drawPolygon(points, 3);
            break; }
        case Ellipse:
            //рис эллипс на основе прямоугольника
            painter.drawEllipse(figure.rect);
            break;
        default:
            break;
        }
    }
    //отрисовка всех связей
    for (const auto &connection : connections) {
        painter.drawLine(connection.first, connection.second);
    }

    //проверка, какая фигура в данный момент создается
    if (current_shape == Rectangle || current_shape == Triangle || current_shape == Ellipse) {
        QRect rect(startPoint, endPoint);
        switch (current_shape) {
        case Rectangle:
            painter.drawRect(rect);
            break;
        case Triangle: {
            QPoint points[3] = {
                QPoint(rect.left(), rect.bottom()),
                QPoint(rect.right(), rect.bottom()),
                QPoint(rect.center().x(), rect.top())
            };
            painter.drawPolygon(points, 3);
            break;
        }
        case Ellipse:
            painter.drawEllipse(rect);
            break;
        default:
            break;
        }
    }
    //проверяет, создается ли линия связи
    if (current_shape == Connect && connecting) {
        painter.drawLine(connect_startPoint, endPoint);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (current_shape == Move && moving_figure) {
        // Перемещение фигуры
        QPoint delta = event->pos() - last_pos; //Вычисляет разницу между текущим положением мыши и последним записанным положением мыши ( lastMousePos).
        int index = figures.indexOf(*moving_figure);//Находит индекс перемещаемой фигуры в figures списке
        if (index != -1) {
            moveConnectedFigures(index, delta); //Вызывает moveConnectedFigures() функцию, передавая индекс фигуры и вычисленную дельту, для обновления позиций всех связанных фигур.
            last_pos = event->pos(); //Обновляет lastMousePos переменную с учетом текущего положения мыши.
            update();  // Перерисовка только после перемещения фигуры
        }
    } else if (current_shape == Connect && connecting) {
        // Обновление конечной точки связи
        endPoint = event->pos(); //Обновляет endPoin tпеременную с учетом текущего положения мыши.
        update();  // Перерисовка для обновления конечной точки
    } else {
        // Обновление конечной точки для создания фигуры
        endPoint = event->pos();
        update();  // Перерисовка для обновления конечной точки
    }

    // Закрепление начальной точки для создания связи
    if (current_shape == Connect && event->buttons() & Qt::LeftButton && !connecting)
    {
        // Проверяем, что текущая фигура - "Connect" и нажата левая кнопка мыши, но пользователь еще не начал процесс создания связи
        for (int i = 0; i < figures.size(); ++i)
        {
            // Итерируемся по списку фигур
            if (figures.at(i).rect.contains(event->pos()))
            {
                // Проверяем, что текущая позиция мыши находится внутри прямоугольника текущей фигуры
                connect_startPoint = figures.at(i).rect.center(); // Устанавливаем начальную точку связи в центр прямоугольника фигуры
                connecting = true; // Устанавливаем флаг connecting в true, показывая, что пользователь начал процесс создания связи
                break; // Выходим из цикла, так как нашли начальную точку
            }
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    //функция сначала проверяет, отпустил ли пользователь левую кнопку мыши, проверяя event->button() значение.
    // Затем функция сохраняет текущее положение мыши ( event->pos())endPoint.
    if (event->button() == Qt::LeftButton) {
        endPoint = event->pos();
        QRect rect(startPoint, endPoint);
        //В зависимости от текущего currentShape значения функция выполняет различные действия:
        //addFigure()функция, передающая тип фигуры и прямоугольник, определяемые с помощью startPointи endPoint.
        switch (current_shape) {
        case Rectangle:
        case Triangle:
        case Ellipse:
            // Добавление новой фигуры
            addFigure({ current_shape, rect });
            break;
        case Connect:
            if (connecting) {
                // Завершение создания связи
                for (auto &figure : figures) {
                    if (figure.rect.contains(endPoint) && figure.rect.center() != connect_startPoint) {
                        connections.append(qMakePair(connect_startPoint, figure.rect.center()));
                        updateGraph();
                        break;
                    }
                }
                connecting = false;
            }
            break;
        default:
            break;
        }
        //После выполнения необходимых действийupdate()метод инициирования перерисовки главного окна приложения.
        update();
    }
    //функция устанавливает movingFigureуказатель на nullptr, указывающий на то, что в данный момент ни одна фигура не перемещается.
    moving_figure = nullptr;
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    // Если нажата левая кнопка мыши
    if (event->button() == Qt::LeftButton) {
        // Сохраняем начальную точку нажатия
        startPoint = event->pos();

        // Проверяем текущий режим
        if (current_shape == Move) {
            // Поиск фигуры для перемещения
            moving_figure = nullptr;
            for (auto &figure : figures) {
                // Если фигура содержит точку нажатия
                if (figure.rect.contains(startPoint)) {
                    // Помечаем эту фигуру как перемещаемую
                    moving_figure = &figure;
                    // Сохраняем последнюю позицию мыши
                    last_pos = event->pos();
                    break;
                }
            }
        } else if (current_shape == Connect) {
            // Начало создания связи
            connecting = false;
            for (auto &figure : figures) {
                // Если фигура содержит точку нажатия
                if (figure.rect.contains(startPoint)) {
                    // Устанавливаем флаг, что создание связи началось
                    connecting = true;
                    // Сохраняем центральную точку фигуры как начало связи
                    connect_startPoint = figure.rect.center();
                    break;
                }
            }
        } else if (current_shape == Delete) {
            // Удаление фигуры и ее связей
            QList<QPair<QPoint, QPoint>> toRemoveConnections;
            for (auto &figure : figures) {
                // Если фигура содержит точку нажатия
                if (figure.rect.contains(startPoint)) {
                    // Удаление связей, в которых участвует удаляемая фигура
                    for (auto it = connections.begin(); it != connections.end(); ) {
                        if (it->first == figure.rect.center() || it->second == figure.rect.center()) {
                            it = connections.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    // Удаление самой фигуры
                    figures.removeOne(figure);
                    updateGraph();
                    update();
                    break; // Выходим после удаления первой найденной фигуры
                }
            }
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        //отмена текущего режима
        current_shape = None;
        moving_figure = nullptr;
        connecting = false;
        update();
    }
}

void MainWindow::set_rectangle() {
    current_shape = Rectangle;
}

void MainWindow::set_triangle() {
    current_shape = Triangle;
}

void MainWindow::set_ellipse() {
    current_shape = Ellipse;
}

void MainWindow::set_move() {
    current_shape = Move;
}

void MainWindow::set_delete() {
    current_shape = Delete;
}

void MainWindow::set_connect() {
    current_shape = Connect;
}

void MainWindow::save_to_file() {
    // 1. Открытие диалога сохранения файла
    QString fileName = QFileDialog::getSaveFileName(this, "Save File", "", "Text Files (*.txt)");

    // 2. Проверка, был ли выбран файл для сохранения
    if (!fileName.isEmpty()) {
        // 3. Создание объекта файла с выбранным именем
        QFile file(fileName);

        // 4. Открытие файла в режиме записи
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // 5. Создание объекта текстового потока для записи данных в файл
            QTextStream out(&file);

            // 6. Записываем количество фигур и количество связей
            out << "Figures: " << figures.size() << "\n";
            out << "Connections: " << connections.size() << "\n";

            // 7. Запись данных фигур в файл
            for (const auto& figure : figures) {
                QString shapeType;
                switch (figure.shape) {
                case Rectangle: shapeType = "Rectangle"; break;
                case Triangle: shapeType = "Triangle"; break;
                case Ellipse: shapeType = "Ellipse"; break;
                default: shapeType = "Unknown"; break;
                }
                out << shapeType << ": " << figure.rect.left() << " " << figure.rect.top() << " "
                    << figure.rect.width() << " " << figure.rect.height() << "\n";
            }

            // 8. Запись данных связей в файл
            for (const auto& connection : connections) {
                out << "Connection: " << connection.first.x() << " " << connection.first.y() << " "
                    << connection.second.x() << " " << connection.second.y() << "\n";
            }

            // 9. Закрытие файла
            file.close();

            qDebug() << "Файл успешно сохранен и закрыт.";
        } else {
            qDebug() << "Ошибка: не удалось открыть файл для записи";
        }
    } else {
        qDebug() << "Ошибка: не выбран файл для сохранения";
    }
}

void MainWindow::load_from_file() {
    //Открытие диалога выбора файла для загрузки
    QString fileName = QFileDialog::getOpenFileName(this, "Load File", "", "Text Files (*.txt)");
    //Проверка, был ли выбран файл для загрузки
    if (!fileName.isEmpty()) {
        //Создание объекта файла с выбранным именем
        QFile file(fileName);
        //Открытие файла в режиме чтения
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //Создание объекта текстового потока для чтения данных из файла
            QTextStream in(&file);
            //Считываем количество фигур и количество связей
            QString line = in.readLine();
            int figureCount = 0;
            int connectionCount = 0;

            if (line.startsWith("Figures: ")) {
                figureCount = line.mid(QString("Figures: ").length()).toInt();
            }

            line = in.readLine();
            if (line.startsWith("Connections: ")) {
                connectionCount = line.mid(QString("Connections: ").length()).toInt();
            }

            qDebug() << "Загружено фигур: " << figureCount;
            qDebug() << "Загружено связей: " << connectionCount;

            figures.clear();
            connections.clear();

            //Чтение данных фигур из файла
            for (int i = 0; i < figureCount; ++i) {
                line = in.readLine();
                QStringList parts = line.split(": ");
                if (parts.size() == 2) {
                    QString shapeType = parts[0];
                    QStringList rectParts = parts[1].split(" ");
                    if (rectParts.size() == 4) {
                        QRect rect(rectParts[0].toInt(), rectParts[1].toInt(),
                                   rectParts[2].toInt(), rectParts[3].toInt());
                        Shape shape = None;
                        if (shapeType == "Rectangle") shape = Rectangle;
                        else if (shapeType == "Triangle") shape = Triangle;
                        else if (shapeType == "Ellipse") shape = Ellipse;

                        if (shape != None) {
                            figures.append({ shape, rect });
                        }
                    }
                }
            }
            //Чтение данных связей из файла
            for (int i = 0; i < connectionCount; ++i) {
                line = in.readLine();
                QStringList parts = line.split(": ");
                if (parts.size() == 2 && parts[0] == "Connection") {
                    QStringList pointParts = parts[1].split(" ");
                    if (pointParts.size() == 4) {
                        QPoint point1(pointParts[0].toInt(), pointParts[1].toInt());
                        QPoint point2(pointParts[2].toInt(), pointParts[3].toInt());
                        connections.append(qMakePair(point1, point2));
                    }
                }
            }
            updateGraph();
            update();
            //Закрытие файла
            file.close();
        } else {
            qDebug() << "Ошибка: не удалось открыть файл для чтения";
        }
    } else {
        qDebug() << "Ошибка: не выбран файл для загрузки";
    }
}
QDataStream &operator<<(QDataStream &out, const Figure &figure) {
    //запись типа фигуры
    out << static_cast<qint32>(figure.shape);
    //запись размера данных фигуры
    qint32 dataSize = 0;
    QByteArray data;
    {
        QDataStream dataStream(&data, QIODevice::WriteOnly);
        dataStream << figure.rect;
        dataSize = data.size();
    }
    out << dataSize;
    //запись данных фигуры
    out.writeRawData(data.constData(), dataSize);

    return out;
}

QDataStream &operator>>(QDataStream &in, Figure &figure) {
    //чтение фигуры из потока данных
    int shape;
    in >> shape;
    figure.shape = static_cast<Shape>(shape);
    in >> figure.rect;
    return in;
}

void MainWindow::updateGraph() {
    //обновление графа связей между фигурами
    graph.clear();
    //функция проходит по всем существующим связям (connections). Для каждой связи она пытается найти индексы фигуры, соответствующие началу и концу этой связи
    for (int i = 0; i < connections.size(); ++i) {
        int startIdx = -1, endIdx = -1;
        for (int j = 0; j < figures.size(); ++j) {
            if (figures[j].rect.center() == connections[i].first) {
                startIdx = j;
            }
            if (figures[j].rect.center() == connections[i].second) {
                endIdx = j;
            }
        }
        if (startIdx != -1 && endIdx != -1) {
            graph[startIdx].insert(endIdx);
            graph[endIdx].insert(startIdx);
        }
    }
}

void MainWindow::moveConnectedFigures(int index, const QPoint &delta) {
    // Перемещение выбранной фигуры
   figures[index].rect.moveTopLeft(figures[index].rect.topLeft() + delta);
    // Обновление позиций связей, связанных с перемещаемой фигурой
    for (auto &connection : connections) {
        // Обновляем только концы линий, привязанные к перемещаемой фигуре
        if (figures[index].rect.contains(connection.first)) {
            connection.first = figures[index].rect.center();
        } else if (figures[index].rect.contains(connection.second)) {
            connection.second = figures[index].rect.center();
        }
    }
    // Инициация перерисовки окна
   update();
}

void MainWindow::addFigure(const Figure &figure) {
    // Добавление новой фигуры в список фигур
    figures.append(figure);
    updateGraph();
}

void MainWindow::clear_all() {
    // Очистка всех фигур и связей
    figures.clear();
    connections.clear();
    graph.clear();
    update();
}
