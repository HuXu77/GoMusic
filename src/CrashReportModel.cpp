/*
 * CrashReportModel.cpp
 *
 *  Created on: Jun 16, 2016
 *      Author: mitchellclay
 */

#include <CrashReportModel.h>
#include <bb/pim/account/Account>
#include <bb/pim/account/AccountService>
#include <bb/pim/message/Attachment>
#include <bb/pim/message/MessageBody>
#include <bb/pim/message/MessageBuilder>
#include <bb/pim/message/MessageContact>
#include <bb/pim/message/MessageService>
#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>
#include <bb/system/SystemDialog>

using namespace bb::pim::message;
using namespace bb::pim::account;
using bb::system::InvokeManager;
using bb::system::InvokeRequest;

QString CrashReportModel::applicationName = "GoMusic";
QString CrashReportModel::developerEmailAddress = "mitchellrclay@gmail.com";

CrashReportModel::CrashReportModel(GoMusic* mainApp) :
        mainApp(mainApp)
{
    // TODO Auto-generated constructor stub

}

CrashReportModel::~CrashReportModel()
{
    // TODO Auto-generated destructor stub
}

// The core file.
QString CrashReportModel::coreFilePath()
{
    // When doing development, the core file is named this way.
    QString defaultFile = QString("logs/") + applicationName + ".core";

    if (QFileInfo(defaultFile).exists())
    {
        return defaultFile;
    }
    else
    {
        // Release builds name the core file this way.
        QString releaseBuildDefaultFile = "logs/cascades.core";

        if (QFileInfo(releaseBuildDefaultFile).exists())
        {
            return releaseBuildDefaultFile;
        }
        else
        {
            // Otherwise, do a directory listing to see if the
            // core file got named in an unexpected way.

            QStringList nameFilters = QStringList() << "*.core";

            QFileInfoList fileInfoList =
                    QDir("logs").entryInfoList(
                            nameFilters,
                            QDir::Files,
                            // Sort by time so that newer files are first.
                            // This is so that if there are for some
                            // reason multiple core files, we'll only
                            // send the most recent one.
                            QDir::Time | QDir::Reversed
                    );

            if (fileInfoList.size() >= 1)
            {
                // Return the newest core file.
                return fileInfoList[0].absoluteFilePath();
            }
            else
            {
                // We wouldn't find any core files. We'll
                // return releaseBuildDefaultFile as our best
                // guess at what it would be if it were there.
                return releaseBuildDefaultFile;
            }
        }
    }
}

// The log file.
QString CrashReportModel::logFilePath()
{
    return "logs/log";
}

void CrashReportModel::sendCrashReport()
{
    qDebug() << "sendCrashReport()";

    QList<QString> attachmentFilePaths = filesToAttach();

    if (attachmentFilePaths.size() == 0)
    {
        showDialog("Couldn't Find Files", "The crash report files couldn't be found.", "OK", "");
    }
    else
    {
        sendEmail(developerEmailAddress, attachmentFilePaths);

        showDialog("Thank You", "Thanks for reporting this error.\n\nThe application will now be started normally.", "OK", "");

        // Clear the core file so that when the application launches next
        // time the user won't falsely be notified of a crash event.
        deleteCoreFile();

        mainApp->startApplication();
    }
}

QList<QString> CrashReportModel::filesToAttach()
{
    QList<QString> retList;

    QList<QString> filesToConsiderAttaching = QList<QString>() << coreFilePath() << logFilePath();
    //QList<QString> filesToConsiderAttaching = QList<QString>() << logFilePath();

    foreach(QString filePath, filesToConsiderAttaching)
    {
        if (QFileInfo(filePath).exists())
        {
            retList << QFileInfo(filePath).absoluteFilePath();
        }
    }

    return retList;
}

