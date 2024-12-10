/****************************************************************************
 *      mywindow.cc: the main window for the yafray GUI
 *      This is part of the yafray package
 *      Copyright (C) 2008 Gustavo Pichorim Boiko
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "yafqtapi.h"
#include "mywindow.h"
#include "worker.h"
#include "qtoutput.h"
#include "renderwidget.h"
#include "windowbase.h"
#include <interface/yafrayinterface.h>
#include <yafraycore/EXR_io.h>

#include <iostream>

#include <QtGui/QHBoxLayout>
#include <QtGui/QScrollArea>
#include <QtGui/QFileDialog>
#include <QtCore/QDir>
#include <QtGui/QImageWriter>
#include <QtGui/QErrorMessage>
#include <QtGui/QKeyEvent>
#include <QtGui/QDesktopWidget>
#include <string>


static QCoreApplication *app=0;

void initGui()
{
	static int argc=0;

	if(!QCoreApplication::instance())
	{
		std::cout << "creating new QApplication\n";
		app = new QApplication(argc, 0);
	}
	else app = QCoreApplication::instance();
}

int createRenderWidget(yafaray::yafrayInterface_t *interf, int xsize, int ysize, int bStartX, int bStartY, const char* fileName)
{
	MainWindow w(interf, xsize, ysize, bStartX, bStartY, std::string(fileName));
	w.show();
	w.slotRender();

	return app->exec();
}


MainWindow::MainWindow(yafaray::yafrayInterface_t *env, int resx, int resy, int bStartX, int bStartY, std::string fileName)
: QMainWindow(), interf(env), res_x(resx), res_y(resy)
{
	m_ui = new Ui::WindowBase();
	m_ui->setupUi(this);
	m_render = new RenderWidget(this);
	m_output = new QtOutput(m_render);
	m_worker = new Worker(env, m_output);
	errorMessage = new QErrorMessage(this);

	m_output->setRenderSize(QSize(resx, resy));

	// resize the window to be as large as the rendered image but at
	// most slighty smaller than the desktop resolution
	int inset = 20;

 	QRect screenGeom = QApplication::desktop()->availableGeometry();

	this->move(inset + screenGeom.x(), inset + screenGeom.y());

	this->setMaximumSize(
		screenGeom.width() - 2 * inset,
		screenGeom.height() - 2 * inset);

	this->resize( resx + 20, resy + 100);

	m_ui->renderArea->setWidgetResizable(false);
	m_ui->renderArea->setWidget(m_render);
	m_ui->renderArea->resize(resx, resy);
	m_ui->renderArea->setBackgroundRole(QPalette::Dark);
	m_render->setScaledContents(true);
	connect(m_ui->renderButton, SIGNAL(clicked()), this, SLOT(slotRender()));
	connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(slotCancel()));
	connect(m_ui->quitButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(m_worker, SIGNAL(finished()), this, SLOT(slotFinished()));
	connect(m_output, SIGNAL(flushImage(QRect)), m_render, SLOT(updatePixmap(QRect)));
	connect(app, SIGNAL(aboutToQuit()), this, SLOT(slotCancel()));

	//	connect(m_ui->alphaCheck, SIGNAL(stateChanged(int)), this, SLOT(slotUseAlpha(int)));

	// actions
	connect(m_ui->actionOpen, SIGNAL(triggered(bool)),
			this, SLOT(slotOpen()));
	connect(m_ui->actionSave, SIGNAL(triggered(bool)),
			this, SLOT(slotSave()));
	connect(m_ui->actionSave_As, SIGNAL(triggered(bool)),
			this, SLOT(slotSaveAs()));
	connect(m_ui->actionQuit, SIGNAL(triggered(bool)),
			this, SLOT(close()));
	connect(m_ui->actionZoom_In, SIGNAL(triggered(bool)),
			this, SLOT(zoomIn()));
	connect(m_ui->actionZoom_Out, SIGNAL(triggered(bool)),
			this, SLOT(zoomOut()));

	m_ui->alphaCheck->setHidden(true);

	// disable saving actions until we have something to save
	//m_ui->actionSave->setEnabled(false);
	//m_ui->actionSave_As->setEnabled(false);
	
	// offset when using border rendering
	m_render->borderStart = QPoint(bStartX, bStartY);

	if ((autoRender = fileName.length() > 0)) {
		this->fileName = fileName;
		this->setWindowTitle(this->windowTitle() + QString(" Output: ") + QString(fileName.c_str()));
	}

	scaleFactor = 1.0;
}

MainWindow::~MainWindow()
{
	delete m_output;
	delete m_render;
	delete m_worker;
	delete m_ui;
	delete errorMessage;
}

void MainWindow::slotRender()
{
	slotEnableDisable(false);
	m_worker->start();
}

void MainWindow::slotFinished()
{
	if (autoRender) {
		m_render->saveImage(QString(fileName.c_str()));
		app->exit(0);
	}
	else {
		std::cout << "finished, setting pixmap" << std::endl;
		m_render->finishedRender();
		slotEnableDisable(true);
	}
}

void MainWindow::slotEnableDisable(bool enable)
{
	m_ui->renderButton->setEnabled(enable);
	m_ui->cancelButton->setEnabled(!enable);
 	m_ui->actionZoom_In->setEnabled(enable);
 	m_ui->actionZoom_Out->setEnabled(enable);
	//m_ui->actionSave->setEnabled(enable);
	//m_ui->actionSave_As->setEnabled(enable);
}

void MainWindow::slotOpen()
{
	if (m_lastPath.isNull())
		m_lastPath = QDir::currentPath();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Yafaray Scene"), m_lastPath, tr("Yafaray Scenes (*.xml)"));

	if (m_worker->isRunning())
		m_worker->terminate();
	delete m_worker;
	m_worker = new Worker(interf, m_output);

	m_lastPath = QDir(fileName).absolutePath();
	slotEnableDisable(true);

	// disable the save and save as actions
	//m_ui->actionSave->setEnabled(false);
	//m_ui->actionSave_As->setEnabled(false);
}

void MainWindow::slotSave()
{
	if (m_outputPath.isNull())
	{
		slotSaveAs();
		return;
	}

	//	if (!m_output->saveImage(m_outputPath))
	//	{
	//TODO show an error message
	//	}
}

void MainWindow::slotSaveAs()
{
	QString formats;
	QList<QByteArray> formatList = QImageWriter::supportedImageFormats();
	foreach (QByteArray format, formatList)
		formats += QString(format).toUpper() + ";;";
	formats += "EXR";

	if (m_lastPath.isNull())
		m_lastPath = QDir::currentPath();

	QString selectedFilter;

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), m_lastPath,
			formats, &selectedFilter);

	if (!fileName.endsWith("." + selectedFilter, Qt::CaseInsensitive))
		fileName += "." + selectedFilter.toLower();

	if (!fileName.isNull())
	{
		m_lastPath = QDir(fileName).absolutePath();
		if(fileName.endsWith(".exr", Qt::CaseInsensitive))
		{
#if HAVE_EXR
			//std::cout << "saving EXR file\n" << qPrintable(fileName) << std::endl;
			std::string fname = std::string(m_lastPath.toLocal8Bit().constData());
			yafaray::outEXR_t exrout(res_x, res_y, fname.c_str(), "");
			interf->getRenderedImage(exrout);
#else
			errorMessage->showMessage(tr("This build has been compiled without OpenEXR."));
#endif
		}
		else if (m_render->saveImage(fileName))
		{
			m_outputPath = fileName;
		}
		// TODO: show error message on !saving
	}
}

void MainWindow::slotUseAlpha(int state)
{
	/* 	bool alpha = (state == Qt::Checked);

	if (alpha != m_output->useAlpha())
		m_output->setUseAlpha(alpha); */
}

void MainWindow::slotCancel() {
	// cancel the render and cleanup, especially wait for the worker to finish up
	// (otherwise the app will crash (if this is followed by a quit))
	interf->abort();
	m_worker->wait();
}

void MainWindow::close() {
	// this will call slotCancel as well, since it's slotted into aboutToQuit signal
	app->quit();
}


void MainWindow::keyPressEvent(QKeyEvent* event) {
	if (event->key() == Qt::Key_Escape) {
		app->exit(1);
	}
}

void MainWindow::zoomOut() {
	if (scaleFactor < 0.2) return;
	scaleFactor *= 0.8;
	m_render->resize(scaleFactor * m_render->pixmap()->size());
}

void MainWindow::zoomIn() {
	if (scaleFactor > 5) return;
	scaleFactor *= 1.25;
	m_render->resize(scaleFactor * m_render->pixmap()->size());
}
