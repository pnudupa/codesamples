/****************************************************************************
** Copyright 2020 Prashanth N Udupa <prashanth.udupa@gmail.com>
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
** notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
** notice, this list of conditions and the following disclaimer in the
** documentation and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
** contributors may be used to endorse or promote products derived from t
** his software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
** FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
** COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
** BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
** OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
** AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
** OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
**
****************************************************************************/

#include <QSettings>
#include <QApplication>
#include <QVersionNumber>
#include <QMetaProperty>

#include "screenruler.h"

void loadSettings(QObject *object, const QSettings &settings)
{
    const QMetaObject *mo = object->metaObject();
    const QString className = QString::fromLatin1(mo->className());

    for(int i=mo->propertyOffset(); i<mo->propertyCount(); i++)
    {
        const QMetaProperty prop = mo->property(i);
        if(!prop.isWritable())
            continue;

        const QString key = className + QStringLiteral("/") + QString::fromLatin1(prop.name());
        if(settings.contains(key))
            prop.write(object, settings.value(key));
    }
}

void saveSettings(const QObject *object, QSettings &settings)
{
    const QMetaObject *mo = object->metaObject();
    const QString className = QString::fromLatin1(mo->className());

    for(int i=mo->propertyOffset(); i<mo->propertyCount(); i++)
    {
        const QMetaProperty prop = mo->property(i);
        if(!prop.isWritable())
            continue;

        const QString key = className + QStringLiteral("/") + QString::fromLatin1(prop.name());
        settings.setValue(key, prop.read(object));
    }
}

void ScreenRulerQtMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString &message)
{
#ifdef QT_NO_DEBUG
    Q_UNUSED(type)
    Q_UNUSED(context)
    Q_UNUSED(message)
#else
    QString logMessage;

    QTextStream ts(&logMessage, QIODevice::WriteOnly);
    switch(type)
    {
    case QtDebugMsg: ts << "Debug: "; break;
    case QtWarningMsg: ts << "Warning: "; break;
    case QtCriticalMsg: ts << "Critical: "; break;
    case QtFatalMsg: ts << "Fatal: "; break;
    case QtInfoMsg: ts << "Info: "; break;
    }

    const char *where = context.function ? context.function : context.file;
    static const char *somewhere = "Somewhere";
    if(where == nullptr)
        where = somewhere;

    ts << "[" << where << " / " << context.line << "] - ";
    ts << message;
    ts.flush();

    fprintf(stderr, "%s\n", qPrintable(logMessage));
#endif
}

int main(int argc, char *argv[])
{
    static const QVersionNumber version(0, 0, 1);
    qInstallMessageHandler(ScreenRulerQtMessageHandler);

    QApplication a(argc, argv);
    a.setOrganizationName( QStringLiteral("TERIFLIX") );
    a.setApplicationName( QStringLiteral("ScreenRuler") );
    a.setApplicationVersion( version.toString() );

    QSettings settings(QSettings::UserScope);

    ScreenRuler w;
    ::loadSettings(&w, settings);

    w.show();

    const int ret = a.exec();

    ::saveSettings(&w, settings);

    return ret;
}
