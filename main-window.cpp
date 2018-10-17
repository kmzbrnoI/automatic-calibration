#include <QMessageBox>
#include <QSlider>
#include <QFileDialog>
#include <QXmlStreamWriter>
#include <utility>

#include "main-window.h"
#include "ui_main-window.h"

const unsigned int WSM_BLINK_TIMEOUT = 250; // ms

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent), xn(this), s(), cm(xn, m_pm, wsm, m_ssm), cr(xn, wsm) {
	ui.setupUi(this);
	QString text;
	text.sprintf("Automatic Claibration v%d.%d", VERSION_MAJOR, VERSION_MINOR);
	this->setWindowTitle(text);
	this->setFixedSize(this->size());
	a_config_load(true);

	// XN init
	QObject::connect(&xn, SIGNAL(onError(QString)), this, SLOT(xn_onError(QString)));
	QObject::connect(&xn, SIGNAL(onLog(QString, Xn::XnLogLevel)), this, SLOT(xn_onLog(QString, Xn::XnLogLevel)));
	QObject::connect(&xn, SIGNAL(onConnect()), this, SLOT(xn_onConnect()));
	QObject::connect(&xn, SIGNAL(onDisconnect()), this, SLOT(xn_onDisconnect()));
	QObject::connect(&xn, SIGNAL(onTrkStatusChanged(Xn::XnTrkStatus)), this, SLOT(xn_onTrkStatusChanged(Xn::XnTrkStatus)));

	// UI signals
	QObject::connect(ui.b_start, SIGNAL(released()), this, SLOT(b_start_handle()));
	QObject::connect(ui.b_addr_set, SIGNAL(released()), this, SLOT(b_addr_set_handle()));
	QObject::connect(ui.b_addr_release, SIGNAL(released()), this, SLOT(b_addr_release_handle()));
	QObject::connect(ui.b_addr_read, SIGNAL(released()), this, SLOT(b_addr_read_handle()));
	QObject::connect(ui.b_speed_set, SIGNAL(released()), this, SLOT(b_speed_set_handle()));
	QObject::connect(ui.b_loco_stop, SIGNAL(released()), this, SLOT(b_loco_stop_handle()));
	QObject::connect(ui.b_loco_idle, SIGNAL(released()), this, SLOT(b_loco_idle_handle()));
	ui.sb_speed->setKeyboardTracking(false);
	QObject::connect(ui.sb_speed, SIGNAL(valueChanged(int)), this, SLOT(sb_speed_changed(int)));
	QObject::connect(ui.vs_speed, SIGNAL(valueChanged(int)), this, SLOT(vs_speed_slider_moved(int)));
	QObject::connect(ui.rb_backward, SIGNAL(toggled(bool)), this, SLOT(rb_direction_toggled(bool)));
	QObject::connect(ui.chb_f0, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));
	QObject::connect(ui.chb_f1, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));
	QObject::connect(ui.chb_f2, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));
	QObject::connect(ui.b_calib_start, SIGNAL(released()), this, SLOT(b_calib_start_handle()));
	QObject::connect(ui.b_calib_stop, SIGNAL(released()), this, SLOT(b_calib_stop_handle()));
	QObject::connect(ui.b_reset, SIGNAL(released()), this, SLOT(b_reset_handle()));

	ui.sb_loco->setKeyboardTracking(false);
	QObject::connect(ui.sb_loco, SIGNAL(valueChanged(int)), this, SLOT(sb_loco_changed(int)));

	QObject::connect(ui.b_test1, SIGNAL(released()), this, SLOT(b_test1_handle()));
	QObject::connect(ui.b_test2, SIGNAL(released()), this, SLOT(b_test2_handle()));
	QObject::connect(ui.b_test3, SIGNAL(released()), this, SLOT(b_test3_handle()));

	QObject::connect(ui.sb_max_speed, SIGNAL(valueChanged(int)), this, SLOT(sb_max_speed_changed(int)));
	QObject::connect(ui.lv_log, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(lv_log_dblclick(QListWidgetItem*)));
	QObject::connect(ui.tw_xn_log, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
	                 this, SLOT(tw_xn_log_dblclick(QTreeWidgetItem*, int)));

	t_xn_disconnect.setSingleShot(true);
	QObject::connect(&t_xn_disconnect, SIGNAL(timeout()), this, SLOT(t_xn_disconnect_tick()));

	t_wsm_disconnect.setSingleShot(true);
	QObject::connect(&t_wsm_disconnect, SIGNAL(timeout()), this, SLOT(t_mc_disconnect_tick()));

	t_slider.start(100);
	QObject::connect(&t_slider, SIGNAL(timeout()), this, SLOT(t_slider_tick()));

	QObject::connect(ui.b_ad_read, SIGNAL(released()), this, SLOT(b_ad_read_handle()));
	QObject::connect(ui.b_ad_write, SIGNAL(released()), this, SLOT(b_ad_write_handle()));

	QObject::connect(ui.b_decel_measure, SIGNAL(released()), this, SLOT(b_decel_measure_handle()));

	// UI set defaults
	widget_set_color(*(ui.l_xn), Qt::red);
	widget_set_color(*(ui.l_dcc), Qt::gray);
	widget_set_color(*(ui.l_wsm), Qt::red);
	widget_set_color(*(ui.l_wsm_alive), Qt::gray);

	// XN UI
	ui.cb_xn_loglevel->setCurrentIndex(s["XN"]["loglevel"].toInt());
	xn.loglevel = static_cast<Xn::XnLogLevel>(s["XN"]["loglevel"].toInt());
	QObject::connect(ui.cb_xn_loglevel, SIGNAL(currentIndexChanged(int)), this, SLOT(cb_xn_ll_index_changed(int)));

	QObject::connect(ui.a_xn_connect, SIGNAL(triggered(bool)), this, SLOT(a_xn_connect(bool)));
	QObject::connect(ui.a_xn_disconnect, SIGNAL(triggered(bool)), this, SLOT(a_xn_disconnect(bool)));
	QObject::connect(ui.a_xn_dcc_go, SIGNAL(triggered(bool)), this, SLOT(a_dcc_go(bool)));
	QObject::connect(ui.a_xn_dcc_stop, SIGNAL(triggered(bool)), this, SLOT(a_dcc_stop(bool)));

	QObject::connect(ui.a_wsm_connect, SIGNAL(triggered(bool)), this, SLOT(a_wsm_connect(bool)));
	QObject::connect(ui.a_wsm_disconnect, SIGNAL(triggered(bool)), this, SLOT(a_wsm_disconnect(bool)));
	QObject::connect(ui.b_wsm_lt, SIGNAL(released()), this, SLOT(b_wsm_lt_handle()));

	QObject::connect(ui.a_power_graph, SIGNAL(triggered(bool)), this, SLOT(a_power_graph(bool)));
	QObject::connect(ui.a_loco_load, SIGNAL(triggered(bool)), this, SLOT(a_loco_load(bool)));
	QObject::connect(ui.a_loco_save, SIGNAL(triggered(bool)), this, SLOT(a_loco_save(bool)));
	QObject::connect(ui.a_config_load, SIGNAL(triggered(bool)), this, SLOT(a_config_load(bool)));
	QObject::connect(ui.a_config_save, SIGNAL(triggered(bool)), this, SLOT(a_config_save(bool)));

	// WSM init
	QObject::connect(&wsm, SIGNAL(speedRead(double, uint16_t)), this, SLOT(mc_speedRead(double, uint16_t)));
	QObject::connect(&wsm, SIGNAL(onError(QString)), this, SLOT(mc_onError(QString)));
	QObject::connect(&wsm, SIGNAL(batteryRead(double, uint16_t)), this, SLOT(mc_batteryRead(double, uint16_t)));
	QObject::connect(&wsm, SIGNAL(batteryCritical()), this, SLOT(mc_batteryCritical()));
	QObject::connect(&wsm, SIGNAL(distanceRead(double, uint32_t)), this, SLOT(mc_distanceRead(double, uint32_t)));
	QObject::connect(&wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(mc_speedReceiveTimeout()));
	QObject::connect(&wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(mc_longTermMeasureDone(double, double)));
	QObject::connect(&wsm, SIGNAL(speedReceiveRestore()), this, SLOT(mc_speedReceiveRestore()));

	// Calibration Manager signals
	QObject::connect(&cm, SIGNAL(onStepStart(unsigned)), this, SLOT(cm_stepStart(unsigned)));
	QObject::connect(&cm, SIGNAL(onStepDone(unsigned, unsigned)),
	                 this, SLOT(cm_stepDone(unsigned, unsigned)));
	QObject::connect(&cm, SIGNAL(onError(Cm::CmError, unsigned)),
	                 this, SLOT(cm_stepError(Cm::CmError, unsigned)));
	QObject::connect(&cm, SIGNAL(onLocoSpeedChanged(unsigned)),
	                 this, SLOT(cm_locoSpeedChanged(unsigned)));
	QObject::connect(&cm, SIGNAL(onDone()), this, SLOT(cm_done()));
	QObject::connect(&cm, SIGNAL(onStepPowerChanged(unsigned, unsigned)),
	                 this, SLOT(cm_step_power_changed(unsigned, unsigned)));
	QObject::connect(&cm, SIGNAL(onProgressUpdate(size_t)), this, SLOT(cm_progress_update(size_t)));
	QObject::connect(&cm, SIGNAL(onAccelChanged(unsigned)), this, SLOT(cm_accelChanged(unsigned)));
	QObject::connect(&cm, SIGNAL(onDecelChanged(unsigned)), this, SLOT(cm_decelChanged(unsigned)));

	// Connect power-to-map with GUI
	QObject::connect(&m_pm, SIGNAL(onAddOrUpdate(unsigned, float)), &w_pg, SLOT(addOrUpdate(unsigned, float)));
	QObject::connect(&m_pm, SIGNAL(onClear()), &w_pg, SLOT(clear()));
	m_pm.clear();

	init_calib_graph();

	// Steps to Speed map
	QObject::connect(&m_ssm, SIGNAL(onAddOrUpdate(unsigned, unsigned)), this, SLOT(ssm_onAddOrUpdate(unsigned, unsigned)));
	QObject::connect(&m_ssm, SIGNAL(onClear()), this, SLOT(ssm_onClear()));
	m_ssm.load("speed.csv");

	// Range Calibration
	QObject::connect(&cr, SIGNAL(on_error(Cr::CrError, unsigned)), this, SLOT(cr_error(Cr::CrError, unsigned)));
	QObject::connect(&cr, SIGNAL(measured(double)), this, SLOT(cr_measured(double)));

	w_pg.setAttribute(Qt::WA_QuitOnClose, false);

	ui.tw_main->setCurrentIndex(0);
	log("Application launched.");
}

