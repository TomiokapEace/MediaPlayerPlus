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

#pragma once

#include <QMPlay2Extensions.hpp>

#include <NetworkAccess.hpp>

#include <QTextEdit>
#include <QPointer>

class Tekstowo : public QTextEdit, public QMPlay2Extensions
{
	Q_OBJECT

public:
	Tekstowo(Module &module);
	~Tekstowo();

	DockWidget *getDockWidget() override final;

private slots:
	void visibilityChanged(bool v);

	void updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName);

	void finished(NetworkReply *reply);

private:
	void search();

	DockWidget *m_dW;

	bool m_visible, m_pending;

	NetworkAccess m_net;

	QString m_title, m_artist, m_name;

	QPointer<NetworkReply> m_searchReply;
	QPointer<NetworkReply> m_lyricsReply;
};

#define TekstowoName "Tekstowo"