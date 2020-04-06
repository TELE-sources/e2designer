/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2018-2019, Antony jr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @filename    : appimageupdaterdialog.cc
 * @description : The implementation of the GUI Updater dialog.
*/
#include "../include/helpers_p.hpp"
#include "../include/softwareupdatedialog_p.hpp"
#include "../include/appimageupdaterdialog.hpp"
#include <ui_AppImageUpdaterDialog.h>

using namespace AppImageUpdaterBridge;

AppImageUpdaterDialog::AppImageUpdaterDialog(QPixmap img, QWidget *parent, int flags)
    : QDialog(parent),
      p_Flags(flags) {
    bool found = false;
    m_Ui.reset(new Ui::AppImageUpdaterDialog);

    m_Ui->setupUi(this);

    /* Set AppImage icon if given. */
    if(!img.isNull()) {
        (m_Ui->softwareIcon)->setPixmap(img);
        (m_Ui->softwareIconOnUpdating)->setPixmap(img);
        setWindowIcon(img);
    } else {
        foreach (QWidget *widget, QApplication::allWidgets()) {
            if(!((widget->windowIcon()).isNull())) {
                img = widget->windowIcon().pixmap(100, 100);
                (m_Ui->softwareIcon)->setPixmap(img);
                (m_Ui->softwareIconOnUpdating)->setPixmap(img);
                setWindowIcon(img);
                found = true;
                break;
            }
            QCoreApplication::processEvents();
        }

        if(!found) {
            (m_Ui->softwareIcon)->setVisible(false);
            (m_Ui->softwareIconOnUpdating)->setVisible(false);
        }
    }

    m_ConfirmationDialog = new SoftwareUpdateDialog(this, img, flags);
    connect(m_ConfirmationDialog, &SoftwareUpdateDialog::rejected, this, &AppImageUpdaterDialog::handleRejected);
    connect(m_ConfirmationDialog, &SoftwareUpdateDialog::accepted, this, &AppImageUpdaterDialog::doUpdate);
    connect(this, SIGNAL(finished(QJsonObject)), this, SLOT(resetConnections()));
    connect(this, SIGNAL(quit()), this, SLOT(resetConnections()));
    connect(this, SIGNAL(canceled()), this, SLOT(resetConnections()));
    connect(this, SIGNAL(error(QString, short)), this, SLOT(resetConnections()));
    connect(this, SIGNAL(requiresAuthorization(QString, short, QString)), this, SLOT(resetConnections()));
    return;

}

AppImageUpdaterDialog::~AppImageUpdaterDialog() {
    return;
}

void AppImageUpdaterDialog::init(AppImageDeltaRevisioner *revisioner,
                                 const QString &applicationName) {

    getMethod(this, "doInit(QObject*,const QString&)")
    .invoke(this, Qt::QueuedConnection, Q_ARG(QObject*, revisioner),Q_ARG(QString,applicationName));
}

void AppImageUpdaterDialog::doInit(QObject *revisioner,
                                   const QString &applicationName) {
    if(b_Busy) {
        return;
    }
    resetConnections();

    m_ApplicationName = applicationName;

    /* Delta Revisioner. */
    p_DRevisioner = (!revisioner) ? new AppImageDeltaRevisioner(/*single threaded=*/true, /*parent=*/this) :
                    (AppImageDeltaRevisioner*)revisioner;

    /* Program Logic. */
    connect((m_Ui->updateCancelBtn), &QPushButton::clicked, p_DRevisioner, &AppImageDeltaRevisioner::cancel);
    connect(this, &QDialog::rejected, p_DRevisioner, &AppImageDeltaRevisioner::cancel);
    connect(p_DRevisioner, &AppImageDeltaRevisioner::canceled, this, &QDialog::hide);
    connect(p_DRevisioner, &AppImageDeltaRevisioner::canceled, this, &AppImageUpdaterDialog::canceled, Qt::DirectConnection);
    connect(p_DRevisioner, &AppImageDeltaRevisioner::updateAvailable, this, &AppImageUpdaterDialog::handleUpdateAvailable);
    connect(p_DRevisioner, &AppImageDeltaRevisioner::error, this, &AppImageUpdaterDialog::handleError);
    connect(p_DRevisioner, &AppImageDeltaRevisioner::started, this, &AppImageUpdaterDialog::started, Qt::DirectConnection);
    connect(p_DRevisioner, &AppImageDeltaRevisioner::finished, this, &AppImageUpdaterDialog::handleFinished);
    connect(p_DRevisioner, &AppImageDeltaRevisioner::progress, this, &AppImageUpdaterDialog::handleProgress);
    connect(p_DRevisioner, &AppImageDeltaRevisioner::operatingAppImagePath, 
		    this, &AppImageUpdaterDialog::handleOperatingAppImagePath);
    n_MegaBytesTotal = 0;
    p_DRevisioner->checkForUpdate();
    if(p_Flags & ShowBeforeProgress) {
        (m_Ui->mainStack)->setCurrentIndex(0);
        showWidget();
    }
    b_Busy = true;
    return;
}

