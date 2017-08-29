/*
 * FormBuilder.h
 *
 *  Created on: Nov 29, 2012
 *      Author: nmmrc18
 */

#ifndef FORMBUILDER_H_
#define FORMBUILDER_H_

#include <QObject>
#include <QtCore/QVariant>
#include <QtCore/qdatetime.h>
#include <strstream>

class FormBuilder : public QObject {
	Q_OBJECT
public:
	FormBuilder(bool);
	virtual ~FormBuilder();
	void addFields(QVariantMap *fields);
	void createJson(QVariantMap *fields);
	void addFieldsAsLine(QVariantMap *fields);
	void close();
	QString outputStream;

	QString contentType;
private:
	QString boundry;
	bool addForm;
	void addField(const QString &key, QVariant value);
};

#endif /* FORMBUILDER_H_ */