MainWindow::~MainWindow() {
	try {
		a_config_save(true);
	}
	catch (...) {
		// No exceptions in destructor!
	}
}

///////////////////////////////////////////////////////////////////////////////
// UI general functions

void MainWindow::widget_set_color(QWidget& widget, const QColor color) {
	QPalette palette = widget.palette();
	palette.setColor(QPalette::WindowText, color);
	widget.setPalette(palette);
}

void MainWindow::t_xn_disconnect_tick() {
	log("XN error, disconnecting from XpressNET...");
	a_xn_disconnect(true);

	QMessageBox::warning(this, "Error!",
		"XN serial port error, more information in lo!", QMessageBox::Ok);
}

void MainWindow::cb_xn_ll_index_changed(int index) {
	s["XN"]["loglevel"] = index;
	xn.loglevel = static_cast<Xn::XnLogLevel>(index);
}

void MainWindow::show_error(const QString error) {
	log(error);
	QMessageBox::warning(this, "Error!", error, QMessageBox::Ok);
}

void MainWindow::b_start_handle() {
	if (!xn.connected()) {
		m_starting = true;
		a_xn_connect(true);
	} else {
		if (!wsm.connected())
			a_wsm_connect(true);
	}
}

void MainWindow::lv_log_dblclick(QListWidgetItem*) {
	ui.lv_log->clear();
}

