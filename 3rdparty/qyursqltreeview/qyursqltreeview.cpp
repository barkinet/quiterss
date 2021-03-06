/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2015 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
/*******************************************************
**
** Implemention of QyurSqlTreeView
**
** Copyright (C) 2009 Yuri Yurachkovsky.
**
** QyurSqlTreeView is free software; you can redistribute
** it and/or modify it under the terms of the GNU
** Library General Public License as published by the
** Free Software Foundation; either version 2, or (at
** your option) any later version.
**
********************************************************/
#ifdef HAVE_QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QtSql>

#include "qyursqltreeview.h"

namespace {
/*
Invariant: source model is always sorted asc by parid column. Then real user sort is perfomes in context of parid sort.
class QSqlTableModelEx take care about it;
*/
const int DROP_MULTIORDER=-2;
class QSqlTableModelEx: public QSqlTableModel {
  Q_OBJECT
public:
  QSqlTableModelEx(QObject* parent=0, QSqlDatabase db = QSqlDatabase());
  void setSort(int column=-1, Qt::SortOrder order=Qt::AscendingOrder);
protected:
  struct SortingStruct {
    SortingStruct(int field_=0, Qt::SortOrder sortingOrder_=Qt::AscendingOrder) {
      field=field_;
      sortingOrder=sortingOrder_;
    }
    int field;
    Qt::SortOrder sortingOrder;
  };
  QString orderByClause() const;
  QVector<SortingStruct> sortingFields;
};

QSqlTableModelEx::QSqlTableModelEx(QObject* parent, QSqlDatabase db):QSqlTableModel(parent, db) {
  setEditStrategy(QSqlTableModel::OnManualSubmit);
}

QString QSqlTableModelEx::orderByClause() const {
  QString s;
  if (sortingFields.isEmpty())
    return s;
  s.append("ORDER BY ");
  foreach(SortingStruct sortingField,sortingFields)
    s.append(record().fieldName(sortingField.field)).append(sortingField.sortingOrder==Qt::AscendingOrder?" ASC":" DESC").append(",");
  s.chop(1);
  return s;
}

void QSqlTableModelEx::setSort(int column, Qt::SortOrder order) {
  if (!record().field(column).isValid())
    return;
  if (order==DROP_MULTIORDER) {
    sortingFields.clear();
    sortingFields.append(SortingStruct(column,Qt::AscendingOrder));
    emit headerDataChanged(Qt::Horizontal,0,columnCount()-1);//update icons
  }
  else {
    for(int i=0; i<sortingFields.size();i++)
      if (sortingFields[i].field==column) {
        sortingFields[i].sortingOrder= (Qt::SortOrder)!sortingFields[i].sortingOrder;//order;
        return;
      }
    sortingFields.append(SortingStruct(column,order));
  }
}

struct UserData {
  UserData(int id, int parid):id(id),parid(parid) {
  }
  ~UserData() {
  }
  int id,parid;
};
}//namespace

class QyurSqlTreeModelPrivate {
  Q_DECLARE_PUBLIC(QyurSqlTreeModel)
  QyurSqlTreeModel* q_ptr;
  mutable QMap<int,int> id2row,parid2row,id2ParentRow,id2rowCount;
  mutable QMap<QPair<int,int>,int> row2id;
  mutable QMap<int,UserData*> id2UserData;
  mutable QMap<QModelIndex,QModelIndex> proxy2source;
  mutable QHash<int,int> proxyColumn2Original;

  QMap<QObject*,QSet<int> > expandedSet;

public:
  QSqlTableModelEx sourceModel;
  int indexOfId, indexOfParid, rootParentId;
  int originalColumnByProxy(int proxyColumn) const {
    return proxyColumn2Original.value(proxyColumn);
  }
  int getRowById(int) const;
  int getRowByParid(int) const;
  UserData* getUserDataById(int) const;
  QyurSqlTreeModelPrivate() {
    indexOfId = -1;
    indexOfParid = -1;
    rootParentId = -1;
  }
  ~QyurSqlTreeModelPrivate() {
    clear();
  }
  void clear() {
    qDeleteAll(id2UserData);
    id2row.clear();
    parid2row.clear();
    id2ParentRow.clear();
    id2rowCount.clear();
    row2id.clear();
    id2UserData.clear();
    proxy2source.clear();
  }
};

UserData* QyurSqlTreeModelPrivate::getUserDataById(int id) const {
  if (!id2UserData[id]) {
    int parid = sourceModel.index(getRowById(id),indexOfParid).data().toInt();
    id2UserData[id]= new UserData(id,parid);
  }
  return id2UserData[id];
}

