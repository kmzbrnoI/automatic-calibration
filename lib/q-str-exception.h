#ifndef _Q_STR_EXCEPTION_H_
#define _Q_STR_EXCEPTION_H_

class QStrException {
private:
	QString m_err_msg;

public:
	QStrException(const QString msg) : m_err_msg(msg) {}
	~QStrException() noexcept {}
	QString str() const noexcept { return this->m_err_msg; }
	operator QString() const { return this->m_err_msg; }
};

#endif
