/*
This file is part of pcsp.

pcsp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pcsp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pcsp.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "qpcspxmb.h"
#include <QtCore>
#include <QtGui>

#include "types.h"
#include "../loaders.h"
#include <stdio.h>

#include "qumdmodel.h"

QPcspXmb::QPcspXmb(QWidget *parent, Qt::WFlags flags)
:   QMainWindow(parent, flags)
,   Ui::PcspXmbUi()
,   m_selectionModel(0)
,   m_ini("pcsp-xmb.ini", QSettings::IniFormat)
{
    setupUi(this);

    m_umdisospath = m_ini.value("/default/games/path").toString();
    m_model  = new QUmdTableModel(m_umdisospath, this);
    m_mapper = new QDataWidgetMapper(this);
    m_mapper->setModel(m_model);
    m_mapper->addMapping(discIdEdit,   1);
    m_mapper->addMapping(regionEdit, 2);
    m_mapper->addMapping(firmwareEdit, 3);
	m_mapper->addMapping(publishEdit,4);
	m_mapper->addMapping(languageEdit,5);
	m_mapper->addMapping(genreEdit,6);
	m_mapper->addMapping(crc32Edit,    7);
	m_mapper->addMapping(statusEdit,   8);
	m_mapper->addMapping(gameSizeEdit,9);
	

   // m_mapper->addMapping(gameNameEdit, 2);
    
	//
    m_mapper->toFirst();

    
	m_selectionModel = gameList->selectionModel();

    connect(m_model, SIGNAL(modelReset()), this, SLOT(onModelReset()));

   // icon0List->setModel(m_model);
   // icon0List->setModelColumn(0);
    gameList->setModel(m_model);
    connect(gameList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClicked(QModelIndex)));
//   connect(icon0List, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClicked(QModelIndex)));
    connect(runButton, SIGNAL(clicked()), this, SLOT(onPressedButton()));


    QMenu *menu = new QMenu(tr("Settings"), this);
    QAction *action1 = new QAction(tr("Change UMD Games folder"), this);
    QAction *action2 = new QAction(tr("Auto-rename UMD Games"), this);
    action2->setCheckable(true);
    action2->setChecked(m_ini.value("/default/games/autorename", false).toBool());
    connect(action1, SIGNAL(triggered()), this, SLOT(onChangeUmdPath()));
    connect(action2, SIGNAL(toggled(bool)), this, SLOT(onAutoRenameToggled(bool)));
    menu->addAction(action1);
    menu->addAction(action2);
    m_systray = new QSystemTrayIcon(QIcon(":/icons/pcsp.ico"), this);
    connect(m_systray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(showNormal()));
    m_systray->setContextMenu(menu);
    m_systray->show();

    if (m_umdisospath.isEmpty())
    {
        onChangeUmdPath();
    }
    else
    {
        refresh();
    }
}

QPcspXmb::~QPcspXmb()
{
}

void QPcspXmb::onModelReset()
{
    if (m_selectionModel)
    {
        delete m_selectionModel;
    }

    m_selectionModel = new QItemSelectionModel(m_model, this);
    gameList->setSelectionModel(m_selectionModel);
	gameList->hideColumn(5);
	gameList->hideColumn(6);
	gameList->hideColumn(7);
	gameList->hideColumn(8);
	gameList->hideColumn(9);
	gameList->setColumnWidth(0,250);
    gameList->setColumnWidth(1,80);
	gameList->setColumnWidth(2,60);
    gameList->setColumnWidth(3,40);

    connect(m_selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(onCurrentChanged(QModelIndex)));
}

void QPcspXmb::onCurrentChanged(QModelIndex const &index)
{
    m_mapper->setCurrentModelIndex(index);
	 icon0pic->setPixmap(QPixmap(index.data(Qt::UserRole+1).toString()));
	 coverpic->setPixmap(QPixmap(index.data(Qt::UserRole+2).toString()));
	 coverpicback->setPixmap(QPixmap(index.data(Qt::UserRole+3).toString()));
     previewpic1->setPixmap(QPixmap(index.data(Qt::UserRole+4).toString()));
	 previewpic2->setPixmap(QPixmap(index.data(Qt::UserRole+5).toString()));
//   pic1Label->setPixmap(QPixmap(index.sibling(index.row(), 3).data().toString()));
  //  gameList->scrollTo(index, QAbstractItemView::PositionAtCenter/*QAbstractItemView::EnsureVisible*/);
}

void QPcspXmb::onDoubleClicked(QModelIndex const &index)
{
    QString umdpath = index.data(Qt::UserRole).toString();
    QString discID  = index.sibling(index.row(), 1).data(Qt::DisplayRole).toString();  
    QProcess launcher(this);
    QStringList arguments;
    arguments << "-umd" << umdpath;
    if (!launcher.startDetached("pcsp-" + discID, arguments))
    {
        if (!launcher.startDetached("pcsp", arguments))
        {
            QMessageBox::critical(0, tr("PCSP - XMB"), tr("Cannot find an emulator to play %1!").arg(index.sibling(index.row(), 2).data(Qt::DisplayRole).toString()));
        }
    }
}

void QPcspXmb::onPressedButton()
{
    QModelIndex index = m_selectionModel->currentIndex();
    onDoubleClicked(index);
}

void QPcspXmb::refresh()
{
    m_model->updateModel(m_umdisospath, m_ini.value("/default/games/autorename", false).toBool());
}

void QPcspXmb::onChangeUmdPath()
{
    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
    QString newPath = QFileDialog::getExistingDirectory(0, tr("Select UMD Games folder"), m_umdisospath, options);
    if (newPath.size())
    {
        m_umdisospath = newPath;
        m_ini.setValue("/default/games/path", m_umdisospath);
        refresh();
    }
}

void QPcspXmb::onAutoRenameToggled(bool checked)
{
    m_ini.setValue("/default/games/autorename", checked);
}