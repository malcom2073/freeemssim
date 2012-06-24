/***************************************************************************
*   Copyright (C) 2012  Michael Carpenter (malcom2073)                     *
*                                                                          *
*   This file is a part of FreeEmsSim                                      *
*                                                                          *
*   FreeEmsSim is free software: you can redistribute it and/or modify     *
*   it under the terms of the GNU General Public License version 2 as      *
*   published by the Free Software Foundation.                             *
*                                                                          *
*   FreeEmsSim is distributed in the hope that it will be useful,          *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
*   GNU General Public License for more details.                           *
									   *
*   You should have received a copy of the GNU General Public License      *
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
****************************************************************************/

#ifndef SIMTHREAD_H
#define SIMTHREAD_H

#include <QThread>
#include <QMap>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

class SimThread : public QThread
{
	Q_OBJECT
	class Packet
	{
	public:
		Packet(bool valid = true) { isValid = valid; }
		bool isNAK;
		bool isValid;
		QByteArray header;
		QByteArray payload;
		unsigned short payloadid;
		unsigned short length;
		bool haslength;
		bool hasseq;
		unsigned short sequencenum;
		QByteArray origionalPacket;
		unsigned short locationid;

	};

public:
	explicit SimThread(QObject *parent = 0);
	void startSim(QString filename);
	QList<Packet> packetList;
	QList<Packet> locationidList;
	QList<Packet> locationidInfoList;
	QByteArray generatePacket(QByteArray header,QByteArray payload);
	bool init;
	int currentLocationPacket;
	bool nextPacketIsLocation;
	int currentLocationInfoPacket;
	unsigned short nextPacketLocationId;
	QMap<unsigned short,Packet> m_memoryMap;
	Packet parseBuffer(QByteArray buffer);
private:
	QString m_filename;
protected:
	void run();
signals:
	
public slots:
	
};

#endif // SIMTHREAD_H
