/****************************************************************************
 *      qtoutput.cc: a Qt color output for yafray
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

#include "qtoutput.h"
#include <iostream>
#include <cstdlib>

QtOutput::QtOutput(RenderWidget *render): widgy(render)
{
}

void QtOutput::setRenderSize(const QSize &s)
{
	widgy->img = QImage(s, QImage::Format_ARGB32);
	widgy->alphaChannel = (unsigned char*) calloc(s.width() * s.height() * sizeof(unsigned char), sizeof(unsigned char));
	widgy->img.fill(0);
	widgy->resize(s);
	QPalette palette;
	palette.setColor(QPalette::Background, QColor(20, 20, 20));
	widgy->setPalette(palette);
}

void QtOutput::clear()
{
	// clear image...
	widgy->m_updateLock.lock();
	widgy->img = QImage();
	widgy->m_updateLock.unlock();
	widgy->update();
}

/*====================================
/ colorOutput_t implementations
=====================================*/

bool QtOutput::putPixel(int x, int y, const float *c, int channels)
{
	int r=c[0] * 255, g=c[1] * 255,b= c[2] * 255, a;
	QColor color(r<=255?r:255, g<=255?g:255, b<=255?b:255);
	if (channels > 3)
	{
		a = c[3]*255;
		if (a > 255) a = 255;
		if (a < 0) a = 0;
		widgy->alphaChannel[x + widgy->borderStart.x() + (y + widgy->borderStart.y()) * widgy->img.width()] = a;
	}
	//widgy->m_updateLock.lock();
	m_mutex.lock();
	widgy->img.setPixel(x + widgy->borderStart.x(),y + widgy->borderStart.y(), color.rgb());

	m_mutex.unlock();
	//widgy->m_updateLock.unlock();
	return true;
}

void QtOutput::flush()
{
	widgy->m_updateLock.lock();
	//widgy->update();
	emit flushImage(QRect());
	//emit flushImage(QRect());
	//widgy->m_cond.wait(&(widgy->m_updateLock));
	widgy->m_updateLock.unlock();

}

void QtOutput::flushArea(int x0, int y0, int x1, int y1)
{
	widgy->m_updateLock.lock();
	QRect r(x0, y0, x1-x0, y1-y0);
	/* QRect r2 = widgy->img.rect();
	r2.moveCenter(widgy->rect().center());
	r.translate(r2.topLeft()); */
	//widgy->update(r);
	//std::cout << "emit flushImage " << r.width() << " " << r.height() << std::endl;
	emit flushImage(r);
	//emit flushImage(QRect(x0, y0, x1-x0, y1-y0));
	//widgy->m_cond.wait(&(widgy->m_updateLock));
	widgy->m_updateLock.unlock();
}
