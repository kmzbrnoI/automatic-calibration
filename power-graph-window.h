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

private:
	Ui::PowerGraphWindow ui;
	QChartView m_chart;
};

#endif // POWERGRAPHWINDOW_H
