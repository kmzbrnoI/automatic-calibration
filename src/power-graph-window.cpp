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

	auto& axisX = dynamic_cast<QValueAxis&>(*chart.axisX());
	axisX.setRange(0, 256);
	axisX.setTickCount(9);
	axisX.setMinorTickCount(1);
	axisX.setLabelFormat("%d");
	axisX.setTitleText("Power [decoder step]");

	auto& axisY = dynamic_cast<QValueAxis&>(*chart.axisY());
	axisY.setRange(0, 120);
	axisY.setTickCount(7);
	axisY.setMinorTickCount(1);
	axisY.setLabelFormat("%d");
	axisY.setTitleText("Speed [model km/h]");

	chart.setTitle("Locomotive power-to-speed profile");
	chart.setTitleFont(QFont("Sans Serif", 16, QFont::Bold));

	auto *chartView = new QChartView(&chart);
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
