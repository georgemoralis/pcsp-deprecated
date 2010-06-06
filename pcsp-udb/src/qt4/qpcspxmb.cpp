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
#include <qtconcurrentrun.h>

#include "types.h"
#include "../loaders.h"
#include <stdio.h>
#include "ProgressCtrl.h"
#include "qumdmodel.h"

QPcspXmb::QPcspXmb(QWidget *parent, Qt::WFlags flags)
:   QMainWindow(parent, flags)
,   Ui::PcspXmbUi()
,   m_selectionModel(0)
,   m_ini("pcsp-xmb.ini", QSettings::IniFormat)
,   thisThread(NULL)
,   progressCtrl(NULL)
,   m_stop(false)
{
    setupUi(this);

    m_umdisospath = m_ini.value("/default/games/path").toString();
    m_sourceModel = new QUmdTableModel(m_umdisospath, this);

    m_model = new QSortFilterProxyModel(this);
    m_model->setSourceModel(m_sourceModel);
    m_mapper = new QDataWidgetMapper(this);
    m_mapper->setModel(m_model);
    m_mapper->addMapping(discIdEdit,   1);
    m_mapper->addMapping(regionEdit,   2);
    m_mapper->addMapping(firmwareEdit, 3);
	m_mapper->addMapping(publishEdit,  4);

    m_mapper->toFirst();
    
	m_selectionModel = gameList->selectionModel();

    connect(m_model, SIGNAL(modelReset()), this, SLOT(onModelReset()));

   // icon0List->setModel(m_model);
   // icon0List->setModelColumn(0);
    gameList->setModel(m_model);
    QHeaderView *header = gameList->header();
    header->setStretchLastSection(true);
    header->setResizeMode(QHeaderView::ResizeToContents);

    connect(gameList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClicked(QModelIndex)));
//   connect(icon0List, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClicked(QModelIndex)));
    connect(runButton, SIGNAL(clicked()), this, SLOT(onPressedButton()));


    QMenu *menu = new QMenu(tr("Settings"), this);
    QAction *action1 = new QAction(tr("Change UMD Games folder"), this);
    connect(action1, SIGNAL(triggered()), this, SLOT(onChangeUmdPath()));
    menu->addAction(action1);
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
	m_model->setFilterKeyColumn(-1);//search to all columns by default
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
     languageEdit->setText(index.data(Qt::UserRole+6).toString());
	 genreEdit->setText(index.data(Qt::UserRole+7).toString());
	 crc32Edit->setText(index.data(Qt::UserRole+8).toString());
	 statusEdit->setText(index.data(Qt::UserRole+9).toString());
	 gameSizeEdit->setText(index.data(Qt::UserRole+10).toString());


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
            QMessageBox::critical(0, tr("PCSP - XMB"), tr("Cannot find an emulator to play %1!").arg(index.sibling(index.row(), 0).data(Qt::DisplayRole).toString()));
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
	 if (!thisThread) thisThread = new MainWindowThread(this);
     if(thisThread->isRunning()) return;// false;
     connect(thisThread, SIGNAL(started()), this, SLOT(thisThreadStarted()), Qt::BlockingQueuedConnection);
     connect(thisThread, SIGNAL(finished()), this, SLOT(thisThreadFinished()), Qt::BlockingQueuedConnection);
     connect(thisThread, SIGNAL(terminated()), this, SLOT(thisThreadFinished()), Qt::BlockingQueuedConnection);
     if (!progressCtrl) progressCtrl = new ProgressCtrl(this);
     progressCtrl->show();
	 m_stop = false;
	 
	 thisThread->start();
}
void QPcspXmb::thisThreadStarted()
{

    connect(this, SIGNAL(range(int, int)), progressCtrl->progress(), SLOT(setRange(int, int)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(progress(int)), progressCtrl->progress(), SLOT(setValue(int)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(label(const QString &)), progressCtrl->label(), SLOT(setText(const QString &)), Qt::BlockingQueuedConnection);
    connect(progressCtrl->stop(), SIGNAL(clicked()), this, SLOT(setStop()));

}
void QPcspXmb::thisThreadFinished()
{


    disconnect(this, SIGNAL(range(int, int)), progressCtrl->progress(), SLOT(setRange(int, int)));
    disconnect(this, SIGNAL(progress(int)), progressCtrl->progress(), SLOT(setValue(int)));
    disconnect(this, SIGNAL(label(const QString &)), progressCtrl->label(), SLOT(setText(const QString &)));
    disconnect(progressCtrl->stop(), SIGNAL(clicked()), this, SLOT(setStop()));
    progressCtrl->hide(); 
    progressCtrl->progress()->setValue(0);
}
void QPcspXmb::setStop()
{
  m_stop = true;
  if (progressCtrl) progressCtrl->hide();
}
void QPcspXmb::run()
{
    QDir dir(m_umdisospath);
    QFileInfoList entries(dir.entryInfoList(QStringList() << "*.ISO" << "*.CSO", QDir::Files, QDir::Name|QDir::IgnoreCase));
	emit range(0, entries.size()-1);
	int i=0;

	      m_sourceModel->m_infos.clear();
         m_sourceModel->startupdatemodel();
		  QListIterator< QFileInfo > entry(entries);
          if (entry.hasNext())
          {
            while (entry.hasNext())
            {
				emit progress(i);
		        
                QFileInfo fi = entry.next();
                emit label(tr("Loading %1...").arg(fi.baseName()));
                UmdInfos infos(fi, false);
                m_sourceModel->m_infos.push_back(infos);
                if (!m_sourceModel->m_infos.last().id.size())
                {
                    m_sourceModel->m_infos.removeLast();
                }
				i++;
				
				m_sourceModel->endupdatemodel();
                if (m_stop) break;
            }
       // m_sourceModel->endupdatemodel();
	      }
          m_sourceModel->endupdatemodel();
		 
		  
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

void QPcspXmb::runInSeparateThread(QString text)
{
     QRegExp regExp(text,Qt::CaseInsensitive, QRegExp::PatternSyntax(2));
    m_model->setFilterRegExp(regExp);
}
void QPcspXmb::textFilterChanged(QString text)
{
  QFutureWatcher<void> futureWatcher;
  QFuture<void> future = QtConcurrent::run(this,&QPcspXmb::runInSeparateThread,text);
  futureWatcher.setFuture(future);
  futureWatcher.waitForFinished();

}
void QPcspXmb::filterRegExpChanged(int column)
{ 
  if(column==0)
  {
    m_model->setFilterKeyColumn(-1);
  }
  else
  {
     m_model->setFilterKeyColumn(column-1);
  }

}