#include <QFileDialog>
#include <QMessageBox>
#include <QSlider>
#include <QXmlStreamWriter>
#include <fstream>
#include <utility>

#include "main-window.h"
#include "ui_main-window.h"

const unsigned int WSM_BLINK_TIMEOUT = 250; // ms

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), xn(this), cm(xn, m_pm, wsm, m_ssm), cr(xn, wsm) {
	ui.setupUi(this);
	QString text = QString::asprintf("Automatic Calibration v%d.%d", VERSION_MAJOR, VERSION_MINOR);
	this->setWindowTitle(text);
	this->setFixedSize(this->size());

	const QStringList args = QCoreApplication::arguments();
	this->config_fn = args.size() > 1 ? args.at(1) : DEFAULT_CONFIG_FN;
	a_config_load(true);
	ui.b_start->setFocus();

	init_calib_graph();

	// Steps to Speed map
	QObject::connect(&m_ssm, SIGNAL(onAddOrUpdate(uint,uint)), this,
	                 SLOT(ssm_onAddOrUpdate(uint,uint)));
	QObject::connect(&m_ssm, SIGNAL(onClear()), this, SLOT(ssm_onClear()));
	a_speed_load(true);
	m_ssm.setMaxSpeed(m_ssm.maxSpeedInFile());

	// XN init
	QObject::connect(&xn, SIGNAL(onError(QString)), this, SLOT(xn_onError(QString)));
	QObject::connect(&xn, SIGNAL(onLog(QString,Xn::LogLevel)), this,
                     SLOT(xn_onLog(QString,Xn::LogLevel)));
	QObject::connect(&xn, SIGNAL(onConnect()), this, SLOT(xn_onConnect()));
	QObject::connect(&xn, SIGNAL(onDisconnect()), this, SLOT(xn_onDisconnect()));
	QObject::connect(&xn, SIGNAL(onTrkStatusChanged(Xn::TrkStatus)), this,
	                 SLOT(xn_onTrkStatusChanged(Xn::TrkStatus)));

	// UI signals
	QObject::connect(ui.b_vmax_read, SIGNAL(released()), this, SLOT(b_vmax_read_handle()));
	QObject::connect(ui.b_volt_ref_read, SIGNAL(released()), this, SLOT(b_volt_ref_read_handle()));
	QObject::connect(ui.chb_vmax, SIGNAL(clicked(bool)), this, SLOT(chb_vmax_clicked(bool)));
	QObject::connect(ui.chb_volt_ref, SIGNAL(clicked(bool)), this, SLOT(chb_volt_ref_clicked(bool)));
	QObject::connect(ui.b_start, SIGNAL(released()), this, SLOT(b_start_handle()));
	QObject::connect(ui.b_addr_set, SIGNAL(released()), this, SLOT(b_addr_set_handle()));
	QObject::connect(ui.b_addr_release, SIGNAL(released()), this, SLOT(b_addr_release_handle()));
	QObject::connect(ui.b_addr_read, SIGNAL(released()), this, SLOT(b_addr_read_handle()));
	QObject::connect(ui.b_speed_set, SIGNAL(released()), this, SLOT(b_speed_set_handle()));
	QObject::connect(ui.b_loco_stop, SIGNAL(released()), this, SLOT(b_loco_stop_handle()));
	QObject::connect(ui.b_loco_idle, SIGNAL(released()), this, SLOT(b_loco_idle_handle()));
	ui.sb_speed->setKeyboardTracking(false);
	QObject::connect(ui.sb_speed, SIGNAL(valueChanged(int)), this, SLOT(sb_speed_changed(int)));
	QObject::connect(ui.vs_speed, SIGNAL(valueChanged(int)), this,
	                 SLOT(vs_speed_slider_moved(int)));
	QObject::connect(ui.rb_backward, SIGNAL(toggled(bool)), this, SLOT(rb_direction_toggled(bool)));
	QObject::connect(ui.chb_f0, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));
	QObject::connect(ui.chb_f1, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));
	QObject::connect(ui.chb_f2, SIGNAL(clicked(bool)), this, SLOT(chb_f_clicked(bool)));
	QObject::connect(ui.b_calib_start, SIGNAL(released()), this, SLOT(b_calib_start_handle()));
	QObject::connect(ui.b_calib_stop, SIGNAL(released()), this, SLOT(b_calib_stop_handle()));
	QObject::connect(ui.b_reset, SIGNAL(released()), this, SLOT(b_reset_handle()));

	ui.sb_loco->setKeyboardTracking(false);
	QObject::connect(ui.sb_loco, SIGNAL(valueChanged(int)), this, SLOT(sb_loco_changed(int)));

#ifdef QT_NO_DEBUG
	ui.b_test1->setVisible(false);
	ui.b_test2->setVisible(false);
	ui.b_test3->setVisible(false);
