/*
 * CrashReportModel.h
 *
 *  Created on: Jun 16, 2016
 *      Author: mitchellclay
 */

#ifndef CRASHREPORTMODEL_H_
#define CRASHREPORTMODEL_H_

#include <QObject>
#include <QtCore/QtCore>
#include "GoMusic.hpp"

class GoMusic;

class CrashReportModel : public QObject
{
    Q_OBJECT
public:
    CrashReportModel(GoMusic* mainApp);
    virtual ~CrashReportModel();

    static QString coreFilePath();
    static QString logFilePath();

    // Displays a system dialog.
    static bool showDialog(QString title, QString body, QString confirmLabel, QString cancelLabel);

    // Opens the "Applications Permissions" settings page.
    void viewPermissions();

    void invoke(QString target, QString action, QString mimeType, QString uri);

    // The files to attach to the email.
    QList<QString> filesToAttach();

    // Creates an email message and displays it on the user's screen.
    void sendEmail(QString toAddress, QList<QString> attachmentFilePaths);

    // Creates an email message with the core file and log file
    // attached and the application developer's email address
    // in the "To" field.
    Q_INVOKABLE void sendCrashReport();

    // Starts the application. Called when the 'Start Application' button
    // is tapped on the Crash Report screen.
    Q_INVOKABLE void startApplication();
private:
    static QString applicationName;
    static QString developerEmailAddress;

    GoMusic* mainApp;

    // Clear the core file so that when the application launches next
    // time the user won't falsely be notified of a crash event.
    void deleteCoreFile();
};

#endif /* CRASHREPORTMODEL_H_ */
