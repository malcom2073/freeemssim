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

#include "mainwindow.h"
#include <QFileDialog>
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	ui.setupUi(this);
	connect(ui.browsePushButton,SIGNAL(clicked()),this,SLOT(browseButtonClicked()));
	connect(ui.startPushButton,SIGNAL(clicked()),this,SLOT(startButtonClicked()));
}
void MainWindow::browseButtonClicked()
{
	QFileDialog tmp;
	if (tmp.exec())
	{
		if (tmp.selectedFiles().size() > 0)
		{
			ui.logFileLineEdit->setText(tmp.selectedFiles()[0]);
		}
	}
}

void MainWindow::startButtonClicked()
{
	SimThread *thread = new SimThread();
	thread->startSim(ui.logFileLineEdit->text());
}

MainWindow::~MainWindow()
{

}
