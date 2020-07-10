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

#include "screenruler.h"

#include <QMenu>
#include <QAction>
#include <QScreen>
#include <QWindow>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>

static const QList<QColor> &backgroundColorOptions()
{
    static QList<QColor> ret = QList<QColor>() << QColor("#ffbf00") << QColor("#00bfff") << QColor("#00ff80");
    return ret;
}

class Scale
{
public:
    Scale(const QScreen *screen, Qt::Orientation orientation, ScreenRuler::Unit unit)
        : m_screen(screen), m_unit(unit), m_orientation(orientation),
          m_pixelsPerInch(m_orientation == Qt::Horizontal ? m_screen->physicalDotsPerInchX() : m_screen->physicalDotsPerInchY()),
          m_pixelsPerCentimeter(m_pixelsPerInch / m_centimetersPerInch) { }
    ~Scale() { }

    const QScreen *screen() const { return m_screen; }
    Qt::Orientation orientation() const { return m_orientation; }
    ScreenRuler::Unit unit() const { return m_unit; }

    qreal fromPixels(int val) const {
        switch(m_unit) {
        case ScreenRuler::Inch: return qreal(val)/m_pixelsPerInch;
        case ScreenRuler::Centimeter: return qreal(val)/m_pixelsPerCentimeter;
        default: break;
        }
        return qreal(val);
    }

    qreal toPixels(qreal val) const {
        switch(m_unit) {
        case ScreenRuler::Inch: return val * m_pixelsPerInch;
        case ScreenRuler::Centimeter: return val * m_pixelsPerCentimeter;
        default: break;
        }
        return val;
    }

    // How many minor-ticks should pass for us to paint a major-tick
    int minorTicksPerMajorTick() const {
        return (m_unit == ScreenRuler::Pixel) ? 5 : ((m_unit == ScreenRuler::Inch) ? 10 : 2);
    }

    // Returns a number in m_units
    qreal majorTick() const {
        return this->minorTick() * this->minorTicksPerMajorTick();
    }
    qreal minorTick() const {
        return m_unit == ScreenRuler::Pixel ? 20 : (m_unit == ScreenRuler::Inch) ? 0.1 : 0.5;
    }

    // Returns a number in pixels
    qreal majorTickPixels() const { return this->toPixels(minorTick()); }
    qreal minorTickPixels() const { return this->toPixels(minorTick()); }

private:
    const QScreen *m_screen = nullptr;
    const ScreenRuler::Unit m_unit = ScreenRuler::Pixel;
    const Qt::Orientation m_orientation = Qt::Horizontal;

    const qreal m_pixelsPerInch = 0;
    const qreal m_centimetersPerInch = 2.54;
    const qreal m_pixelsPerCentimeter = 0;
};

