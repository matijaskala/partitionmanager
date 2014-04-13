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

#include "kcm/partitionmanagerkcm.h"

#include "gui/partitionmanagerwidget.h"
#include "gui/listdevices.h"
#include "gui/applyprogressdialog.h"

#include "util/helpers.h"
#include <core/devicescanner.h>
#include <ops/newoperation.h>
#include <ops/resizeoperation.h>
#include <ops/deleteoperation.h>
#include <ops/copyoperation.h>

#include <config.h>

#include <kgenericfactory.h>
#include <klocale.h>
#include <kactioncollection.h>
#include <ktoolbar.h>
#include <kapplication.h>
#include <kcmultidialog.h>

#include <QTimer>
#include <QReadLocker>
#include <KMessageBox>

K_PLUGIN_FACTORY(
		PartitionManagerKCMFactory,
		registerPlugin<PartitionManagerKCM>();
)
K_EXPORT_PLUGIN(
		PartitionManagerKCMFactory("kcm_partitionmanager", "partitionmanager")
)

PartitionManagerKCM::PartitionManagerKCM(QWidget* parent, const QVariantList&) :
	KCModule(PartitionManagerKCMFactory::componentData(), parent),
	Ui::PartitionManagerKCMBase(),
	m_ActionCollection(new KActionCollection(this, PartitionManagerKCMFactory::componentData())),
	m_OperationStack(new OperationStack(this)),
	m_OperationRunner(new OperationRunner(this, operationStack())),
	m_DeviceScanner(new DeviceScanner(this, operationStack())),
	m_ApplyProgressDialog(new ApplyProgressDialog(this, operationRunner()))
{
	Config::instance("kcm_partitionmanagerrc");
	setupObjectNames();
	setupUi(this);
	init();
}

void PartitionManagerKCM::setupObjectNames()
{
	m_OperationStack->setObjectName("m_OperationStack");
	m_DeviceScanner->setObjectName("m_DeviceScanner");
}

void PartitionManagerKCM::init()
{
	connect(GlobalLog::instance(), SIGNAL(newMessage(Log::Level, const QString&)), SLOT(onNewLogMessage(Log::Level, const QString&)));

	registerMetaTypes();
	loadBackend();

	setButtons(Apply);
	setNeedsAuthorization(true);
	setupConnections();

	listDevices().setActionCollection(actionCollection());
	listOperations().setActionCollection(actionCollection());
	deviceScanner().start();
	pmWidget().init(&operationStack());

	const char* actionNames[] =
	{
		"newPartition",
		"resizePartition",
		"deletePartition",
		"copyPartition",
		"pastePartition",
		"checkPartition",
		"propertiesPartition",
		"backupPartition",
		"restorePartition",
		"",
		"createNewPartitionTable",
		"refreshDevices"
	};

	for (size_t i = 0; i < sizeof(actionNames) / sizeof(actionNames[0]); i++)
		if (strlen(actionNames[i]) > 0)
			toolBar().addAction(actionCollection()->action(actionNames[i]));
		else
			toolBar().addSeparator();

	toolBar().setIconSize(QSize(22, 22));
	toolBar().setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	splitterHorizontal().setStretchFactor(0, 1);
	splitterHorizontal().setStretchFactor(1, 4);
	splitterVertical().setStretchFactor(0, 1);
	splitterVertical().setStretchFactor(1, 3);

	setupKCMWorkaround();
	setAboutData(createPartitionManagerAboutData());
}

void PartitionManagerKCM::onNewLogMessage(Log::Level, const QString& s)
{
	kDebug() << s;
}

void PartitionManagerKCM::setupConnections()
{
	connect(&listDevices(), SIGNAL(selectionChanged(const QString&)), &pmWidget(), SLOT(setSelectedDevice(const QString&)));
	connect(&listDevices(), SIGNAL(deviceDoubleClicked(const QString&)), &pmWidget(), SLOT(onPropertiesDevice(const QString&)));
	connect(newButton, SIGNAL(clicked()), &pmWidget(), SLOT(onNewPartition()));
	connect(resizeButton, SIGNAL(clicked()), &pmWidget(), SLOT(onResizePartition()));
	connect(deleteButton, SIGNAL(clicked()), &pmWidget(), SLOT(onDeletePartition()));
	connect(shredButton, SIGNAL(clicked()), &pmWidget(), SLOT(onShredPartition()));
	connect(copyButton, SIGNAL(clicked()), &pmWidget(), SLOT(onCopyPartition()));
	connect(pasteButton, SIGNAL(clicked()), &pmWidget(), SLOT(onPastePartition()));
	connect(mountButton, SIGNAL(clicked()), &pmWidget(), SLOT(onMountPartition()));
}

