#ifndef POWERGRAPHWINDOW_H
#define POWERGRAPHWINDOW_H

/*
This file defines a PowerGraphWindow class which manages Power Graph UI form.
The main object at this form is QChart displaying mapping of real speed to
power steps in decoder.
*/

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include "ui_power-graph-window.h"

class PowerGraphWindow : public QMainWindow {
	Q_OBJECT

public:
	PowerGraphWindow(QWidget *parent = nullptr);
	~PowerGraphWindow() override;

public slots:
	void addOrUpdate(unsigned step, float speed);
	void clear();

private:
	Ui::PowerGraphWindow ui;
	QtCharts::QLineSeries series;
	QtCharts::QChart chart;
};

#endif // POWERGRAPHWINDOW_H