ScreenRuler::ScreenRuler(QWidget *parent)
    : QWidget(parent, Qt::CustomizeWindowHint|Qt::FramelessWindowHint|Qt::NoDropShadowWindowHint)
{
    this->winId();
    this->placeSelf();
    this->setStayOnTop(true);
    this->setMouseTracking(true);
    this->setBackgroundColor(::backgroundColorOptions().first());

    QMenu *unitsMenu = new QMenu(this);
    unitsMenu->setTitle(QStringLiteral("Units"));
    this->addAction(unitsMenu->menuAction());

    QActionGroup *unitsActionGroup = new QActionGroup(unitsMenu);
    unitsActionGroup->addAction( unitsMenu->addAction( QStringLiteral("Inch"), nullptr, nullptr, QKeySequence("Ctrl+I") ) );
    unitsActionGroup->addAction( unitsMenu->addAction( QStringLiteral("Centimeter"), nullptr, nullptr, QKeySequence("Ctrl+C") ) );
    unitsActionGroup->addAction( unitsMenu->addAction( QStringLiteral("Pixel"), nullptr, nullptr, QKeySequence("Ctrl+P") ) );
    Q_FOREACH(QAction *action, unitsActionGroup->actions())
        action->setCheckable(true);
    unitsActionGroup->actions().at(m_unit)->setChecked(true);
    unitsActionGroup->setExclusive(true);
    connect(unitsActionGroup, &QActionGroup::triggered, [=](QAction *action) {
        const int index = unitsActionGroup->actions().indexOf(action);
        this->setUnit( Unit(index) );
    });

    QMenu *lengthMenu = new QMenu(this);
    lengthMenu->setTitle(QStringLiteral("Length"));
    this->addAction(lengthMenu->menuAction());

    QActionGroup *lengthActionGroup = new QActionGroup(lengthMenu);
    lengthActionGroup->addAction( lengthMenu->addAction( QStringLiteral("Small"), nullptr, nullptr, QKeySequence("Ctrl+L") ) );
    lengthActionGroup->addAction( lengthMenu->addAction( QStringLiteral("Medium"), nullptr, nullptr, QKeySequence("Ctrl+M") ) );
    lengthActionGroup->addAction( lengthMenu->addAction( QStringLiteral("Big"), nullptr, nullptr, QKeySequence("Ctrl+G") ) );
    Q_FOREACH(QAction *action, lengthActionGroup->actions())
        action->setCheckable(true);
    lengthActionGroup->actions().at(m_length)->setChecked(true);
    lengthActionGroup->setExclusive(true);
    connect(lengthActionGroup, &QActionGroup::triggered, [=](QAction *action) {
        const int index = lengthActionGroup->actions().indexOf(action);
        this->setLength( Length(index) );
    });

    QMenu *orientationMenu = new QMenu(this);
    orientationMenu->setTitle(QStringLiteral("Orientation"));
    this->addAction(orientationMenu->menuAction());

    QActionGroup *orientationActionGroup = new QActionGroup(orientationMenu);
    orientationActionGroup->addAction( orientationMenu->addAction( QStringLiteral("Horizontal"), nullptr, nullptr, QKeySequence("Ctrl+H") ) );
    orientationActionGroup->addAction( orientationMenu->addAction( QStringLiteral("Vertical"), nullptr, nullptr, QKeySequence("Ctrl+V") ) );
    Q_FOREACH(QAction *action, orientationActionGroup->actions())
        action->setCheckable(true);
    orientationActionGroup->actions().at(m_orientation-Qt::Horizontal)->setChecked(true);
    orientationActionGroup->setExclusive(true);
    connect(orientationActionGroup, &QActionGroup::triggered, [=](QAction *action) {
        const int index = orientationActionGroup->actions().indexOf(action);
        this->setOrientation( Qt::Orientation(index+Qt::Horizontal) );
    });

    QMenu *transparencyMenu = new QMenu(this);
    transparencyMenu->setTitle(QStringLiteral("Transparency"));
    this->addAction(transparencyMenu->menuAction());

    QActionGroup *transparencyActionGroup = new QActionGroup(transparencyMenu);
    transparencyActionGroup->addAction( transparencyMenu->addAction( QStringLiteral("Opaque"), nullptr, nullptr, QKeySequence("Ctrl+O") ) );
    transparencyActionGroup->addAction( transparencyMenu->addAction( QStringLiteral("Semi Transparent"), nullptr, nullptr, QKeySequence("Ctrl+S") ) );
    transparencyActionGroup->addAction( transparencyMenu->addAction( QStringLiteral("Super Transparent"), nullptr, nullptr, QKeySequence("Ctrl+R") ) );
    Q_FOREACH(QAction *action, transparencyActionGroup->actions())
        action->setCheckable(true);
    transparencyActionGroup->actions().at(m_transparency)->setChecked(true);
    transparencyActionGroup->setExclusive(true);
    connect(transparencyActionGroup, &QActionGroup::triggered, [=](QAction *action) {
        const int index = transparencyActionGroup->actions().indexOf(action);
        this->setTransparency( ScreenRuler::Transparency(index) );
    });

    QMenu *colorsMenu = new QMenu(this);
    colorsMenu->setTitle(QStringLiteral("Colors"));
    this->addAction(colorsMenu->menuAction());

    QActionGroup *colorsActionGroup = new QActionGroup(colorsMenu);
    Q_FOREACH(QColor color, backgroundColorOptions())
    {
        QAction *colorAction = colorsMenu->addAction(color.name());
        colorAction->setProperty(QByteArrayLiteral("#colorCode"), color.name());
        colorAction->setCheckable(true);
        colorAction->setChecked(colorsActionGroup->actions().isEmpty());
        colorsActionGroup->addAction(colorAction);

        QPixmap pixmap(64, 64);
        pixmap.fill(color);
        colorAction->setIcon(QIcon(pixmap));
    }
    colorsActionGroup->setExclusive(true);
    connect(colorsActionGroup, &QActionGroup::triggered, [=](QAction *action) {
        this->setBackgroundColor( QColor(action->property(QByteArrayLiteral("#colorCode")).toString()) );
    });

    QAction *movableAction = new QAction(QStringLiteral("Movable"), this);
    this->addAction(movableAction);
    movableAction->setCheckable(true);
    movableAction->setChecked(m_movable);
    connect(movableAction, &QAction::toggled, [=]() {
        this->setMovable(movableAction->isChecked());
    });

    QAction *staysOnTopAction = new QAction(QStringLiteral("Stays On Top"), this);
    this->addAction(staysOnTopAction);
    staysOnTopAction->setCheckable(true);
    staysOnTopAction->setChecked(this->isStayOnTop());
    connect(staysOnTopAction, &QAction::toggled, [=]() {
        this->setStayOnTop(staysOnTopAction->isChecked());
    });

    QAction *clearMarksAction = new QAction(QStringLiteral("Clear Marks"), this);
    this->addAction(clearMarksAction);
    connect(clearMarksAction, &QAction::triggered, this, &ScreenRuler::clearMarks);

    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    this->addAction(separator);

    QAction *quitAction = new QAction(QStringLiteral("Quit"), this);
    this->addAction(quitAction);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    this->setContextMenuPolicy(Qt::ActionsContextMenu);
}