#endif

	QObject::connect(ui.b_test1, SIGNAL(released()), this, SLOT(b_test1_handle()));
	QObject::connect(ui.b_test2, SIGNAL(released()), this, SLOT(b_test2_handle()));
	QObject::connect(ui.b_test3, SIGNAL(released()), this, SLOT(b_test3_handle()));

	ui.sb_max_speed->setValue(m_ssm.maxSpeedInFile());
	QObject::connect(ui.sb_max_speed, SIGNAL(valueChanged(int)), this,
	                 SLOT(sb_max_speed_changed(int)));
	QObject::connect(ui.lv_log, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
                     SLOT(lv_log_dblclick(QListWidgetItem*)));
	QObject::connect(ui.tw_xn_log, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this,
	                 SLOT(tw_xn_log_dblclick(QTreeWidgetItem*,int)));

	t_xn_disconnect.setSingleShot(true);
	QObject::connect(&t_xn_disconnect, SIGNAL(timeout()), this, SLOT(t_xn_disconnect_tick()));

	t_wsm_disconnect.setSingleShot(true);
	QObject::connect(&t_wsm_disconnect, SIGNAL(timeout()), this, SLOT(t_mc_disconnect_tick()));

	t_slider.start(100);
	QObject::connect(&t_slider, SIGNAL(timeout()), this, SLOT(t_slider_tick()));

	QObject::connect(ui.b_ad_read, SIGNAL(released()), this, SLOT(b_ad_read_handle()));
	QObject::connect(ui.b_ad_write, SIGNAL(released()), this, SLOT(b_ad_write_handle()));

	t_calib_active.start(500);
	QObject::connect(&t_calib_active, SIGNAL(timeout()), this, SLOT(t_calib_active_tick()));
	QObject::connect(ui.b_decel_measure, SIGNAL(released()), this, SLOT(b_decel_measure_handle()));

	widget_set_color(*ui.l_calib_state, Qt::gray);

	// UI set defaults
	widget_set_color(*(ui.l_xn), Qt::red);
	widget_set_color(*(ui.l_dcc), Qt::gray);
	widget_set_color(*(ui.l_wsm), Qt::red);
	widget_set_color(*(ui.l_wsm_alive), Qt::gray);
	gui_update_enabled();

	// XN UI
	ui.cb_xn_loglevel->setCurrentIndex(s["XN"]["loglevel"].toInt());
	xn.loglevel = static_cast<Xn::LogLevel>(s["XN"]["loglevel"].toInt());
	QObject::connect(ui.cb_xn_loglevel, SIGNAL(currentIndexChanged(int)), this,
	                 SLOT(cb_xn_ll_index_changed(int)));

	QObject::connect(ui.a_xn_connect, SIGNAL(triggered(bool)), this, SLOT(a_xn_connect(bool)));
	QObject::connect(ui.a_xn_disconnect, SIGNAL(triggered(bool)), this,
	                 SLOT(a_xn_disconnect(bool)));
	QObject::connect(ui.a_xn_dcc_go, SIGNAL(triggered(bool)), this, SLOT(a_dcc_go(bool)));
	QObject::connect(ui.a_xn_dcc_stop, SIGNAL(triggered(bool)), this, SLOT(a_dcc_stop(bool)));

	QObject::connect(ui.a_wsm_connect, SIGNAL(triggered(bool)), this, SLOT(a_wsm_connect(bool)));
	QObject::connect(ui.a_wsm_disconnect, SIGNAL(triggered(bool)), this,
	                 SLOT(a_wsm_disconnect(bool)));
	QObject::connect(ui.b_wsm_lt, SIGNAL(released()), this, SLOT(b_wsm_lt_handle()));

	QObject::connect(ui.a_power_graph, SIGNAL(triggered(bool)), this, SLOT(a_power_graph(bool)));
	QObject::connect(ui.a_loco_load, SIGNAL(triggered(bool)), this, SLOT(a_loco_load(bool)));
	QObject::connect(ui.a_loco_save, SIGNAL(triggered(bool)), this, SLOT(a_loco_save(bool)));
	QObject::connect(ui.a_config_load, SIGNAL(triggered(bool)), this, SLOT(a_config_load(bool)));
	QObject::connect(ui.a_config_save, SIGNAL(triggered(bool)), this, SLOT(a_config_save(bool)));
	QObject::connect(ui.a_speed_load, SIGNAL(triggered(bool)), this, SLOT(a_speed_load(bool)));

	// WSM init
	QObject::connect(&wsm, SIGNAL(speedRead(double,uint16_t)), this,
	                 SLOT(mc_speedRead(double,uint16_t)));
	QObject::connect(&wsm, SIGNAL(onError(QString)), this, SLOT(mc_onError(QString)));
	QObject::connect(&wsm, SIGNAL(batteryRead(double,uint16_t)), this,
	                 SLOT(mc_batteryRead(double,uint16_t)));
	QObject::connect(&wsm, SIGNAL(batteryCritical()), this, SLOT(mc_batteryCritical()));
	QObject::connect(&wsm, SIGNAL(distanceRead(double,uint32_t)), this,
	                 SLOT(mc_distanceRead(double,uint32_t)));
	QObject::connect(&wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(mc_speedReceiveTimeout()));
	QObject::connect(&wsm, SIGNAL(longTermMeasureDone(double,double)), this,
	                 SLOT(mc_longTermMeasureDone(double,double)));
	QObject::connect(&wsm, SIGNAL(speedReceiveRestore()), this, SLOT(mc_speedReceiveRestore()));

	// Calibration Manager signals

	QObject::connect(&cm, SIGNAL(onStepStart(uint)), this, SLOT(cm_stepStart(uint)));
	QObject::connect(&cm, SIGNAL(onStepDone(uint,uint)),
	                 this, SLOT(cm_stepDone(uint,uint)));
	QObject::connect(&cm, SIGNAL(onError(Cm::CmError,uint,const QString&)),
	                 this, SLOT(cm_stepError(Cm::CmError,uint,const QString&)));
	QObject::connect(&cm, SIGNAL(onLog(const QString&, Cm::LogLevel)),
	                 this, SLOT(cm_onLog(const QString&, Cm::LogLevel)));
	QObject::connect(&cm, SIGNAL(onLocoSpeedChanged(uint)),
	                 this, SLOT(cm_locoSpeedChanged(uint)));
	QObject::connect(&cm, SIGNAL(onDone()), this, SLOT(cm_done()));
	QObject::connect(&cm, SIGNAL(onStepPowerChanged(uint,uint)),
	                 this, SLOT(cm_step_power_changed(uint,uint)));
	QObject::connect(&cm, SIGNAL(onProgressUpdate(size_t)), this, SLOT(cm_progress_update(size_t)));

	// Connect power-to-map with GUI
	QObject::connect(&m_pm, SIGNAL(onAddOrUpdate(uint,float)), &w_pg,
	                 SLOT(addOrUpdate(uint,float)));
	QObject::connect(&m_pm, SIGNAL(onClear()), &w_pg, SLOT(clear()));
	m_pm.clear();

	// Range Calibration
	QObject::connect(&cr, SIGNAL(on_error(Cr::CrError,uint,QString)), this,
	                 SLOT(cr_error(Cr::CrError,uint,QString)));
	QObject::connect(&cr, SIGNAL(measured(double)), this, SLOT(cr_measured(double)));

	w_pg.setAttribute(Qt::WA_QuitOnClose, false);

	ui.tw_main->setCurrentIndex(0);
	log("Application launched.");
}

MainWindow::~MainWindow() {
	try {
		QObject::disconnect(&xn, SIGNAL(onError(QString)), this, SLOT(xn_onError(QString)));
		QObject::disconnect(&xn, SIGNAL(onLog(QString, Xn::LogLevel)), this,
		                    SLOT(xn_onLog(QString, Xn::LogLevel)));
		QObject::disconnect(&xn, SIGNAL(onDisconnect()), this, SLOT(xn_onDisconnect()));
		a_config_save(true);
	}
	catch (...) {
		// No exceptions in destructor!
	}
}

///////////////////////////////////////////////////////////////////////////////
// UI general functions

void MainWindow::widget_set_color(QWidget &widget, const QColor &color) {
	QPalette palette = widget.palette();
	palette.setColor(QPalette::WindowText, color);
	widget.setPalette(palette);
}

void MainWindow::t_xn_disconnect_tick() {
	log("XN error, disconnecting from XpressNET...");
	a_xn_disconnect(true);

	QMessageBox::warning(this, "Error!",
		"XN serial port error, more information in log!", QMessageBox::Ok);
}

