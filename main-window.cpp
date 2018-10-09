#include <QMessageBox>
#include <QSlider>

#include "main-window.h"
#include "ui_main-window.h"

const unsigned int WSM_BLINK_TIMEOUT = 250; // ms
MainWindow* wref = nullptr;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent), xn(this), s(_CONFIG_FN) {
	ui.setupUi(this);
	wref = this;

	QObject::connect(&xn, SIGNAL(onError(QString)), this, SLOT(xn_onError(QString)));
	QObject::connect(&xn, SIGNAL(onLog(QString, Xn::XnLogLevel)), this, SLOT(xn_onLog(QString, Xn::XnLogLevel)));
	QObject::connect(&xn, SIGNAL(onConnect()), this, SLOT(xn_onConnect()));
	QObject::connect(&xn, SIGNAL(onDisconnect()), this, SLOT(xn_onDisconnect()));
	QObject::connect(&xn, SIGNAL(onTrkStatusChanged(Xn::XnTrkStatus)), this, SLOT(xn_onTrkStatusChanged(Xn::XnTrkStatus)));

	QObject::connect(ui.b_start, SIGNAL(released()), this, SLOT(b_start_handle()));

	QObject::connect(ui.b_addr_set, SIGNAL(released()), this, SLOT(b_addr_set_handle()));
	QObject::connect(ui.b_addr_release, SIGNAL(released()), this, SLOT(b_addr_release_handle()));
	QObject::connect(ui.b_addr_read, SIGNAL(released()), this, SLOT(b_addr_read_handle()));
	QObject::connect(ui.b_speed_set, SIGNAL(released()), this, SLOT(b_speed_set_handle()));
	QObject::connect(ui.b_loco_stop, SIGNAL(released()), this, SLOT(b_loco_stop_handle()));
	QObject::connect(ui.vs_speed, SIGNAL(valueChanged(int)), this, SLOT(vs_speed_slider_moved(int)));
	QObject::connect(ui.rb_backward, SIGNAL(toggled(bool)), this, SLOT(rb_direction_toggled(bool)));
	QObject::connect(ui.chb_f0, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));
	QObject::connect(ui.chb_f1, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));
	QObject::connect(ui.chb_f2, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));

	t_xn_disconnect.setSingleShot(true);
	QObject::connect(&t_xn_disconnect, SIGNAL(timeout()), this, SLOT(t_xn_disconnect_tick()));

	t_slider.start(100);
	QObject::connect(&t_slider, SIGNAL(timeout()), this, SLOT(t_slider_tick()));

	widget_set_color(*(ui.l_xn), Qt::red);
	widget_set_color(*(ui.l_dcc), Qt::gray);
	widget_set_color(*(ui.l_wsm), Qt::red);
	widget_set_color(*(ui.l_wsm_alive), Qt::gray);

	ui.cb_xn_loglevel->setCurrentIndex(s.xn.loglevel);
	xn.loglevel = static_cast<Xn::XnLogLevel>(s.xn.loglevel);
	QObject::connect(ui.cb_xn_loglevel, SIGNAL(currentIndexChanged(int)), this, SLOT(cb_xn_ll_index_changed(int)));

	QObject::connect(ui.a_xn_connect, SIGNAL(triggered(bool)), this, SLOT(a_xn_connect(bool)));
	QObject::connect(ui.a_xn_disconnect, SIGNAL(triggered(bool)), this, SLOT(a_xn_disconnect(bool)));
	QObject::connect(ui.a_xn_dcc_go, SIGNAL(triggered(bool)), this, SLOT(a_dcc_go(bool)));
	QObject::connect(ui.a_xn_dcc_stop, SIGNAL(triggered(bool)), this, SLOT(a_dcc_stop(bool)));

	QObject::connect(ui.a_wsm_connect, SIGNAL(triggered(bool)), this, SLOT(a_wsm_connect(bool)));
	QObject::connect(ui.a_wsm_disconnect, SIGNAL(triggered(bool)), this, SLOT(a_wsm_disconnect(bool)));

	QObject::connect(ui.m_power_graph, SIGNAL(aboutToShow()), this, SLOT(a_power_graph()));

	init_calib_graph();

	ui.tw_main->setCurrentIndex(0);
	log("Application launched.");
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
	disconnect();

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
		if (nullptr == wsm)
			a_wsm_connect(true);
	}
}

///////////////////////////////////////////////////////////////////////////////
// XN Events