void MainWindow::tw_xn_log_dblclick(QTreeWidgetItem*, int) {
	ui.tw_xn_log->clear();
}

///////////////////////////////////////////////////////////////////////////////
// XN Events

void MainWindow::xn_onError(QString error) {
	xn_onLog(error, Xn::XnLogLevel::Error);

	if (!t_xn_disconnect.isActive() && xn.connected())
		t_xn_disconnect.start(0);
}

void MainWindow::xn_onLog(QString message, Xn::XnLogLevel loglevel) {
	if (ui.tw_xn_log->topLevelItemCount() > 300)
		ui.tw_xn_log->clear();

	QTreeWidgetItem *item = new QTreeWidgetItem(ui.tw_xn_log);
	item->setText(0, QTime::currentTime().toString("hh:mm:ss"));

	if (loglevel == Xn::XnLogLevel::None)
		item->setText(1, "None");
	else if (loglevel == Xn::XnLogLevel::Error)
		item->setText(1, "Error");
	else if (loglevel == Xn::XnLogLevel::Warning)
		item->setText(1, "Warning");
	else if (loglevel == Xn::XnLogLevel::Info)
		item->setText(1, "Info");
	else if (loglevel == Xn::XnLogLevel::Data)
		item->setText(1, "Data");
	else if (loglevel == Xn::XnLogLevel::Debug)
		item->setText(1, "Debug");

	item->setText(2, message);
	ui.tw_xn_log->addTopLevelItem(item);
}

void MainWindow::xn_onConnect() {
	widget_set_color(*(ui.l_xn), Qt::green);
	ui.a_xn_connect->setEnabled(false);
	ui.a_xn_disconnect->setEnabled(true);
	ui.a_xn_dcc_go->setEnabled(true);
	ui.a_xn_dcc_stop->setEnabled(true);
	ui.gb_speed->setEnabled(true);
	ui.gb_cal_graph->setEnabled(true);

	ui.vs_speed->setEnabled(false);
	ui.sb_speed->setEnabled(false);
	ui.b_speed_set->setEnabled(false);
	ui.b_loco_stop->setEnabled(false);
	ui.b_loco_idle->setEnabled(false);
	ui.chb_f0->setEnabled(false);
	ui.chb_f1->setEnabled(false);
	ui.chb_f2->setEnabled(false);
	ui.rb_forward->setEnabled(false);
	ui.rb_backward->setEnabled(false);
	ui.b_addr_release->setEnabled(false);
	ui.sb_loco->setEnabled(true);
	ui.b_addr_set->setEnabled(true);
	ui.b_addr_read->setEnabled(true);
	ui.b_speed_set->setEnabled(false);
	ui.gb_ad->setEnabled(true);

	log("Connected to XpressNET.");

	try {
		xn.getLIVersion(
			[this](void *s, unsigned hw, unsigned sw) { xn_gotLIVersion(s, hw, sw); },
			std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_onLIVersionError(s, d); })
		);
	}
	catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::xn_onDisconnect() {
	widget_set_color(*(ui.l_xn), Qt::red);
	widget_set_color(*(ui.l_dcc), Qt::gray);
	ui.a_xn_connect->setEnabled(true);
	ui.a_xn_disconnect->setEnabled(false);
	ui.a_xn_dcc_go->setEnabled(false);
	ui.a_xn_dcc_stop->setEnabled(false);
	ui.gb_speed->setEnabled(false);
	ui.b_addr_set->setEnabled(true);
	ui.b_addr_release->setEnabled(false);
	ui.gb_ad->setEnabled(false);
	ui.gb_cal_graph->setEnabled(false);
	loco_released();
	log("Disconnected from XpressNET");
}

void MainWindow::xn_onTrkStatusChanged(Xn::XnTrkStatus status) {
	if (status == Xn::XnTrkStatus::Unknown) {
		widget_set_color(*(ui.l_dcc), Qt::gray);
		log("CS status: Unknown");
	} else if (status == Xn::XnTrkStatus::Off) {
		widget_set_color(*(ui.l_dcc), Qt::red);
		log("CS status: Off");
	} else if (status == Xn::XnTrkStatus::On) {
		widget_set_color(*(ui.l_dcc), Qt::green);
		log("CS status: On");
	} else if (status == Xn::XnTrkStatus::Programming) {
		widget_set_color(*(ui.l_dcc), Qt::yellow);
		log("CS status: Programming");
	}
}

void MainWindow::xn_onDccGoError(void*, void*) {
	show_response_error("DCC GO");
}

void MainWindow::xn_onDccStopError(void*, void*) {
	show_response_error("DCC STOP");
}

void MainWindow::show_response_error(QString command) {
	log("Command station did not respond to " + command + " command!");
	QMessageBox::warning(this, "Error!",
		"Command station did not respond to " + command + " command!");
}

void MainWindow::xn_onLIVersionError(void*, void*) {
	m_starting = false;
	log("LI did not respond to version request!");
	QMessageBox::warning(this, "Error!",
		"LI did not respond to version request, are you really connected to the LI?!");
}

void MainWindow::xn_onCSVersionError(void*, void*) {
	log("Coomand station did not respond to version request!");
	QMessageBox::warning(this, "Error!",
		"Command station did not respond to version request"
		", is the LI really connected to the command station?!");
	m_starting = false;
}

void MainWindow::xn_onCSStatusError(void*, void*) {
	show_response_error("STATUS");
	m_starting = false;
}

void MainWindow::xn_onCSStatusOk(void*, void*) {
	if (m_starting) {
		m_starting = false;
		log("Succesfully connected to Command station");
		if (!wsm.connected())
			a_wsm_connect(true);
	}
}