void MainWindow::cb_xn_ll_index_changed(int index) {
	s["XN"]["loglevel"] = index;
	xn.loglevel = static_cast<Xn::LogLevel>(index);
}

void MainWindow::show_error(const QString &error) {
	log(error, LOGC_ERROR);
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

void MainWindow::lv_log_dblclick(QListWidgetItem *) {
	ui.lv_log->clear();
}

void MainWindow::tw_xn_log_dblclick(QTreeWidgetItem *, int) {
	ui.tw_xn_log->clear();
}

void MainWindow::step_set_color(const unsigned stepi, const QColor &color) {
	widget_set_color(*ui_steps[stepi].step, color);
	widget_set_color(*ui_steps[stepi].speed_want, color);
	widget_set_color(*ui_steps[stepi].value, color);
}

void MainWindow::gui_update_enabled() {
	// WSM
	ui.a_wsm_connect->setEnabled(!wsm.connected());
	ui.a_wsm_disconnect->setEnabled(wsm.connected());

	// XpressNET
	ui.a_xn_connect->setEnabled(!xn.connected());
	ui.a_xn_disconnect->setEnabled(xn.connected());
	ui.a_xn_dcc_go->setEnabled(xn.connected());
	ui.a_xn_dcc_stop->setEnabled(xn.connected());
	ui.gb_speed->setEnabled(xn.connected() && !cm.inProgress());

	ui.gb_cal_graph->setEnabled(xn.connected() && !cm.inProgress());
	ui.b_calib_start->setEnabled(!cm.inProgress());
	ui.b_calib_stop->setEnabled(cm.inProgress());
	ui.sb_max_speed->setEnabled(!cm.inProgress());
	ui.sb_vmax->setEnabled(!cm.inProgress() && ui.chb_vmax->isChecked());
	ui.b_vmax_read->setEnabled(xn.connected());
	ui.sb_volt_ref->setEnabled(!cm.inProgress() && ui.chb_volt_ref->isChecked());
	ui.b_volt_ref_read->setEnabled(xn.connected());
	ui.gb_ad->setEnabled(xn.connected() && !cm.inProgress());
	ui.b_wsm_lt->setEnabled(wsm.connected() && !cm.inProgress());
	ui.a_loco_load->setEnabled(!cm.inProgress());
	ui.a_speed_load->setEnabled(!cm.inProgress());
	ui.b_reset->setEnabled(!cm.inProgress());
	ui.b_read_all_steps->setEnabled(xn.connected() && !cm.inProgress());

	for (auto& ui_step : ui_steps)
		gui_step_update_enabled(ui_step);

	if (!xn.connected()) {
		ui.b_addr_set->setEnabled(true);
		ui.b_addr_release->setEnabled(false);
	}
}

void MainWindow::gui_step_update_enabled(UiStep& ui_step) {
	ui_step.slider->setEnabled(xn.connected() && !cm.inProgress() && ui_step.selected->isChecked());
	ui_step.read->setEnabled(xn.connected() && !cm.inProgress());
	ui_step.write->setEnabled(ui_step.selected->isChecked());
	ui_step.calibrate->setEnabled(wsm.connected() && !ui_step.selected->isChecked());
}

///////////////////////////////////////////////////////////////////////////////
// XN Events

void MainWindow::xn_onError(const QString &error) {
	xn_onLog(error, Xn::LogLevel::Error);

	if (!t_xn_disconnect.isActive() && xn.connected())
		t_xn_disconnect.start(0);
}

void MainWindow::xn_onLog(const QString &message, const Xn::LogLevel &loglevel) {
	constexpr size_t COLUMN_COUNT = 3;

	if (ui.tw_xn_log->topLevelItemCount() > 300)
		ui.tw_xn_log->clear();

	auto *item = new QTreeWidgetItem(ui.tw_xn_log);
	item->setText(0, QTime::currentTime().toString("hh:mm:ss"));

	if (message.startsWith("GET:"))
		for (size_t i = 0; i < COLUMN_COUNT; i++)
			item->setBackground(i, LOGC_GET);
	if (message.startsWith("PUT:"))
		for (size_t i = 0; i < COLUMN_COUNT; i++)
			item->setBackground(i, LOGC_PUT);

	if (loglevel == Xn::LogLevel::None)
		item->setText(1, "None");
	else if (loglevel == Xn::LogLevel::Error) {
		item->setText(1, "Error");
		for (size_t i = 0; i < COLUMN_COUNT; i++)
			item->setBackground(i, LOGC_ERROR);
	} else if (loglevel == Xn::LogLevel::Warning) {
		item->setText(1, "Warning");
		for (size_t i = 0; i < COLUMN_COUNT; i++)
			item->setBackground(i, LOGC_WARN);
	} else if (loglevel == Xn::LogLevel::Info)
		item->setText(1, "Info");
	else if (loglevel == Xn::LogLevel::Commands)
		item->setText(1, "Commands");
	else if (loglevel == Xn::LogLevel::Info)
		item->setText(1, "Raw Data");
	else if (loglevel == Xn::LogLevel::RawData)
		item->setText(1, "Debug");

	item->setText(2, message);
	ui.tw_xn_log->addTopLevelItem(item);

	if (s["XN"]["logfile"] != "") {
		std::ofstream out(s["XN"]["logfile"].toString().toUtf8().data(), std::ofstream::app);
		out << QTime::currentTime().toString("hh:mm:ss.zzz").toUtf8().data() << ": "
		    << message.toUtf8().data() << std::endl;
	}
}

void MainWindow::xn_onConnect() {
	widget_set_color(*(ui.l_xn), Qt::green);
	gui_update_enabled();

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
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_onLIVersionError(s, d); })
		);
	}
	catch (const Xn::QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::xn_onDisconnect() {
	if (cm.inProgress())
		cm.stop();
	widget_set_color(*(ui.l_xn), Qt::red);
	widget_set_color(*(ui.l_dcc), Qt::gray);
	loco_released();
	gui_update_enabled();
	log("Disconnected from XpressNET");
}

void MainWindow::xn_onTrkStatusChanged(Xn::TrkStatus status) {
	if (status == Xn::TrkStatus::Unknown) {
		widget_set_color(*(ui.l_dcc), Qt::gray);
		log("CS status: Unknown");
	} else if (status == Xn::TrkStatus::Off) {
		widget_set_color(*(ui.l_dcc), Qt::red);
		log("CS status: Off");
	} else if (status == Xn::TrkStatus::On) {
		widget_set_color(*(ui.l_dcc), Qt::green);
		log("CS status: On");
	} else if (status == Xn::TrkStatus::Programming) {
		widget_set_color(*(ui.l_dcc), Qt::yellow);
		log("CS status: Programming");
	}
}

void MainWindow::xn_onDccGoError(void *, void *) { show_response_error("DCC GO"); }

void MainWindow::xn_onDccStopError(void *, void *) { show_response_error("DCC STOP"); }

void MainWindow::show_response_error(const QString &command) {
	log("Command station did not respond to " + command + " command!");
	QMessageBox::warning(this, "Error!",
	                     "Command station did not respond to " + command + " command!");
}

void MainWindow::xn_onLIVersionError(void *, void *) {
	m_starting = false;
	log("LI did not respond to version request!");
	QMessageBox::warning(this, "Error!",
		"LI did not respond to version request, are you really connected to the LI?!");
}

void MainWindow::xn_onCSVersionError(void *, void *) {
	log("Coomand station did not respond to version request!");
	QMessageBox::warning(this, "Error!",
		"Command station did not respond to version request"
		", is the LI really connected to the command station?!");
	m_starting = false;
}

void MainWindow::xn_onCSStatusError(void *, void *) {
	show_response_error("STATUS");
	m_starting = false;
}

void MainWindow::xn_onCSStatusOk(void *, void *) {
	if (m_starting) {
		m_starting = false;
		log("Succesfully connected to Command station");
		if (!wsm.connected())
			a_wsm_connect(true);
	}
}

void MainWindow::xn_gotLIVersion(void *, unsigned hw, unsigned sw) {
	log("Got LI version. HW: " + QString::number(hw) + ", SW: " + QString::number(sw));
	try {
		xn.getCommandStationStatus(
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_onCSStatusOk(s, d); }),
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_onCSStatusError(s, d); })
		);
	}
	catch (const Xn::QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::xn_gotCSVersion(void *, unsigned major, unsigned minor) {
	log("Got command station version:" + QString::number(major) + "." + QString::number(minor));
}

void MainWindow::xn_gotLocoInfo(void *, bool used, Xn::Direction direction, unsigned speed,
                                Xn::FA fa, Xn::FB fb) {
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
	if (direction == Xn::Direction::Forward)
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

void MainWindow::xn_onLocoInfoError(void *, void *) {
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

void MainWindow::xn_addrReadError(void *, void *) {
	show_error("Unable to read address: no response from command station!");
	ui.sb_loco->setEnabled(true);
	ui.b_addr_set->setEnabled(true);
	ui.b_addr_read->setEnabled(true);
}

void MainWindow::xn_adReadError(void *, void *) {
	show_error("Unable to read CV: no response from command station!");
	ui.gb_ad->setEnabled(true);
}

void MainWindow::xn_cvRead(void *, Xn::ReadCVStatus st, uint8_t cv, uint8_t value) {
	if (st != Xn::ReadCVStatus::Ok) {
		show_error("Unable to read CV " + QString::number(cv) + ": " +
		           Xn::XpressNet::xnReadCVStatusToQString(st));

		if (cv == Cm::CV_ADDR_LO || cv == CV_ADDR_HI || cv == Cm::CV_ADDR_SHORT || cv == Cm::CV_BASIC_CONFIG) {
			ui.sb_loco->setEnabled(true);
			ui.b_addr_set->setEnabled(true);
			ui.b_addr_read->setEnabled(true);
		} else if (cv == Cm::CV_ACCEL || cv == Cm::CV_DECEL) {
			ui.gb_ad->setEnabled(true);
		}
		return;
	}

	if (cv == Cm::CV_ADDR_LO) {
		try {
			ui.sb_loco->setValue(Xn::LocoAddr(value, 0xC0));
		}
		catch (const Xn::EInvalidAddr&) {
			show_error("Invalid address!");
		}
		xn.readCVdirect(
			CV_ADDR_HI,
			[this](void *s, Xn::ReadCVStatus st, uint8_t cv, uint8_t value) { xn_cvRead(s, st, cv, value); },
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_addrReadError(s, d); })
		);
	} else if (cv == Cm::CV_ADDR_HI) {
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
	} else if (cv == CV_ADDR_SHORT) {
		ui.sb_loco->setValue(value);
		ui.sb_loco->setEnabled(true);
		ui.b_addr_set->setEnabled(true);
		ui.b_addr_read->setEnabled(true);
		a_dcc_go(true);
	} else if (cv == Cm::CV_ACCEL) {
		ui.sb_accel->setValue(value);
		xn.readCVdirect(Cm::CV_DECEL,
			[this](void *s, Xn::ReadCVStatus st, uint8_t cv, uint8_t value) { xn_cvRead(s, st, cv, value); },
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_adReadError(s, d); })
		);
	} else if (cv == Cm::CV_DECEL) {
		ui.sb_decel->setValue(value);
		ui.gb_ad->setEnabled(true);
		a_dcc_go(true);
	}
}

