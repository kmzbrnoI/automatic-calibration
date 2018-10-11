#ifndef _CALIB_MAN_H_
#define _CALIB_MAN_H_

#include <QObject>

#include "lib/xn/xn.h"
#include "lib/wsm/wsm.h"
#include "power-map.h"

namespace Cm {

class CalibMan : public QObject {
	Q_OBJECT

public:
	CalibMan(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	         QObject *parent = nullptr);

private:

private slots:

signals:

};

}//end namespace

#endif