void MainWindow::xn_gotLIVersion(void*, unsigned hw, unsigned sw) {
	log("Got LI version. HW: " + QString::number(hw) + ", SW: " + QString::number(sw));
	try {
		xn.getCommandStationStatus(
			std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_onCSStatusOk(s, d); }),
			std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_onCSStatusError(s, d); })
		);
	}
	catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::xn_gotCSVersion(void*, unsigned major, unsigned minor) {
	log("Got command station version:" + QString::number(major) + "." + QString::number(minor));
}

void MainWindow::xn_gotLocoInfo(void*, bool used, Xn::XnDirection direction, unsigned speed,
                                Xn::XnFA fa, Xn::XnFB fb) {
	(void)used;
	(void)fb;

	m_fa = fa;

	ui.b_addr_release->setEnabled(true);

	ui.vs_speed->setValue(speed);
	ui.vs_speed->setEnabled(true);
	m_sent_speed = speed;

	ui.sb_speed->setValue(speed);
	ui.sb_speed->setEnabled(true);

	ui.rb_backward->setEnabled(true);
	ui.rb_forward->setEnabled(true);
	if (direction == Xn::XnDirection::Forward)
		ui.rb_forward->setChecked(true);
	else
		ui.rb_backward->setChecked(true);

	ui.chb_f0->setChecked(fa.sep.f0);
	ui.chb_f0->setEnabled(true);

	ui.chb_f1->setChecked(fa.sep.f1);
	ui.chb_f1->setEnabled(true);

	ui.chb_f2->setChecked(fa.sep.f2);
	ui.chb_f2->setEnabled(true);

	ui.b_loco_stop->setEnabled(true);
	ui.b_loco_idle->setEnabled(true);
	ui.b_speed_set->setEnabled(true);

	log("Acquired loco " + QString::number(ui.sb_loco->value()));
}

void MainWindow::xn_onLocoInfoError(void*, void*) {
	show_error("Unable to get loco information from Command station!");
	ui.b_addr_set->setEnabled(true);
	ui.sb_loco->setEnabled(true);
	ui.b_addr_read->setEnabled(true);
}

void MainWindow::loco_released() {
	ui.vs_speed->setValue(0);
	ui.vs_speed->setEnabled(false);
	m_sent_speed = 0;

	ui.sb_speed->setValue(0);
	ui.sb_speed->setEnabled(false);

	ui.rb_backward->setChecked(false);
	ui.rb_forward->setChecked(false);
	ui.rb_backward->setEnabled(false);
	ui.rb_forward->setEnabled(false);

	ui.chb_f0->setChecked(false);
	ui.chb_f0->setEnabled(false);

	ui.chb_f1->setChecked(false);
	ui.chb_f1->setEnabled(false);

	ui.chb_f2->setChecked(false);
	ui.chb_f2->setEnabled(false);

	ui.b_loco_stop->setEnabled(false);
	ui.b_loco_idle->setEnabled(false);
	ui.b_speed_set->setEnabled(false);

	log("Released loco " + QString::number(ui.sb_loco->value()));
}

void MainWindow::xn_addrReadError(void*, void*) {
	show_error("Unable to read address: no response from command station!");
	ui.sb_loco->setEnabled(true);
	ui.b_addr_set->setEnabled(true);
	ui.b_addr_read->setEnabled(true);
}

void MainWindow::xn_adReadError(void*, void*) {
	show_error("Unable to read CV: no response from command station!");
	ui.gb_ad->setEnabled(true);
}

void MainWindow::xn_cvRead(void*, Xn::XnReadCVStatus st, uint8_t cv, uint8_t value) {
	if (st != Xn::XnReadCVStatus::Ok) {
		show_error("Unable to read CV " + QString::number(cv) + ": " +
		           Xn::XpressNet::xnReadCVStatusToQString(st));

		if (cv == _CV_ADDR_LO || cv == _CV_ADDR_HI) {
			ui.sb_loco->setEnabled(true);
			ui.b_addr_set->setEnabled(true);
			ui.b_addr_read->setEnabled(true);
		} else if (cv == _CV_ACCEL || cv == _CV_DECEL) {
			ui.gb_ad->setEnabled(true);
		}
		return;
	}

	if (cv == _CV_ADDR_LO) {
		try {
			ui.sb_loco->setValue(Xn::LocoAddr(value, 0xC0));
		}
		catch (const Xn::EInvalidAddr&) {
			show_error("Invalid address!");
		}
		xn.readCVdirect(
			_CV_ADDR_HI,
			[this](void *s, Xn::XnReadCVStatus st, uint8_t cv, uint8_t value) { xn_cvRead(s, st, cv, value); },
			std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_addrReadError(s, d); })
		);
	} else if (cv == _CV_ADDR_HI) {
		try {
			ui.sb_loco->setValue(Xn::LocoAddr(ui.sb_loco->value(), value));
		}
		catch (const Xn::EInvalidAddr&) {
			show_error("Invalid address!");
		}
		ui.sb_loco->setEnabled(true);
		ui.b_addr_set->setEnabled(true);
		ui.b_addr_read->setEnabled(true);
		a_dcc_go(true);
	} else if (cv == _CV_ACCEL) {
		ui.sb_accel->setValue(value);
		xn.readCVdirect(_CV_DECEL,
			[this](void *s, Xn::XnReadCVStatus st, uint8_t cv, uint8_t value) { xn_cvRead(s, st, cv, value); },
			std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_adReadError(s, d); })
		);
	} else if (cv == _CV_DECEL) {
		ui.sb_decel->setValue(value);
		ui.gb_ad->setEnabled(true);
		a_dcc_go(true);
	}
}

void MainWindow::xn_adWriteError(void*, void*) {
	ui.gb_ad->setEnabled(true);
	show_error("XN POM no response!");
}

void MainWindow::xn_accelWritten(void*, void*) {
	xn.pomWriteCv(Xn::LocoAddr(ui.sb_loco->value()), _CV_DECEL, ui.sb_decel->value(),
	              std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_decelWritten(s, d); }),
	              std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_adWriteError(s, d); }));
}

void MainWindow::xn_decelWritten(void*, void*) {
	ui.gb_ad->setEnabled(true);
}

///////////////////////////////////////////////////////////////////////////////
// XpressNET connect/disconnect

