#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ui_main-window.h"
#include "lib/xn/xn.h"
#include "settings.h"

const QString _CONFIG_FN = "config.ini";

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	Ui::MainWindow ui;
	XpressNet xn;
	Settings s;

	void widget_set_color(QWidget&, const QColor);
};

#endif // MAINWINDOW_H
