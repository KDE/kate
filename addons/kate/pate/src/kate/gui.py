# This file is part of Pate, Kate' Python scripting plugin.
#
# Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) version 3.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

''' Useful widgets '''

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def loadIcon(iconName):
    # overwrite this with your own icon loading function
    # in your project. 
    return QPixmap(iconName)


def setBackgroundColor(widget, color):
    ''' Utility function to set the background color of a QWidget '''
    p = widget.palette()
    p.setColor(QPalette.Active, QPalette.Window, color)
    p.setColor(QPalette.Inactive, QPalette.Window, color)
    widget.setPalette(p)
    widget.setAutoFillBackground(True)


class FunctionIntervalTimer(QTimer):
    def __init__(self, parent, interval, func, *args):
        QTimer.__init__(self, parent)
        self.func = func
        self.args = args
        self.connect(self, SIGNAL('timeout()'), self.timeOut)
        self.start(interval)

    def timeOut(self):
        self.func(*self.args)

    def stop(self):
        QTimer.stop(self)
        self.deleteLater()


def slideInFromBottomRight(widget, step=5, interval=20, offsetRight=0, offsetBottom=0):
    parent = widget.parent()
    x = parent.width() - (widget.width() + offsetRight)
    # shpeed
    widget.move(x, parent.height())
    originalHeight = widget.height()
    widget.setFixedHeight(0)
    def slideInFromBottomLeftInner():
        if (widget.height() + step) > originalHeight:
            slideInTimer.stop()
            widget.setFixedHeight(originalHeight)
            widget.move(x, parent.height() - originalHeight - offsetBottom)
            try:
                widget.effectFinished('slideInFromBottomLeft')
            except AttributeError:
                pass
        else:
            widget.setFixedHeight(widget.height() + step)
            widget.move(x, parent.height() - widget.height() - offsetBottom)
            #timer(widget, interval, slideInFromBottomLeftInner)
    slideInTimer = FunctionIntervalTimer(widget, interval, slideInFromBottomLeftInner)


def slideOutFromBottomRight(widget, step=5, interval=20, offsetRight=0, offsetBottom=0):
    parent = widget.parent()
    x = parent.width() - (widget.width() + offsetRight)
    # shpeed
    originalHeight = widget.height()
    widget.move(x, parent.height() - originalHeight - offsetBottom)
    def slideOutFromBottomLeftInner():
        if (widget.height() - step) < 0:
            slideOutTimer.stop()
            widget.setFixedHeight(0)
            widget.move(x, parent.height() - offsetBottom)
            try:
                widget.effectFinished('slideOutFromBottomLeft')
            except AttributeError:
                pass
        else:
            widget.setFixedHeight(widget.height() - step)
            widget.move(x, parent.height() - widget.height() - offsetBottom)
    slideOutTimer = FunctionIntervalTimer(widget, interval, slideOutFromBottomLeftInner)


class VerticalProgressWidget(QFrame):
    Brush = QBrush(QColor(200, 210, 240))
    def __init__(self, parent):
        QFrame.__init__(self, parent)
        self.setFixedWidth(10)
        setBackgroundColor(self, Qt.white)
        palette = self.palette()
        palette.setColor(QPalette.Foreground, QColor(180, 180, 180))
        self.setPalette(palette)
        self.setFrameStyle(QFrame.Box | QFrame.Plain)
        self.percent = 100
        self.oldHeight = self.height()

    def paintEvent(self, e):
        painter = QPainter()
        painter.begin(self)
        self_height = self.height()
        self_width = self.width()
        height = int(self_height * (self.percent / 100.0))
#         painter.fillRect(0, 0, self_width, self_height, QColor(255, 255, 255
        painter.fillRect(0, self_height - height, self_width, height, self.Brush)
        self.oldHeight = height
        painter.end()
        QFrame.paintEvent(self, e)

    def increasedDrawnPercentage(self, p):
        return int(self.height() * p / 100.0) > self.oldPercent

    def decreaseDrawnPercentage(self, p):
        return int(self.height() * p / 100.0) < self.oldHeight


class PassivePopupLabel(QLabel):
    def __init__(self, parent, message, maxTextWidth=None, minTextWidth=None):
        QLabel.__init__(self, parent)
        # if '<' in message and '>' in message:
            # self.setTextFormat(Qt.RichText)
        self.setTextInteractionFlags(Qt.TextSelectableByMouse | Qt.LinksAccessibleByMouse)
        if maxTextWidth is not None:
            self.setMaximumWidth(maxTextWidth)
        if minTextWidth is not None:
            self.setMinimumWidth(minTextWidth)
        self.setWordWrap(True)
        self.setText(message)


