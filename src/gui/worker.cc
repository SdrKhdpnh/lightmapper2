/****************************************************************************
 *      worker.cc: a thread for running the rendering process
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

#include "worker.h"
#include "qtoutput.h"
//#include <core_api/scene.h>
#include <interface/yafrayinterface.h>
//#include <core_api/imagefilm.h>
#include <yafraycore/xmlparser.h>

Worker::Worker(yafaray::yafrayInterface_t *env, QtOutput *output)
: QThread(), m_env(env),  m_output(output)
{
}

void Worker::run()
{
	//m_output->clear();
	m_env->render(*m_output);
}