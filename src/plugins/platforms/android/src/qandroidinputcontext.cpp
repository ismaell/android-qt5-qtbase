/*
    Copyright (c) 2009-2011, BogDan Vatra <bog_dan_ro@yahoo.com>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the  BogDan Vatra <bog_dan_ro@yahoo.com> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY BogDan Vatra <bog_dan_ro@yahoo.com> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL BogDan Vatra <bog_dan_ro@yahoo.com> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "qandroidinputcontext.h"
#include "androidjnimain.h"
#include <QDebug>

QAndroidInputContext::QAndroidInputContext(QObject *parent) :
    QInputContext(parent)
{
}

QAndroidInputContext::~QAndroidInputContext()
{
}


QString QAndroidInputContext::identifierName()
{
    return "QAndroidInputContext";
}

bool QAndroidInputContext::isComposing() const
{
    return false;
}

QString QAndroidInputContext::language()
{
    return "";
}

void QAndroidInputContext::reset()
{

}

void QAndroidInputContext::update()
{
    // QWidget * w= focusWidget();
    /// try to move the window to a new position when this widget is visible
}

bool QAndroidInputContext::filterEvent( const QEvent * event )
{
    qDebug()<<"QAndroidInputContext::filterEvent"<<event->type();
    switch (event->type())
    {
        case QEvent::RequestSoftwareInputPanel:
                QtAndroid::showSoftwareKeyboard();
            break;
        case QEvent::CloseSoftwareInputPanel:
                QtAndroid::hideSoftwareKeyboard();
            break;
#if 0 // FIXME
        default:
            return QInputContext::filterEvent(event);
#endif
    }
    return true;
}