void MainWindow::xn_onError(QString error) {
	xn_onLog(error, Xn::XnLogLevel::Error);

	if (!t_xn_disconnect.isActive())
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
	ui.chb_f0->setEnabled(false);
	ui.chb_f1->setEnabled(false);
	ui.chb_f2->setEnabled(false);
	ui.rb_forward->setEnabled(false);
	ui.rb_backward->setEnabled(false);
	ui.b_addr_release->setEnabled(false);
	ui.sb_loco->setEnabled(true);
	ui.b_addr_set->setEnabled(true);
	ui.b_addr_read->setEnabled(true);
	ui.b_loco_stop->setEnabled(false);
	ui.b_speed_set->setEnabled(false);

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
	loco_released();
	log("Disconnected from XpressNET");
}

void MainWindow::xn_onTrkStatusChanged(Xn::XnTrkStatus status) {
	if (status == Xn::XnTrkStatus::Unknown)
		widget_set_color(*(ui.l_dcc), Qt::gray);
	else if (status == Xn::XnTrkStatus::Off)
		widget_set_color(*(ui.l_dcc), Qt::red);
	if (status == Xn::XnTrkStatus::On)
		widget_set_color(*(ui.l_dcc), Qt::green);
	if (status == Xn::XnTrkStatus::Programming)
		widget_set_color(*(ui.l_dcc), Qt::yellow);
}

void MainWindow::xns_onDccGoError(void* s, void* d) { wref->xn_onDccGoError(s, d); }
void MainWindow::xns_onDccStopError(void* s, void* d) { wref->xn_onDccStopError(s, d); }
void MainWindow::xns_onLIVersionError(void* s, void* d) { wref->xn_onLIVersionError(s, d); }
void MainWindow::xns_onCSVersionError(void* s, void* d) { wref->xn_onCSVersionError(s, d); }
void MainWindow::xns_onCSStatusError(void* s, void* d) { wref->xn_onCSStatusError(s, d); }
void MainWindow::xns_onCSStatusOk(void* s, void* d) { wref->xn_onCSStatusOk(s, d); }
void MainWindow::xns_gotLIVersion(void* s, unsigned hw, unsigned sw) { wref->xn_gotLIVersion(s, hw, sw); }
void MainWindow::xns_gotCSVersion(void* s, unsigned major, unsigned minor) { wref->xn_gotCSVersion(s, major, minor); }
void MainWindow::xns_gotLocoInfo(void* s, bool used, bool direction, unsigned speed, Xn::XnFA fa, Xn::XnFB fb) {
	wref->xn_gotLocoInfo(s, used, direction, speed, fa, fb);
}
void MainWindow::xns_onLocoInfoError(void* s, void* d) { wref->xn_onLocoInfoError(s, d); }

void MainWindow::xn_onDccGoError(void* sender, void* data) {
	(void)sender; (void)data;
	show_response_error("DCC GO");
}

void MainWindow::xn_onDccStopError(void* sender, void* data) {
	(void)sender; (void)data;
	show_response_error("DCC STOP");
}

void MainWindow::show_response_error(QString command) {
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
	QMessageBox m(
		QMessageBox::Icon::Warning,
		"Error!",
		"LI not respond to version request, are you really connected to the LI?!",
		QMessageBox::Ok
	);
}