void MainWindow::xn_adWriteError(void *, void *) {
	ui.gb_ad->setEnabled(true);
	show_error("XN POM no response!");
}

void MainWindow::xn_accelWritten(void *, void *) {
	xn.pomWriteCv(Xn::LocoAddr(ui.sb_loco->value()), Cm::CV_DECEL, ui.sb_decel->value(),
	              std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_decelWritten(s, d); }),
	              std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_adWriteError(s, d); }));
}

void MainWindow::xn_decelWritten(void *, void *) {
	ui.gb_ad->setEnabled(true);
}

void MainWindow::xn_stepWritten(void *, void *d) {
	unsigned stepi = reinterpret_cast<intptr_t>(d);
	step_set_color(stepi, STEPC_DONE);
}

void MainWindow::xn_stepWriteError(void *, void *d) {
	unsigned stepi = reinterpret_cast<intptr_t>(d);
	step_set_color(stepi, STEPC_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
// XpressNET connect/disconnect

void MainWindow::a_xn_connect(bool) {
	if (xn.connected())
		return;

	try {
		widget_set_color(*(ui.l_xn), Qt::yellow);
		log("Connecting to XN: "+s["XN"]["port"].toString()+":"+s["XN"]["interface"].toString()+":"+
		    s["XN"]["baudrate"].toString()+"...");
		xn.connect(s["XN"]["port"].toString(), s["XN"]["baudrate"].toInt(),
		           static_cast<QSerialPort::FlowControl>(s["XN"]["flowcontrol"].toInt()),
				   Xn::liInterface(s["XN"]["interface"].toString()));
	} catch (const Xn::QStrException &e) {
		widget_set_color(*(ui.l_xn), Qt::red);
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
	} catch (const Xn::QStrException &e) {
		show_error("XN disconnect error:\n" + e.str());
	}
}

void MainWindow::a_dcc_go(bool) {
	try {
		if (xn.connected())
			xn.setTrkStatus(
				Xn::TrkStatus::On, nullptr,
				std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_onDccGoError(s, d); })
			);
	} catch (const Xn::QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::a_dcc_stop(bool) {
	try {
		if (xn.connected())
			xn.setTrkStatus(
				Xn::TrkStatus::Off, nullptr,
				std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_onDccStopError(s, d); })
			);
	} catch (const Xn::QStrException& e) {
		show_error(e.str());
	}
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::log(const QString &message, const QColor &color) {
	if (ui.lv_log->count() > 200)
		ui.lv_log->clear();
	ui.lv_log->insertItem(0, QTime::currentTime().toString("hh:mm:ss") + ": " + message);
	ui.lv_log->item(0)->setBackground(color);

	if (s["Logging"]["file"] != "") {
		std::ofstream out(s["Logging"]["file"].toString().toUtf8().data(), std::ofstream::app);
		out << QTime::currentTime().toString("hh:mm:ss.zzz").toUtf8().data() << ": "
		    << message.toUtf8().data() << std::endl;
	}
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
		[this](void *s, bool used, Xn::Direction dir, unsigned speed, Xn::FA fa, Xn::FB fb) {
			xn_gotLocoInfo(s, used, dir, speed, fa, fb);
		},
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_onLocoInfoError(s, d); })
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
		Cm::CV_BASIC_CONFIG,
		[this](void*, Xn::ReadCVStatus, uint8_t, uint8_t value) {
			// Configuration successfully read
			const auto& next_cv = (((value >> 5) & 0x1) == 1) ? CV_ADDR_LO : CV_ADDR_SHORT;
			xn.readCVdirect(
				next_cv,
				[this](void *s, Xn::ReadCVStatus st, uint8_t cv, uint8_t value) {
					xn_cvRead(s, st, cv, value);
				},
				std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_addrReadError(s, d); })
			);
		},
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_addrReadError(s, d); })
	);
}

