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

#include "simthread.h"
#include <QFile>
#include <QDebug>
#define NAK 0x02
SimThread::SimThread(QObject *parent) : QThread(parent)
{
}
void SimThread::startSim(QString filename)
{
	m_filename = filename;
	start();
}

void SimThread::run()
{
	init = true;
	currentLocationPacket=0;
	currentLocationInfoPacket=0;
	QFile logfile(m_filename);
	if (!logfile.open(QIODevice::ReadOnly))
	{
		qDebug() << "Unable to open file. Exiting sim thread";
		return;
	}
	QByteArray logbytes = logfile.readAll();
	nextPacketIsLocation = false;

	QByteArray qbuffer;
	QList<QByteArray> m_queuedMessages;
	QString byteoutofpacket;
	bool m_inpacket = false;
	bool m_inescape = false;
	for (int i=0;i<logbytes.size();i++)
	{
		if ((unsigned char)logbytes[i] == 0xAA)
		{
			if (m_inpacket)
			{
				//Start byte in the middle of a packet
				//Clear out the buffer and start fresh
				m_inescape = false;
				qbuffer.clear();
			}
			//qbuffer.append(buffer[i]);
			//qDebug() << "Start of packet";
			//Start of packet
			m_inpacket = true;
		}
		else if ((unsigned char)logbytes[i] == 0xCC && m_inpacket)
		{
			//qDebug() << "End of packet. Size:" << qbuffer.size();
			//End of packet
			m_inpacket = false;
			//qbuffer.append(buffer[i]);

			//m_logFile->flush();
			//emit parseBuffer(qbuffer);


			//New Location of checksum
			unsigned char sum = 0;
			for (int i=0;i<qbuffer.size()-1;i++)
			{
				sum += qbuffer[i];
			}
			//qDebug() << "Payload sum:" << QString::number(sum);
			//qDebug() << "Checksum sum:" << QString::number((unsigned char)currPacket[currPacket.length()-1]);
			if (sum != (unsigned char)qbuffer[qbuffer.size()-1])
			{
				qDebug() << "BAD CHECKSUM!";
				//return QPair<QByteArray,QByteArray>();
			}
			else
			{
				m_queuedMessages.append(qbuffer.mid(0,qbuffer.length()-1));
			}
			//return qbuffer;
			QString output;
			for (int i=0;i<qbuffer.size();i++)
			{
				int num = (unsigned char)qbuffer[i];
				output.append(" ").append((num < 0xF) ? "0" : "").append(QString::number(num,16));
			}
			//qDebug() << "Full packet:";
			//qDebug() << output;
			qbuffer.clear();
		}
		else
		{
			if (m_inpacket && !m_inescape)
			{
				if ((unsigned char)logbytes[i] == 0xBB)
				{
					//Need to escape the next byte
					//retval = logfile.read(1);
					m_inescape = true;
				}
				else
				{
					qbuffer.append(logbytes[i]);
				}

			}
			else if (m_inpacket && m_inescape)
			{
				if ((unsigned char)logbytes[i] == 0x55)
				{
					qbuffer.append((char)0xAA);
				}
				else if ((unsigned char)logbytes[i] == 0x44)
				{
					qbuffer.append((char)0xBB);
				}
				else if ((unsigned char)logbytes[i] == 0x33)
				{
					qbuffer.append((char)0xCC);
				}
				else
				{
					qDebug() << "Error, escaped character is not valid!:" << QString::number(logbytes[i],16);
				}
				m_inescape = false;
			}
			else
			{
				//qDebug() << "Byte out of a packet:" << QString::number(buffer[i],16);
				byteoutofpacket += QString::number(logbytes[i],16) + " ";
			}
		}
	}
	//qDebug() << "Bytes out of a packet:" << byteoutofpacket;


	//Done.
	qDebug() << m_queuedMessages.size() << "Messages read in";
	for (int i=0;i<m_queuedMessages.size();i++)
	{
		Packet parsedPacket = parseBuffer(m_queuedMessages[i]);
		packetList.append(parsedPacket);
		//parsePacket(parsedPacket);
	}
	int fd=0;
#ifdef HAVE_POSIX_OPENPT
		fd = posix_openpt(O_RDWR | O_NOCTTY);
#else
		fd = open("/dev/ptmx",O_RDWR | O_NOCTTY);
#endif //HAVE_POSIX_OPENPT

		if(-1 == fd) {
#ifdef HAVE_POSIX_OPENPT
			perror("Error in posix_openpt");
#else
			perror("Error opening /dev/ptmx");
#endif //HAVE_POSIX_OPENPT
			return;
		}
		grantpt(fd);
		unlockpt(fd);

		struct termios oldtio;
		if(0 != tcgetattr(fd,&oldtio)) {
			perror("tcgetattr openpt warning");
		}
		//bzero(&newtio,sizeof(newtio));

		oldtio.c_cflag = CS8 | CLOCAL | CREAD; // CBAUD
		oldtio.c_iflag = IGNPAR | ICRNL;
		oldtio.c_oflag = 0;
		oldtio.c_lflag = ICANON & (~ECHO);

		oldtio.c_cc[VEOL]     = '\r';
		// oldtio.c_cc[VEOL2]    = 0;     /* '\0' */

		tcflush(fd,TCIFLUSH);
		if(0 != tcsetattr(fd,TCSANOW,&oldtio)) {
			perror("tcsetattr warning");
		}

#ifdef HAVE_PTSNAME_R
		if(0 != ptsname_r(fd, portname, sizeof(portname))) {
			perror("Couldn't get pty slave name");
		}
#else
		char *ptsname_val = ptsname(fd);
		if(NULL == ptsname_val) {
			perror("Couldn't get pty slave name");
		}
		QString portname = QString(ptsname_val);
		//strncpy(portname, ptsname_val, sizeof(portname));
#endif //HAVE_PTSNAME_R


	fcntl(fd,F_SETFL,O_NONBLOCK); // O_NONBLOCK + fdopen/stdio == bad

	qDebug() << "Port:" << portname << "opened for sim!";
	unsigned char buffer[1024];
	unsigned short nextlocationid;
	bool nextislocation = false;
	init = false;
	while (true)
	{
		int readlen = read(fd,buffer,1024);
		if (readlen < 0)
		{
			//Nothing on the port
			usleep(10000);
		}
		if (readlen == 0)
		{
			usleep(10000);
		}
		for (int i=0;i<readlen;i++)
		{
			if (buffer[i] == 0xAA)
			{
				if (m_inpacket)
				{
					//Start byte in the middle of a packet
					//Clear out the buffer and start fresh
					m_inescape = false;
					qbuffer.clear();
				}
				//qbuffer.append(buffer[i]);
				//qDebug() << "Start of packet";
				//Start of packet
				m_inpacket = true;
			}
			else if (buffer[i] == 0xCC && m_inpacket)
			{
				//qDebug() << "End of packet. Size:" << qbuffer.size();
				//End of packet
				m_inpacket = false;
				//qbuffer.append(buffer[i]);

				//m_logFile->flush();
				//emit parseBuffer(qbuffer);


				//New Location of checksum
				unsigned char sum = 0;
				for (int i=0;i<qbuffer.size()-1;i++)
				{
					sum += qbuffer[i];
				}
				//qDebug() << "Payload sum:" << QString::number(sum);
				//qDebug() << "Checksum sum:" << QString::number((unsigned char)currPacket[currPacket.length()-1]);
				if (sum != (unsigned char)qbuffer[qbuffer.size()-1])
				{
					qDebug() << "BAD CHECKSUM!";
					//return QPair<QByteArray,QByteArray>();
				}
				else
				{
					QByteArray fullmsg = qbuffer.mid(0,qbuffer.length()-1);
					Packet p = parseBuffer(fullmsg);
					if (p.payloadid == 0x0104 || p.payloadid == 0x0106)
					{
						Packet p2 = locationidList[currentLocationPacket++];
						QByteArray tosend = generatePacket(p2.header,p2.payload);
						write(fd,tosend.data(),tosend.length());
					}
					else if (p.payloadid == 0xF8E0)
					{
						Packet p2 = locationidInfoList[currentLocationInfoPacket++];
						QByteArray tosend = generatePacket(p2.header,p2.payload);
						write(fd,tosend.data(),tosend.length());

					}
					/*if (p.payloadid == 0x0106 || p.payloadid == 0x0108)
					{
						//Request for ram.
						if (nextislocation)
						{
							nextislocation = false;
							unsigned short lookforpayload = p.payloadid+1;
							for (int i=0;i<packetList.size();i++)
							{
								if (packetList[i].payloadid == lookforpayload)
								{
									unsigned short tmp = (((unsigned char)p.payload[0]) << 8) + (unsigned char)p.payload[1];
									//if (tmp == nextlocationid)
									//{
										QByteArray tosend = generatePacket(packetList[i].header,packetList[i].payload);
										write(fd,tosend.data(),tosend.length());
										break;
									//}
								}
							}
						}
					}*/
					else
					{
						unsigned short lookforpayload = p.payloadid+1;
						for (int i=0;i<packetList.size();i++)
						{
							if (packetList[i].payloadid == lookforpayload)
							{
								QByteArray tosend = generatePacket(packetList[i].header,packetList[i].payload);
								write(fd,tosend.data(),tosend.length());
								break;
							}
						}
					}

				}
				//return qbuffer;
				QString output;
				for (int i=0;i<qbuffer.size();i++)
				{
					int num = (unsigned char)qbuffer[i];
					output.append(" ").append((num < 0xF) ? "0" : "").append(QString::number(num,16));
				}
				//qDebug() << "Full packet:";
				//qDebug() << output;
				qbuffer.clear();
			}
			else
			{
				if (m_inpacket && !m_inescape)
				{
					if (buffer[i] == 0xBB)
					{
						//Need to escape the next byte
						//retval = logfile.read(1);
						m_inescape = true;
					}
					else
					{
						qbuffer.append(buffer[i]);
					}

				}
				else if (m_inpacket && m_inescape)
				{
					if (buffer[i] == 0x55)
					{
						qbuffer.append((char)0xAA);
					}
					else if (buffer[i] == 0x44)
					{
						qbuffer.append((char)0xBB);
					}
					else if (buffer[i] == 0x33)
					{
						qbuffer.append((char)0xCC);
					}
					else
					{
						qDebug() << "Error, escaped character is not valid!:" << QString::number(buffer[i],16);
					}
					m_inescape = false;
				}
				else
				{
					//qDebug() << "Byte out of a packet:" << QString::number(buffer[i],16);
					byteoutofpacket += QString::number(buffer[i],16) + " ";
				}
			}
		}
		//qDebug() << "Bytes out of a packet:" << byteoutofpacket;
	}
}

