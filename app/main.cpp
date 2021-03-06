/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (C) 2018 Konsulko Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QtCore/QDebug>
#include <QtCore/QCommandLineParser>
#include <QtCore/QUrlQuery>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQml/qqml.h>
#include <QQuickWindow>
#include <QtQuickControls2/QQuickStyle>

#include <qlibwindowmanager.h>

int main(int argc, char *argv[])
{
    // Slight hack, using the homescreen role greatly simplifies things wrt
    // the windowmanager
    QString myname = QString("homescreen");

    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument("port", app.translate("main", "port for binding"));
    parser.addPositionalArgument("secret", app.translate("main", "secret for binding"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);
    QStringList positionalArguments = parser.positionalArguments();

    QQmlApplicationEngine engine;

    if (positionalArguments.length() == 2) {
        int port = positionalArguments.takeFirst().toInt();
        QString secret = positionalArguments.takeFirst();
        QUrl bindingAddress;
        bindingAddress.setScheme(QStringLiteral("ws"));
        bindingAddress.setHost(QStringLiteral("localhost"));
        bindingAddress.setPort(port);
        bindingAddress.setPath(QStringLiteral("/api"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("token"), secret);
        bindingAddress.setQuery(query);
        QQmlContext *context = engine.rootContext();
        context->setContextProperty(QStringLiteral("bindingAddress"), bindingAddress);

        std::string token = secret.toStdString();
        QLibWindowmanager* qwm = new QLibWindowmanager();

        // WindowManager
        if(qwm->init(port, secret) != 0){
            exit(EXIT_FAILURE);
        }

        // Request a surface as described in layers.json windowmanager’s file
        if (qwm->requestSurface(myname) != 0) {
            exit(EXIT_FAILURE);
        }

        // Create an event callback against an event type. Here a lambda is called when SyncDraw event occurs
        qwm->set_event_handler(QLibWindowmanager::Event_SyncDraw, [qwm, myname](json_object*) {
            fprintf(stderr, "Surface got syncDraw!\n");
            qwm->endDraw(myname);
        });

        engine.load(QUrl(QStringLiteral("qrc:/cluster-gauges.qml")));

        // Find the instantiated model QObject and connect the signals/slots
        QList<QObject *> mobjs = engine.rootObjects();

        QQuickWindow *window = qobject_cast<QQuickWindow *>(mobjs.first());
        QObject::connect(window, SIGNAL(frameSwapped()), qwm, SLOT(slotActivateSurface()));
    }

    return app.exec();
}
