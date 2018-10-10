#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>

#include "ui_main-window.h"
#include "lib/xn/xn.h"
#include "lib/wsm/measure-car.h"
#include "settings.h"
#include "power-graph-window.h"
#include "speed-map.h"
#include "power-map.h"
#include "calib-man.h"

const QString _CONFIG_FN = "config.ini";
const unsigned _STEPS_CNT = 28;

const unsigned _CV_ACCEL = 3;
const unsigned _CV_DECEL = 4;
const unsigned _CV_ADDR_LO = 18;
const unsigned _CV_ADDR_HI = 17;
const unsigned _CV_BASIC = 29;

struct UiStep {
	QSlider *slider;
	QLabel *step, *speed_want, *value, *speed_measured;
	QCheckBox *selected;
	QPushButton *calibrate;
};

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
	void xn_addrReadError(void*, void*);
	void xn_adReadError(void*, void*);
	void xn_cvRead(void*, Xn::XnReadCVStatus, uint8_t cv, uint8_t value);
	void xn_adWriteError(void*, void*);
	void xn_accelWritten(void*, void*);
	void xn_decelWritten(void*, void*);

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
	void b_loco_idle_handle();
	void vs_speed_slider_moved(int);
	void rb_direction_toggled(bool);
	void t_slider_tick();
	void b_start_handle();
	void chb_f_clicked(bool);
	void b_ad_read_handle();
	void b_ad_write_handle();

	void a_xn_connect(bool);
	void a_xn_disconnect(bool);
	void a_dcc_go(bool);
	void a_dcc_stop(bool);

	void a_wsm_connect(bool);
	void a_wsm_disconnect(bool);

	void a_power_graph();
	void vs_steps_moved(int);
	void b_calibrate_handle();

	void mc_speedRead(double speed, uint16_t speed_raw);
	void mc_onError(QString error);
	void mc_batteryRead(double voltage, uint16_t voltage_raw);
	void mc_batteryCritical();
	void mc_distanceRead(double distance, uint32_t distance_raw);
	void t_mc_disconnect_tick();
	void mc_speedReceiveTimeout();
	void mc_speedReceiveRestore();
	void mc_longTermMeasureDone(double speed, double diffusion);

	void ssm_onAddOrUpdate(unsigned step, unsigned speed);
	void ssm_onClear();

	void cm_diffusion_error();
	void cm_loco_stopped();
	void cm_done();
	void cm_xn_error();
	void cm_step_power_changed(unsigned step, unsigned power);

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
	PowerGraphWindow w_pg;
	UiStep ui_steps[_STEPS_CNT];
	Xn::XnFA m_fa;
	Pm::PowerToSpeedMap m_pm;
	Ssm::StepsToSpeedMap m_ssm;
	std::unique_ptr<Cm::CalibStep> m_cm;

	void widget_set_color(QWidget&, const QColor);
	void show_response_error(QString command);
	void log(QString message);
	void wsm_status_blink();
	void show_error(const QString error);
	void loco_released();
	void init_calib_graph();

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
	static void xns_addrReadError(void*, void*);
	static void xns_adReadError(void*, void*);
	static void xns_cvRead(void*, Xn::XnReadCVStatus, uint8_t cv, uint8_t value);
	static void xns_adWriteError(void*, void*);
	static void xns_accelWritten(void*, void*);
	static void xns_decelWritten(void*, void*);
};

#endif // MAINWINDOW_H