int QyurSqlTreeModelPrivate::getRowById(int id) const {
  if (!id2row.contains(id) || sourceModel.index(id2row[id],indexOfId).data().toInt() != id) {
    for (int i = 0; i < sourceModel.rowCount(); i++) {
      if (sourceModel.index(i,indexOfId).data().toInt() == id) {
        id2row[id] = i;
        break;
      }
    }
  }
  return id2row.value(id,-1);
}

int QyurSqlTreeModelPrivate::getRowByParid(int parid) const {
  if (!parid2row.contains(parid) ||
      (sourceModel.index(parid2row[parid],indexOfParid).data().toInt() != parid) ||
      (parid2row[parid]&&sourceModel.index(parid2row[parid]-1,indexOfParid).data().toInt() == parid) ) {
    for (int i = 0; i < sourceModel.rowCount(); i++) {
      if (sourceModel.index(i,indexOfParid).data().toInt() == parid) {
        parid2row[parid]= i;
        break;
      }
    }
  }
  return parid2row.value(parid,-1);
}

QyurSqlTreeModel::QyurSqlTreeModel(const QString& tableName,
                                   const QStringList& fieldNames,
                                   QObject* parent)
  : QAbstractItemModel(parent)
  , d_ptr(new QyurSqlTreeModelPrivate)
{
  Q_D(QyurSqlTreeModel);
  d->q_ptr = this;
  d->rootParentId = 0;
  d->sourceModel.setTable(tableName);
  d->indexOfId = d->sourceModel.record().indexOf("id");
  Q_ASSERT(d->indexOfId >= 0);
  d->indexOfParid = d->sourceModel.record().indexOf("parentId");
  Q_ASSERT(d->indexOfParid >= 0);
  for (int i = 0; i < d->sourceModel.columnCount(); i++) {
    d->proxyColumn2Original[i] = i;
  }
  d->proxyColumn2Original[0] = d->sourceModel.record().indexOf(fieldNames[0]);
  d->proxyColumn2Original[d->sourceModel.record().indexOf(fieldNames[0])] = 0;

  d->sourceModel.setSort(d->indexOfParid, (Qt::SortOrder)DROP_MULTIORDER);
  d->sourceModel.setSort(d->sourceModel.record().indexOf("rowToParent"), Qt::AscendingOrder);
  refresh();
}

QyurSqlTreeModel::~QyurSqlTreeModel() {
  delete d_ptr;
}

int QyurSqlTreeModel::originalColumnByProxy(int proxyColumn) const {
  Q_D(const QyurSqlTreeModel);
  return d->originalColumnByProxy(proxyColumn);
}

int QyurSqlTreeModel::proxyColumnByOriginal(int originalColumn) const {
  Q_D(const QyurSqlTreeModel);
  return d->proxyColumn2Original.value(originalColumn, originalColumn);
}

int QyurSqlTreeModel::proxyColumnByOriginal(const QString& field) const {
  Q_D(const QyurSqlTreeModel);
  return proxyColumnByOriginal(d->sourceModel.record().indexOf(field));
}

void QyurSqlTreeModel::refresh() {
  Q_D(QyurSqlTreeModel);
  d->sourceModel.select();
  while (d->sourceModel.canFetchMore())
    d->sourceModel.fetchMore();
#ifdef HAVE_QT5
  beginResetModel();
  d->clear();
  endResetModel();
#else
  reset();
  d->clear();
#endif

  for (int i = 0; i < d->sourceModel.rowCount(); i++) {
    int id = d->sourceModel.index(i,d->indexOfId).data().toInt();
    d->id2row[id] = i;
    int parid = d->sourceModel.index(i,d->indexOfParid).data().toInt();
    d->parid2row[parid] = i;
    d->id2UserData[id] = new UserData(id,parid);
  }
}

QVariant QyurSqlTreeModel::data(const QModelIndex &index, int role) const
{
  Q_D(const QyurSqlTreeModel);
  return d->sourceModel.data(mapToSource(index), role);
}

bool QyurSqlTreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  Q_D(QyurSqlTreeModel);
  return d->sourceModel.setData(mapToSource(index),value,role);
}

int QyurSqlTreeModel::columnCount(const QModelIndex&) const {
  Q_D(const QyurSqlTreeModel);
  return d->sourceModel.record().count();
}

