/***************************************************************************
 *   Copyright (C) 2008 by Volker Lanz <vl@fidra.de>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "kpart/partitionmanagerpart.h"

#include <gui/createpartitiontabledialog.h>
#include <gui/partitionmanagerwidget.h>

#include "util/helpers.h"
#include <ops/createpartitiontableoperation.h>
#include <core/devicescanner.h>

#include <kpluginfactory.h>
#include "config.h"

K_PLUGIN_FACTORY(PartitionManagerPartFactory, registerPlugin<PartitionManagerPart>();)
K_EXPORT_PLUGIN(PartitionManagerPartFactory("partitionmanagerpart"))

PartitionManagerPart::PartitionManagerPart(QWidget*, QObject* parent, const QVariantList&) :
	KParts::Part(parent),
	m_PartitionManagerWidget(new PartitionManagerWidget),
	m_OperationStack(new OperationStack(this)),
	m_OperationRunner(new OperationRunner(this, operationStack())),
	m_DeviceScanner(new DeviceScanner(this, operationStack()))
{
	setComponentData(PartitionManagerPartFactory::componentData(), false);

	connect(&operationStack(), SIGNAL(operationsChanged()), SLOT(on_operationStack_operationsChanged()));
	connect(&deviceScanner(), SIGNAL(finished()), SLOT(on_deviceScanner_finished()));

	registerMetaTypes();
	Config::instance("partitionmanagerrc");
	loadBackend();

	QApplication::setOverrideCursor(Qt::WaitCursor);

	deviceScanner().start();

	pmWidget().init(&operationStack());
	setWidget(&pmWidget());
	//setXMLFile("partitionmanagerpart.rc");
}

void PartitionManagerPart::onNewPartition()
{
	pmWidget().onNewPartition();
}

void PartitionManagerPart::onResizePartition()
{
	pmWidget().onResizePartition();
}

void PartitionManagerPart::onPropertiesPartition()
{
	pmWidget().onPropertiesPartition();
}

void PartitionManagerPart::onDeletePartition()
{
	pmWidget().onDeletePartition();
}

void PartitionManagerPart::onClearAllOperations()
{
	operationStack().clearOperations();

	pmWidget().updatePartitions();

}

/*
void PartitionManagerPart::onCreateNewPartitionTable()
{
	CreatePartitionTableDialog dlg(0, *pmWidget().selectedDevice());
	if (dlg.exec() == KDialog::Accepted)
		operationStack().push(new CreatePartitionTableOperation(*pmWidget().selectedDevice(), dlg.type()));
}
*/

void PartitionManagerPart::on_operationStack_operationsChanged()
{
	pmWidget().updatePartitions();

//	enableActions();
}

void PartitionManagerPart::on_deviceScanner_finished()
{
	QApplication::restoreOverrideCursor();

	pmWidget().setSelectedDevice("/dev/sda");
	if (!pmWidget().selectedDevice() && !operationStack().previewDevices().isEmpty())
		pmWidget().setSelectedDevice(operationStack().previewDevices()[0]);
}



