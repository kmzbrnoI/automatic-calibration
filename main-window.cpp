#include <QMessageBox>

#include "main-window.h"
#include "ui_main-window.h"

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

	QObject::connect(ui.b_addr_set, SIGNAL(released()), this, SLOT(b_addr_set_handle()));
	QObject::connect(ui.b_addr_release, SIGNAL(released()), this, SLOT(b_addr_release_handle()));
	QObject::connect(ui.b_addr_read, SIGNAL(released()), this, SLOT(b_addr_read_handle()));
	QObject::connect(ui.b_speed_set, SIGNAL(released()), this, SLOT(b_speed_set_handle()));
	QObject::connect(ui.b_loco_stop, SIGNAL(released()), this, SLOT(b_loco_stop_handle()));
	QObject::connect(ui.vs_speed, SIGNAL(sliderMoved(int)), this, SLOT(vs_speed_slider_moved(int)));
	QObject::connect(ui.rb_backward, SIGNAL(toggled(bool)), this, SLOT(rb_direction_toggled(bool)));

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
	ui.tw_xn_log->insertTopLevelItem(0, item);
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

	log("Connected to XpressNET.");
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
	log("Disconnected from XpressNET");
	xn.getLIVersion(&xns_gotLIVersion, std::make_unique<Xn::XnCb>(&xns_onLIVersionError));
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
void MainWindow::xns_gotLIVersion(void* s, unsigned hw, unsigned sw) { wref->xn_gotLIVersion(s, hw, sw); }
void MainWindow::xns_gotCSVersion(void* s, unsigned major, unsigned minor) { wref->xn_gotCSVersion(s, major, minor); }

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
}

void MainWindow::xn_onCSStatusError(void* sender, void* data) {
	(void)sender; (void)data;
	show_response_error("STATUS");
}

void MainWindow::xn_gotLIVersion(void*, unsigned hw, unsigned sw) {
	log("Got LI version. HW: " + QString::number(hw) + ", SW: " + QString::number(sw));
	xn.getCommandStationStatus(nullptr, std::make_unique<Xn::XnCb>(xns_onCSStatusError));
}

void MainWindow::xn_gotCSVersion(void*, unsigned major, unsigned minor) {
	log("Got command station version:" + QString::number(major) + "." + QString::number(minor));
}

///////////////////////////////////////////////////////////////////////////////
// XpressNET connect/disconnect

void MainWindow::a_xn_connect(bool) {
	if (xn.connected())
		return;

	try {
		xn.connect(s.xn.portname, s.xn.br, s.xn.fc);
	} catch (const QStrException& e) {
		QMessageBox m(
			QMessageBox::Icon::Warning,
			"Error!",
			e.str(),
			QMessageBox::Ok
		);
		m.exec();
	}
}

void MainWindow::a_xn_disconnect(bool) {
	if (!xn.connected())
		return;

	try {
		xn.disconnect();
	} catch (const QStrException& e) {
		QMessageBox m(
			QMessageBox::Icon::Warning,
			"Error!",
			e.str(),
			QMessageBox::Ok
		);
		m.exec();
	}
}

void MainWindow::a_dcc_go(bool) {
	if (xn.connected())
		xn.setTrkStatus(Xn::XnTrkStatus::On, nullptr,
		                std::make_unique<Xn::XnCb>(&xns_onDccGoError));
}

void MainWindow::a_dcc_stop(bool) {
	if (xn.connected())
		xn.setTrkStatus(Xn::XnTrkStatus::Off, nullptr,
		                std::make_unique<Xn::XnCb>(&xns_onDccStopError));
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::log(QString message) {
}

///////////////////////////////////////////////////////////////////////////////
// Speed settings

void MainWindow::b_addr_set_handle() {
	ui.b_addr_set->setEnabled(false);
	ui.b_addr_release->setEnabled(true);
	ui.sb_loco->setEnabled(false);
	ui.b_addr_read->setEnabled(false);
}

void MainWindow::b_addr_release_handle() {
	ui.b_addr_set->setEnabled(true);
	ui.b_addr_release->setEnabled(false);
	ui.sb_loco->setEnabled(true);
	ui.b_addr_read->setEnabled(true);

	ui.vs_speed->setEnabled(false);
	ui.sb_speed->setEnabled(false);
	ui.b_speed_set->setEnabled(false);
	ui.b_loco_stop->setEnabled(false);
	ui.chb_f0->setEnabled(false);
	ui.chb_f1->setEnabled(false);
	ui.chb_f2->setEnabled(false);
}

void MainWindow::b_addr_read_handle() {
}

void MainWindow::b_speed_set_handle() {
	xn.setSpeed(Xn::LocoAddr(ui.sb_loco->value()), ui.sb_speed->value(),
	            ui.rb_backward->isChecked());
	ui.vs_speed->setValue(ui.sb_loco->value());
}

void MainWindow::b_loco_stop_handle() {
	xn.emergencyStop(Xn::LocoAddr(ui.sb_loco->value()));
}

void MainWindow::vs_speed_slider_moved(int value) {
	ui.sb_speed->setValue(value);
}

void MainWindow::rb_direction_toggled(bool backward) {
	(void)backward;
	b_speed_set_handle();
}

void MainWindow::t_slider_tick() {
	if (ui.vs_speed->value() != m_sent_speed) {
		m_sent_speed = ui.vs_speed->value();
		xn.setSpeed(LocoAddr(ui.sb_loco->value()), ui.sb_speed->value(),
		            ui.rb_backward->isChecked());
	}
}

///////////////////////////////////////////////////////////////////////////////
