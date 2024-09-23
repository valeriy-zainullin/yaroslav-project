#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTimer>

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

private:
    void show_event(const Event& event);

private slots:
    void on_newEventBtn_clicked();

    void on_todayEventsLabel_linkActivated(const QString &link);

    void on_calendarWidget_selectionChanged();

    void on_eventList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_eventName_textEdited(const QString &arg1);

    void on_deleteEventBtn_clicked();

    void on_eventTime_userTimeChanged(const QTime &time);

    void on_eventDate_userDateChanged(const QDate &date);

private:
    void update_selected_event();
private:
    Ui::MainWindow *ui;
    QString token_;
    std::optional<quint64> selected_event_id;

    QVector<quint64> dirty_events;
    QTimer server_commit_timer;

    QVector<Event> events;
};
#endif // MAINWINDOW_H