class TimeoutPassivePopup(QFrame):
    popups = {}
    def __init__(self, parent, message, timeout=5, icon=None, maxTextWidth=200, minTextWidth=None):
        QFrame.__init__(self, parent)
        setBackgroundColor(self, QColor(255, 255, 255, 200))
        palette = self.palette()
        originalWindowText = palette.color(QPalette.WindowText)
        # why does the style use WindowText!?
        palette.setColor(QPalette.WindowText, QColor(80, 80, 80))
        self.setPalette(palette)
        self.setFrameStyle(QFrame.Plain | QFrame.Box)
        self.timeout = timeout
        self.progress = 0
        self.interval = None
        self.hasMouseOver = False
        self.timer = QTimer(self)
        self.connect(self.timer, SIGNAL('timeout()'), self.updateProgress)
        layout = QVBoxLayout(self)
        layout.setMargin(self.frameWidth() + 7)
        layout.setSpacing(0)
        grid = QGridLayout()
        grid.setMargin(4)
        grid.setSpacing(5)
        self.message = PassivePopupLabel(self, message, maxTextWidth, minTextWidth)
        self.message.palette().setColor(QPalette.WindowText, originalWindowText)
        self.icon = QLabel(self)
        self.icon.setAlignment(Qt.AlignHCenter | Qt.AlignTop)
        self.timerWidget = VerticalProgressWidget(self)
        if icon is not None:
            pixmap = loadIcon(icon)
            self.icon.setPixmap(pixmap)
            self.icon.setMargin(3)
        grid.addWidget(self.timerWidget, 0, 0)
        grid.addWidget(self.icon, 0, 1)
        grid.addWidget(self.message, 0, 2, Qt.AlignVCenter)
        layout.addLayout(grid, 1)
        # start further up if there is another popup
        self.stackList = TimeoutPassivePopup.popups.setdefault(parent, [])
        self.stackHeight = len(self.stackList)
        # resize according to the layout
        self.adjustSize()
        self.originalHeight = self.height()
        self.offsetBottom = 0
        self.move(0, 0)

    def updateProgress(self):
        if self.progress >= 100:
            self.timer.stop()
            self.hide()
        else:
            # if drawing at this percentage won't cause a visible difference to the widget, then don't draw
            if self.timerWidget.decreaseDrawnPercentage(100 - self.progress):
                self.timerWidget.percent = (100 - self.progress)
                #print self.timerWidget.percent
                self.timerWidget.repaint()
            self.progress += 1

    def enterEvent(self, e):
        self.hasMouseOver = True
        self.timer.stop()
    def leaveEvent(self, e):
        self.hasMouseOver = False
        if self.interval is not None:
            self.timer.start(self.interval)

    def effectFinished(self, name):
        if name == 'slideInFromBottomLeft':
            # kickstart timeout
            interval = int(self.timeout * 1000) / 100
            self.interval = interval
            if not self.hasMouseOver:
                self.timer.start(interval)
        elif name == 'slideOutFromBottomLeft':
            self.stackList.remove(self)
            self.deleteLater()

    def show(self):
        if self.stackList:
            maxOffsetBottomWidget = self.stackList[0]
            for x in self.stackList[1:]:
                if x.offsetBottom > maxOffsetBottomWidget.offsetBottom:
                    maxOffsetBottomWidget = x
            self.offsetBottom = maxOffsetBottomWidget.originalHeight + maxOffsetBottomWidget.offsetBottom + 2
        else:
            self.offsetBottom = 0
        self.stackList.append(self)
        QFrame.show(self)
        slideInFromBottomRight(self, offsetRight=21, offsetBottom=self.offsetBottom)

    def hide(self):
        slideOutFromBottomRight(self, offsetRight=21, offsetBottom=self.offsetBottom)


def popup(message, timeout, icon=None, maxTextWidth=None, minTextWidth=None, parent=None):
    if parent is None:
        import kate
        parent = kate.mainWindow()
    popup = TimeoutPassivePopup(parent, message, timeout, icon, maxTextWidth, minTextWidth)
    popup.show()
    return popup

# kate: space-indent on; indent-width 4;
