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
 * @filename    : zsyncblockrangedownloader_p.cc
 * @description : This the main class which manages all block requests and reply
 * also emits the progress overall , This is where the class is implemented.
*/
#include "../include/zsyncblockrangedownloader_p.hpp"
#include "../include/zsyncblockrangereply_p.hpp"
#include "../include/zsyncremotecontrolfileparser_p.hpp"
#include "../include/zsyncwriter_p.hpp"

using namespace AppImageUpdaterBridge;

/*
 * This is the main class which manages the block downloads for ZsyncWriterPrivate ,
 * This class provides progress overall and also gives the control to cancel the download
 * anytime without any kind of data races.
*/
ZsyncBlockRangeDownloaderPrivate::ZsyncBlockRangeDownloaderPrivate(ZsyncWriterPrivate *w, QNetworkAccessManager *nm)
    : QObject(),
      p_Manager(nm),
      p_Writer(w)
{

    connect(p_Writer, SIGNAL(download(qint64, qint64, QUrl)),
            this, SLOT(initDownloader(qint64, qint64, QUrl)), Qt::QueuedConnection);
    connect(this, SIGNAL(finished()),
            p_Writer, SLOT(verifyDownloadAndFinish()), Qt::QueuedConnection);
    connect(p_Writer, SIGNAL(blockRange(qint32, qint32)),
            this, SLOT(handleBlockRange(qint32, qint32)),Qt::QueuedConnection);
    connect(this, SIGNAL(blockRangesRequested()),
            p_Writer, SLOT(getBlockRanges()), Qt::QueuedConnection);
    return;
}

ZsyncBlockRangeDownloaderPrivate::~ZsyncBlockRangeDownloaderPrivate()
{
    return;
}

/* Cancels all ZsyncBlockRangeReplyPrivate QObjects. */
void ZsyncBlockRangeDownloaderPrivate::cancel(void)
{
    b_CancelRequested = true;
    emit cancelAllReply();
    return;
}

/* Starts the download of all the required blocks. */
void ZsyncBlockRangeDownloaderPrivate::initDownloader(qint64 bytesReceived, qint64 bytesTotal, QUrl targetFileUrl)
{
    disconnect(p_Writer, SIGNAL(download(qint64, qint64, QUrl)),
               this, SLOT(initDownloader(qint64, qint64, QUrl)));

    u_TargetFileUrl = targetFileUrl;
    n_BytesTotal = bytesTotal;
    n_BytesReceived = bytesReceived;
    b_Errored = false;
    b_CancelRequested = false;
    n_BlockReply = 0;

    /*
     * Start the download , if the host cannot accept range requests then
     * blockRange signal will return with both 'from' and 'to' ranges 0.
    */
    emit started();
    emit blockRangesRequested();
    return;
}

/* This is connected to the blockRange signal of ZsyncWriterPrivate and starts the
 * download of a new block range.
*/
void ZsyncBlockRangeDownloaderPrivate::handleBlockRange(qint32 fromRange, qint32 toRange)
{
    QNetworkRequest request;
    request.setUrl(u_TargetFileUrl);
    if(fromRange || toRange) {
        QByteArray rangeHeaderValue = "bytes=" + QByteArray::number(fromRange) + "-";
        rangeHeaderValue += QByteArray::number(toRange);
        request.setRawHeader("Range", rangeHeaderValue);
    }
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    ++n_BlockReply;

    auto blockReply = new ZsyncBlockRangeReplyPrivate(p_Writer, p_Manager->get(request), fromRange, toRange);
    connect(this, &ZsyncBlockRangeDownloaderPrivate::cancelAllReply,
            blockReply, &ZsyncBlockRangeReplyPrivate::cancel);
    connect(blockReply, &ZsyncBlockRangeReplyPrivate::canceled,
            this, &ZsyncBlockRangeDownloaderPrivate::handleBlockReplyCancel,
            Qt::QueuedConnection);
    connect(blockReply, &ZsyncBlockRangeReplyPrivate::finished,
            this, &ZsyncBlockRangeDownloaderPrivate::handleBlockReplyFinished,
            Qt::QueuedConnection);
    connect(blockReply, &ZsyncBlockRangeReplyPrivate::error,
            this, &ZsyncBlockRangeDownloaderPrivate::handleBlockReplyError,
            Qt::QueuedConnection);
    if(!(fromRange || toRange)) {
        connect(blockReply, &ZsyncBlockRangeReplyPrivate::seqProgress,
                this, &ZsyncBlockRangeDownloaderPrivate::progress, Qt::DirectConnection);
    } else {
        connect(blockReply, &ZsyncBlockRangeReplyPrivate::progress,
                this, &ZsyncBlockRangeDownloaderPrivate::handleBlockReplyProgress,
                Qt::QueuedConnection);
    }
    return;
}

