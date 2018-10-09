#include <QValueAxis>

#include "power-graph-window.h"
#include "ui_power-graph-window.h"

PowerGraphWindow::PowerGraphWindow(QWidget *parent)
	: QMainWindow(parent) {
	ui.setupUi(this);

	series.setPointsVisible(true);

	series.append(2, 4);
	series.append(3, 8);
	series.append(7, 4);
	series.append(10, 5);

	chart.legend()->hide();
	chart.addSeries(&series);
	chart.createDefaultAxes();
	chart.axisX()->setRange(0, 256);
	static_cast<QValueAxis*>(chart.axisX())->setTickCount(9);
	static_cast<QValueAxis*>(chart.axisX())->setMinorTickCount(1);
	chart.axisY()->setRange(0, 120);
	static_cast<QValueAxis*>(chart.axisY())->setTickCount(7);
	static_cast<QValueAxis*>(chart.axisY())->setMinorTickCount(1);
	chart.setTitle("Power to speed graph");

	QChartView *chartView = new QChartView(&chart);
	chartView->setRenderHint(QPainter::Antialiasing);
	this->setCentralWidget(chartView);

	series.append(100, 100);
	//chart->createDefaultAxes();
}

void PowerGraphWindow::addOrUpdate(unsigned step, float speed) {
	for(int i = 0; i < series.count(); i++) {
		if (series.at(i).x() == step) {
			series.replace(i, QPointF(step, speed));
			return;
		}
		if (series.at(i).x() > step) {
			series.insert(i, QPointF(step, speed));
			return;
		}
	}
	series.append(step, speed);
}

void PowerGraphWindow::clear() {
	series.clear();
}