void PartitionManagerKCM::on_m_OperationStack_operationsChanged()
{
	listOperations().updateOperations(operationStack().operations());
	pmWidget().updatePartitions();
	enableButtons();

	emit changed(operationStack().size() > 0);
}

void PartitionManagerKCM::on_m_OperationStack_devicesChanged()
{
	QReadLocker lockDevices(&operationStack().lock());

	listDevices().updateDevices(operationStack().previewDevices());
}

void PartitionManagerKCM::on_m_PartitionManagerWidget_selectedPartitionChanged(const Partition* p)
{
	Q_UNUSED(p);
	enableButtons();
}

void PartitionManagerKCM::setupKCMWorkaround()
{
	// The Partition Manager kcm must be run as root, for obvious reasons. system-settings will
	// open kcms that require root privileges in a separate kcmshell4 process with a window of
	// its own. This window (a KDialog, actually) has a couple of buttons at the bottom, one of
	// them an Ok-button. The user will expect to have his changes applied if he clicks that button.
	// Unfortunately, we cannot do that: The kcmshell will kill us and our OperationRunner thread
	// without asking us as soon as we return from PartitionManagerKCM::save(). Even worse, we
	// have no way to find out if PartitionMangerKCM::save() was called because the user clicked
	// on "Ok" or "Apply" -- if we had that way we could at least do nothing in the case of the
	// Ok button...
	// Anyway, there seems to be no other solution than find the KDialog and turn off all buttons we
	// cannot handle... Nasty, but effective for now.
	foreach(QWidget* w, KApplication::topLevelWidgets())
	{
		KCMultiDialog* dlg = qobject_cast<KCMultiDialog*>(w);
		if (dlg != NULL)
		{
			dlg->setButtons(KDialog::Cancel|KDialog::Apply);
			dlg->enableButtonApply(false);
			connect(dlg, SIGNAL(applyClicked()), SLOT(onApplyClicked()));
		}
	}
}

void PartitionManagerKCM::enableButtons()
{
	Partition* p = pmWidget().selectedPartition();
	newButton->setEnabled(NewOperation::canCreateNew(p));
	bool canResize = ResizeOperation::canGrow(p) || ResizeOperation::canShrink(p) || ResizeOperation::canMove(p);
	resizeButton->setEnabled(canResize);
	deleteButton->setEnabled(DeleteOperation::canDelete(p));
	shredButton->setEnabled(DeleteOperation::canDelete(p));
	copyButton->setEnabled(CopyOperation::canCopy(p));
	pasteButton->setEnabled(CopyOperation::canPaste(p, pmWidget().clipboardPartition()));
	mountButton->setEnabled(p && (p->canMount() || p->canUnmount()));
	mountButton->setText(p && p->isMounted() ? "Unmount" : "Mount");
}

void PartitionManagerKCM::load()
{
    KCModule::load();
}

void PartitionManagerKCM::save()
{
	QStringList opList;

	foreach (const Operation* op, operationStack().operations())
		opList.append(op->description());

	if (KMessageBox::warningContinueCancelList(this,
		i18nc("@info",
			"<para>Do you really want to apply the pending operations listed below?</para>"
			"<para><warning>This will permanently modify your disks.</warning></para>"),
		opList, i18nc("@title:window", "Apply Pending Operations?"),
		KGuiItem(i18nc("@action:button", "Apply Pending Operations"), "arrow-right"),
		KStandardGuiItem::cancel()) == KMessageBox::Continue)
	{
		Log() << i18nc("@info/plain", "Applying operations...");

		applyProgressDialog().show();

		operationRunner().setReport(&applyProgressDialog().report());

		// Undo all operations so the runner has a defined starting point
		for (int i = operationStack().operations().size() - 1; i >= 0; i--)
		{
			operationStack().operations()[i]->undo();
			operationStack().operations()[i]->setStatus(Operation::StatusNone);
		}

		pmWidget().updatePartitions();

		operationRunner().start();
	}

	QTimer::singleShot(0, this, SLOT(on_m_OperationStack_operationsChanged()));
}

