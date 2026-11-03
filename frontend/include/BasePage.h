
#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QWidget>
#include <QString>

class BasePage : public QWidget
{
public:
    explicit BasePage(
        const QString &title,
        const QString &description,
        const QString &message,
        QWidget *parent = nullptr
    );
};

#endif // BASEPAGE_H
