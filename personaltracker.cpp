#include "personaltracker.h"
#include "ui_personaltracker.h"

PersonalTracker::PersonalTracker(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::PersonalTracker)
{
    ui->setupUi(this);
}

PersonalTracker::~PersonalTracker()
{
    delete ui;
}