void AppImageUpdaterDialog::resetConnections() {
    if(!p_DRevisioner) {
        return;
    }
    hide();
    disconnect((m_Ui->updateCancelBtn), &QPushButton::clicked, p_DRevisioner, &AppImageDeltaRevisioner::cancel);
    disconnect(this, &QDialog::rejected, p_DRevisioner, &AppImageDeltaRevisioner::cancel);
    disconnect(p_DRevisioner, &AppImageDeltaRevisioner::canceled, this, &QDialog::hide);
    disconnect(p_DRevisioner, &AppImageDeltaRevisioner::canceled, this, &AppImageUpdaterDialog::canceled);
    disconnect(p_DRevisioner, &AppImageDeltaRevisioner::updateAvailable, this, &AppImageUpdaterDialog::handleUpdateAvailable);
    disconnect(p_DRevisioner, &AppImageDeltaRevisioner::error, this, &AppImageUpdaterDialog::handleError);
    disconnect(p_DRevisioner, &AppImageDeltaRevisioner::started, this, &AppImageUpdaterDialog::started);
    disconnect(p_DRevisioner, &AppImageDeltaRevisioner::finished, this, &AppImageUpdaterDialog::handleFinished);
    disconnect(p_DRevisioner, &AppImageDeltaRevisioner::progress, this, &AppImageUpdaterDialog::handleProgress);
    disconnect(p_DRevisioner, &AppImageDeltaRevisioner::operatingAppImagePath, 
		    this, &AppImageUpdaterDialog::handleOperatingAppImagePath);
    b_Busy = false;
    p_DRevisioner = nullptr; /* Dereference */
}

void AppImageUpdaterDialog::showWidget(void) {
    if(!(p_Flags & ShowProgressDialog) &&
            (m_Ui->mainStack)->currentIndex() != 0) {
        return;
    }
    show();
    return;
}

void AppImageUpdaterDialog::handleRejected(void) {
    emit finished(QJsonObject());
}

void AppImageUpdaterDialog::doUpdate(void) {
    p_DRevisioner->start();
    showWidget();
}

void AppImageUpdaterDialog::handleUpdateAvailable(bool isUpdateAvailable, QJsonObject UpdateInfo) {
    hide();
    /* Move to the correct position. */
    auto prevPos = pos() + rect().center();
    (m_Ui->mainStack)->setCurrentIndex(1);
    move(prevPos - rect().center());

    bool showUpdateDialog = p_Flags & ShowUpdateConfirmationDialog;
    bool showNoUpdateDialog = p_Flags & NotifyWhenNoUpdateIsAvailable;
    setWindowTitle(QString::fromUtf8("Updating ") +
                   QFileInfo(UpdateInfo["AbsolutePath"].toString()).baseName() +
                   QString::fromUtf8("... "));

    s_CurrentAppImagePath = UpdateInfo["AbsolutePath"].toString();

    if(isUpdateAvailable) {
        if(showUpdateDialog) {
            QString oldSha = UpdateInfo["Sha1Hash"].toString(),
                    newSha = UpdateInfo["RemoteSha1Hash"].toString(),
                    notes = UpdateInfo["ReleaseNotes"].toString();
            /* we don't need the full sha to show the diff. */
            oldSha.resize(7);
            newSha.resize(7);

            oldSha = oldSha.toLower();
            newSha = newSha.toLower();

            m_ConfirmationDialog->init( m_ApplicationName, oldSha, newSha, notes);
        }else{
	    // Auto confirm if we are not showing the confirm dialog.
	    doUpdate();
	}
    } else {
        if(showNoUpdateDialog) {
            QMessageBox box(this);
            QString currentAppImageName = QFileInfo(UpdateInfo["AbsolutePath"].toString()).fileName();
            box.setWindowTitle(QString::fromUtf8("No Updates Available!"));
            box.setText(QString::fromUtf8("You are currently using the lastest version of ") +
                        currentAppImageName +
                        QString::fromUtf8("."));
            box.exec();
        }
        emit finished(QJsonObject());
    }
    return;
}

