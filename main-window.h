#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "ui_main-window.h"
#include "lib/xn/xn.h"
#include "lib/wsm/measure-car.h"
#include "settings.h"

const QString _CONFIG_FN = "config.ini";

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	void xn_onDccGoError(void*, void*);
	void xn_onDccStopError(void*, void*);
	void xn_onLIVersionError(void*, void*);
	void xn_onCSVersionError(void*, void*);
	void xn_onCSStatusError(void*, void*);
	void xn_onCSStatusOk(void*, void*);
	void xn_gotLIVersion(void*, unsigned hw, unsigned sw);
	void xn_gotCSVersion(void*, unsigned major, unsigned minor);
	void xn_gotLocoInfo(void*, bool used, bool direction, unsigned speed,
	                            Xn::XnFA, Xn::XnFB);
	void xn_onLocoInfoError(void*, void*);

private slots:
	void xn_onError(QString error);
	void xn_onLog(QString, Xn::XnLogLevel);
	void xn_onConnect();
	void xn_onDisconnect();
	void xn_onTrkStatusChanged(Xn::XnTrkStatus);

	void t_xn_disconnect_tick();
	void cb_xn_ll_index_changed(int index);
	void b_addr_set_handle();
	void b_addr_release_handle();
	void b_addr_read_handle();
	void b_speed_set_handle();
	void b_loco_stop_handle();
	void vs_speed_slider_moved(int);
	void rb_direction_toggled(bool);
	void t_slider_tick();
	void b_start_handle();

	void a_xn_connect(bool);
	void a_xn_disconnect(bool);
	void a_dcc_go(bool);
	void a_dcc_stop(bool);

	void a_wsm_connect(bool);
	void a_wsm_disconnect(bool);

	void mc_speedRead(double speed, uint16_t speed_raw);
	void mc_onError(QString error);
	void mc_batteryRead(double voltage, uint16_t voltage_raw);
	void mc_batteryCritical();
	void mc_distanceRead(double distance, uint32_t distance_raw);
	void t_mc_disconnect_tick();

private:
	Ui::MainWindow ui;
	Xn::XpressNet xn;
	std::unique_ptr<Wsm::MeasureCar> wsm;
	Settings s;
	QTimer t_xn_disconnect;
	QTimer t_wsm_disconnect;
	QTimer t_slider;
	int m_sent_speed = 0;
	QDateTime m_canBlink;
	bool m_starting = false;

	void widget_set_color(QWidget&, const QColor);
	void show_response_error(QString command);
	void log(QString message);
	void wsm_status_blink();
	void show_error(const QString error);
	void loco_released();

	static void xns_onDccGoError(void*, void*);
	static void xns_onDccStopError(void*, void*);
	static void xns_onLIVersionError(void*, void*);
	static void xns_onCSVersionError(void*, void*);
	static void xns_onCSStatusError(void*, void*);
	static void xns_onCSStatusOk(void*, void*);
	static void xns_gotLIVersion(void*, unsigned hw, unsigned sw);
	static void xns_gotCSVersion(void*, unsigned major, unsigned minor);
	static void xns_gotLocoInfo(void*, unsigned major, unsigned minor);
	static void xns_gotLocoInfo(void*, bool used, bool direction, unsigned speed,
	                            Xn::XnFA, Xn::XnFB);
	static void xns_onLocoInfoError(void*, void*);
};

#endif // MAINWINDOW_H
