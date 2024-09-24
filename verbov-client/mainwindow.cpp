#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QToolTip>
#include <QListWidgetItem>

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
        QToolTip::showText(QCursor::pos(), QString(result.content));
    } else {
        QDataStream stream(result.content);
        stream >> events;
        // Обновим список в графическом интерфейсе.
        on_calendarWidget_selectionChanged();
    }

    server_commit_timer.setInterval(2500);
    connect(&server_commit_timer, &QTimer::timeout, [this]() {
        // Удалим повторые изменения событий.
        // В самом событии записана актуальная информация.
        // Благодаря обработчикам слотов виджетов. Они, по
        // моему представлению, все запускаются в event-loop-е
        // внутри QApplication::exec().
        std::sort(dirty_events.begin(), dirty_events.end());
        dirty_events.erase(std::unique(dirty_events.begin(), dirty_events.end()), dirty_events.end());
        size_t num_done = 0;
        for (const quint64 event_id: dirty_events) {
            auto event_it = std::find_if(
                events.begin(),
                events.end(),
                [event_id](const auto& event) { return event.get_id() == event_id; }
                );
            if (event_it == events.end()) {
                // Уже удалили событие. Тогда обновлять не надо.
                continue;
            }
            const Event& event = *event_it;

            api::Result result = api::request(
                "event",
                {"event_id", "name", "timestamp"},
                {QString::number(event.get_id()), event.name, QString::number(event.timestamp)},
                token_,
                api::HttpMethod::Patch
                );

            if (result.success) {
                num_done += 1;

                // Обновляем вид в списке
                for (int i = 0; i < ui->eventList->count(); ++i) {
                    QListWidgetItem* item = ui->eventList->item(i);
                    auto event_item = dynamic_cast<EventListItem*>(item);
                    assert(event_item != 0);
                    if (event_item->event_id == event.get_id()) {
                        event_item->setText(event.name);
                        break;
                    }
                }
            } else {
                QToolTip::showText(QCursor::pos(), QString(result.content));
                break;
            }
        }
        dirty_events.erase(dirty_events.begin(), dirty_events.begin() + num_done);
    });
    server_commit_timer.start();
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

        // 8 утра выбранного дня :)
        // Может быть нужно вечером запланировать на утро.
        // А затем планировать уже от времени последнего события.
        // Типо планировать на 00:00 точно редкость, но 8 часов
        // тоже не оптимально, зачастую будут события в середине дня..
        constexpr auto eight_hours = std::chrono::milliseconds(8 * 60 * 1000);
        auto datetime = ui->calendarWidget->selectedDate().startOfDay().addDuration(eight_hours);

        qint64 timestamp = datetime.toSecsSinceEpoch();

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

            // Добавляем в конец, не сортируем.
            // Ничего страшного.
            new EventListItem(event, ui->eventList);
            // Не будем ждать несколько секунд, пока список обновится.
            // Хотя это хорошо, что обновления накапливаются, иначе
            // работать будет медленно. Например, насколько помню,
            // для виждета список в документации wxWidgets все элементы
            // сразу рекомендуют устанавливать.
            ui->eventList->update();
        } else {
            QToolTip::showText(QCursor::pos(), QString(result.content));
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
    // Не будем ждать несколько секунд, пока список обновится..
    ui->eventList->update();
}

void MainWindow::show_event(const Event& event) {
    switching_events = true;

    ui->eventName->setText(event.name);

    QDateTime event_time = QDateTime::fromSecsSinceEpoch(event.timestamp);
    ui->eventDate->setDate(event_time.date());
    ui->eventTime->setTime(event_time.time());

    switching_events = false;
}

void MainWindow::on_eventList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (current == nullptr) {
        // Выделение сбросилось.
        // Возможно, список очистился. Тогда очистим формы.
        // И сбросим id активного события.

        selected_event_id.reset();

        ui->eventName->clear();
        ui->eventDate->setDate(ui->calendarWidget->selectedDate());
        ui->eventTime->setTime(QTime(8, 0));

        return;
    }

    EventListItem* event_item = dynamic_cast<EventListItem*>(current);
    assert(event_item != nullptr);

    // Надо заново искать событие в массиве, т.к.
    // могли быть перевыделения памяти, нельзя
    // было хранить указатель. Хотя можно хранить
    // номер в массиве, но мы стираем события. Потому
    // это тоже не подходит так сразу.

    for (const auto& event: events) {
        if (event.get_id() == event_item->event_id) {
            show_event(event);
            selected_event_id = event.get_id();
            break;
        }
    }    
}

void MainWindow::update_selected_event() {
    // При пользовательском вводе помечаем событие как "грязное",
    // требующее обновления на сервере.
    // Обновлением на сервере будет заниматься QTimer. Он будет
    // работать в нашем же потоке, т.к. у нас есть event loop
    // внутри QApplication::exec() (было по другой ссылке
    // документации, об обработке событий).
    // https://doc.qt.io/qt-6/qtimer.html

    if (!selected_event_id.has_value()) {
        return;
    }

    for (auto& event: events) {
        if (event.get_id() == selected_event_id.value()) {
            event.name = ui->eventName->text();
            event.timestamp = QDateTime(
                                  ui->eventDate->date(),
                                  ui->eventTime->time()
                                ).toSecsSinceEpoch();
            dirty_events.push_back(selected_event_id.value());
            break;
        }
    }
}

void MainWindow::on_eventName_textEdited(const QString &new_value)
{
    // qInfo() << "switching_events" << switching_events;
    update_selected_event();
}

void MainWindow::on_deleteEventBtn_clicked()
{
    if (!selected_event_id.has_value()) {
        return;
    }

    api::Result result = api::request(
        "event",
        {"event_id"},
        {QString::number(*selected_event_id)},
        token_,
        api::HttpMethod::Delete
        );

    if (result.success) {
        ui->eventList->takeItem(ui->eventList->currentRow());
        events.erase(std::find_if(events.begin(), events.end(), [this](const auto& event) {
            return event.get_id() == *selected_event_id;
        }));
    } else {
        QToolTip::showText(QCursor::pos(), QString(result.content));
    }
}


void MainWindow::on_eventTime_userTimeChanged(const QTime &time)
{
    // qInfo() << "switching_events" << switching_events;
    if (switching_events) {
        // Игнорируем обновления, которые вызвал наш код,
        // а не пользовательский ввод.
        // Для редактирования имени события не надо такое
        // отсечение, т.к. сигнал onTextEdited выдается
        // только при редактировании пользователем.
        // Можно проверить на практике, тоже.
        return;
    }
    update_selected_event();
}


void MainWindow::on_eventDate_userDateChanged(const QDate &date)
{
    // qInfo() << "switching_events" << switching_events;
    if (switching_events) {
        // Игнорируем обновления, которые вызвал наш код,
        // а не пользовательский ввод.
        // Для редактирования имени события не надо такое
        // отсечение, т.к. сигнал onTextEdited выдается
        // только при редактировании пользователем.
        // Можно проверить на практике, тоже.
        return;
    }
    update_selected_event();
}

