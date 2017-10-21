/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <PanGestureEventFilter.hpp>

#include <QAbstractNativeEventFilter>
#include <QAbstractItemView>
#include <QApplication>
#include <QTouchDevice>
#include <QScrollBar>
#include <QLibrary>
#include <QPointer>
#include <QEvent>
#include <QTimer>

#ifdef WINVER
	#undef WINVER
#endif
#define WINVER 0x0601
#include <windows.h>
#ifndef WM_GESTURE
	#define WM_GESTURE 0x0119
#endif
#ifndef WM_GESTURENOTIFY
	#define WM_GESTURENOTIFY 0x011A
#endif

namespace User32 {
	using UnregisterTouchWindowFunc = WINBOOL WINAPI (*)(HWND hwnd);
	static UnregisterTouchWindowFunc UnregisterTouchWindow;

	using SetGestureConfigFunc = WINBOOL WINAPI (*)(HWND hwnd, DWORD dwReserved, UINT cIDs, PGESTURECONFIG pGestureConfig, UINT cbSize);
	static SetGestureConfigFunc SetGestureConfig;

	using GetGestureInfoFunc = WINBOOL WINAPI (*)(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
	static GetGestureInfoFunc GetGestureInfo;

	using CloseGestureInfoHandleFunc = WINBOOL WINAPI (*)(HGESTUREINFO hGestureInfo);
	static CloseGestureInfoHandleFunc CloseGestureInfoHandle;
}

class PanGestureEventFilterPriv final : public QObject, public QAbstractNativeEventFilter
{
	bool eventFilter(QObject *watched, QEvent *event)
	{
		if (event->type() == QEvent::Show)
		{
			QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(watched);
			if (scrollArea && !m_installedGestures.contains(scrollArea))
			{
				QTimer *timer = new QTimer(scrollArea);
				timer->setSingleShot(true);
				timer->setInterval(10);

				connect(timer, &QTimer::timeout, scrollArea, [=] {
					if (HWND winId = (HWND)scrollArea->viewport()->winId())
					{
						GESTURECONFIG gc = {
							.dwID    = GID_PAN,
							.dwWant  = 0,
							.dwBlock = 0,
						};

						if (scrollArea->horizontalScrollBar()->maximum() > 0)
							gc.dwWant |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
						else
							gc.dwBlock |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;

						if (scrollArea->verticalScrollBar()->maximum() > 0)
							gc.dwWant |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
						else
							gc.dwBlock |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;

						User32::UnregisterTouchWindow(winId);
						if (User32::SetGestureConfig(winId, 0, 1, &gc, sizeof gc))
						{
							if (QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(scrollArea))
							{
								itemView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
								itemView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
							}
						}
					}
				});

				connect(scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, scrollArea, [=] {
					timer->start();
				});
				connect(scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, scrollArea, [=] {
					timer->start();
				});
				timer->start();

				m_installedGestures.insert(scrollArea);
				connect(scrollArea, &QAbstractScrollArea::destroyed, this, [=] {
					m_installedGestures.remove(scrollArea);
				});
			}
		}
		return QObject::eventFilter(watched, event);
	}

	bool nativeEventFilter(const QByteArray &eventType, void *message, long *result)
	{
		if (eventType == "windows_generic_MSG")
		{
			const MSG *winMsg = (const MSG *)message;
			if (winMsg->message == WM_GESTURENOTIFY)
			{
				return true;
			}
			else if (winMsg->message == WM_GESTURE)
			{
				GESTUREINFO gi;
				memset(&gi, 0, sizeof gi);
				gi.cbSize = sizeof gi;
				if (User32::GetGestureInfo((HGESTUREINFO)winMsg->lParam, &gi))
				{
					const QPoint pos(gi.ptsLocation.x, gi.ptsLocation.y);
					if (gi.dwID == GID_BEGIN)
					{
						if (QWidget *widget = QWidget::find((WId)winMsg->hwnd))
						{
							m_scrollArea = qobject_cast<QAbstractScrollArea *>(widget->parentWidget());
							m_lastPos = pos;
						}
					}
					else if (gi.dwID == GID_END)
					{
						m_scrollArea.clear();
						m_lastPos = QPoint();
					}
					else if (m_scrollArea)
					{
						const QPoint diff = m_lastPos - pos;

						QScrollBar *verticalScrollBar = m_scrollArea->verticalScrollBar();
						QScrollBar *horizontalScrollBar = m_scrollArea->horizontalScrollBar();

						verticalScrollBar->setValue(verticalScrollBar->value() + diff.y());
						horizontalScrollBar->setValue(horizontalScrollBar->value() + diff.x());

						m_lastPos = pos;
					}
					User32::CloseGestureInfoHandle((HGESTUREINFO)winMsg->lParam);
				}
				return true;
			}
		}
		return false;
	}

	QSet<QAbstractScrollArea *> m_installedGestures;
	QPointer<QAbstractScrollArea> m_scrollArea;
	QPoint m_lastPos;
};

/**/

void PanGestureEventFilter::install()
{
	static bool installed;

	if (QTouchDevice::devices().isEmpty())
		return;

	if (installed)
		return;

	QLibrary user32("user32");
	if (user32.load())
	{
		User32::UnregisterTouchWindow = (User32::UnregisterTouchWindowFunc)user32.resolve("UnregisterTouchWindow");
		User32::SetGestureConfig = (User32::SetGestureConfigFunc)user32.resolve("SetGestureConfig");
		User32::GetGestureInfo = (User32::GetGestureInfoFunc)user32.resolve("GetGestureInfo");
		User32::CloseGestureInfoHandle = (User32::CloseGestureInfoHandleFunc)user32.resolve("CloseGestureInfoHandle");
		if (User32::UnregisterTouchWindow && User32::SetGestureConfig && User32::GetGestureInfo && User32::CloseGestureInfoHandle)
		{
			PanGestureEventFilterPriv *panGestureEventFilter = new PanGestureEventFilterPriv;
			qApp->installNativeEventFilter(panGestureEventFilter);
			qApp->installEventFilter(panGestureEventFilter);
			installed = true;
		}
	}
}