/* Calculates the overall progress and also emits it when done. */
void ZsyncBlockRangeDownloaderPrivate::handleBlockReplyProgress(qint64 bytesReceived, double speed, QString units)
{
    n_BytesReceived += bytesReceived;
    int nPercentage = static_cast<int>(
                          (static_cast<float>
                           (n_BytesReceived) * 100.0
                          ) / static_cast<float>
                          (n_BytesTotal)
                      );
    emit progress(nPercentage, n_BytesReceived, n_BytesTotal, speed, units);
    return;
}

/* Boilerplate code to finish a ZsyncBlockRangeReplyPrivate. */
void ZsyncBlockRangeDownloaderPrivate::handleBlockReplyFinished(void)
{
    --n_BlockReply;

    if(n_BlockReply <= 0) {
        if(b_CancelRequested == true) {
            b_CancelRequested = false;
            connect(p_Writer, SIGNAL(download(qint64, qint64, QUrl)),
                    this, SLOT(initDownloader(qint64, qint64, QUrl)), (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
            emit canceled();
        } else {
            connect(p_Writer, SIGNAL(download(qint64, qint64, QUrl)),
                    this, SLOT(initDownloader(qint64, qint64, QUrl)), (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
            emit finished();
        }
    }
    return;
}

void ZsyncBlockRangeDownloaderPrivate::handleBlockReplyError(QNetworkReply::NetworkError errorCode)
{
    if(b_Errored == true) {
        return;
    }
    b_Errored = true;
    short e = 0;
    if(errorCode > 0 && errorCode < 101) {
        e = ConnectionRefusedError + ((short)errorCode - 1);
    } else if(errorCode == QNetworkReply::UnknownNetworkError) {
        e = UnknownNetworkError;
    } else if(errorCode == QNetworkReply::UnknownProxyError) {
        e = UnknownProxyError;
    } else if(errorCode >= 101 && errorCode < 201) {
        e = ProxyConnectionRefusedError + ((short)errorCode - 101);
    } else if(errorCode == QNetworkReply::ProtocolUnknownError) {
        e = ProtocolUnknownError;
    } else if(errorCode == QNetworkReply::ProtocolInvalidOperationError) {
        e = ProtocolInvalidOperationError;
    } else if(errorCode == QNetworkReply::UnknownContentError) {
        e = UnknownContentError;
    } else if(errorCode == QNetworkReply::ProtocolFailure) {
        e = ProtocolFailure;
    } else if(errorCode >= 201 && errorCode < 401) {
        e = ContentAccessDenied + ((short)errorCode - 201);
    } else if(errorCode >= 401 && errorCode <= 403) {
        e = InternalServerError + ((short)errorCode - 401);
    } else {
        e = UnknownServerError;
    }
    emit error(e);
    return;
}

/* When canceled , check if the current emitting ZsyncBlockRangeReply is the last
 * object emitting it , If so then emit canceled from this class.
*/
void ZsyncBlockRangeDownloaderPrivate::handleBlockReplyCancel(void)
{
    auto blockReply = (ZsyncBlockRangeReplyPrivate*)QObject::sender();
    blockReply->deleteLater();

    --n_BlockReply;

    if(n_BlockReply <= 0) {
        b_CancelRequested = false;
        connect(p_Writer, SIGNAL(download(qint64, qint64, QUrl)),
                this, SLOT(initDownloader(qint64, qint64, QUrl)), (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));
        emit canceled();
    }
    return;
}