SimThread::Packet SimThread::parseBuffer(QByteArray buffer)
{
	if (buffer.size() <= 2)
	{

		qDebug() << "Not long enough to even contain a header!";
		//emit decoderFailure(buffer);
		return Packet(false);
	}


	//qDebug() << "Packet:" << QString::number(buffer[1],16) << QString::number(buffer[buffer.length()-2],16);
	Packet retval;
	retval.origionalPacket = buffer;
	QByteArray header;
	//currPacket.clear();
	//Parse the packet here
	int headersize = 3;
	int iloc = 0;
	bool seq = false;
	bool len = false;
	if (buffer[iloc] & 0x100)
	{
		//Has header
		seq = true;
		//qDebug() << "Has seq";
		headersize += 1;
	}
	if (buffer[iloc] & 0x1)
	{
		//Has length
		len = true;
		//qDebug() << "Has length";
		headersize += 2;
	}
	header = buffer.mid(0,headersize);
	iloc++;
	unsigned int payloadid = (unsigned int)buffer[iloc] << 8;

	payloadid += (unsigned char)buffer[iloc+1];
	retval.payloadid = payloadid;
	iloc += 2;
	if (seq)
	{
		//qDebug() << "Sequence number" << QString::number(currPacket[iloc]);
		iloc += 1;
		retval.hasseq = true;
	}
	else
	{
		retval.hasseq = false;
	}
	QByteArray payload;
	if (len)
	{
		retval.haslength = true;
		//qDebug() << "Length found, buffer size:" << buffer.length() << "iloc:" << QString::number(iloc);
		unsigned int length = buffer[iloc] << 8;
		length += (unsigned char)buffer[iloc+1];
		retval.length = length;
		//qDebug() << "Length:" << length;
		iloc += 2;
		//curr += length;
		if ((unsigned int)buffer.length() > (unsigned int)(length + iloc))
		{
			qDebug() << "Packet length should be:" << length + iloc << "But it is" << buffer.length();
			//emit decoderFailure(buffer);
			return Packet(false);
		}
		payload.append(buffer.mid(iloc,length));
	}
	else
	{
		retval.haslength = false;
		//qDebug() << "Buffer length:" << buffer.length();
		//qDebug() << "Attempted cut:" << buffer.length() - iloc;
		payload.append(buffer.mid(iloc),(buffer.length()-iloc));
	}
	//qDebug() << "Payload";
	QString output;
	for (int i=0;i<payload.size();i++)
	{
		int num = (unsigned char)payload[i];
		output.append(" ").append((num < 0xF) ? "0" : "").append(QString::number(num,16));
	}
	//qDebug() << output;
	output.clear();
	//qDebug() << "Header";
	for (int i=0;i<header.size();i++)
	{
		int num = (unsigned char)header[i];
		output.append(" ").append((num < 0xF) ? "0" : "").append(QString::number(num,16));
	}
	//qDebug() << output;
	//Last byte of currPacket should be out checksum.
	retval.header = header;
	retval.payload = payload;
	if (header[0] & NAK)
	{
		retval.isNAK = true;
	}
	else
	{
		retval.isNAK = false;
	}
	if (init)
	{
		if (retval.payloadid == 0x0105 || retval.payloadid == 0x0107)
		{
			locationidList.append(retval);
		}
		if (retval.payloadid == 0xF8E1)
		{
			locationidInfoList.append(retval);
		}
		else
		{
			//qDebug() << "Bad packet?";
		}
	}
	if (retval.header.size() >= 3)
	{
		return retval;
	}

	return Packet(false);
}
QByteArray SimThread::generatePacket(QByteArray header,QByteArray payload)
{
	QByteArray packet;
	packet.append((char)0xAA);
	unsigned char checksum = 0;
	for (int i=0;i<header.size();i++)
	{
		checksum += header[i];
	}
	for (int i=0;i<payload.size();i++)
	{
		checksum += payload[i];
	}
	payload.append(checksum);
	for (int j=0;j<header.size();j++)
	{
		if (header[j] == (char)0xAA)
		{
			packet.append((char)0xBB);
			packet.append((char)0x55);
		}
		else if (header[j] == (char)0xBB)
		{
			packet.append((char)0xBB);
			packet.append((char)0x44);
		}
		else if (header[j] == (char)0xCC)
		{
			packet.append((char)0xBB);
			packet.append((char)0x33);
		}
		else
		{
			packet.append(header[j]);
		}
	}
	for (int j=0;j<payload.size();j++)
	{
		if (payload[j] == (char)0xAA)
		{
			packet.append((char)0xBB);
			packet.append((char)0x55);
		}
		else if (payload[j] == (char)0xBB)
		{
			packet.append((char)0xBB);
			packet.append((char)0x44);
		}
		else if (payload[j] == (char)0xCC)
		{
			packet.append((char)0xBB);
			packet.append((char)0x33);
		}
		else
		{
			packet.append(payload[j]);
		}
	}
	//packet.append(header);
	//packet.append(payload);
	packet.append((char)0xCC);
	return packet;
}