ScreenRuler::~ScreenRuler()
{
}

void ScreenRuler::setUnit(ScreenRuler::Unit val)
{
    if(m_unit == val)
        return;

    m_unit = val;
    emit unitChanged();

    this->update();
}

void ScreenRuler::setLength(ScreenRuler::Length val)
{
    if(m_length == val)
        return;

    m_length = val;
    emit lengthChanged();

    this->placeSelf();
}

void ScreenRuler::setOrientation(Qt::Orientation val)
{
    if(m_orientation == val)
        return;

    m_orientation = val;
    emit orientationChanged();

    this->placeSelf();
}

void ScreenRuler::setMovable(bool val)
{
    if(m_movable == val)
        return;

    m_movable = val;
    emit movableChanged();
}

void ScreenRuler::setStayOnTop(bool val)
{
    if(val == this->isStayOnTop())
        return;

    this->setWindowFlag(Qt::WindowStaysOnTopHint, val);
    emit stayOnTopChanged();
}

void ScreenRuler::setBackgroundColor(const QColor &val)
{
    if(m_backgroundColor == val)
        return;

    m_backgroundColor = val;
    emit backgroundColorChanged();

    this->update();
}

void ScreenRuler::setForegroundColor(const QColor &val)
{
    if(m_foregroundColor == val)
        return;

    m_foregroundColor = val;
    emit foregroundColorChanged();

    this->update();
}

void ScreenRuler::setMajorTickColor(const QColor &val)
{
    if(m_majorTickColor == val)
        return;

    m_majorTickColor = val;
    emit majorTickColorChanged();

    this->update();
}

void ScreenRuler::setMinorTickColor(const QColor &val)
{
    if(m_minorTickColor == val)
        return;

    m_minorTickColor = val;
    emit minorTickColorChanged();

    this->update();
}

void ScreenRuler::setTransparency(ScreenRuler::Transparency val)
{
    if(m_transparency == val)
        return;

    m_transparency = val;
    switch(val)
    {
    case Opaque:
        this->setWindowOpacity(1.0);
        break;
    case SemiTransparent:
        this->setWindowOpacity(0.6);
        break;
    case SuperTransparent:
        this->setWindowOpacity(0.2);
        break;
    }

    emit transparencyChanged();
}

void ScreenRuler::mark()
{
    if(m_marks.length() >= 20)
        return;

    const QPointF pos = this->mapFromGlobal( QCursor::pos() );
    const qreal mark = (m_orientation == Qt::Horizontal) ? pos.x() : pos.y();
    m_marks << mark;
    this->update();
}

void ScreenRuler::clearMarks()
{
    m_marks.clear();
    this->update();
}

void ScreenRuler::setMoving(bool val)
{
    if(m_moving == val)
        return;

    m_moving = val;
    emit movingChanged();
}