void MainWindow::a_xn_connect(bool) {
	if (xn.connected())
		return;

	try {
		log("Connecting to XN...");
		xn.connect(s["XN"]["port"].toString(), s["XN"]["baudrate"].toInt(),
		           static_cast<QSerialPort::FlowControl>(s["XN"]["flowcontrol"].toInt()));
	} catch (const QStrException& e) {
		show_error("XN connect error while opening serial port '" +
		           s["XN"]["port"].toString() + "':\n" + e);
	}
}

void MainWindow::a_xn_disconnect(bool) {
	if (!xn.connected())
		return;

	try {
		log("Disconnecting from XN...");
		xn.disconnect();
		if (cm.inProgress())
			b_calib_stop_handle();
	} catch (const QStrException& e) {
		show_error("XN disconnect error:\n" + e.str());
	}
}

void MainWindow::a_dcc_go(bool) {
	try {
		if (xn.connected())
			xn.setTrkStatus(
				Xn::XnTrkStatus::On, nullptr,
				std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_onDccGoError(s, d); })
			);
	} catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::a_dcc_stop(bool) {
	try {
		if (xn.connected())
			xn.setTrkStatus(
				Xn::XnTrkStatus::Off, nullptr,
				std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_onDccStopError(s, d); })
			);
	} catch (const QStrException& e) {
		show_error(e.str());
	}
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::log(QString message) {
	if (ui.lv_log->count() > 200)
		ui.lv_log->clear();
	ui.lv_log->insertItem(0, QTime::currentTime().toString("hh:mm:ss") + ": " + message);
}

///////////////////////////////////////////////////////////////////////////////
// Speed settings

void MainWindow::b_addr_set_handle() {
	if (!xn.connected() || !ui.sb_loco->isEnabled())
		return;

	ui.b_addr_set->setEnabled(false);
	ui.sb_loco->setEnabled(false);
	ui.b_addr_read->setEnabled(false);

	xn.getLocoInfo(
		Xn::LocoAddr(ui.sb_loco->value()),
		[this](void *s, bool used, Xn::XnDirection dir, unsigned speed, Xn::XnFA fa, Xn::XnFB fb) {
			xn_gotLocoInfo(s, used, dir, speed, fa, fb);
		},
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_onLocoInfoError(s, d); })
	);
}

void MainWindow::sb_loco_changed(int) {
	b_addr_set_handle();
}

void MainWindow::b_addr_release_handle() {
	ui.b_addr_set->setEnabled(true);
	ui.b_addr_release->setEnabled(false);
	ui.sb_loco->setEnabled(true);
	ui.b_addr_read->setEnabled(true);
	loco_released();
}

void MainWindow::b_addr_read_handle() {
	ui.sb_loco->setEnabled(false);
	ui.b_addr_set->setEnabled(false);
	ui.b_addr_read->setEnabled(false);
	xn.readCVdirect(
		_CV_ADDR_LO,
		[this](void *s, Xn::XnReadCVStatus st, uint8_t cv, uint8_t value) { xn_cvRead(s, st, cv, value); },
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_addrReadError(s, d); })
	);
}

