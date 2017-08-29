/*
 * FormBuilder.cpp
 *
 *  Created on: Nov 29, 2012
 *      Author: nmmrc18
 */

#include "FormBuilder.h"
#include <bb/data/JsonDataAccess>

using namespace bb::data;

FormBuilder::FormBuilder(bool addForm) :
    QObject(),
	outputStream(""){
	this->addForm = addForm;
	if (addForm) {
		boundry = QString("----------%1").arg(QDateTime::currentDateTime().toMSecsSinceEpoch(), 0, 16);
		contentType = QString("multipart/form-data; boundary=%1").arg(boundry);
	}
}

void FormBuilder::addFields(QVariantMap *fields) {
	Q_FOREACH(const QString &key, fields->keys()) {
		addField(key, fields->value(key));
	}
}

void FormBuilder::addField(const QString &key, QVariant value) {
	QString sb;
	if (key.isEmpty()) {
		sb.append(value.toString());
	} else {
		sb = QString("\r\n--%1\r\n").arg(boundry);
		sb.append(QString("Content-Disposition: form-data;"));
		QString field;
		if (key.isEmpty()) {
			field = QString("\r\n%1").arg(value.toString());
		} else {
			field = QString("name=\"%1\";\r\n\r\n%2").arg(key, value.toString());
		}
		sb.append(field);
	}

	outputStream.append(sb);
}

void FormBuilder::createJson(QVariantMap *fields) {
    JsonDataAccess jda;
    QByteArray jsonString = QByteArray();
    QVariant theFuck = QVariant::fromValue(*fields);
    jda.saveToBuffer(theFuck, &jsonString);
    outputStream = QString(jsonString);
}

void FormBuilder::addFieldsAsLine(QVariantMap *fields) {
    QList<QString> keys = fields->keys();
    int start = 0;
    int length = keys.length();
    Q_FOREACH(const QString &key, keys) {
        QVariant value = fields->value(key);
        if (key.isEmpty()) {
            outputStream.append(value.toString());
        } else {
            outputStream.append(QString("%1=%2").arg(key).arg(value.toString()));
        }
        if (start+1 != length) {
            start++;
            outputStream.append("&");
        }
    }
}

void FormBuilder::close() {
	if (addForm) {
		QString closingString(QString("\r\n--%1--\r\n").arg(boundry));
		outputStream.append(closingString);
	}
}

FormBuilder::~FormBuilder() {
	// TODO Auto-generated destructor stub
}

