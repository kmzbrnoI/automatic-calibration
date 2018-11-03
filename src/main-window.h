#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*
This file defines MainWindow class which is associated with main form of
the application.

It handles all the UI communication. It also contains CalibrationManager
instance. It also owns instances of Xn and Wsm library classes, which are
forwarded to Calibration Manager as a references.
*/

#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>

#include "ui_main-window.h"
#include "lib/xn/xn.h"
#include "lib/wsm/wsm.h"
#include "settings.h"
#include "power-graph-window.h"
#include "speed-map.h"
#include "power-map.h"
#include "calib-step.h"
#include "calib-man.h"
#include "calib-range.h"

constexpr char _CONFIG_FN[] = "config.ini";
constexpr unsigned _STEPS_CNT = 28;

constexpr unsigned _CV_ACCEL = 3;
constexpr unsigned _CV_DECEL = 4;
constexpr unsigned _CV_ADDR_LO = 18;
constexpr unsigned _CV_ADDR_HI = 17;
constexpr unsigned _CV_BASIC = 29;

const QColor _STEPC_DONE = QColor(50, 200, 50);
const QColor _STEPC_ERROR = Qt::red;
const QColor _STEPC_CHANGED = Qt::blue;

const QColor _LOGC_ERROR = QColor(0xFF, 0xAA, 0xAA);
const QColor _LOGC_WARN = QColor(0xFF, 0xFF, 0xAA);
const QColor _LOGC_DONE = QColor(0xAA, 0xFF, 0xAA);

struct UiStep {
	QSlider *slider;
	QLabel *step, *speed_want, *value;
	QCheckBox *selected;
	QPushButton *calibrate;
};

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	// Signals from XpressNET library
	void xn_onError(const QString& error);
	void xn_onLog(const QString&, const Xn::XnLogLevel&);
	void xn_onConnect();
	void xn_onDisconnect();
	void xn_onTrkStatusChanged(Xn::XnTrkStatus);

	// Signals from GUI
	void sb_loco_changed(int value);
	void sb_speed_changed(int value);
	void b_addr_set_handle();
	void b_addr_release_handle();
	void b_addr_read_handle();
	void b_speed_set_handle();
	void b_loco_stop_handle();
	void b_loco_idle_handle();
	void vs_speed_slider_moved(int);
	void rb_direction_toggled(bool);
	void t_slider_tick();
	void chb_f_clicked(bool);

	void b_start_handle();
	void b_calib_start_handle();
	void b_calib_stop_handle();
	void b_ad_read_handle();
	void b_ad_write_handle();
	void b_decel_measure_handle();
	void b_wsm_lt_handle();
	void sb_max_speed_changed(int value);
	void lv_log_dblclick(QListWidgetItem*);
	void tw_xn_log_dblclick(QTreeWidgetItem*, int column);
	void b_reset_handle();
	void t_calib_active_tick();
	void vs_steps_moved(int);
	void b_calibrate_handle();
	void chb_step_selected_clicked(bool);
	void t_xn_disconnect_tick();
	void cb_xn_ll_index_changed(int index);

	// Test buttons:
	void b_test1_handle();
	void b_test2_handle();
	void b_test3_handle();

	// GUI menu actions:
	void a_xn_connect(bool);
	void a_xn_disconnect(bool);
	void a_dcc_go(bool);
	void a_dcc_stop(bool);
	void a_wsm_connect(bool);
	void a_wsm_disconnect(bool);
	void a_power_graph(bool);
	void a_loco_load(bool);
	void a_loco_save(bool);
	void a_config_load(bool);
	void a_config_save(bool);
	void a_speed_load(bool);

	// Wsm events:
	void mc_speedRead(double speed, uint16_t speed_raw);
	void mc_onError(QString error);
	void mc_batteryRead(double voltage, uint16_t voltage_raw);
	void mc_batteryCritical();
	void mc_distanceRead(double distance, uint32_t distance_raw);
	void t_mc_disconnect_tick();
	void mc_speedReceiveTimeout();
	void mc_speedReceiveRestore();
	void mc_longTermMeasureDone(double speed, double diffusion);

	// Steps-to-speed map events
	void ssm_onAddOrUpdate(unsigned step, unsigned speed);
	void ssm_onClear();

	// Calibration manager events:
	void cm_stepStart(unsigned step);
	void cm_stepDone(unsigned step, unsigned power);
	void cm_stepError(Cm::CmError, unsigned step);
	void cm_locoSpeedChanged(unsigned step);
	void cm_done();
	void cm_step_power_changed(unsigned step, unsigned power);
	void cm_progress_update(size_t val);
	void cm_done_gui();
	void cm_accelChanged(unsigned);
	void cm_decelChanged(unsigned);

	// Calibration range events:
	void cr_measured(double distance);
	void cr_error(Cr::CrError, unsigned step);

private:
	Ui::MainWindow ui;
	Xn::XpressNet xn;
	Wsm::Wsm wsm;
	Settings s;
	QTimer t_xn_disconnect;
	QTimer t_wsm_disconnect;
	QTimer t_slider;
	QTimer t_calib_active;
	int m_sent_speed = 0;
	QDateTime m_canBlink;
	bool m_starting = false;
	PowerGraphWindow w_pg;
	UiStep ui_steps[_STEPS_CNT];
	Xn::XnFA m_fa;
	Pm::PowerToSpeedMap m_pm;
	Ssm::StepsToSpeedMap m_ssm;
	Cm::CalibMan cm;
	Cr::CalibRange cr;

	// Callbacks from XpressNET library:
	void xn_onDccGoError(void*, void*);
	void xn_onDccStopError(void*, void*);
	void xn_onLIVersionError(void*, void*);
	void xn_onCSVersionError(void*, void*);
	void xn_onCSStatusError(void*, void*);
	void xn_onCSStatusOk(void*, void*);
	void xn_gotLIVersion(void*, unsigned hw, unsigned sw);
	void xn_gotCSVersion(void*, unsigned major, unsigned minor);
	void xn_gotLocoInfo(void*, bool used, Xn::XnDirection direction, unsigned speed,
	                    Xn::XnFA, Xn::XnFB);
	void xn_onLocoInfoError(void*, void*);
	void xn_addrReadError(void*, void*);
	void xn_adReadError(void*, void*);
	void xn_cvRead(void*, Xn::XnReadCVStatus, uint8_t cv, uint8_t value);
	void xn_adWriteError(void*, void*);
	void xn_accelWritten(void*, void*);
	void xn_decelWritten(void*, void*);
	void xn_stepWritten(void*, void*);
	void xn_stepWriteError(void*, void*);

	void widget_set_color(QWidget&, const QColor&);
	void show_response_error(const QString& command);
	void log(const QString& message, const QColor& color = Qt::white);
	void wsm_status_blink();
	void show_error(const QString& error);
	void loco_released();
	void init_calib_graph();
	void reset();
	void step_set_color(const unsigned stepi, const QColor& color);
};

#endif // MAINWINDOW_H