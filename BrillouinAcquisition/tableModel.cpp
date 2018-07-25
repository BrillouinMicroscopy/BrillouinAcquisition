#include "stdafx.h"
#include "tableModel.h"

#include <QtGui>

ButtonDelegate::ButtonDelegate(QObject *parent)
	: QItemDelegate(parent) {
}

void ButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
	QRect r = option.rect; // rectangle of the cell
	int buttonLeft = 0, buttonTop = r.top() + 3;

	// move to button
	QStyleOptionButton moveButton;
	buttonLeft = r.left() + r.width() - 35;
	moveButton.rect = QRect(buttonLeft, buttonTop, m_buttonWidth, m_buttonHeight);
	moveButton.state = QStyle::State_Enabled;
	
	moveButton.icon = QIcon(":/BrillouinAcquisition/assets/moveToPosition.png");
	moveButton.iconSize = QSize(24, 24);

	QApplication::style()->drawControl(QStyle::CE_PushButton, &moveButton, painter);

	// delete button
	QStyleOptionButton deleteButton;
	buttonLeft = r.left() + r.width() - 70;
	deleteButton.rect = QRect(buttonLeft, buttonTop, m_buttonWidth, m_buttonHeight);
	deleteButton.state = QStyle::State_Enabled;

	deleteButton.icon = QIcon(":/BrillouinAcquisition/assets/deletePosition.png");
	deleteButton.iconSize = QSize(24, 24);

	QApplication::style()->drawControl(QStyle::CE_PushButton, &deleteButton, painter);
}

bool ButtonDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) {
	if (event->type() == QEvent::MouseButtonRelease) {
		QMouseEvent *e = (QMouseEvent *)event;
		int clickX = e->x();
		int clickY = e->y();
		QRect r = option.rect; // rectangle of the cell
		int buttonLeft = 0, buttonTop = r.top() + 3;

		int buttonLeftDelete = r.left() + r.width() - 70;
		int buttonLeftMove = r.left() + r.width() - 35;

		if (clickY > buttonTop && clickY < buttonTop + m_buttonHeight) {
			if (clickX > buttonLeftDelete && clickX < buttonLeftDelete + m_buttonWidth) {
				emit(deletePosition(index.row()));
			}
			if (clickX > buttonLeftMove && clickX < buttonLeftMove + m_buttonWidth) {
				emit(moveToPosition(index.row()));
			}
		}
	}
	return true;
}

TableModel::TableModel(QObject *parent)
	:QAbstractTableModel(parent) {
}

TableModel::TableModel(QObject *parent, std::vector<POINT3> storage)
	:QAbstractTableModel(parent), m_storage(storage) {
}

void TableModel::setStorage(std::vector<POINT3> storage) {
	m_storage = storage;

	QModelIndex topLeft = index(0, 0);
	QModelIndex bottomRight = index(m_storage.size(), 4);
	emit layoutChanged();
	emit dataChanged(topLeft, bottomRight);
}

int TableModel::rowCount(const QModelIndex &parent) const {
	return m_storage.size();
}

int TableModel::columnCount(const QModelIndex &parent) const {
	return 4;
}

QVariant TableModel::data(const QModelIndex &index, int role) const {
	if (role == Qt::DisplayRole) {
		if (index.column() == 0) {
			return QString("%1").arg(m_storage[index.row()].x);
		} else if (index.column() == 1) {
			return QString("%1").arg(m_storage[index.row()].y);
		} else if (index.column() == 2) {
			return QString("%1").arg(m_storage[index.row()].z);
		}
	}
	return QVariant();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
				case 0:
					return QString::fromUtf8("x [\xc2\xb5m]");
				case 1:
					return QString::fromUtf8("y [\xc2\xb5m]");
				case 2:
					return QString::fromUtf8("z [\xc2\xb5m]");
			}
		}
		if (orientation == Qt::Vertical) {
			return section + 1;
		}
	}
	return QVariant();
}