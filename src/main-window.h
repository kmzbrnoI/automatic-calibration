#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*
This file defines MainWindow class which is associated with main form of
the application.

It handles all the UI communication. It also contains CalibrationManager
instance. It also owns instances of Xn and Wsm library classes, which are
forwarded to Calibration Manager as a references.
*/

#include <QCheckBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>

#include "calib-man.h"
#include "calib-range.h"
#include "calib-step.h"
#include "lib/wsm/wsm.h"
#include "lib/xn/xn.h"
#include "power-graph-window.h"
#include "power-map.h"
#include "settings.h"
#include "speed-map.h"
#include "ui_main-window.h"

constexpr char DEFAULT_CONFIG_FN[] = "config.ini";
constexpr unsigned STEPS_CNT = 28;

constexpr unsigned CV_ADDR_SHORT = 1;
constexpr unsigned CV_ACCEL = 3;
constexpr unsigned CV_DECEL = 4;
constexpr unsigned CV_ADDR_LO = 18;
constexpr unsigned CV_ADDR_HI = 17;
constexpr unsigned CV_BASIC_CONFIG = 29;

const QColor STEPC_DONE = QColor(50, 200, 50);
const QColor STEPC_ERROR = Qt::red;
const QColor STEPC_CHANGED = Qt::blue;

const QColor LOGC_ERROR = QColor(0xFF, 0xAA, 0xAA);
const QColor LOGC_WARN = QColor(0xFF, 0xFF, 0xAA);
const QColor LOGC_DONE = QColor(0xAA, 0xFF, 0xAA);
const QColor LOGC_GET = QColor(0xE0, 0xE0, 0xFF);
const QColor LOGC_PUT = QColor(0xE0, 0xFF, 0xE0);

struct UiStep {
	QSlider *slider;
	QLabel *step, *speed_want, *value;
	QCheckBox *selected;
	QPushButton *read;
	QPushButton *write;
	QPushButton *calibrate;
};

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	// Signals from XpressNET library
	void xn_onError(const QString &error);
	void xn_onLog(const QString &, const Xn::LogLevel &);
	void xn_onConnect();
	void xn_onDisconnect();
	void xn_onTrkStatusChanged(Xn::TrkStatus);

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
	void chb_vmax_clicked(bool);
	void chb_volt_ref_clicked(bool);

	void b_start_handle();
	void b_calib_start_handle();
	void b_calib_stop_handle();
	void b_ad_read_handle();
	void b_ad_write_handle();
	void b_decel_measure_handle();
	void b_wsm_lt_handle();
	void sb_max_speed_changed(int value);
	void lv_log_dblclick(QListWidgetItem *);
	void tw_xn_log_dblclick(QTreeWidgetItem *, int column);
	void b_reset_handle();
	void t_calib_active_tick();
	void vs_steps_moved(int);
	void b_step_read_handle();
	void b_step_write_handle();
	void b_step_calibrate_handle();
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
	void cm_stepError(Cm::CmError, unsigned step, const QString& note);
	void cm_locoSpeedChanged(unsigned step);
	void cm_done();
	void cm_step_power_changed(unsigned step, unsigned power);
	void cm_progress_update(size_t val);
	void cm_done_gui();
	void cm_accelChanged(unsigned);
	void cm_decelChanged(unsigned);

	// Calibration range events:
	void cr_measured(double distance);
	void cr_error(Cr::CrError, unsigned step, const QString&);

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
	UiStep ui_steps[STEPS_CNT];
	Xn::FA m_fa;
	Pm::PowerToSpeedMap m_pm;
	Ssm::StepsToSpeedMap m_ssm;
	Cm::CalibMan cm;
	Cr::CalibRange cr;
	QString config_fn;

	// Callbacks from XpressNET library:
	void xn_onDccGoError(void *, void *);
	void xn_onDccStopError(void *, void *);
	void xn_onLIVersionError(void *, void *);
	void xn_onCSVersionError(void *, void *);
	void xn_onCSStatusError(void *, void *);
	void xn_onCSStatusOk(void *, void *);
	void xn_gotLIVersion(void *, unsigned hw, unsigned sw);
	void xn_gotCSVersion(void *, unsigned major, unsigned minor);
	void xn_gotLocoInfo(void *, bool used, Xn::Direction direction, unsigned speed, Xn::FA, Xn::FB);
	void xn_onLocoInfoError(void *, void *);
	void xn_addrReadError(void *, void *);
	void xn_adReadError(void *, void *);
	void xn_cvRead(void *, Xn::ReadCVStatus, uint8_t cv, uint8_t value);
	void xn_adWriteError(void *, void *);
	void xn_accelWritten(void *, void *);
	void xn_decelWritten(void *, void *);
	void xn_stepWritten(void *, void *);
	void xn_stepWriteError(void *, void *);

	void widget_set_color(QWidget &, const QColor &);
	void show_response_error(const QString &command);
	void log(const QString &message, const QColor &color = Qt::white);
	void wsm_status_blink();
	void show_error(const QString &error);
	void loco_released();
	void init_calib_graph();
	void reset();
	void step_set_color(unsigned stepi, const QColor &color);
	void gui_update_enabled();
	void gui_step_update_enabled(UiStep&);
};

#endif // MAINWINDOW_H