void ScreenRuler::placeSelf()
{
    const qreal smallLengthFactor = 0.33;
    const qreal mediumLengthFactor = 0.66;
    const qreal bigLengthFactor = 1.0;
    const qreal lengthFactor = m_length == Big ? bigLengthFactor : (m_length == Medium ? mediumLengthFactor : smallLengthFactor);
    const qreal breadthFactor = 0.125;

    const QScreen *screen = this->windowHandle()->screen();
    const QSizeF screenSize = screen->size();
    const qreal breadth = qMin(screenSize.width(), screenSize.height()) * breadthFactor;
    const QSize size = m_orientation == Qt::Horizontal ?
                QSizeF( screenSize.width()*lengthFactor, breadth ).toSize() :
                QSizeF( breadth, screenSize.height()*lengthFactor ).toSize();

    QRect geometry( QPoint(0,0), size );
    if(m_initialized)
        geometry.moveCenter( this->geometry().center() );
    else
        geometry.moveCenter(screen->virtualGeometry().center());
    this->setGeometry(geometry);

    this->update();
}

void ScreenRuler::handleClick(const QPointF &localPos, const QPointF &screenPos)
{
    Q_UNUSED(localPos)
    Q_UNUSED(screenPos)
}

void ScreenRuler::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    const QFont defaultFont = paint.font();

    const QRectF rect = this->rect();
    const Scale scale(this->windowHandle()->screen(), m_orientation, m_unit);

    paint.setBrush(Qt::NoBrush);
    paint.setPen(Qt::NoPen);
    paint.setFont(this->font());
    paint.fillRect(rect, m_backgroundColor);

    const qreal end = m_orientation == Qt::Horizontal ? rect.width() : rect.height();
    const qreal minorTickSize = m_orientation == Qt::Horizontal ? rect.height() * 0.075 : rect.width() * 0.075;
    const qreal majorTickSize = 1.5 * minorTickSize;
    const qreal labelMargin = 0;
    qreal t = 0;
    int counter = 0;

    paint.setOpacity( m_marks.isEmpty() ? 1.0 : 0.5 );

    while(t < end)
    {
        const bool isMajorTick = counter > 0 && (counter%scale.minorTicksPerMajorTick()) == 0;
        const qreal tickSize = isMajorTick ? majorTickSize : minorTickSize;
        const int value = isMajorTick ? int(counter * scale.minorTick()) : -1;
        const QString label = isMajorTick ? QString::number(value) : QString();
        QRectF labelRect = isMajorTick ? paint.fontMetrics().boundingRect(label) : QRectF();
        QList<QPointF> labelPos;

        paint.setPen(isMajorTick ? m_majorTickColor : m_minorTickColor);

        if(m_orientation == Qt::Horizontal)
        {
            paint.drawLine(t, rect.top(), t, rect.top()+tickSize);
            paint.drawLine(t, rect.bottom(), t, rect.bottom()-tickSize);

            if(isMajorTick)
            {
                labelPos << QPointF(t, rect.top()+tickSize+labelRect.height()+labelMargin);
                labelPos << QPointF(t, rect.bottom()-tickSize-labelRect.height()-labelMargin);
            }
        }
        else
        {
            paint.drawLine(rect.left(), t, rect.left()+tickSize, t);
            paint.drawLine(rect.right(), t, rect.right()-tickSize, t);

            if(isMajorTick)
            {
                labelPos << QPointF(rect.left()+tickSize+labelRect.width()+labelMargin, t);
                labelPos << QPointF(rect.right()-tickSize-labelRect.width()-labelMargin, t);
            }
        }

        if(isMajorTick)
        {
            labelRect.moveCenter( labelPos.at(0) );
            paint.drawText(labelRect, Qt::AlignCenter, label);
            labelRect.moveCenter( labelPos.at(1) );
            paint.drawText(labelRect, Qt::AlignCenter, label);
        }

        t += scale.minorTickPixels();
        ++counter;
    }

    paint.setOpacity(1);

    const qreal penWidth = scale.screen()->devicePixelRatio() > 1 ? 0.5 : 1;
    paint.setPen(QPen(m_foregroundColor, penWidth));

    if(!m_marks.isEmpty())
    {
        QFont font = defaultFont;
        font.setPointSize(font.pointSize()-2);
        paint.setFont(font);

        Q_FOREACH(qreal mark, m_marks)
        {
            const qreal val = scale.fromPixels(mark);
            const QString text = m_unit == Pixel ? QString::number(int(val)) : QString::number(val, 'f', 2);
            QRectF textRect = paint.fontMetrics().boundingRect(text).adjusted(-2, -2, 2, 2);
            if(m_orientation == Qt::Horizontal)
            {
                paint.drawLine(mark, rect.top(), mark, rect.bottom());
                textRect.moveCenter( QPoint(mark, rect.center().y()) );
            }
            else
            {
                paint.drawLine(rect.left(), mark, rect.right(), mark);
                textRect.moveCenter( QPoint(rect.center().x(), mark) );
            }
            paint.fillRect(textRect, m_backgroundColor);
            paint.drawText(textRect, Qt::AlignCenter, text);
        }
    }

    if(!m_hoverMousePos.isNull())
    {
        QFont font = defaultFont;
        font.setPointSize(font.pointSize()+2);
        font.setBold(true);
        paint.setFont(font);

        const qreal val = scale.fromPixels( m_orientation == Qt::Horizontal ? m_hoverMousePos.x() : m_hoverMousePos.y() );
        const QString text = m_unit == Pixel ? QString::number(int(val)) : QString::number(val, 'f', 2);
        QRectF textRect = paint.fontMetrics().boundingRect(text).adjusted(-5, -5, 5, 5);
        textRect.moveCenter(m_hoverMousePos);

        paint.setPen(QPen(m_foregroundColor, 1, Qt::DashDotDotLine));
        if(m_orientation == Qt::Horizontal)
        {
            paint.drawLine(m_hoverMousePos.x(), rect.top(), m_hoverMousePos.x(), rect.bottom());
            textRect.moveRight(m_hoverMousePos.x() - 10);
            if(textRect.left() < 0)
                textRect.moveLeft(m_hoverMousePos.x() + 20);
        }
        else
        {
            paint.drawLine(rect.left(), m_hoverMousePos.y(), rect.right(), m_hoverMousePos.y());
            textRect.moveBottom(m_hoverMousePos.y() - 10);
            if(textRect.top() < 0)
                textRect.moveTop(m_hoverMousePos.y() + 20);
        }

        paint.setOpacity(0.5);
        paint.fillRect(textRect, Qt::white);
        paint.setOpacity(1);

        paint.drawText(textRect, Qt::AlignCenter, text);
    }

    paint.setBrush(Qt::NoBrush);
    paint.setPen(QPen(m_foregroundColor));
    paint.drawRect(this->rect());
}

