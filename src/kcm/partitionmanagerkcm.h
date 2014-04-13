/***************************************************************************
 *   Copyright (C) 2009 by Volker Lanz <vl@fidra.de>                       *
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

#if !defined(PARTITIONMANAGERKCM__H)

#define PARTITIONMANAGERKCM__H

#include "ui_partitionmanagerkcmbase.h"

#include "util/globallog.h"

#include <kcmodule.h>
#include <kdebug.h>

class PartitionManagerWidget;
class ListDevices;
class KActionCollection;
class Device;
class KToolBar;

class PartitionManagerKCM : public KCModule, public Ui::PartitionManagerKCMBase
{
	Q_OBJECT
	Q_DISABLE_COPY(PartitionManagerKCM)

	public:
		PartitionManagerKCM(QWidget* parent, const QVariantList& args);
		virtual ~PartitionManagerKCM() {}

	public:
		void load() {}
		void save() {}

	protected:
		void init();
		void setupObjectNames();
		void setupConnections();
		void setupKCMWorkaround();
		void enableButtons();

		PartitionManagerWidget& pmWidget() { Q_ASSERT(m_PartitionManagerWidget); return *m_PartitionManagerWidget; }
		ListDevices& listDevices() { Q_ASSERT(m_ListDevices); return *m_ListDevices; }
		ListOperations& listOperations() { Q_ASSERT(m_ListOperations); return *m_ListOperations; }
		QSplitter& splitterHorizontal() { Q_ASSERT(m_SplitterHorizontal); return *m_SplitterHorizontal; }
		QSplitter& splitterVertical() { Q_ASSERT(m_SplitterVertical); return *m_SplitterVertical; }
		KToolBar& toolBar() { Q_ASSERT(m_ToolBar); return *m_ToolBar; }

		KActionCollection* actionCollection() { return m_ActionCollection; }

		OperationStack& operationStack() { Q_ASSERT(m_OperationStack); return *m_OperationStack; }
		const OperationStack& operationStack() const { Q_ASSERT(m_OperationStack); return *m_OperationStack; }

		DeviceScanner& deviceScanner() { Q_ASSERT(m_DeviceScanner); return *m_DeviceScanner; }
		const DeviceScanner& deviceScanner() const { Q_ASSERT(m_DeviceScanner); return *m_DeviceScanner; }

	protected slots:
		void onNewLogMessage(Log::Level logLevel, const QString& s);
		void onApplyClicked();

		void on_m_OperationStack_devicesChanged();
		void on_m_OperationStack_operationsChanged();
		void on_m_PartitionManagerWidget_selectedPartitionChanged(const Partition* p);

	private:
		KActionCollection* m_ActionCollection;
		OperationStack* m_OperationStack;
		DeviceScanner* m_DeviceScanner;
};


#endif
