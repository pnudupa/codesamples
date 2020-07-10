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

#ifndef SCREENRULER_H
#define SCREENRULER_H

#include <QWidget>

class ScreenRuler : public QWidget
{
    Q_OBJECT

public:
    ScreenRuler(QWidget *parent = nullptr);
    ~ScreenRuler();

    enum Unit { Inch, Centimeter, Pixel };
    Q_ENUM(Unit)
    Q_PROPERTY(Unit unit READ unit WRITE setUnit NOTIFY unitChanged)
    void setUnit(Unit val);
    Unit unit() const { return m_unit; }
    Q_SIGNAL void unitChanged();

    enum Length { Small, Medium, Big };
    Q_ENUM(Length)
    Q_PROPERTY(Length length READ length WRITE setLength NOTIFY lengthChanged)
    void setLength(Length val);
    Length length() const { return m_length; }
    Q_SIGNAL void lengthChanged();

    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    void setOrientation(Qt::Orientation val);
    Qt::Orientation orientation() const { return m_orientation; }
    Q_SIGNAL void orientationChanged();

    Q_PROPERTY(bool movable READ isMovable WRITE setMovable NOTIFY movableChanged)
    void setMovable(bool val);
    bool isMovable() const { return m_movable; }
    Q_SIGNAL void movableChanged();

    Q_PROPERTY(bool moving READ isMoving NOTIFY movingChanged)
    bool isMoving() const { return m_moving; }
    Q_SIGNAL void movingChanged();

    Q_PROPERTY(bool stayOnTop READ isStayOnTop WRITE setStayOnTop NOTIFY stayOnTopChanged)
    void setStayOnTop(bool val);
    bool isStayOnTop() const { return this->windowFlags().testFlag(Qt::WindowStaysOnTopHint); }
    Q_SIGNAL void stayOnTopChanged();

    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    void setBackgroundColor(const QColor &val);
    QColor backgroundColor() const { return m_backgroundColor; }
    Q_SIGNAL void backgroundColorChanged();

    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor NOTIFY foregroundColorChanged)
    void setForegroundColor(const QColor &val);
    QColor foregroundColor() const { return m_foregroundColor; }
    Q_SIGNAL void foregroundColorChanged();

    Q_PROPERTY(QColor majorTickColor READ majorTickColor WRITE setMajorTickColor NOTIFY majorTickColorChanged)
    void setMajorTickColor(const QColor &val);
    QColor majorTickColor() const { return m_majorTickColor; }
    Q_SIGNAL void majorTickColorChanged();

    Q_PROPERTY(QColor minorTickColor READ minorTickColor WRITE setMinorTickColor NOTIFY minorTickColorChanged)
    void setMinorTickColor(const QColor &val);
    QColor minorTickColor() const { return m_minorTickColor; }
    Q_SIGNAL void minorTickColorChanged();

    enum Transparency { Opaque, SemiTransparent, SuperTransparent };
    Q_ENUM(Transparency)
    Q_PROPERTY(Transparency transparency READ transparency WRITE setTransparency NOTIFY transparencyChanged)
    void setTransparency(Transparency val);
    Transparency transparency() const { return m_transparency; }
    Q_SIGNAL void transparencyChanged();

    Q_INVOKABLE void mark();
    Q_SLOT void clearMarks();

protected:
    void setMoving(bool val);

    void placeSelf();
    void handleClick(const QPointF &localPos, const QPointF &screenPos);

protected:
    void paintEvent(QPaintEvent *pe);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    Unit m_unit = Pixel;
    bool m_moving = false;
    bool m_movable = true;
    qreal m_opacity = 1.0;
    Length m_length = Medium;
    bool m_initialized = false;
    bool m_mousePressed = false;
    QList<qreal> m_marks;
    QPointF m_lastMousePos;
    QPointF m_hoverMousePos;
    QColor m_majorTickColor = Qt::black;
    QColor m_minorTickColor = Qt::darkGray;
    QColor m_foregroundColor = Qt::black;
    QColor m_backgroundColor = Qt::yellow;
    Transparency m_transparency = Opaque;
    Qt::Orientation m_orientation = Qt::Horizontal;
};
#endif // SCREENRULER_H