QModelIndex QyurSqlTreeModel::index(int row, int column, const QModelIndex& parent) const {
  Q_D(const QyurSqlTreeModel);
  if (d->sourceModel.rowCount()==0)
    return QModelIndex();
  int id = parent.isValid()?getIdByIndex(parent):getRootParentId();
  if (!d->row2id.contains(QPair<int,int>(id,row))) {
    int i= 0;
    if (parent.isValid())
      i= d->getRowByParid(id);
    if (d->sourceModel.index(i+row,d->indexOfParid).data().toInt() == id)
      d->row2id[QPair<int,int>(id,row)]= d->sourceModel.index(i+row,d->indexOfId).data().toInt();
    else
      return QModelIndex();
  }
  UserData *userData = d->getUserDataById(d->row2id[QPair<int,int>(id,row)]);
  return createIndex(row,column,userData);
}

QModelIndex QyurSqlTreeModel::parent(const QModelIndex& index) const {
  Q_D(const QyurSqlTreeModel);
  if (d->sourceModel.rowCount()==0)
    return QModelIndex();
  if (!index.isValid())
    return QModelIndex();
  int parid= getParidByIndex(index);
  if (parid==getRootParentId())
    return QModelIndex();

  if (!d->id2ParentRow.contains(getIdByIndex(index))) {
    int paridOfParent = d->sourceModel.index(d->getRowById(parid),d->indexOfParid).data().toInt();
    int rowOfParent = 0;
    if (paridOfParent==getRootParentId()) {
      while (d->sourceModel.index(rowOfParent,d->indexOfParid).data().toInt() == getRootParentId() &&
             d->sourceModel.index(rowOfParent,d->indexOfId).data().toInt() != parid) {
        rowOfParent++;
      }
    } else {
      rowOfParent= d->getRowByParid(paridOfParent);
      if (rowOfParent>=0)
        while (d->sourceModel.index(rowOfParent,d->indexOfId).data().toInt() != parid) {
          rowOfParent++;
        }
      rowOfParent-=d->getRowByParid(paridOfParent);
    }
    if (d->sourceModel.index(rowOfParent+((paridOfParent==getRootParentId())?0:d->getRowByParid(paridOfParent)),d->indexOfId).data().toInt() == parid)
      d->id2ParentRow[getIdByIndex(index)]= rowOfParent;
    else return QModelIndex();
  }
  return createIndex(d->id2ParentRow[getIdByIndex(index)],0,d->getUserDataById(parid));
}

int QyurSqlTreeModel::rowCount(const QModelIndex& parent) const {
  Q_D(const QyurSqlTreeModel);
  if (d->sourceModel.rowCount()==0)
    return 0;
  int id= parent.isValid()?getIdByIndex(parent):getRootParentId();
  if (!d->id2rowCount.contains(id)) {
    int i=0;
    if (!parent.isValid()) {
      while (d->sourceModel.index(i,d->indexOfParid).data().toInt() == getRootParentId() &&
             i < d->sourceModel.rowCount()) {
        i++;
      }
    } else {
      i= d->getRowByParid(getIdByIndex(parent));
      if (i>=0)
        while (d->sourceModel.index(i,d->indexOfParid).data().toInt() == getIdByIndex(parent)) {
          i++;
        }
      i-=d->getRowByParid(getIdByIndex(parent));
    }
    d->id2rowCount[id]=i;
  }
  return d->id2rowCount[id];
}

QModelIndex QyurSqlTreeModel::mapToSource(const QModelIndex& proxyIndex) const {
  Q_D(const QyurSqlTreeModel);
  if (!d->proxy2source.contains(proxyIndex))
    d->proxy2source[proxyIndex]= d->sourceModel.index(d->getRowById(getIdByIndex(proxyIndex)),d->originalColumnByProxy(proxyIndex.column()));
  return d->proxy2source[proxyIndex];
}

QModelIndex QyurSqlTreeModel::getIndexById(int id) const {
  Q_D(const QyurSqlTreeModel);
  QModelIndex parentIndex = QModelIndex();
  if (id > 0)
    parentIndex = getIndexById(d->getUserDataById(id)->parid);
  for (int i=0; i < rowCount(parentIndex); i++) {
    if (getIdByIndex(index(i,0,parentIndex)) == id)
      return index(i,0,parentIndex);
  }
  return QModelIndex();
}

int QyurSqlTreeModel::getIdByIndex(const QModelIndex& index) const {
  if (index.isValid())
    return static_cast<UserData*>(index.internalPointer())->id;
  return 0;
}

int QyurSqlTreeModel::getParidByIndex(const QModelIndex& index) const {
  if (index.isValid())
    return static_cast<UserData*>(index.internalPointer())->parid;
  return 0;
}

int QyurSqlTreeModel::getRootParentId() const {
  Q_D(const QyurSqlTreeModel);
  return d->rootParentId;
}

#include "qyursqltreeview.moc"
