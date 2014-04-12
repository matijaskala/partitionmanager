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

#if !defined(PARTITIONMANAGERPART__H)

#define PARTITIONMANAGERPART__H

#include <KParts/ReadOnlyPart>

class OperationRunner;
class OperationStack;
class DeviceScanner;
class PartitionManagerWidget;
class PartitionManagerPart : public KParts::Part
{
	Q_OBJECT
	public:
		PartitionManagerPart(QWidget* parentWidget, QObject* parent, const QVariantList& args);

	public slots:
		void onNewPartition();
		void onResizePartition();
		void onPropertiesPartition();
		void onDeletePartition();
		void onClearAllOperations();
		//void onCreateNewPartitionTable();

	protected:

		PartitionManagerWidget& pmWidget() { Q_ASSERT(m_PartitionManagerWidget); return *m_PartitionManagerWidget; }
		const PartitionManagerWidget& pmWidget() const { Q_ASSERT(m_PartitionManagerWidget); return *m_PartitionManagerWidget; }

		OperationStack& operationStack() { Q_ASSERT(m_OperationStack); return *m_OperationStack; }
		const OperationStack& operationStack() const { Q_ASSERT(m_OperationStack); return *m_OperationStack; }

		OperationRunner& operationRunner() { Q_ASSERT(m_OperationRunner); return *m_OperationRunner; }
		const OperationRunner& operationRunner() const { Q_ASSERT(m_OperationRunner); return *m_OperationRunner; }

		DeviceScanner& deviceScanner() { Q_ASSERT(m_DeviceScanner); return *m_DeviceScanner; }
		const DeviceScanner& deviceScanner() const { Q_ASSERT(m_DeviceScanner); return *m_DeviceScanner; }

	private:
		PartitionManagerWidget* m_PartitionManagerWidget;
		OperationStack* m_OperationStack;
		OperationRunner* m_OperationRunner;
		DeviceScanner* m_DeviceScanner;

	private slots:
		void on_operationStack_operationsChanged();
		void on_deviceScanner_finished();
};

#endif
