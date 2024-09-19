#include "tooltip.h"
#include "ui_tooltip.h"

Tooltip::Tooltip(QString text, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tooltip)
{
    ui->setupUi(this);

    connect(&timer, &QTimer::timeout, this, &QWidget::deleteLater);
    ui->label->setText(text);
    resize(width(), 2 * ui->label->heightForWidth(ui->label->width()));

    // https://stackoverflow.com/questions/32040202/qt-5-get-the-mouse-position-in-a-screen
    // Перемещаем подсказку так, чтобы под мышкой находился верх ее
    // середины по горизонтали.
    QPoint mousePos = QCursor::pos();
    auto newWndPos = QPoint(mousePos.x() - width() / 2, mousePos.y());
    move(newWndPos);

    show();
    timer.start(7000); // 7000мс = 7с.
}

Tooltip::~Tooltip()
{
    delete ui;
}
