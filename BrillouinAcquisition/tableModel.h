#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include <QAbstractTableModel>
#include "scancontrol.h"

class ButtonDelegate : public QItemDelegate {
	Q_OBJECT

private:
	int m_buttonWidth = 18;
	int m_buttonHeight = 18;

public:
	ButtonDelegate(QObject *parent = 0);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);

signals:
	void deletePosition(int);
	void moveToPosition(int);
};

class TableModel : public QAbstractTableModel {
	Q_OBJECT
private:
	std::vector<POINT3> m_storage;
public:
	TableModel(QObject *parent);
	TableModel(QObject *parent, std::vector<POINT3> storage);
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
public slots:
	void setStorage(std::vector<POINT3> storage);
};

#endif // TABLEMODEL_H