void MainWindow::b_speed_set_handle() {
	try {
		xn.setSpeed(Xn::LocoAddr(ui.sb_loco->value()), ui.sb_speed->value(),
		            static_cast<Xn::Direction>(ui.rb_forward->isChecked()));
		m_sent_speed = ui.sb_speed->value();
		ui.vs_speed->setValue(ui.sb_speed->value());
	}
	catch (const Xn::QStrException& e) {
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
	catch (const Xn::QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::b_loco_idle_handle() {
	ui.sb_speed->setValue(0);
	b_speed_set_handle();
}

void MainWindow::vs_speed_slider_moved(int value) { ui.sb_speed->setValue(value); }

void MainWindow::rb_direction_toggled(bool) { b_speed_set_handle(); }

void MainWindow::t_slider_tick() {
	try {
		if (ui.vs_speed->value() != m_sent_speed) {
			m_sent_speed = ui.vs_speed->value();
			xn.setSpeed(Xn::LocoAddr(ui.sb_loco->value()), ui.sb_speed->value(),
			            static_cast<Xn::Direction>(ui.rb_forward->isChecked()));
		}
	}
	catch (const Xn::QStrException& e) {
		show_error(e.str());
	}
}

void MainWindow::chb_f_clicked(bool) {
	m_fa.sep.f0 = ui.chb_f0->isChecked();
	m_fa.sep.f1 = ui.chb_f1->isChecked();
	m_fa.sep.f2 = ui.chb_f2->isChecked();
	xn.setFuncA(Xn::LocoAddr(ui.sb_loco->value()), m_fa);
}

void MainWindow::sb_max_speed_changed(int value) { m_ssm.setMaxSpeed(value); }

void MainWindow::sb_speed_changed(int) {
	if (m_sent_speed != ui.sb_speed->value())
		b_speed_set_handle();
}

///////////////////////////////////////////////////////////////////////////////
// WSM functions

void MainWindow::wsm_status_blink() {
	QPalette palette = ui.l_wsm_alive->palette();
	const QColor &color = palette.color(QPalette::WindowText);
	if (color == Qt::green)
		widget_set_color(*(ui.l_wsm_alive), palette.color(QPalette::Window));
	else
		widget_set_color(*(ui.l_wsm_alive), Qt::green);
}

void MainWindow::a_wsm_connect(bool) {
	log("Connecting to WSM...");

	try {
		widget_set_color(*(ui.l_wsm), Qt::yellow);
		wsm.connect(s["WSM"]["port"].toString());

		gui_update_enabled();
		widget_set_color(*(ui.l_wsm), Qt::green);

		log("Connected to WSM");
		widget_set_color(*(ui.l_wsm), Qt::green);
	} catch (const Wsm::EOpenError &e) {
		widget_set_color(*(ui.l_wsm), Qt::red);
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
	gui_update_enabled();
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
	QString text = QString::asprintf(
		"%4.2f V (%s)",
		voltage,
		QTime::currentTime().toString().toUtf8().data()
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
	log("WSM error, disconnecting...", LOGC_ERROR);
	a_wsm_disconnect(true);
}

void MainWindow::mc_speedReceiveTimeout() {
	log("WSM receive timeout!", LOGC_ERROR);
	widget_set_color(*(ui.l_wsm_alive), Qt::red);
}

void MainWindow::mc_speedReceiveRestore() {
	ui.b_wsm_lt->setEnabled(true);
	log("WSM speed receive restored");
}

void MainWindow::mc_longTermMeasureDone(double speed, double diffusion) {
	ui.b_wsm_lt->setEnabled(true);
	log("WSM long term done: sp=" + QString::number(speed, 'f', 1) +
	    ", diff=" + QString::number(diffusion, 'f', 1), LOGC_DONE);
}

void MainWindow::b_wsm_lt_handle() {
	if (wsm.connected()) {
		try {
			wsm.startLongTermMeasure(30); // 3 s
		}
		catch (const Wsm::QStrException& e) {
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
	for(size_t i = 0; i < STEPS_CNT; i++) {
		{
			auto *step = new QLabel(QString::number(i+1), ui.gb_cal_graph);
			step->setFont(QFont("Sans Serif", 9, QFont::Bold));
			step->setAlignment(Qt::AlignmentFlag::AlignHCenter);
			ui_steps[i].step = step;
			ui.l_cal_graph->addWidget(step, 0, i);
		}

		{
			auto *speed_want = new QLabel("-", ui.gb_cal_graph);
			speed_want->setFont(QFont("Sans Serif", 8));
			speed_want->setAlignment(Qt::AlignmentFlag::AlignHCenter);
			ui_steps[i].speed_want = speed_want;
			ui.l_cal_graph->addWidget(speed_want, 1, i);
		}

		{
			auto *value = new QLabel("0", ui.gb_cal_graph);
			value->setFont(QFont("Sans Serif", 8));
			value->setAlignment(Qt::AlignmentFlag::AlignHCenter);
			ui_steps[i].value = value;
			ui.l_cal_graph->addWidget(value, 2, i);
		}

		{
			auto *slider = new QSlider(Qt::Orientation::Vertical, ui.gb_cal_graph);
			slider->setMinimum(0);
			slider->setMaximum(255);
			slider->setProperty("step", static_cast<uint>(i));
			slider->setEnabled(false);
			QObject::connect(slider, SIGNAL(valueChanged(int)), this, SLOT(vs_steps_moved(int)));
			ui_steps[i].slider = slider;
			ui.l_cal_graph->addWidget(slider, 3, i);
		}

		{
			auto *selected = new QCheckBox(ui.gb_cal_graph);
			selected->setProperty("step", static_cast<uint>(i));
			ui_steps[i].selected = selected;
			QObject::connect(selected, SIGNAL(clicked(bool)), this,
			                 SLOT(chb_step_selected_clicked(bool)));
			ui.l_cal_graph->addWidget(selected, 4, i);
		}

		{
			auto *read = new QPushButton("R", ui.gb_cal_graph);
			read->setProperty("step", static_cast<uint>(i));
			read->setEnabled(false);
			read->setFixedHeight(20);
			read->setToolTip("Read value of this step from decoder");
			ui_steps[i].read = read;
			QObject::connect(read, SIGNAL(released()), this, SLOT(b_step_read_handle()));
			ui.l_cal_graph->addWidget(read, 5, i);
		}

		{
			auto *write = new QPushButton("W", ui.gb_cal_graph);
			write->setProperty("step", static_cast<uint>(i));
			write->setEnabled(false);
			write->setFixedHeight(20);
			write->setToolTip("Write this step to decoder");
			ui_steps[i].write = write;
			QObject::connect(write, SIGNAL(released()), this, SLOT(b_step_write_handle()));
			ui.l_cal_graph->addWidget(write, 6, i);
		}

		{
			auto *calibrate = new QPushButton("C", ui.gb_cal_graph);
			calibrate->setProperty("step", static_cast<uint>(i));
			calibrate->setEnabled(false);
			calibrate->setFixedHeight(20);
			calibrate->setToolTip("Calibrate this step");
			ui_steps[i].calibrate = calibrate;
			QObject::connect(calibrate, SIGNAL(released()), this, SLOT(b_step_calibrate_handle()));
			ui.l_cal_graph->addWidget(calibrate, 7, i);
		}
	}
}

void MainWindow::vs_steps_moved(int value) {
	unsigned stepi = qobject_cast<QSlider*>(QObject::sender())->property("step").toUInt();
	ui_steps[stepi].value->setText(QString::number(value));

	if (ui_steps[stepi].slider->isEnabled())
		step_set_color(stepi, STEPC_CHANGED);
}

void MainWindow::b_step_calibrate_handle() {
	unsigned stepi = qobject_cast<QPushButton*>(QObject::sender())->property("step").toUInt();
}

void MainWindow::b_step_read_handle() {
	unsigned stepi = qobject_cast<QPushButton*>(QObject::sender())->property("step").toUInt();
}

void MainWindow::b_step_write_handle() {
	unsigned stepi = qobject_cast<QPushButton*>(QObject::sender())->property("step").toUInt();

	if (xn.connected() && !ui.sb_loco->isEnabled()) {
		ui_steps[stepi].selected->setChecked(true);
		log("Setting power of step " + QString::number(stepi+1) + " manually.");
		xn.pomWriteCv(
			Xn::LocoAddr(ui.sb_loco->value()),
			Cs::CV_START + stepi,
			ui_steps[stepi].slider->value(),
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_stepWritten(s, d); }, reinterpret_cast<void*>(stepi)),
			std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_stepWriteError(s, d); }, reinterpret_cast<void*>(stepi))
		);
		cm.setStepManually(stepi+1, ui_steps[stepi].slider->value());
	} else {
		QMessageBox::warning(this, "Command not executed", "Command not executed: either not connected to XpressNet or DCC address not set!");
	}
}

void MainWindow::chb_step_selected_clicked(bool checked) {
	unsigned stepi = qobject_cast<QCheckBox*>(QObject::sender())->property("step").toUInt();
	gui_step_update_enabled(ui_steps[stepi]);

	if (!checked)
		cm.unsetStep(stepi);
}

///////////////////////////////////////////////////////////////////////////////
// Steps-to-speed events:

void MainWindow::ssm_onAddOrUpdate(unsigned step, unsigned speed) {
	ui_steps[step].speed_want->setText(QString::number(speed));
}

void MainWindow::ssm_onClear() {
	for (const auto &s : ui_steps)
		s.speed_want->setText("0");
}

///////////////////////////////////////////////////////////////////////////////
// Calibration Manager events & gui interaction:

void MainWindow::cm_step_power_changed(unsigned step, unsigned power) {
	ui_steps[step-1].slider->setValue(power);
}

void MainWindow::cm_stepDone(unsigned step, unsigned power) {
	(void)power;
	step_set_color(step-1, STEPC_DONE);
}

void MainWindow::cm_stepStart(unsigned step) {
	step_set_color(step-1, STEPC_CHANGED);
}

void MainWindow::cm_stepError(Cm::CmError ce, unsigned step, const QString& note) {
	if (step != 0)
		step_set_color(step-1, STEPC_ERROR);
	widget_set_color(*ui.l_calib_state, Qt::red);

	if (ce == Cm::CmError::Exception)
		log("Exception!", LOGC_ERROR);
	else if (ce == Cm::CmError::LargeDiffusion)
		log("Loco speed too diffused!", LOGC_ERROR);
	else if (ce == Cm::CmError::XnNoResponse)
		log("No response from XpressNET!", LOGC_ERROR);
	else if (ce == Cm::CmError::LocoStopped)
		log("Loco stopped!", LOGC_ERROR);
	else if (ce == Cm::CmError::NoStep)
		log("No suitable power step for this speed!", LOGC_ERROR);
	else if (ce == Cm::CmError::Oscilation)
		log("Unable to reach target speed due to low precision (try decreasing Vmax?)", LOGC_ERROR);
	else if (ce == Cm::CmError::WsmError)
		log("WSM read speed error!", LOGC_ERROR);

	if (note != "")
		log(note, LOGC_ERROR);

	cm_done_gui();
}

void MainWindow::cm_locoSpeedChanged(unsigned step) {
	m_sent_speed = step;
	ui.vs_speed->setValue(step);
	ui.sb_speed->setValue(step);
}

void MainWindow::cm_done() {
	widget_set_color(*ui.l_calib_state, Qt::green);
	ui.pb_progress->setValue(100);
	cm_done_gui();
}

void MainWindow::cm_onLog(const QString &message, Cm::LogLevel level) {
	switch (level) {
		case Cm::LogLevel::Error: log(message, LOGC_ERROR); break;
		case Cm::LogLevel::Warning: log(message, LOGC_WARN); break;
		case Cm::LogLevel::Success: log(message, LOGC_DONE); break;
		default: log(message, Qt::white); break;
	}
}

void MainWindow::cm_progress_update(size_t val) { ui.pb_progress->setValue(val); }

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

	if (ui.sb_loco->isEnabled()) {
		show_error("Set loco address first!");
		return;
	}

	bool changed = false;
	changed |= ((ui.chb_vmax->isChecked() != (cm.init_cvs.find(Cm::CV_VMAX) != cm.init_cvs.end())) ||
		((cm.init_cvs.find(Cm::CV_VMAX) != cm.init_cvs.end()) && (static_cast<int>(cm.init_cvs[Cm::CV_VMAX]) != ui.sb_vmax->value())));
	changed |= ((ui.chb_volt_ref->isChecked() != (cm.init_cvs.find(Cm::CV_UREF) != cm.init_cvs.end())) ||
		((cm.init_cvs.find(Cm::CV_UREF) != cm.init_cvs.end()) && (static_cast<int>(cm.init_cvs[Cm::CV_UREF]) != ui.sb_volt_ref->value())));

	if (m_pm.isAnyRecord() && changed) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(
			this,
			"Question",
			"Vmax and/or Uref have been changed, so all the measured data must be erased and calibrated again.\nContinue?",
			QMessageBox::Yes|QMessageBox::No, QMessageBox::No
		);
		if (reply != QMessageBox::Yes)
			return;

		reset();
	}

	if (ui.chb_vmax->isChecked())
		cm.init_cvs[Cm::CV_VMAX] = ui.sb_vmax->value();
	else
		cm.init_cvs.erase(Cm::CV_VMAX);

	if (ui.chb_volt_ref->isChecked())
		cm.init_cvs[Cm::CV_UREF] = ui.sb_volt_ref->value();
	else
		cm.init_cvs.erase(Cm::CV_UREF);

	ui.pb_progress->setValue(0);
	widget_set_color(*ui.l_calib_state, Qt::yellow);
	cm.calibrateAll(ui.sb_loco->value(),
	                static_cast<Xn::Direction>(ui.rb_forward->isChecked()));
	gui_update_enabled();
}

void MainWindow::b_calib_stop_handle() {
	if (!cm.inProgress())
		return;

	cm.stop();
	log("Calibration manually interrupted!", LOGC_WARN);
	widget_set_color(*ui.l_calib_state, Qt::red);
	cm_done_gui();
}

void MainWindow::cm_done_gui() {
	gui_update_enabled();
}

void MainWindow::b_reset_handle() {
	if (cm.inProgress())
		return;

	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Question", "Really erase all measured data?",
	                              QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
	if (reply != QMessageBox::Yes)
		return;

	reset();

	QMessageBox::information(this, "Info", "All measured data have been erased.");
}

void MainWindow::t_calib_active_tick() {
	if (cm.inProgress()) {
		QPalette palette = ui.l_calib_state->palette();
		const QColor &color = palette.color(QPalette::WindowText);
		if (color == Qt::yellow)
			widget_set_color(*(ui.l_calib_state), Qt::lightGray);
		else
			widget_set_color(*(ui.l_calib_state), Qt::yellow);
	}
}

void MainWindow::chb_vmax_clicked(bool checked) {
	ui.sb_vmax->setEnabled(checked);
}

void MainWindow::chb_volt_ref_clicked(bool checked) {
	ui.sb_volt_ref->setEnabled(checked);
}

void MainWindow::b_vmax_read_handle() {
	ui.b_vmax_read->setEnabled(false);
	xn.readCVdirect(
		Cm::CV_VMAX,
		[this](void *, Xn::ReadCVStatus st, uint8_t cv, uint8_t value) {
			if (st == Xn::ReadCVStatus::Ok) {
				log("Read CV " + QString::number(cv) + " = " + QString::number(value) +
				    ", overriding "+QString::number(ui.sb_vmax->value()));
				ui.sb_vmax->setValue(value);
			} else {
				show_error("Unable to read CV " + QString::number(cv) + ": " +
				           Xn::XpressNet::xnReadCVStatusToQString(st));
			}
			ui.b_vmax_read->setEnabled(true);
		},
		std::make_unique<Xn::Cb>([this](void *, void *) {
			ui.b_vmax_read->setEnabled(true);
			show_error("Unable to read CV: no response from command station!");
		})
	);
}

void MainWindow::b_volt_ref_read_handle() {
	ui.b_volt_ref_read->setEnabled(false);
	xn.readCVdirect(
		Cm::CV_UREF,
		[this](void *, Xn::ReadCVStatus st, uint8_t cv, uint8_t value) {
			if (st == Xn::ReadCVStatus::Ok) {
				log("Read CV " + QString::number(cv) + " = " + QString::number(value) +
				    ", overriding "+QString::number(ui.sb_volt_ref->value()));
				ui.sb_volt_ref->setValue(value);
			} else {
				show_error("Unable to read CV " + QString::number(cv) + ": " +
				           Xn::XpressNet::xnReadCVStatusToQString(st));
			}
			ui.b_volt_ref_read->setEnabled(true);
		},
		std::make_unique<Xn::Cb>([this](void *, void *) {
			ui.b_volt_ref_read->setEnabled(true);
			show_error("Unable to read CV: no response from command station!");
		})
	);
}

///////////////////////////////////////////////////////////////////////////////
// Acceleration & deceleration settings:

void MainWindow::b_ad_read_handle() {
	ui.gb_ad->setEnabled(false);
	xn.readCVdirect(
		Cm::CV_ACCEL,
		[this](void *s, Xn::ReadCVStatus st, uint8_t cv, uint8_t value) { xn_cvRead(s, st, cv, value); },
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_adReadError(s, d); })
	);
}

