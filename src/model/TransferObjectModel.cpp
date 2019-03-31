/*
* Copyright (C) 2019 Veli Tasalı, created on 3/4/19
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <src/util/AppUtils.h>
#include <src/util/TransferUtils.h>
#include "TransferObjectModel.h"

TransferObjectModel::TransferObjectModel(groupid groupId, const QString &deviceId, QObject *parent)
		: QAbstractTableModel(parent), m_deviceId(deviceId)
{
	m_groupId = groupId;
	connect(gDatabase, &AccessDatabase::databaseChanged, this, &TransferObjectModel::databaseChanged);
	databaseChanged(SqlSelection(), ChangeType::Any);
}

int TransferObjectModel::columnCount(const QModelIndex &parent) const
{
	return ColumnNames::__itemCount;
}

int TransferObjectModel::rowCount(const QModelIndex &parent) const
{
	return list()->size();
}

QVariant TransferObjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
				case ColumnNames::Status:
					return tr("Status");
				case ColumnNames::FileName:
					return tr("File name");
				case ColumnNames::Size:
					return tr("Size");
				case ColumnNames::Directory:
					return tr("Directory");
				default:
					return QString("?");
			}
		} else
			return QString("%1").arg(section);
	}

	return QVariant();
}

QVariant TransferObjectModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole) {
		const auto &currentObject = list()->at(index.row());

		switch (index.column()) {
			case ColumnNames::FileName:
				return currentObject.friendlyName;
			case ColumnNames::Status:
				return TransferUtils::getFlagString(currentObject.flag);
			case ColumnNames::Size:
				return TransferUtils::sizeExpression(currentObject.fileSize, false);
			case ColumnNames::Directory:
				return currentObject.directory;
			default:
				return QString("Data id %1x%2")
						.arg(index.row())
						.arg(index.column());
		}
	} else if (role == Qt::DecorationRole) {
		switch (index.column()) {
			case ColumnNames::FileName: {
				const auto &currentGroup = list()->at(index.row());
				return QIcon(currentGroup.type == TransferObject::Type::Incoming
				             ? ":/icon/arrow_down"
				             : ":/icon/arrow_up");
			}
			default: {
				// do nothing
			}
		}
	}

	return QVariant();
}

void TransferObjectModel::databaseChanged(const SqlSelection &change, ChangeType type)
{
	if ((!change.valid() || change.tableName == DB_TABLE_TRANSFER || change.tableName == DB_DIVIS_TRANSFER)
	    && gAccessList(this)) {
		emit layoutAboutToBeChanged();
		clearList();

		SqlSelection selection;
		selection.setTableName(DB_TABLE_TRANSFER);
		selection.setOrderBy(QString("%1 ASC, %2 ASC")
				                     .arg(DB_FIELD_TRANSFER_NAME)
				                     .arg(DB_FIELD_TRANSFER_DIRECTORY));

		if (m_deviceId.isEmpty())
			selection.setWhere(QString("%1 = ?").arg(DB_FIELD_TRANSFER_GROUPID));
		else {
			selection.setWhere(QString("%1 = ? AND %2 = ?")
					                   .arg(DB_FIELD_TRANSFER_DEVICEID)
					                   .arg(DB_FIELD_TRANSFER_GROUPID));
			selection.whereArgs << m_deviceId;
		}

		selection.whereArgs << m_groupId;

		gDatabase->castQuery(selection, *list());

		if (list()->empty()) {
			selection.setTableName(DB_DIVIS_TRANSFER);
			gDatabase->castQuery(selection, *list());
		}

		emit layoutChanged();
	}
}

void TransferObjectModel::setDeviceId(const QString &deviceId)
{
	m_deviceId = deviceId;
}
