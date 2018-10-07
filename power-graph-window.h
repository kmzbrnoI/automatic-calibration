#ifndef POWERGRAPHWINDOW_H
#define POWERGRAPHWINDOW_H

#include <QDialog>
#include "ui_power-graph-window.h"

class PowerGraphWindow : public QDialog
{
	Q_OBJECT

public:
	explicit PowerGraphWindow(QWidget *parent = nullptr);

private:
	Ui::PowerGraphWindow ui;
};

#endif // POWERGRAPHWINDOW_H
