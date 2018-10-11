#include <QMessageBox>
#include <QSlider>

#include "main-window.h"
#include "ui_main-window.h"

const unsigned int WSM_BLINK_TIMEOUT = 250; // ms
MainWindow* wref = nullptr;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent), xn(this), s(_CONFIG_FN), cm(xn, m_pm, wsm, m_ssm) {
	ui.setupUi(this);
	wref = this;

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

	QObject::connect(ui.sb_max_speed, SIGNAL(valueChanged(int)), this, SLOT(sb_max_speed_changed(int)));

	t_xn_disconnect.setSingleShot(true);
	QObject::connect(&t_xn_disconnect, SIGNAL(timeout()), this, SLOT(t_xn_disconnect_tick()));

	t_slider.start(100);
	QObject::connect(&t_slider, SIGNAL(timeout()), this, SLOT(t_slider_tick()));

	QObject::connect(ui.b_ad_read, SIGNAL(released()), this, SLOT(b_ad_read_handle()));
	QObject::connect(ui.b_ad_write, SIGNAL(released()), this, SLOT(b_ad_write_handle()));

	// UI set defaults
	widget_set_color(*(ui.l_xn), Qt::red);
	widget_set_color(*(ui.l_dcc), Qt::gray);
	widget_set_color(*(ui.l_wsm), Qt::red);
	widget_set_color(*(ui.l_wsm_alive), Qt::gray);

	// XN UI
	ui.cb_xn_loglevel->setCurrentIndex(s.xn.loglevel);
	xn.loglevel = static_cast<Xn::XnLogLevel>(s.xn.loglevel);
	QObject::connect(ui.cb_xn_loglevel, SIGNAL(currentIndexChanged(int)), this, SLOT(cb_xn_ll_index_changed(int)));

	QObject::connect(ui.a_xn_connect, SIGNAL(triggered(bool)), this, SLOT(a_xn_connect(bool)));
	QObject::connect(ui.a_xn_disconnect, SIGNAL(triggered(bool)), this, SLOT(a_xn_disconnect(bool)));
	QObject::connect(ui.a_xn_dcc_go, SIGNAL(triggered(bool)), this, SLOT(a_dcc_go(bool)));
	QObject::connect(ui.a_xn_dcc_stop, SIGNAL(triggered(bool)), this, SLOT(a_dcc_stop(bool)));

	QObject::connect(ui.a_wsm_connect, SIGNAL(triggered(bool)), this, SLOT(a_wsm_connect(bool)));
	QObject::connect(ui.a_wsm_disconnect, SIGNAL(triggered(bool)), this, SLOT(a_wsm_disconnect(bool)));
	QObject::connect(ui.b_wsm_lt, SIGNAL(released()), this, SLOT(b_wsm_lt_handle()));

	QObject::connect(ui.m_power_graph, SIGNAL(aboutToShow()), this, SLOT(a_power_graph()));

	// WSM init
	wsm.scale = s.wsm.scale;
	wsm.wheelDiameter = s.wsm.wheelDiameter;

	QObject::connect(&wsm, SIGNAL(speedRead(double, uint16_t)), this, SLOT(mc_speedRead(double, uint16_t)));
	QObject::connect(&wsm, SIGNAL(onError(QString)), this, SLOT(mc_onError(QString)));
	QObject::connect(&wsm, SIGNAL(batteryRead(double, uint16_t)), this, SLOT(mc_batteryRead(double, uint16_t)));
	QObject::connect(&wsm, SIGNAL(batteryCritical()), this, SLOT(mc_batteryCritical()));
	QObject::connect(&wsm, SIGNAL(distanceRead(double, uint32_t)), this, SLOT(mc_distanceRead(double, uint32_t)));
	QObject::connect(&wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(mc_speedReceiveTimeout()));
	QObject::connect(&wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(mc_longTermMeasureDone(double, double)));
	QObject::connect(&wsm, SIGNAL(speedReceiveRestore()), this, SLOT(mc_speedReceiveRestore()));

	// Calibration Manager signals
	QObject::connect(&cm, SIGNAL(onStepDone(unsigned, unsigned)),
	                 this, SLOT(cm_stepDone(unsigned, unsigned)));
	QObject::connect(&cm, SIGNAL(onStepStart(unsigned)), this, SLOT(cm_stepStart(unsigned)));
	QObject::connect(&cm, SIGNAL(onStepError(unsigned)), this, SLOT(cm_stepError(unsigned)));
	QObject::connect(&cm, SIGNAL(onLocoSpeedChanged(unsigned)),
	                 this, SLOT(cm_locoSpeedChanged(unsigned)));
	QObject::connect(&cm, SIGNAL(onDone()), this, SLOT(cm_done()));
	QObject::connect(&cm, SIGNAL(onStepPowerChanged(unsigned, unsigned)),
	                 this, SLOT(cm_stepPowerChanged(unsigned, unsigned)));

	QObject::connect(&cm.cs, SIGNAL(diffusion_error(unsigned)),
	                 this, SLOT(cs_diffusion_error(unsigned)));
	QObject::connect(&cm.cs, SIGNAL(loco_stopped(unsigned)),
	                 this, SLOT(cs_loco_stopped(unsigned)));
	QObject::connect(&cm.cs, SIGNAL(done(unsigned, unsigned)),
	                 this, SLOT(cs_done(unsigned, unsigned)));
	QObject::connect(&cm.cs, SIGNAL(xn_error(unsigned)), this, SLOT(cs_xn_error(unsigned)));
	QObject::connect(&cm.cs, SIGNAL(step_power_changed(unsigned, unsigned)), this, SLOT(cs_step_power_changed(unsigned, unsigned)));

	// Connect power-to-map with GUI
	QObject::connect(&m_pm, SIGNAL(onAddOrUpdate(unsigned, float)), &w_pg, SLOT(addOrUpdate(unsigned, float)));
	QObject::connect(&m_pm, SIGNAL(onClear()), &w_pg, SLOT(clear()));
	m_pm.clear();

	init_calib_graph();

	// Steps to Speed map
	QObject::connect(&m_ssm, SIGNAL(onAddOrUpdate(unsigned, unsigned)), this, SLOT(ssm_onAddOrUpdate(unsigned, unsigned)));
	QObject::connect(&m_ssm, SIGNAL(onClear()), this, SLOT(ssm_onClear()));
	m_ssm.load("speed.csv");
	for(size_t i = 0; i < _STEPS_CNT; i++)
		ui_steps[i].calibrate->setEnabled(nullptr != m_ssm[i]);

	w_pg.setAttribute(Qt::WA_QuitOnClose, false);

	ui.tw_main->setCurrentIndex(0);
	log("Application launched");
}

