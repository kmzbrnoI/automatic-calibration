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

	void xn_onDccGoError(void* sender, void* data);
	void xn_onDccStopError(void* sender, void* data);

private slots:
	void xn_onError(QString error);
	void xn_onLog(QString, XnLogLevel);
	void xn_onConnect();
	void xn_onDisconnect();
	void xn_onTrkStatusChanged(XnTrkStatus);

	void t_xn_disconnect_tick();
	void cb_xn_ll_index_changed(int index);

	void a_xn_connect(bool);
	void a_xn_disconnect(bool);
	void a_dcc_go(bool);
	void a_dcc_stop(bool);

	void a_wsm_connect(bool);
	void a_wsm_disconnect(bool);

private:
	Ui::MainWindow ui;
	XpressNet xn;
	Settings s;
	QTimer t_xn_disconnect;
	QTimer t_wsm_disconnect;

	void widget_set_color(QWidget&, const QColor);
	static void xns_onDccGoError(void* sender, void* data);
	static void xns_onDccStopError(void* sender, void* data);
};

#endif // MAINWINDOW_H