void ScreenRuler::keyPressEvent(QKeyEvent *event)
{
    if(event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z)
    {
        this->mark();
        return;
    }

    const bool control = event->modifiers() & Qt::ControlModifier;
    QPoint dp;

    switch(event->key())
    {
    case Qt::Key_Up:
        dp = QPoint(0, -1);
        break;
    case Qt::Key_Down:
        dp = QPoint(0, 1);
        break;
    case Qt::Key_Left:
        dp = QPoint(-1, 0);
        break;
    case Qt::Key_Right:
        dp = QPoint(1, 0);
        break;
    }

    if(control)
        dp *= 10;

    this->move( this->pos() + dp );
}

void ScreenRuler::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event)
}

void ScreenRuler::mouseMoveEvent(QMouseEvent *event)
{
    if(m_mousePressed)
    {
        m_hoverMousePos = QPointF();

        const QPointF dp = event->screenPos() - m_lastMousePos;
        if(m_moving && m_movable)
        {
            QRectF geometry = this->geometry();
            geometry.moveTopLeft( geometry.topLeft() + dp );
            this->setGeometry(geometry.toRect());
            m_lastMousePos = event->screenPos();
        }
        else if (dp.manhattanLength() >= QApplication::startDragDistance())
        {
            if(m_movable)
                this->setMoving(true);
        }
    }
    else
    {
        m_hoverMousePos = event->localPos();
        this->update();
    }
}

void ScreenRuler::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_mousePressed = true;
        m_lastMousePos = event->screenPos();
    }
}

void ScreenRuler::mouseReleaseEvent(QMouseEvent *event)
{
    if(m_moving)
        this->setMoving(false);
    else
        this->handleClick(event->localPos(), event->screenPos());
    m_mousePressed = false;
}

