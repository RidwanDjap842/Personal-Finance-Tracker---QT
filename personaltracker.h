#ifndef PERSONALTRACKER_H
#define PERSONALTRACKER_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class PersonalTracker;
}
QT_END_NAMESPACE

class PersonalTracker : public QMainWindow
{
    Q_OBJECT

public:
    PersonalTracker(QWidget *parent = nullptr);
    ~PersonalTracker();

private:
    Ui::PersonalTracker *ui;
};
#endif // PERSONALTRACKER_H