void MainWindow::b_speed_set_handle() {
	try {
		xn.setSpeed(Xn::LocoAddr(ui.sb_loco->value()), ui.sb_speed->value(),
		            static_cast<Xn::XnDirection>(ui.rb_forward->isChecked()));
		m_sent_speed = ui.sb_speed->value();
		ui.vs_speed->setValue(ui.sb_speed->value());
	}
	catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::b_loco_stop_handle() {
	try {
		m_sent_speed = 0;
		ui.vs_speed->setValue(0);
		ui.sb_speed->setValue(0);
		xn.emergencyStop(Xn::LocoAddr(ui.sb_loco->value()));
	}
	catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::b_loco_idle_handle() {
	ui.sb_speed->setValue(0);
	b_speed_set_handle();
}

void MainWindow::vs_speed_slider_moved(int value) {
	ui.sb_speed->setValue(value);
}

void MainWindow::rb_direction_toggled(bool) {
	b_speed_set_handle();
}

void MainWindow::t_slider_tick() {
	try {
		if (ui.vs_speed->value() != m_sent_speed) {
			m_sent_speed = ui.vs_speed->value();
			xn.setSpeed(Xn::LocoAddr(ui.sb_loco->value()), ui.sb_speed->value(),
			            static_cast<Xn::XnDirection>(ui.rb_forward->isChecked()));
		}
	}
	catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::chb_f_clicked(bool) {
	m_fa.sep.f0 = ui.chb_f0->isChecked();
	m_fa.sep.f1 = ui.chb_f1->isChecked();
	m_fa.sep.f2 = ui.chb_f2->isChecked();
	xn.setFuncA(Xn::LocoAddr(ui.sb_loco->value()), m_fa);
}

void MainWindow::sb_max_speed_changed(int value) {
	m_ssm.setMaxSpeed(value);
}

void MainWindow::sb_speed_changed(int) {
	if (m_sent_speed != ui.sb_speed->value())
		b_speed_set_handle();
}

///////////////////////////////////////////////////////////////////////////////
// WSM functions

void MainWindow::wsm_status_blink() {
	QPalette palette = ui.l_wsm_alive->palette();
	QColor color = palette.color(QPalette::WindowText);
	if (color == Qt::green)
		widget_set_color(*(ui.l_wsm_alive), palette.color(QPalette::Window));
	else
		widget_set_color(*(ui.l_wsm_alive), Qt::green);
}

void MainWindow::a_wsm_connect(bool) {
	log("Connecting to WSM...");

	try {
		wsm.connect(s["WSM"]["port"].toString());

		ui.a_wsm_connect->setEnabled(false);
		ui.a_wsm_disconnect->setEnabled(true);
		widget_set_color(*(ui.l_wsm), Qt::green);

		log("Connected to WSM");
	} catch (const Wsm::EOpenError& e) {
		show_error("WSM connect error while opening serial port '" +
		           s["WSM"]["port"].toString() + "':\n" + e);
	}
}

void MainWindow::a_wsm_disconnect(bool) {
	wsm.disconnect();

	if (cm.inProgress())
		b_calib_stop_handle();

	ui.l_wsm_speed->setText("??.?");
	ui.l_wsm_bat_voltage->setText("?.?? V");
	widget_set_color(*(ui.l_wsm_alive), ui.l_wsm_speed->palette().color(QPalette::WindowText));
	ui.a_wsm_connect->setEnabled(true);
	ui.a_wsm_disconnect->setEnabled(false);
	widget_set_color(*(ui.l_wsm), Qt::red);
	log("Disconnected from WSM");
}

void MainWindow::mc_speedRead(double speed, uint16_t speed_raw) {
	(void)speed_raw;

	ui.l_wsm_speed->setText(QString::number(speed, 'f', 1));

	if (m_canBlink < QDateTime::currentDateTime()) {
		wsm_status_blink();
		m_canBlink = QDateTime::currentDateTime().addMSecs(WSM_BLINK_TIMEOUT);
	}
}

void MainWindow::mc_distanceRead(double distance, uint32_t distance_raw) {
	(void)distance;
	(void)distance_raw;
}

void MainWindow::mc_onError(QString error) {
	if (!t_wsm_disconnect.isActive() && wsm.connected()) {
		t_wsm_disconnect.start(0);
		show_error("WSM serial port error: " + error + "!");
	}
}

void MainWindow::mc_batteryRead(double voltage, uint16_t voltage_raw) {
	(void)voltage_raw;
	QString text;
	text.sprintf(
		"%4.2f V (%s)",
		voltage,
		QTime::currentTime().toString().toLatin1().data()
	);
	ui.l_wsm_bat_voltage->setText(text);
}

void MainWindow::mc_batteryCritical() {
	QMessageBox::warning(this, "Warning",
		"Battery level critical, device is shutting down!");

	if (!t_wsm_disconnect.isActive())
		t_wsm_disconnect.start(0);
}

void MainWindow::t_mc_disconnect_tick() {
	log("WSM error, disconnecting...");
	a_wsm_disconnect(true);
}

void MainWindow::mc_speedReceiveTimeout() {
	log("WSM receive timeout!");
	widget_set_color(*(ui.l_wsm_alive), Qt::red);
}

void MainWindow::mc_speedReceiveRestore() {
	ui.b_wsm_lt->setEnabled(true);
	log("WSM speed receive restored");
}

void MainWindow::mc_longTermMeasureDone(double speed, double diffusion) {
	ui.b_wsm_lt->setEnabled(true);
	log("WSM long term done: sp=" + QString::number(speed) + ", diff=" + QString::number(diffusion));
}

void MainWindow::b_wsm_lt_handle() {
	if (wsm.connected()) {
		try {
			wsm.startLongTermMeasure(30); // 3 s

		}
		catch (const QStrException& e) {
			show_error(e.str());
		}
		ui.b_wsm_lt->setEnabled(false);
	}
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::a_power_graph(bool) {
	w_pg.move(ui.centralwidget->pos().x() + ui.centralwidget->size().width(),
	          ui.centralwidget->pos().y() - ui.mb_main->size().height());
	w_pg.show();
}

void MainWindow::init_calib_graph() {
	for(size_t i = 0; i < _STEPS_CNT; i++) {
		QLabel *speed_want = new QLabel("-", ui.gb_cal_graph);
		speed_want->setFont(QFont("Sans Serif", 8));
		speed_want->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].speed_want = speed_want;
		ui.l_cal_graph->addWidget(speed_want, 0, i);

		QLabel *speed_measured = new QLabel("??", ui.gb_cal_graph);
		speed_measured->setFont(QFont("Sans Serif", 8));
		speed_measured->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].speed_measured = speed_measured;
		ui.l_cal_graph->addWidget(speed_measured, 1, i);

		QLabel *value = new QLabel("0", ui.gb_cal_graph);
		value->setFont(QFont("Sans Serif", 8));
		value->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].value = value;
		ui.l_cal_graph->addWidget(value, 2, i);

		QSlider *slider = new QSlider(Qt::Orientation::Vertical, ui.gb_cal_graph);
		slider->setMinimum(0);
		slider->setMaximum(255);
		slider->setProperty("step", static_cast<uint>(i));
		slider->setEnabled(false);
		QObject::connect(slider, SIGNAL(valueChanged(int)), this, SLOT(vs_steps_moved(int)));
		ui_steps[i].slider = slider;
		ui.l_cal_graph->addWidget(slider, 3, i);

		QCheckBox *selected = new QCheckBox(ui.gb_cal_graph);
		selected->setProperty("step", static_cast<uint>(i));
		ui_steps[i].selected = selected;
		QObject::connect(selected, SIGNAL(clicked(bool)), this, SLOT(chb_step_selected_clicked(bool)));
		ui.l_cal_graph->addWidget(selected, 4, i);

		QLabel *step = new QLabel(QString::number(i+1), ui.gb_cal_graph);
		step->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].step = step;
		ui.l_cal_graph->addWidget(step, 5, i);

		QPushButton *calibrate = new QPushButton("C", ui.gb_cal_graph);
		calibrate->setProperty("step", static_cast<uint>(i));
		calibrate->setEnabled(false);
		ui_steps[i].calibrate = calibrate;
		QObject::connect(calibrate, SIGNAL(released()), this, SLOT(b_calibrate_handle()));
		ui.l_cal_graph->addWidget(calibrate, 6, i);
	}
}

void MainWindow::vs_steps_moved(int value) {
	unsigned stepi = qobject_cast<QSlider*>(QObject::sender())->property("step").toUInt();
	ui_steps[stepi].value->setText(QString::number(value));
}

