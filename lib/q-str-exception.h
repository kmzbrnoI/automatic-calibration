#ifndef Q_STR_EXCEPTION_H
#define Q_STR_EXCEPTION_H

#include <QString>

class QStrException {
private:
	QString m_err_msg;

public:
	QStrException(const QString msg) : m_err_msg(msg) {}
	~QStrException() noexcept = default;
	QString str() const noexcept { return this->m_err_msg; }
	operator QString() const { return this->m_err_msg; }
};

class QInvalidArgument : public QStrException {
public:
	QInvalidArgument(const QString msg) : QStrException(msg) {}
};

#endif
