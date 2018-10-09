#ifndef POWERGRAPHWINDOW_H
#define POWERGRAPHWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

QT_CHARTS_USE_NAMESPACE

#include "ui_power-graph-window.h"

class PowerGraphWindow : public QMainWindow {
	Q_OBJECT

public:
	PowerGraphWindow(QWidget *parent = nullptr);

public slots:
	void addOrUpdate(unsigned step, float speed);
	void clear();

private:
	Ui::PowerGraphWindow ui;
	QLineSeries series;
	QChart chart;
};

#endif // POWERGRAPHWINDOW_H