void MainWindow::b_calibrate_handle() {
	unsigned stepi = qobject_cast<QPushButton*>(QObject::sender())->property("step").toUInt();

	if (xn.connected() && !ui.sb_loco->isEnabled()) {
		ui_steps[stepi].selected->setChecked(true);
		log("Setting power of step " + QString::number(stepi+1) + " manually.");
		xn.pomWriteCv(
			Xn::LocoAddr(ui.sb_loco->value()),
			Cs::_CV_START + stepi,
			ui_steps[stepi].slider->value()
		);
		cm.setStepManually(stepi, ui_steps[stepi].slider->value());
	}
}

void MainWindow::chb_step_selected_clicked(bool checked) {
	unsigned stepi = qobject_cast<QCheckBox*>(QObject::sender())->property("step").toUInt();

	ui_steps[stepi].calibrate->setEnabled(checked);
	ui_steps[stepi].slider->setEnabled(checked);

	if (!checked)
		cm.unsetStep(stepi);
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::ssm_onAddOrUpdate(unsigned step, unsigned speed) {
	ui_steps[step].speed_want->setText(QString::number(speed));
}

void MainWindow::ssm_onClear() {
	for(size_t i = 0; i < _STEPS_CNT; i++)
		ui_steps[i].speed_want->setText("0");
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::cm_step_power_changed(unsigned step, unsigned power) {
	ui_steps[step-1].slider->setValue(power);
	log("Setting step " + QString::number(step) + " to " + QString::number(power));
}

void MainWindow::cm_stepDone(unsigned step, unsigned power) {
	(void)power;
	log("Step " + QString::number(step) + " done");
}

void MainWindow::cm_stepStart(unsigned step) {
	log("Starting calibration of step " + QString::number(step));
}

void MainWindow::cm_stepError(Cm::CmError ce, unsigned step) {
	log("Step " + QString::number(step) + " calibration error!");

	if (ce == Cm::CmError::LargeDiffusion)
		log("Loco speed too diffused!");
	else if (ce == Cm::CmError::XnNoResponse)
		log("No response from XpressNET!");
	else if (ce == Cm::CmError::LocoStopped)
		log("Loco stopped!");
	else if (ce == Cm::CmError::NoStep)
		log("No suitable power step for this speed!");

	cm_done_gui();
}

void MainWindow::cm_locoSpeedChanged(unsigned step) {
	m_sent_speed = step;
	ui.vs_speed->setValue(step);
	ui.sb_speed->setValue(step);
}

void MainWindow::cm_done() {
	log("Calibration done :)");
	ui.pb_progress->setValue(100);
	cm_done_gui();
}

void MainWindow::cm_progress_update(size_t val) {
	ui.pb_progress->setValue(val);
}

void MainWindow::cm_accelChanged(unsigned accel) {
	ui.sb_accel->setValue(accel);
}

void MainWindow::cm_decelChanged(unsigned decel) {
	ui.sb_decel->setValue(decel);
}

void MainWindow::b_calib_start_handle() {
	if (!xn.connected()) {
		show_error("Not connected to XpressNET!");
		return;
	}

	if (!wsm.connected()) {
		show_error("Not connected to WSM!");
		return;
	}

	if (!wsm.isSpeedOk()) {
		show_error("No data from WSM!");
		return;
	}

	ui.b_calib_start->setEnabled(false);
	ui.b_calib_stop->setEnabled(true);
	ui.sb_max_speed->setEnabled(false);
	ui.gb_cal_graph->setEnabled(false);
	ui.gb_ad->setEnabled(false);
	ui.b_wsm_lt->setEnabled(false);
	ui.pb_progress->setValue(0);
	ui.a_loco_load->setEnabled(false);
	ui.b_reset->setEnabled(false);
	ui.gb_speed->setEnabled(false);
	cm.calibrateAll(ui.sb_loco->value(),
	                static_cast<Xn::XnDirection>(ui.rb_forward->isChecked()));
}

void MainWindow::b_calib_stop_handle() {
	if (!cm.inProgress())
		return;

	cm.stop();
	log("Calibration manually interrupted!");
	cm_done_gui();
}

void MainWindow::cm_done_gui() {
	ui.b_calib_start->setEnabled(true);
	ui.b_calib_stop->setEnabled(false);
	ui.sb_max_speed->setEnabled(true);
	ui.gb_cal_graph->setEnabled(true);
	ui.gb_ad->setEnabled(true);
	ui.b_wsm_lt->setEnabled(true);
	ui.a_loco_load->setEnabled(true);
	ui.b_reset->setEnabled(true);
	ui.gb_speed->setEnabled(true);
}

void MainWindow::b_reset_handle() {
	if (cm.inProgress())
		return;

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Question", "Reaaly erase all measured data?",
	                              QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
	if (reply != QMessageBox::Yes)
		return;

	reset();

	QMessageBox::information(this, "Info", "All measured data have been erased.");
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::b_ad_read_handle() {
	ui.gb_ad->setEnabled(false);
	xn.readCVdirect(
		_CV_ACCEL,
		[this](void *s, Xn::XnReadCVStatus st, uint8_t cv, uint8_t value) { xn_cvRead(s, st, cv, value); },
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_adReadError(s, d); })
	);
}

void MainWindow::b_ad_write_handle() {
	if (ui.sb_loco->isEnabled()) {
		show_error("Loco must be set!");
		return;
	}

	ui.gb_ad->setEnabled(false);

	xn.pomWriteCv(Xn::LocoAddr(
		ui.sb_loco->value()), _CV_ACCEL, ui.sb_accel->value(),
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_accelWritten(s, d); }),
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_adWriteError(s, d); })
	);
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::b_test1_handle() {}

void MainWindow::b_test2_handle() {
	cm.interpolateAll();
}

void MainWindow::b_test3_handle() {}

//////////////////////////////////////////////////////////////////////////////