MainWindow::~MainWindow() {
	try {
		s.save(_CONFIG_FN);
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
	xn.disconnect();

	QMessageBox m(
		QMessageBox::Icon::Warning,
		"Error!",
		"Serial port error!",
		QMessageBox::Ok
	);
	m.exec();
}

void MainWindow::cb_xn_ll_index_changed(int index) {
	s.xn.loglevel = index;
	xn.loglevel = static_cast<Xn::XnLogLevel>(index);
}

void MainWindow::show_error(const QString error) {
	log(error);

	QMessageBox m(
		QMessageBox::Icon::Warning,
		"Error!",
		error,
		QMessageBox::Ok
	);
	m.exec();
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

///////////////////////////////////////////////////////////////////////////////
// XN Events

void MainWindow::xn_onError(QString error) {
	xn_onLog(error, Xn::XnLogLevel::Error);

	if (!t_xn_disconnect.isActive() && xn.connected())
		t_xn_disconnect.start(100);
}

void MainWindow::xn_onLog(QString message, Xn::XnLogLevel loglevel) {
	QTreeWidgetItem* item = new QTreeWidgetItem(ui.tw_xn_log);
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
		xn.getLIVersion(&xns_gotLIVersion, std::make_unique<Xn::XnCb>(&xns_onLIVersionError));
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
	}else if (status == Xn::XnTrkStatus::Programming) {
		widget_set_color(*(ui.l_dcc), Qt::yellow);
		log("CS status: Programming");
	}
}

void MainWindow::xns_onDccGoError(void* s, void* d) { wref->xn_onDccGoError(s, d); }
void MainWindow::xns_onDccStopError(void* s, void* d) { wref->xn_onDccStopError(s, d); }
void MainWindow::xns_onLIVersionError(void* s, void* d) { wref->xn_onLIVersionError(s, d); }
void MainWindow::xns_onCSVersionError(void* s, void* d) { wref->xn_onCSVersionError(s, d); }
void MainWindow::xns_onCSStatusError(void* s, void* d) { wref->xn_onCSStatusError(s, d); }
void MainWindow::xns_onCSStatusOk(void* s, void* d) { wref->xn_onCSStatusOk(s, d); }
void MainWindow::xns_gotLIVersion(void* s, unsigned hw, unsigned sw) { wref->xn_gotLIVersion(s, hw, sw); }
void MainWindow::xns_gotCSVersion(void* s, unsigned major, unsigned minor) { wref->xn_gotCSVersion(s, major, minor); }
void MainWindow::xns_gotLocoInfo(void* s, bool used, Xn::XnDirection direction, unsigned speed, Xn::XnFA fa, Xn::XnFB fb) {
	wref->xn_gotLocoInfo(s, used, direction, speed, fa, fb);
}
void MainWindow::xns_onLocoInfoError(void* s, void* d) { wref->xn_onLocoInfoError(s, d); }
void MainWindow::xns_addrReadError(void* s, void* d) { wref->xn_addrReadError(s, d); }
void MainWindow::xns_adReadError(void* s, void* d) { wref->xn_adReadError(s, d); }
void MainWindow::xns_cvRead(void* s, Xn::XnReadCVStatus st, uint8_t cv, uint8_t value) {
	wref->xn_cvRead(s, st, cv, value);
}
void MainWindow::xns_adWriteError(void* s, void* d) { wref->xn_adWriteError(s, d); }
void MainWindow::xns_accelWritten(void* s, void* d) { wref->xn_accelWritten(s, d); }
void MainWindow::xns_decelWritten(void* s, void* d) { wref->xn_decelWritten(s, d); }

void MainWindow::xn_onDccGoError(void* sender, void* data) {
	(void)sender; (void)data;
	show_response_error("DCC GO");
}

void MainWindow::xn_onDccStopError(void* sender, void* data) {
	(void)sender; (void)data;
	show_response_error("DCC STOP");
}

void MainWindow::show_response_error(QString command) {
	log("Command station did not respond to " + command + " command!");
	QMessageBox m(
		QMessageBox::Icon::Warning,
		"Error!",
		"Command station did not respond to " + command + " command!",
		QMessageBox::Ok
	);
}

void MainWindow::xn_onLIVersionError(void* sender, void* data) {
	(void)sender; (void)data;
	m_starting = false;
	log("LI did not respond to version request!");
	QMessageBox m(
		QMessageBox::Icon::Warning,
		"Error!",
		"LI did not respond to version request, are you really connected to the LI?!",
		QMessageBox::Ok
	);
}

void MainWindow::xn_onCSVersionError(void* sender, void* data) {
	(void)sender; (void)data;
	log("Coomand station did not respond to version request!");
	QMessageBox m(
		QMessageBox::Icon::Warning,
		"Error!",
		"Command station did not respond to version request"
		", is the LI really connected to the command station?!",
		QMessageBox::Ok
	);
	m_starting = false;
}

void MainWindow::xn_onCSStatusError(void* sender, void* data) {
	(void)sender; (void)data;
	show_response_error("STATUS");
	m_starting = false;
}

void MainWindow::xn_onCSStatusOk(void* sender, void* data) {
	(void)sender; (void)data;
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
		xn.getCommandStationStatus(std::make_unique<Xn::XnCb>(xns_onCSStatusOk),
		                           std::make_unique<Xn::XnCb>(xns_onCSStatusError));
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
	ui.rb_forward->setChecked(static_cast<bool>(direction));

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
		xn.readCVdirect(_CV_ADDR_HI, &xns_cvRead, std::make_unique<Xn::XnCb>(&xns_addrReadError));
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
		xn.readCVdirect(_CV_DECEL, &xns_cvRead, std::make_unique<Xn::XnCb>(&xns_adReadError));
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
	xn.PomWriteCv(Xn::LocoAddr(ui.sb_loco->value()), _CV_DECEL, ui.sb_decel->value(),
	              std::make_unique<Xn::XnCb>(&xns_decelWritten),
	              std::make_unique<Xn::XnCb>(&xns_adWriteError));
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
		xn.connect(s.xn.portname, s.xn.br, s.xn.fc);
	} catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::a_xn_disconnect(bool) {
	if (!xn.connected())
		return;

	try {
		log("Disconnecting from XN...");
		xn.disconnect();
	} catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::a_dcc_go(bool) {
	try {
		if (xn.connected())
			xn.setTrkStatus(Xn::XnTrkStatus::On, nullptr,
			                std::make_unique<Xn::XnCb>(&xns_onDccGoError));
	} catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::a_dcc_stop(bool) {
	try {
		if (xn.connected())
			xn.setTrkStatus(Xn::XnTrkStatus::Off, nullptr,
			                std::make_unique<Xn::XnCb>(&xns_onDccStopError));
	} catch (const QStrException& e) {
		show_error(e.str());
	}
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::log(QString message) {
	ui.lv_log->insertItem(0, message);
}

///////////////////////////////////////////////////////////////////////////////
// Speed settings

void MainWindow::b_addr_set_handle() {
	ui.b_addr_set->setEnabled(false);
	ui.sb_loco->setEnabled(false);
	ui.b_addr_read->setEnabled(false);

	xn.getLocoInfo(Xn::LocoAddr(ui.sb_loco->value()), &xns_gotLocoInfo,
	               std::make_unique<Xn::XnCb>(&xns_onLocoInfoError));
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
	xn.readCVdirect(_CV_ADDR_LO, &xns_cvRead, std::make_unique<Xn::XnCb>(&xns_addrReadError));
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

void MainWindow::rb_direction_toggled(bool backward) {
	(void)backward;
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

void MainWindow::a_wsm_connect(bool a) {
	(void)a;
	log("Connecting to WSM...");

	try {
		wsm.connect(s.wsm.portname);

		ui.a_wsm_connect->setEnabled(false);
		ui.a_wsm_disconnect->setEnabled(true);
		widget_set_color(*(ui.l_wsm), Qt::green);

		log("Connected to WSM");
	} catch (const Wsm::EOpenError& e) {
		show_error("Error while opening serial port '" + s.wsm.portname + "':\n" + e);
	}
}

void MainWindow::a_wsm_disconnect(bool a) {
	(void)a;

	wsm.disconnect();
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
	if (!t_wsm_disconnect.isActive()) {
		t_wsm_disconnect.start(100);
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
	QMessageBox m(
		QMessageBox::Icon::Warning,
		"Warning",
		"Battery level critical, device is shutting down!",
		QMessageBox::Ok
	);
	m.exec();

	if (!t_wsm_disconnect.isActive())
		t_wsm_disconnect.start(100);
}

void MainWindow::t_mc_disconnect_tick() {
	log("WSM error, disconnecting...");
	wsm.disconnect();
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

void MainWindow::a_power_graph() {
	w_pg.move(ui.centralwidget->pos().x() + ui.centralwidget->size().width(),
	          ui.centralwidget->pos().y() - ui.mb_main->size().height());
	w_pg.show();
}

void MainWindow::init_calib_graph() {
	for(size_t i = 0; i < _STEPS_CNT; i++) {
		QLabel* speed_want = new QLabel("-", ui.gb_cal_graph);
		speed_want->setFont(QFont("Sans Serif", 8));
		speed_want->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].speed_want = speed_want;
		ui.l_cal_graph->addWidget(speed_want, 0, i);

		QLabel* speed_measured = new QLabel("??", ui.gb_cal_graph);
		speed_measured->setFont(QFont("Sans Serif", 8));
		speed_measured->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].speed_measured = speed_measured;
		ui.l_cal_graph->addWidget(speed_measured, 1, i);

		QLabel* value = new QLabel("0", ui.gb_cal_graph);
		value->setFont(QFont("Sans Serif", 8));
		value->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].value = value;
		ui.l_cal_graph->addWidget(value, 2, i);

		QSlider* slider = new QSlider(Qt::Orientation::Vertical, ui.gb_cal_graph);
		slider->setMinimum(0);
		slider->setMaximum(255);
		slider->setProperty("step", static_cast<uint>(i));
		QObject::connect(slider, SIGNAL(valueChanged(int)), this, SLOT(vs_steps_moved(int)));
		ui_steps[i].slider = slider;
		ui.l_cal_graph->addWidget(slider, 3, i);

		QCheckBox* selected = new QCheckBox(ui.gb_cal_graph);
		selected->setProperty("step", static_cast<uint>(i));
		ui_steps[i].selected = selected;
		ui.l_cal_graph->addWidget(selected, 4, i);

		QLabel* step = new QLabel(QString::number(i+1), ui.gb_cal_graph);
		step->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].step = step;
		ui.l_cal_graph->addWidget(step, 5, i);

		QPushButton* calibrate = new QPushButton("C", ui.gb_cal_graph);
		calibrate->setProperty("step", static_cast<uint>(i));
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
	unsigned step = qobject_cast<QPushButton*>(QObject::sender())->property("step").toUInt() + 1;
	if (nullptr != m_ssm[step-1]) {
		cm.cs.calibrate(ui.sb_loco->value(), step, *(m_ssm[step-1]));
		log("Starting calibration of step " + QString::number(step));
	}
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

void MainWindow::cs_diffusion_error(unsigned) {
	log("Loco is to diffused!");
}

void MainWindow::cs_loco_stopped(unsigned) {
	log("Loco is stopped by outer source!");
}

void MainWindow::cs_done(unsigned, unsigned) {
	log("Calibration of a step done.");
}

void MainWindow::cs_xn_error(unsigned) {
	log("XpressNET error while calibration!");
}

void MainWindow::cs_step_power_changed(unsigned step, unsigned power) {
	ui_steps[step-1].slider->setValue(power);
	log("Setting step " + QString::number(step) + " to " + QString::number(power));
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::cm_stepDone(unsigned step, unsigned power) {
	(void)power;
	log("Step " + QString::number(step) + " done");
}

void MainWindow::cm_stepStart(unsigned step) {
	log("Starting calibration of step " + QString::number(step));
}

void MainWindow::cm_stepError(unsigned step) {
	log("Step " + QString::number(step) + " calibration error!");
	ui.b_calib_start->setEnabled(true);
	ui.gb_cal_graph->setEnabled(true);
	ui.gb_ad->setEnabled(true);
	ui.b_wsm_lt->setEnabled(true);
}

void MainWindow::cm_locoSpeedChanged(unsigned step) {
	ui.vs_speed->setValue(step);
	ui.sb_speed->setValue(step);
	m_sent_speed = step;
}

void MainWindow::cm_done() {
	log("Calibration done :)");
	ui.b_calib_start->setEnabled(true);
	ui.gb_cal_graph->setEnabled(true);
	ui.gb_ad->setEnabled(true);
	ui.b_wsm_lt->setEnabled(true);
}

void MainWindow::cm_stepPowerChanged(unsigned step, unsigned power) {
	cs_step_power_changed(step, power);
}

void MainWindow::b_calib_start_handle() {
	if (wsm.connected() && wsm.isSpeedOk() && xn.connected()) {
		ui.b_calib_start->setEnabled(false);
		ui.gb_cal_graph->setEnabled(false);
		ui.gb_ad->setEnabled(false);
		ui.b_wsm_lt->setEnabled(false);
		cm.calibrateAll(ui.sb_loco->value(),
		                static_cast<Xn::XnDirection>(ui.rb_forward->isChecked()));
		}
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::b_ad_read_handle() {
	ui.gb_ad->setEnabled(false);
	xn.readCVdirect(_CV_ACCEL, &xns_cvRead, std::make_unique<Xn::XnCb>(&xns_adReadError));
}

void MainWindow::b_ad_write_handle() {
	if (ui.sb_loco->isEnabled()) {
		show_error("Loco must be set!");
		return;
	}

	ui.gb_ad->setEnabled(false);

	xn.PomWriteCv(Xn::LocoAddr(ui.sb_loco->value()), _CV_ACCEL, ui.sb_accel->value(),
	              std::make_unique<Xn::XnCb>(&xns_accelWritten),
	              std::make_unique<Xn::XnCb>(&xns_adWriteError));
}

///////////////////////////////////////////////////////////////////////////////
