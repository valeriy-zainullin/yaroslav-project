#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <QWidget>
#include <QTimer>

namespace Ui {
class Tooltip;
}

class Tooltip : public QWidget
{
    Q_OBJECT

public:
    explicit Tooltip(QString text, QWidget *parent = nullptr);
    ~Tooltip();

private:
    Ui::Tooltip *ui;

    QTimer timer;
};

#endif // TOOLTIP_H
