#include "gamesmodel.h"
#include "pb/serverinfo_game.pb.h"
#include <QStringList>
#include <sstream>
#include <time.h>

namespace {
    const unsigned SECS_PER_HALF_MIN  = 30;
    const unsigned SECS_PER_MIN       = 60;
    const unsigned SECS_PER_HALF_HOUR = 1600;  // 60 * 30
    const unsigned SECS_PER_HOUR      = 3600;  // 60 * 60
    const unsigned SECS_PER_DAY       = 86400; // 60 * 60 * 24

    /**
     * Pretty print an integer number of seconds ago. Accurate to only one unit, 
     * rounded.
     *
     * For example...
     *   0-59 seconds will return "X seconds ago"
     *   1-59 minutes will return "X minutes ago"; 90 seconds will return "2 minutes ago"
     *   1-23 hours will return "X hours ago"; 90 minutes will return "2 hours ago"
     *   24+ hours will return "1+ days ago", because it seems unlikely that we care about
     *     an accurate timestamp of day old games.
     */
    std::string prettyPrintSecsAgo(uint32_t secs) {
        std::ostringstream str_stream;

        if (secs < SECS_PER_MIN) {
            str_stream << secs;
            str_stream << "s ago";
        } else if (secs < SECS_PER_HOUR) {
            uint32_t mins = secs / SECS_PER_MIN;
            if (secs % SECS_PER_MIN >= SECS_PER_HALF_MIN)
              mins++;
            str_stream << mins;
            str_stream << "m ago";
        } else if (secs < SECS_PER_DAY) {
            uint32_t hours = secs / SECS_PER_HOUR;
            if (secs % SECS_PER_HOUR >= SECS_PER_HALF_HOUR)
              hours++;
            str_stream << hours;
            str_stream << "h ago";
        } else {
            str_stream << "a long time ago";
        }

        return str_stream.str();
    }
}

GamesModel::GamesModel(const QMap<int, QString> &_rooms, const QMap<int, GameTypeMap> &_gameTypes, QObject *parent)
    : QAbstractTableModel(parent), rooms(_rooms), gameTypes(_gameTypes)
{
}

QVariant GamesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::UserRole)
        return index.row();
    if (role != Qt::DisplayRole && role != SORT_ROLE)
        return QVariant();
    if ((index.row() >= gameList.size()) || (index.column() >= columnCount()))
        return QVariant();

    const ServerInfo_Game &g = gameList[index.row()];
    switch (index.column()) {
        case 0: return rooms.value(g.room_id());
        case 1: {
            uint32_t now = time(NULL);
            uint32_t then = g.start_time();
            int secs = now - then;
 
            switch (role) {
                case Qt::DisplayRole: return QString::fromStdString(prettyPrintSecsAgo(secs));
                case SORT_ROLE: return QVariant(secs);
                default: return QVariant(); // Shouldn't ever be reached.
            }
        }
        case 2: return QString::fromStdString(g.description());
        case 3: return QString::fromStdString(g.creator_info().name());
        case 4: {
            QStringList result;
            GameTypeMap gameTypeMap = gameTypes.value(g.room_id());
            for (int i = g.game_types_size() - 1; i >= 0; --i)
                result.append(gameTypeMap.value(g.game_types(i)));
            return result.join(", ");
        }
        case 5: return g.with_password() ? ((g.spectators_need_password() || !g.spectators_allowed()) ? tr("yes") : tr("yes, free for spectators")) : tr("no");
        case 6: {
            QStringList result;
            if (g.only_buddies())
                result.append(tr("buddies only"));
            if (g.only_registered())
                result.append(tr("reg. users only"));
            return result.join(", ");
        }
        case 7: return QString("%1/%2").arg(g.player_count()).arg(g.max_players());
        case 8: return g.spectators_allowed() ? QVariant(g.spectators_count()) : QVariant(tr("not allowed"));
        default: return QVariant();
    }
}

QVariant GamesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((role != Qt::DisplayRole) || (orientation != Qt::Horizontal))
        return QVariant();
    switch (section) {
        case 0: return tr("Room");
        case 1: return tr("Start time");
        case 2: return tr("Description");
        case 3: return tr("Creator");
        case 4: return tr("Game type");
        case 5: return tr("Password");
        case 6: return tr("Restrictions");
        case 7: return tr("Players");
        case 8: return tr("Spectators");
        default: return QVariant();
    }
}

