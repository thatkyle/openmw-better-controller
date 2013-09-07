#ifndef PLUGINSPROXYMODEL_HPP
#define PLUGINSPROXYMODEL_HPP

#include <QSortFilterProxyModel>

class QVariant;
class QAbstractTableModel;

namespace EsxModel
{
    class ContentModel;

    class PluginsProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:

        explicit PluginsProxyModel(QObject *parent = 0, ContentModel *model = 0);
        ~PluginsProxyModel();

        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

        bool removeRows(int row, int count, const QModelIndex &parent);
    };
}

#endif // PLUGINSPROXYMODEL_HPP
