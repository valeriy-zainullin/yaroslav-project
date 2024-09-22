#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "DB/event.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString token, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_newEventBtn_clicked();

    void on_todayEventsLabel_linkActivated(const QString &link);

    void on_calendarWidget_selectionChanged();

private:
    Ui::MainWindow *ui;
    QString token_;

    QVector<Event> events;
};
#endif // MAINWINDOW_H