void CrashReportModel::sendEmail(QString toAddress, QList<QString> attachmentFilePaths)
{
    AccountService accountService;

    Account defaultClient = accountService.defaultAccount(
            Service::Messages);
    AccountKey accountId = defaultClient.id();

    if (accountId == -1)
    {
        showDialog(
                "Error",
                applicationName + " does not appear to have permission to send email, or email has yet to be configured for your device.\n\nPlease tap the 'View Permissions' button below and review the permissions granted to " + applicationName + ".\n\nBe sure to restart " + applicationName + " once your permissions have been corrected.",
                "View Permissions",
                ""
        );

        viewPermissions();

        return;
    }

    QList<QString> emailAddresses = QList<QString>() << toAddress;

    qDebug() << "Email addresses: " << emailAddresses;

    // Create the message service object
    MessageService service;

    // Create a message builder to create/modify the message
    MessageBuilder *builder = MessageBuilder::create(accountId);

    builder->subject(applicationName + " Crash Report");
    builder->removeAllRecipients();

    for (int i = 0; i < emailAddresses.size(); ++i)
    {
        const MessageContact recipient = MessageContact(-1, MessageContact::To, QString(),
                emailAddresses[i]);
        builder->addRecipient(recipient);
    }

    QByteArray bodyData = QString("").toUtf8();

    bodyData.append("<html><body>");
    bodyData.append("\n");

    for (int i = 0; i < attachmentFilePaths.size(); ++i)
    {
        QVariantMap metaData;

        qDebug() << "Attaching file: " << attachmentFilePaths[i];
        qDebug() << "  Size: " << QFileInfo(attachmentFilePaths[i]).size() << " bytes";

        QFile(attachmentFilePaths[i]).setPermissions(QFile::ReadOther | QFile::ReadGroup | QFile::ReadUser);

        Attachment attachment =
                Attachment(
                        "application/octet-stream",
                        QFileInfo(attachmentFilePaths[i]).fileName(),
                        QUrl(QString("file://") + attachmentFilePaths[i]),
                        metaData
                );

        builder->addAttachment(attachment);
    }

    bodyData.append("Crash report attached.<br>\n");

    bodyData.append("</body></html>");

    builder->body(MessageBody::Html, bodyData);

    // Send the new message via current account
    service.send(accountId, *builder);

    qDebug () << "Sent email to: " << toAddress;
}

// Open the 'Settings' app to the 'Application Permissions' page.
void CrashReportModel::viewPermissions()
{
    invoke("sys.settings.card", "bb.action.OPEN", "settings/view", "settings://permissions");
}

void CrashReportModel::invoke(QString target, QString action, QString mimeType, QString uri)
{
    InvokeManager invokeManager;
    InvokeRequest request;

    request.setTarget(target);
    request.setAction(action);
    request.setMimeType(mimeType);

    request.setUri(QUrl(uri));

    invokeManager.invoke(request);
}

// Show a dialog.
bool CrashReportModel::showDialog(QString title, QString body, QString confirmLabel, QString cancelLabel)
{
    bb::system::SystemDialog* menu = new bb::system::SystemDialog("OK");
    menu->setTitle(title);
    menu->setBody(body);

    menu->confirmButton()->setLabel(confirmLabel);
    menu->cancelButton()->setLabel(cancelLabel);

    // Use 'exec' rather than 'show' so that this operation is blocking.
    bb::system::SystemUiResult::Type result = menu->exec();

    menu->deleteLater();

    return (result == bb::system::SystemUiResult::ConfirmButtonSelection);
}

void CrashReportModel::startApplication()
{
    // Clear the core file so that when the application launches next
    // time the user won't falsely be notified of a crash event.
    deleteCoreFile();

    // The startApplication method should be implemented in
    // ApplicationUI to start the application in the normal way.
    mainApp->startApplication();
}

void CrashReportModel::deleteCoreFile()
{
    QFile qfile(coreFilePath());

    qfile.setPermissions(QFile::ReadOther | QFile::ReadGroup | QFile::ReadUser);

    qfile.remove();
}