void MainWindow::b_ad_write_handle() {
	if (ui.sb_loco->isEnabled()) {
		show_error("Loco must be set!");
		return;
	}

	ui.gb_ad->setEnabled(false);

	xn.pomWriteCv(Xn::LocoAddr(
		ui.sb_loco->value()), Cm::CV_ACCEL, ui.sb_accel->value(),
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_accelWritten(s, d); }),
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_adWriteError(s, d); })
	);
}

///////////////////////////////////////////////////////////////////////////////

void MainWindow::b_test1_handle() {}

void MainWindow::b_test2_handle() { cm.interpolateAll(); }

void MainWindow::b_test3_handle() {}

//////////////////////////////////////////////////////////////////////////////
// Loco file IO:

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

	while (!xr.atEnd()) {
		if (xr.isStartElement()) {
			if (xr.name() == QString("powerToSpeed")) {
				xr.readNext();
				while (xr.name() != QString("powerToSpeed")) {
					if (xr.name() == QString("record") && xr.attributes().hasAttribute("power") &&
						xr.attributes().hasAttribute("speed")) {
						int power = xr.attributes().value("power").toInt();
						float speed = xr.attributes().value("speed").toFloat();
						m_pm.addOrUpdate(power, speed);
					}
					xr.readNext();
				}
			} else if (xr.name() == QString("dcclocoaddress") && xr.attributes().hasAttribute("number")) {
				ui.sb_loco->setValue(xr.attributes().value("number").toInt());
			} else if (xr.name() == QString("locomotive") && xr.attributes().hasAttribute("maxSpeed")) {
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
	for (const auto &s : ui_steps)
		steps += QString::number(s.slider->value()) + ",";

	std::vector<std::pair<QString, QString>> values = {
		std::make_pair("Acceleration Rate", QString::number(ui.sb_accel->value())),
		std::make_pair("Deceleration Rate", QString::number(ui.sb_decel->value())),
		std::make_pair("Use Speed Table", "1"),
		std::make_pair("Speed Table", std::move(steps)),
		std::make_pair("Vhigh", QString::number(ui.sb_vmax->value())),
		std::make_pair("Vmid", "60"),
		std::make_pair("Vstart", "1"),
	};

	for (const auto &val : values) {
		xw.writeStartElement("varValue");
		xw.writeAttribute("item", val.first);
		xw.writeAttribute("value", val.second);
		xw.writeEndElement();
	}

	xw.writeEndElement();
	xw.writeEndElement();

	xw.writeStartElement("powerToSpeed");
	for (size_t i = 0; i < Pm::POWER_CNT; i++) {
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
	log("Range measured: " + QString::number(distance*100, 'f', 1) + " cm");
	ui.b_decel_measure->setEnabled(true);
}

void MainWindow::cr_error(Cr::CrError, unsigned, const QString& message) {
	log("Range calibration error!", LOGC_ERROR);
	ui.b_decel_measure->setEnabled(true);
	log(message, LOGC_ERROR);
}

void MainWindow::b_decel_measure_handle() {
	if (!wsm.connected() || !wsm.isSpeedOk() || !xn.connected())
		return;

	ui.b_decel_measure->setEnabled(false);
	cr.measure(ui.sb_loco->value(), 15, static_cast<Xn::Direction>(ui.rb_forward->isChecked()), 40);
}

void MainWindow::reset() {
	m_pm.clear();
	cm.reset();
	for (size_t i = 0; i < Xn::_STEPS_CNT; i++) {
		ui_steps[i].slider->setValue(0);
		ui_steps[i].slider->setEnabled(false);
		ui_steps[i].selected->setChecked(false);
		ui_steps[i].write->setEnabled(false);
		ui_steps[i].calibrate->setEnabled(false);
		step_set_color(i, Qt::black);
	}
	widget_set_color(*ui.l_calib_state, Qt::gray);
	ui.pb_progress->setValue(0);
}

//////////////////////////////////////////////////////////////////////////////
// Config IO:

void MainWindow::a_config_load(bool) {
	s.load(this->config_fn);

	try {
		bool ok;
		Xn::XNConfig xn_config;
		xn_config.outInterval = s["XN"]["outIntervalMs"].toUInt(&ok);
		if (ok) {
			xn.setConfig(xn_config);
		} else {
			this->log("XN config load: unable to parse outIntervalMs", LOGC_ERROR);
		}
	} catch (const Xn::QStrException& e) {
		this->log("XN config load: " + e.str(), LOGC_ERROR);
	}

	wsm.scale = s["WSM"]["scale"].toInt();
	wsm.wheelDiameter = s["WSM"]["wheelDiameter"].toDouble();
	wsm.ticksPerRevolution = s["WSM"]["ticksPerRevolution"].toDouble();

	auto& calcfg = s["Calibration"];

	Settings::cfgToDouble(calcfg, "absDeviation", cm.cs.abs_deviation);
	Settings::cfgToDouble(calcfg, "relDeviation", cm.cs.rel_deviation);
	Settings::cfgToDouble(calcfg, "maxAbsDiffusion", cm.cs.max_abs_diffusion);
	Settings::cfgToDouble(calcfg, "maxAbsDiffusion", cm.co.max_abs_diffusion);
	Settings::cfgToDouble(calcfg, "maxRelDiffusion", cm.cs.max_rel_diffusion);
	Settings::cfgToDouble(calcfg, "maxRelDiffusion", cm.co.max_rel_diffusion);
	Settings::cfgToUnsigned(calcfg, "measureCount", cm.cs.measure_count);
	Settings::cfgToUnsigned(calcfg, "measureCount", cm.co.measure_count);
	Settings::cfgToUnsigned(calcfg, "spAdaptTimeout", cm.cs.sp_adapt_timeout);
	Settings::cfgToUnsigned(calcfg, "spAdaptTimeout", cm.co.sp_adapt_timeout);
	Settings::cfgToUnsigned(calcfg, "overviewStep", cm.co.overview_step);
	Settings::cfgToUnsigned(calcfg, "overviewStart", cm.co.overview_start);
	Settings::cfgToUnsigned(calcfg, "overviewMinSpeed", cm.co.min_speed);
	Settings::cfgToUnsigned(calcfg, "rangeStopMinTimes", cr.stop_min);

	log("Loaded config from " + this->config_fn);
}

void MainWindow::a_config_save(bool) {
	s.save(this->config_fn);
	log("Saved config to " + this->config_fn);
}

void MainWindow::a_speed_load(bool) {
	for (const auto &s : ui_steps)
		s.speed_want->setText("-");

	QString filename;
	Settings::cfgToQString(s["Speed"], "file", filename);
	try {
		m_ssm.load(filename);
		log("Loaded steps-to-speed mapping from " + filename);
	}
	catch (const std::exception& e) {
		log("Unable to load steps-to-speed file!", LOGC_ERROR);
	}
}

//////////////////////////////////////////////////////////////////////////////