void MainWindow::a_loco_load(bool) {
	QString filename = QFileDialog::getOpenFileName(
		this,
		tr("Save Xml"), ".",
		tr("Xml files (*.xml)")
	);

	if (filename == "")
		return;

	reset();

	QXmlStreamReader xr;
	QFile file(filename);
	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		show_error("Cannot read file " + filename);
		return;
	}

	xr.setDevice(&file);
	xr.readNext();

	while(!xr.atEnd()) {
		if (xr.isStartElement()) {
			if (xr.name() == "powerToSpeed") {
				xr.readNext();
				while(xr.name() != "powerToSpeed") {
					if (xr.name() == "record" && xr.attributes().hasAttribute("power") &&
					    xr.attributes().hasAttribute("speed")) {
						int power = xr.attributes().value("power").toInt();
						float speed = xr.attributes().value("speed").toFloat();
						m_pm.addOrUpdate(power, speed);
					}
					xr.readNext();
				}
			} else if (xr.name() == "dcclocoaddress" && xr.attributes().hasAttribute("number")) {
				ui.sb_loco->setValue(xr.attributes().value("number").toInt());
			} else if (xr.name() == "locomotive" && xr.attributes().hasAttribute("maxSpeed")) {
				ui.sb_max_speed->setValue(xr.attributes().value("maxSpeed").toInt());
			}
		}

		xr.readNext();
	}
}

void MainWindow::a_loco_save(bool) {
	QString filename = QFileDialog::getSaveFileName(
		this,
		tr("Save Xml"), ".",
		tr("Xml files (*.xml)")
	);

	if (filename == "")
		return;

	if (!filename.endsWith(".xml"))
		filename += ".xml";

	QFile file(filename);
	file.open(QIODevice::WriteOnly);

	QXmlStreamWriter xw(&file);
	xw.setAutoFormatting(true);
	xw.writeStartDocument();
	xw.writeDTD("<!DOCTYPE locomotive-config SYSTEM \"/xml/DTD/locomotive-config.dtd\">");

	xw.writeStartElement("locomotive-config");
	xw.writeStartElement("locomotive");
	xw.writeAttribute("id", QString::number(ui.sb_loco->value()));
	xw.writeAttribute("dccAddress", QString::number(ui.sb_loco->value()));
	xw.writeAttribute("maxSpeed", QString::number(ui.sb_max_speed->value()));

	// We must save decoder model, otherwise JMRI crashes at start
	xw.writeStartElement("decoder");
	xw.writeAttribute("model", "MX658N18 version 32+"); // TODO: save decoder nicely?
	xw.writeEndElement();

	xw.writeStartElement("locoaddress");
	xw.writeStartElement("dcclocoaddress");
	xw.writeAttribute("number", QString::number(ui.sb_loco->value()));
	xw.writeAttribute("longaddress", "yes");
	xw.writeEndElement();
	xw.writeTextElement("number", QString::number(ui.sb_loco->value()));
	xw.writeTextElement("protocol", "dcc_long");
	xw.writeEndElement();

	xw.writeStartElement("values");
	xw.writeStartElement("decoderDef");

	QString steps = "";
	for(size_t i = 0; i < Xn::_STEPS_CNT; i++)
		steps += QString::number(ui_steps[i].slider->value()) + ",";

	std::vector<std::pair<QString, QString>> values = {
		std::make_pair<QString, QString>("Acceleration Rate", QString::number(ui.sb_accel->value())),
		std::make_pair<QString, QString>("Deceleration Rate", QString::number(ui.sb_decel->value())),
		std::make_pair<QString, QString>("Use Speed Table", "1"),
		std::make_pair<QString, QString>("Speed Table", std::move(steps)),
	};

	for (const auto& val : values) {
		xw.writeStartElement("varValue");
		xw.writeAttribute("item", val.first);
		xw.writeAttribute("value", val.second);
		xw.writeEndElement();
	}

	xw.writeEndElement();
	xw.writeEndElement();

	xw.writeStartElement("powerToSpeed");
	for(size_t i = 0; i < Pm::_POWER_CNT; i++) {
		if (nullptr != m_pm.speed(i)) {
			xw.writeStartElement("record");
			xw.writeAttribute("power", QString::number(i));
			xw.writeAttribute("speed", QString::number(*m_pm.speed(i)));
			xw.writeEndElement();
		}
	}
	xw.writeEndElement();

	xw.writeEndElement();
	xw.writeEndElement();
	file.close();
}

//////////////////////////////////////////////////////////////////////////////
// Range measuring:

void MainWindow::cr_measured(double distance) {
	log("Range measured: " + QString::number(distance*100) + " cm");
}

void MainWindow::cr_error(Cr::CrError ce, unsigned) {
	log("Range calibration error!");

	if (ce == Cr::CrError::XnNoResponse)
		log("No response from XpressNET!");
}

void MainWindow::b_decel_measure_handle() {
	if (!wsm.connected() || !wsm.isSpeedOk() || !xn.connected())
		return;

	cr.measure(ui.sb_loco->value(), 15, static_cast<Xn::XnDirection>(ui.rb_forward->isChecked()));
}

void MainWindow::reset() {
	m_pm.clear();
	cm.reset();
	for(size_t i = 0; i < Xn::_STEPS_CNT; i++) {
		ui_steps[i].slider->setValue(0);
		ui_steps[i].selected->setChecked(false);
	}
	ui.pb_progress->setValue(0);
}

//////////////////////////////////////////////////////////////////////////////

void MainWindow::a_config_load(bool) {
	s.load(_CONFIG_FN);

	wsm.scale = s["WSM"]["scale"].toInt();
	wsm.wheelDiameter = s["WSM"]["wheelDiameter"].toDouble();
	if (s["CalibStep"].find("epsilon") != s["CalibStep"].end())
		cm.cs.epsilon = s["CalibStep"]["epsilon"].toDouble();
	else
		s["CalibStep"]["epsilon"] = cm.cs.epsilon;

	if (s["CalibStep"].find("maxDiffusion") != s["CalibStep"].end())
		cm.cs.max_diffusion = s["CalibStep"]["maxDiffusion"].toDouble();
	else
		s["CalibStep"]["maxDiffusion"] = cm.cs.max_diffusion;

	log("Loaded config from " + _CONFIG_FN);
}

void MainWindow::a_config_save(bool) {
	s.save(_CONFIG_FN);
	log("Saved config to " + _CONFIG_FN);
}

//////////////////////////////////////////////////////////////////////////////
