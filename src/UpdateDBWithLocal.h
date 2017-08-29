/*
 * UpdateDBWithLocal.h
 *
 *  Created on: Oct 6, 2014
 *      Author: nmmrc18
 */

#ifndef UPDATEDBWITHLOCAL_H_
#define UPDATEDBWITHLOCAL_H_

#include <QObject>
#include <QtCore/QObject>
#include <bb/data/SqlConnection>

using namespace bb::data;

class UpdateDBWithLocal : public QObject
{
    Q_OBJECT
public:
    UpdateDBWithLocal();
    virtual ~UpdateDBWithLocal();

public Q_SLOTS:
    void runUpdate();

Q_SIGNALS:
    void complete();

private:
    SqlConnection *connection;
};

#endif /* UPDATEDBWITHLOCAL_H_ */