void AppImageUpdaterDialog::handleError(short errorCode) {
    bool show = p_Flags & ShowErrorDialog,
         alert = p_Flags &  AlertWhenAuthorizationIsRequired,
         doAlert = false;
    QString errorString = errorCodeToDescriptionString(errorCode);

    if(errorCode == NoReadPermission ||
            errorCode == NoPermissionToReadSourceFile ||
            errorCode == NoPermissionToReadWriteTargetFile) {
        show = (alert) ? false : show;
        doAlert = alert;
    }

    if(show) {
        QMessageBox box(this);
        box.setWindowTitle(QString::fromUtf8("Update Failed"));
        box.setIcon(QMessageBox::Critical);
        box.setText(QString::fromUtf8("Update failed for '") + 
		    QFileInfo(s_CurrentAppImagePath).fileName() + 
		    QString::fromUtf8("': ") + errorString);
        box.exec();
    }

    if(doAlert) {
        emit requiresAuthorization(errorString, errorCode, s_CurrentAppImagePath);
    } else {
        emit error(errorString, errorCode);
    }
    hide();
    return;
}

void AppImageUpdaterDialog::handleFinished(QJsonObject newVersion, QString oldVersionPath) {
    (void)oldVersionPath;
    (m_Ui->updateSpeedLbl)->setText(QString::fromUtf8("Finalizing Update... "));

    bool execute = false;
    bool show = p_Flags & ShowFinishedDialog;

    if(show) {
        QString currentAppImageName = QFileInfo(oldVersionPath).fileName();
        QMessageBox box(this);
        box.setWindowTitle(QString::fromUtf8("Update Completed!"));
        box.setText(QString::fromUtf8("Update completed successfully for ") +
                    currentAppImageName +
                    QString::fromUtf8(", the new version is saved at '") +
                    newVersion["AbsolutePath"].toString() +
                    QString::fromUtf8("', do you want to open it?"));
        box.addButton(QMessageBox::Yes);
        box.addButton(QMessageBox::No);
        execute = (box.exec() == QMessageBox::Yes);
    }

    if(execute) {
        QFileInfo info(newVersion["AbsolutePath"].toString());
        if(!info.isExecutable()) {
            {
                QFile file(newVersion["AbsolutePath"].toString());
                file.setPermissions(QFileDevice::ExeUser |
                                    QFileDevice::ExeOther|
                                    QFileDevice::ExeGroup|
                                    info.permissions());
            }
        }
        QProcess::startDetached(newVersion["AbsolutePath"].toString());
        emit quit();
    }
    hide();
    emit finished(newVersion);
    return;
}

void AppImageUpdaterDialog::handleProgress(int percent,
        qint64 bytesReceived,
        qint64 bytesTotal,
        double speed,
        QString units) {
    (m_Ui->progressBar)->setValue(percent);
    double MegaBytesReceived = bytesReceived / 1048576;
    if(!n_MegaBytesTotal) {
        n_MegaBytesTotal = bytesTotal / 1048576;
    }
    const QString progressTemplate = QString::fromUtf8("Updating %1 MiB of %2 MiB at %3 %4...");
    QString statusText = progressTemplate.arg(MegaBytesReceived).arg(n_MegaBytesTotal).arg(speed).arg(units);
    (m_Ui->updateSpeedLbl)->setText(statusText);
    return;
}

void AppImageUpdaterDialog::handleOperatingAppImagePath(QString path){
  s_CurrentAppImagePath = path;
}