const ServerInfo_Game &GamesModel::getGame(int row)
{
    Q_ASSERT(row < gameList.size());
    return gameList[row];
}

void GamesModel::updateGameList(const ServerInfo_Game &game)
{
    for (int i = 0; i < gameList.size(); i++) {
        if (gameList[i].game_id() == game.game_id()) {
            if (game.closed()) {
                beginRemoveRows(QModelIndex(), i, i);
                gameList.removeAt(i);
                endRemoveRows();
            } else {
                gameList[i].MergeFrom(game);
                emit dataChanged(index(i, 0), index(i, NUM_COLS-1));
            }
            return;
        }
    }
    if (game.player_count() <= 0)
        return;
    beginInsertRows(QModelIndex(), gameList.size(), gameList.size());
    gameList.append(game);
    endInsertRows();
}

GamesProxyModel::GamesProxyModel(QObject *parent, ServerInfo_User *_ownUser)
    : QSortFilterProxyModel(parent),
          ownUser(_ownUser),
          unavailableGamesVisible(false),
          passwordProtectedGamesVisible(false),
          maxPlayersFilterMin(-1),
          maxPlayersFilterMax(-1)
{
    setSortRole(GamesModel::SORT_ROLE);
    setDynamicSortFilter(true);
}

void GamesProxyModel::setUnavailableGamesVisible(bool _unavailableGamesVisible)
{
    unavailableGamesVisible = _unavailableGamesVisible;
    invalidateFilter();
}

void GamesProxyModel::setPasswordProtectedGamesVisible(bool _passwordProtectedGamesVisible)
{
    passwordProtectedGamesVisible = _passwordProtectedGamesVisible;
    invalidateFilter();
}

void GamesProxyModel::setGameNameFilter(const QString &_gameNameFilter)
{
    gameNameFilter = _gameNameFilter;
    invalidateFilter();
}

void GamesProxyModel::setCreatorNameFilter(const QString &_creatorNameFilter)
{
    creatorNameFilter = _creatorNameFilter;
    invalidateFilter();
}

void GamesProxyModel::setGameTypeFilter(const QSet<int> &_gameTypeFilter)
{
    gameTypeFilter = _gameTypeFilter;
    invalidateFilter();
}

void GamesProxyModel::setMaxPlayersFilter(int _maxPlayersFilterMin, int _maxPlayersFilterMax)
{
    maxPlayersFilterMin = _maxPlayersFilterMin;
    maxPlayersFilterMax = _maxPlayersFilterMax;
    invalidateFilter();
}

void GamesProxyModel::resetFilterParameters()
{
    unavailableGamesVisible = false;
    passwordProtectedGamesVisible = false;
    gameNameFilter = QString();
    creatorNameFilter = QString();
    gameTypeFilter.clear();
    maxPlayersFilterMin = -1;
    maxPlayersFilterMax = -1;

    invalidateFilter();
}

bool GamesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &/*sourceParent*/) const
{
    GamesModel *model = qobject_cast<GamesModel *>(sourceModel());
    if (!model)
        return false;

    const ServerInfo_Game &game = model->getGame(sourceRow);
    if (!unavailableGamesVisible) {
        if (game.player_count() == game.max_players())
            return false;
        if (game.started())
            return false;
        if (!(ownUser->user_level() & ServerInfo_User::IsRegistered))
            if (game.only_registered())
                return false;
    }
    if (!passwordProtectedGamesVisible && game.with_password())
        return false;
    if (!gameNameFilter.isEmpty())
        if (!QString::fromStdString(game.description()).contains(gameNameFilter, Qt::CaseInsensitive))
            return false;
    if (!creatorNameFilter.isEmpty())
        if (!QString::fromStdString(game.creator_info().name()).contains(creatorNameFilter, Qt::CaseInsensitive))
            return false;

    QSet<int> gameTypes;
    for (int i = 0; i < game.game_types_size(); ++i)
        gameTypes.insert(game.game_types(i));
    if (!gameTypeFilter.isEmpty() && gameTypes.intersect(gameTypeFilter).isEmpty())
        return false;

    if ((maxPlayersFilterMin != -1) && ((int)game.max_players() < maxPlayersFilterMin))
        return false;
    if ((maxPlayersFilterMax != -1) && ((int)game.max_players() > maxPlayersFilterMax))
        return false;

    return true;
}
