#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "api.h"
#include "EventListItem.h"

MainWindow::MainWindow(QString token, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , token_(std::move(token))
{
    ui->setupUi(this);

    api::Result result = api::request("event", {}, {}, token_);
    if (!result.success) {
        ui->statusbar->showMessage(QString(result.content), 10);
    } else {
        QDataStream stream(result.content);
        stream >> events;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_newEventBtn_clicked()
{
    for (size_t i = 0; i < events.size() + 1; ++i) {
        QString name = "Событие #" + QString::number(events.size() + i);

        bool found_event = false;
        for (const Event& event: events) {
            if (event.name == name) {
                found_event = true;
                break;
            }
        }

        if (found_event) {
            continue;
        }

        qint64 timestamp = QDateTime::currentSecsSinceEpoch();

        api::Result result = api::request(
            "event",
            {"name", "timestamp"},
            {std::move(name), QString::number(timestamp)},
            token_,
            api::HttpMethod::Post
        );

        if (result.success) {
            Event event;
            QDataStream stream(result.content);
            stream >> event;
            events.push_back(event);
            // TODO: add to event list, if date is the same
            //   Items should be in chronological order!
            statusBar()->showMessage(QString(result.content), 10);
            statusBar()->update();
        } else {
            statusBar()->showMessage(QString(result.content), 10);
            qInfo() << QString(result.content);
            statusBar()->update();
        }

        break;
    }
}

void MainWindow::on_todayEventsLabel_linkActivated(const QString &link)
{
    ui->calendarWidget->showToday();
}

void MainWindow::on_calendarWidget_selectionChanged()
{
    const QDate& date = ui->calendarWidget->selectedDate();
    qint64 start_timestamp = date.startOfDay().toSecsSinceEpoch();
    qint64 end_timestamp   = date.endOfDay().toSecsSinceEpoch();

    QVector<Event> date_events;
    for (const Event& event: events) {
        if (event.timestamp >= start_timestamp && event.timestamp <= end_timestamp) {
            date_events.push_back(event);
        }
    }
    std::sort(date_events.begin(), date_events.end(), [](const Event& lhs, const Event& rhs) {
        return lhs.timestamp < rhs.timestamp;
    });

    ui->eventList->clear();
    for (const auto& event: date_events) {
        // https://doc.qt.io/qt-6/qlistwidgetitem.html
        new EventListItem(event, ui->eventList);
    }
}
