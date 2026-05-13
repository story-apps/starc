/*
 * Copyright (C) 2015-2017 Dimka Novikov, to@dimkanovikov.pro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Full license: https://github.com/dimkanovikov/WidgetAnimationFramework/blob/master/LICENSE
 */

#include "SideSlideDecorator.h"

#include <QPaintEvent>
#include <QPainter>

using WAF::SideSlideDecorator;


SideSlideDecorator::SideSlideDecorator(QWidget* _parent)
    : QWidget(_parent)
{
    resize(_parent->size());
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);

    m_timeline.setDuration(260);
    m_timeline.setUpdateInterval(16);
    m_timeline.setEasingCurve(QEasingCurve::OutQuad);
    m_timeline.setStartFrame(0);
    m_timeline.setEndFrame(100);

    m_decorationColor = QColor(0, 0, 0, 0);

    //
    // Анимируем затемнение/осветление
    //
    connect(&m_timeline, &QTimeLine::frameChanged, this, [this](int _value) {
        const int alpha = qBound(0, _value, 160);
        if (m_decorationColor.alpha() != alpha) {
            m_decorationColor.setAlpha(alpha);
            update();
        }
    });
}

void SideSlideDecorator::grabParentSize()
{
    const QSize parentSize = parentWidget()->size();
    if (size() == parentSize) {
        return;
    }

    resize(parentSize);

    // Для больших экранов снижаем частоту обновлений декоратора, чтобы уменьшить нагрузку
    const int pixels = parentSize.width() * parentSize.height();
    m_timeline.setUpdateInterval(pixels >= (2560 * 1440) ? 33 : 16);
}

void SideSlideDecorator::decorate(bool _dark)
{
    if (m_timeline.state() == QTimeLine::Running) {
        m_timeline.stop();
    }

    m_timeline.setDirection(_dark ? QTimeLine::Forward : QTimeLine::Backward);
    m_timeline.start();
}

void SideSlideDecorator::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.fillRect(_event->rect(), m_decorationColor);
}

void SideSlideDecorator::mousePressEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event)

    emit clicked();
}
