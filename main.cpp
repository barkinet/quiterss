/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "qtsingleapplication/qtsingleapplication.h"
#include "rsslisting.h"

void LoadLang (QString &lang){
    QString AppFileName = qApp->applicationDirPath()+"/QtRSS.ini";
    QSettings *m_settings = new QSettings(AppFileName, QSettings::IniFormat);
    QString strLocalLang = QLocale::system().name();

    m_settings->beginGroup("/Settings");
    lang = m_settings->value("/Lang", strLocalLang).toString();
    m_settings->endGroup();
}

int main(int argc, char **argv)
{
    QtSingleApplication app(argc, argv);
    if (app.isRunning()) {
      if (1 == argc) {
        app.sendMessage("--show");
      }
      else {
        QString message = app.arguments().value(1);
        for (int i = 2; i < argc; ++i)
          message += '\n' + app.arguments().value(i);
        app.sendMessage(message);
      }
      return 0;
    }
    app.setApplicationName("QtRss");
    app.setWindowIcon(QIcon(":/images/QtRSS.ico"));
    app.setQuitOnLastWindowClosed(false);

//    QString fileString = ":/style/qstyle";
    QFile file(app.applicationDirPath() + "/Style/QtRSS.qss");
    file.open(QFile::ReadOnly);
    app.setStyleSheet(QLatin1String(file.readAll()));

    QString lang;
    LoadLang(lang);
    QTranslator translator;
    translator.load(lang, app.applicationDirPath() + QString("/lang"));
    app.installTranslator(&translator);

    RSSListing rsslisting;

    app.setActivationWindow(&rsslisting, true);
    QObject::connect(&app, SIGNAL(messageReceived(const QString&)), &rsslisting, SLOT(receiveMessage(const QString&)));

    rsslisting.show();

    return app.exec();
}