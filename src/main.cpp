#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QDir>
#include "settings.h"
#include "main-window.h"
#include "main.h"

std::vector<std::unique_ptr<QTranslator>> cz_translators;

std::unique_ptr<QTranslator> load_translation(const QString& filename, const QString& directory = QString());

///////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
#ifdef Q_OS_WIN32
	a.setStyle("windowsvista");
#endif

	cz_translators.push_back(load_translation(":/i18n/auto_calib_cs_CZ"));
	cz_translators.push_back(load_translation("qt_cs", QLibraryInfo::path(QLibraryInfo::TranslationsPath))); // messageBox buttons etc. translations

	for (const auto& ptr : cz_translators)
		if (!ptr)
			return 1;

	MainWindow w;
	w.show();
	int result = a.exec();
	cz_translators.clear();
	return result;
}

///////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<QTranslator> load_translation(const QString& filename, const QString& directory) {
	std::unique_ptr<QTranslator> trans = std::make_unique<QTranslator>();
	bool success = trans->load(filename, directory);
	if (!success) {
		QMessageBox::critical(nullptr, "Error", "Unable to load translation " + directory + QDir::separator() + filename);
		return {};
	}
	return trans;
}
