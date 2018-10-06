#include <QMessageBox>

#include "main-window.h"
#include "ui_main-window.h"

MainWindow* wref = nullptr;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent), xn(this), s(_CONFIG_FN) {
	ui.setupUi(this);
	wref = this;

	QObject::connect(&xn, SIGNAL(onError(QString)), this, SLOT(xn_onError(QString)));
	QObject::connect(&xn, SIGNAL(onLog(QString, XnLogLevel)), this, SLOT(xn_onLog(QString, XnLogLevel)));
	QObject::connect(&xn, SIGNAL(onConnect()), this, SLOT(xn_onConnect()));
	QObject::connect(&xn, SIGNAL(onDisconnect()), this, SLOT(xn_onDisconnect()));
	QObject::connect(&xn, SIGNAL(onTrkStatusChanged(XnTrkStatus)), this, SLOT(xn_onTrkStatusChanged(XnTrkStatus)));

	t_xn_disconnect.setSingleShot(true);
	QObject::connect(&t_xn_disconnect, SIGNAL(timeout()), this, SLOT(t_xn_disconnect_tick()));

	widget_set_color(*(ui.l_xn), Qt::red);
	widget_set_color(*(ui.l_dcc), Qt::gray);
	widget_set_color(*(ui.l_wsm), Qt::red);
	widget_set_color(*(ui.l_wsm_alive), Qt::gray);

	ui.cb_xn_loglevel->setCurrentIndex(s.xn.loglevel);
	xn.loglevel = static_cast<XnLogLevel>(s.xn.loglevel);
	QObject::connect(ui.cb_xn_loglevel, SIGNAL(currentIndexChanged(int)), this, SLOT(cb_xn_ll_index_changed(int)));

	QObject::connect(ui.a_xn_connect, SIGNAL(triggered(bool)), this, SLOT(a_xn_connect(bool)));
	QObject::connect(ui.a_xn_disconnect, SIGNAL(triggered(bool)), this, SLOT(a_xn_disconnect(bool)));
	QObject::connect(ui.a_xn_dcc_go, SIGNAL(triggered(bool)), this, SLOT(a_dcc_go(bool)));
	QObject::connect(ui.a_xn_dcc_stop, SIGNAL(triggered(bool)), this, SLOT(a_dcc_stop(bool)));

	ui.tw_main->setCurrentIndex(0);
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
	xn.loglevel = static_cast<XnLogLevel>(index);
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::xn_onError(QString error) {
	xn_onLog(error, XnLogLevel::Error);

	if (!t_xn_disconnect.isActive())
		t_xn_disconnect.start(100);
}

void MainWindow::xn_onLog(QString message, XnLogLevel loglevel) {
	QTreeWidgetItem* item = new QTreeWidgetItem(ui.tw_xn_log);
	item->setText(0, QTime::currentTime().toString("hh:mm:ss"));

	if (loglevel == XnLogLevel::None)
		item->setText(1, "None");
	else if (loglevel == XnLogLevel::Error)
		item->setText(1, "Error");
	else if (loglevel == XnLogLevel::Warning)
		item->setText(1, "Warning");
	else if (loglevel == XnLogLevel::Info)
		item->setText(1, "Info");
	else if (loglevel == XnLogLevel::Data)
		item->setText(1, "Data");
	else if (loglevel == XnLogLevel::Debug)
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
}

void MainWindow::xn_onDisconnect() {
	widget_set_color(*(ui.l_xn), Qt::red);
	widget_set_color(*(ui.l_dcc), Qt::gray);
	ui.a_xn_connect->setEnabled(true);
	ui.a_xn_disconnect->setEnabled(false);
	ui.a_xn_dcc_go->setEnabled(false);
	ui.a_xn_dcc_stop->setEnabled(false);
}

void MainWindow::xn_onTrkStatusChanged(XnTrkStatus status) {
	if (status == XnTrkStatus::Unknown)
		widget_set_color(*(ui.l_dcc), Qt::gray);
	else if (status == XnTrkStatus::Off)
		widget_set_color(*(ui.l_dcc), Qt::red);
	if (status == XnTrkStatus::On)
		widget_set_color(*(ui.l_dcc), Qt::green);
	if (status == XnTrkStatus::Programming)
		widget_set_color(*(ui.l_dcc), Qt::yellow);
}

void MainWindow::xns_onDccGoError(void* sender, void* data) { wref->xn_onDccGoError(sender, data); }
void MainWindow::xns_onDccStopError(void* sender, void* data) { wref->xn_onDccStopError(sender, data); }

void MainWindow::xn_onDccGoError(void* sender, void* data) {
}

void MainWindow::xn_onDccStopError(void* sender, void* data) {
}

///////////////////////////////////////////////////////////////////////////////

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
		xn.setTrkStatus(XnTrkStatus::On, nullptr, std::make_unique<XnCb>(&xns_onDccGoError));
}

void MainWindow::a_dcc_stop(bool) {
	if (xn.connected())
		xn.setTrkStatus(XnTrkStatus::Off);
}

void MainWindow::a_wsm_connect(bool) {
}

void MainWindow::a_wsm_disconnect(bool) {
}

///////////////////////////////////////////////////////////////////////////////