void MainWindow::xn_onCSVersionError(void* sender, void* data) {
	(void)sender; (void)data;
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
		if (nullptr == wsm)
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

void MainWindow::xn_gotLocoInfo(void*, bool used, bool direction, unsigned speed,
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

	ui.rb_backward->setChecked(direction);
	ui.rb_backward->setEnabled(true);
	ui.rb_forward->setEnabled(true);

	ui.chb_f0->setChecked(fa.sep.f0);
	ui.chb_f0->setEnabled(true);

	ui.chb_f1->setChecked(fa.sep.f1);
	ui.chb_f1->setEnabled(true);

	ui.chb_f2->setChecked(fa.sep.f2);
	ui.chb_f2->setEnabled(true);

	ui.b_loco_stop->setEnabled(true);
	ui.b_speed_set->setEnabled(true);
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
	ui.b_speed_set->setEnabled(false);
}

///////////////////////////////////////////////////////////////////////////////
// XpressNET connect/disconnect

void MainWindow::a_xn_connect(bool) {
	if (xn.connected())
		return;

	try {
		xn.connect(s.xn.portname, s.xn.br, s.xn.fc);
	} catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::a_xn_disconnect(bool) {
	if (!xn.connected())
		return;

	try {
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
}

void MainWindow::b_speed_set_handle() {
	try {
		xn.setSpeed(Xn::LocoAddr(ui.sb_loco->value()), ui.sb_speed->value(),
		            ui.rb_backward->isChecked());
		ui.vs_speed->setValue(ui.sb_speed->value());
	}
	catch (const QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::b_loco_stop_handle() {
	try {
		xn.emergencyStop(Xn::LocoAddr(ui.sb_loco->value()));
	}
	catch (const QStrException& e) {
		show_error(e.str());
	}
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
			            ui.rb_backward->isChecked());
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

///////////////////////////////////////////////////////////////////////////////
// WSM functions

void MainWindow::wsm_status_blink() {
	QPalette palette = ui.l_wsm_alive->palette();
	QColor color = palette.color(QPalette::WindowText);
	if (color == Qt::gray)
		widget_set_color(*(ui.l_wsm_alive), palette.color(QPalette::Window));
	else
		widget_set_color(*(ui.l_wsm_alive), Qt::gray);
}

void MainWindow::a_wsm_connect(bool a) {
	(void)a;

	try {
		wsm = std::make_unique<Wsm::MeasureCar>(s.wsm.portname, s.wsm.scale, s.wsm.wheelDiameter);
		QObject::connect(wsm.get(), SIGNAL(speedRead(double, uint16_t)), this, SLOT(mc_speedRead(double, uint16_t)));
		QObject::connect(wsm.get(), SIGNAL(onError(QString)), this, SLOT(mc_onError(QString)));
		QObject::connect(wsm.get(), SIGNAL(batteryRead(double, uint16_t)), this, SLOT(mc_batteryRead(double, uint16_t)));
		QObject::connect(wsm.get(), SIGNAL(batteryCritical()), this, SLOT(mc_batteryCritical()));
		QObject::connect(wsm.get(), SIGNAL(distanceRead(double, uint32_t)), this, SLOT(mc_distanceRead(double, uint32_t)));

		ui.a_wsm_connect->setEnabled(false);
		ui.a_wsm_disconnect->setEnabled(true);
		widget_set_color(*(ui.l_wsm), Qt::green);
	} catch (const Wsm::EOpenError& e) {
		show_error("Error while opening serial port '" + s.wsm.portname + "':\n" + e);
	}
}

void MainWindow::a_wsm_disconnect(bool a) {
	(void)a;

	wsm = nullptr;
	ui.l_wsm_speed->setText("??.?");
	ui.l_wsm_bat_voltage->setText("?.?? V");
	widget_set_color(*(ui.l_wsm_alive), ui.l_wsm_speed->palette().color(QPalette::WindowText));
	ui.a_wsm_connect->setEnabled(true);
	ui.a_wsm_disconnect->setEnabled(false);
	widget_set_color(*(ui.l_wsm), Qt::red);
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
	wsm->disconnect();
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::a_power_graph() {
	w_pg.show();
}

void MainWindow::init_calib_graph() {
	for(size_t i = 0; i < _STEPS_CNT; i++) {
		QLabel* speed = new QLabel("??", ui.gb_cal_graph);
		speed->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].speed = speed;
		ui.l_cal_graph->addWidget(speed, 0, i);

		QLabel* value = new QLabel("0", ui.gb_cal_graph);
		value->setFont(QFont("Sans Serif", 8));
		value->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].value = value;
		ui.l_cal_graph->addWidget(value, 1, i);

		QSlider* slider = new QSlider(Qt::Orientation::Vertical, ui.gb_cal_graph);
		slider->setMinimum(0);
		slider->setMaximum(255);
		slider->setProperty("step", static_cast<uint>(i));
		QObject::connect(slider, SIGNAL(valueChanged(int)), this, SLOT(vs_steps_moved(int)));
		ui_steps[i].slider = slider;
		ui.l_cal_graph->addWidget(slider, 2, i);

		QCheckBox* selected = new QCheckBox(ui.gb_cal_graph);
		selected->setProperty("step", static_cast<uint>(i));
		ui_steps[i].selected = selected;
		ui.l_cal_graph->addWidget(selected, 3, i);

		QLabel* step = new QLabel(QString::number(i+1), ui.gb_cal_graph);
		step->setAlignment(Qt::AlignmentFlag::AlignHCenter);
		ui_steps[i].step = step;
		ui.l_cal_graph->addWidget(step, 4, i);

		QPushButton* calibrate = new QPushButton("C", ui.gb_cal_graph);
		calibrate->setProperty("step", static_cast<uint>(i));
		ui_steps[i].calibrate = calibrate;
		ui.l_cal_graph->addWidget(calibrate, 5, i);
	}
}

void MainWindow::vs_steps_moved(int value) {
	unsigned stepi = qobject_cast<QSlider*>(QObject::sender())->property("step").toUInt();
	ui_steps[stepi].value->setText(QString::number(value));
}

///////////////////////////////////////////////////////////////////////////////
