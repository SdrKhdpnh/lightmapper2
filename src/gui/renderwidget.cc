/****************************************************************************
 *      renderwidget.cc: a widget for displaying the rendering output
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

#include <QtGui/QApplication>
#include <QtGui/QPushButton>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include "renderwidget.h"
#include <iostream>

/*=====================================
/	RenderWidget implementation
/=====================================*/

RenderWidget::RenderWidget(QWidget *parent): QLabel(parent) {
	borderStart = QPoint(0, 0);
	rendering = true;
}

void RenderWidget::updatePixmap(QRect r)
{
	m_mutex.lock();
	/* if(r.isNull() || pixmap.isNull())
	{
		pixmap = QPixmap::fromImage(img);
	}
	else
	{
		QPainter painter(&pixmap);
		painter.setClipRegion(r);
		QRect r = img.rect();
		painter.drawImage(r.topLeft(), img);
	}
	m_cond.wakeAll();
	m_updateLock.unlock(); */
	/* if(r.isNull())
		this->update();
	else
	{
		QRect r2=pixmap.rect();
		r2.moveCenter(this->rect().center());
		r.translate(r2.topLeft());
		this->repaint(r);
	} */
	//m_cond.wakeAll();
	//std::cout << "flush image " << r.width() << " " << r.height() << std::endl;
	this->update();
	m_mutex.unlock();
}

void RenderWidget::paintEvent(QPaintEvent *e)
{
	if (rendering) {
		//std::cout << "repainting" << std::endl;
		//m_updateLock.lock();
		QPainter painter(this);
		painter.fillRect(rect(), Qt::black);
		//if (pixmap.isNull()) {
		if (img.isNull()) {
			painter.setPen(Qt::white);
			painter.drawText(rect(), Qt::AlignCenter, tr("<no image data>"));
			//m_cond.wakeAll();
			m_updateLock.unlock();
			return;
		}

		//QRect r = pixmap.rect();
		QRect r = img.rect();
		r.moveCenter(this->rect().center());

		painter.setClipRegion(e->region());
		painter.setPen(Qt::black);
		painter.setBrush(palette().window());
		painter.drawRect(r);
		//painter.drawPixmap(r.topLeft(), pixmap);
		//painter.drawImage(r.topLeft() + borderStart, img);
		painter.drawImage(r.topLeft(), img);
		//m_cond.wakeAll();
		//m_updateLock.unlock();
	}
	else {
		QLabel::paintEvent(e);
	}
}

bool RenderWidget::saveImage(const QString &path)
{
	for (int y = 0; y < img.height(); ++y) {
		for (int x = 0; x < img.width(); ++x) {
			QColor c = QColor(img.pixel(x, y));
			c.setAlpha(alphaChannel[x + y * img.width()]);
			img.setPixel(x, y, c.rgba());
		}
	}
	return img.save(path, 0);
}

void RenderWidget::finishedRender() {
	rendering = false;
	QPixmap px = QPixmap::fromImage(img);
	setPixmap(px);
}
