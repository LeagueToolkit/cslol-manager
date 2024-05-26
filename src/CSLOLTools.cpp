#include <QDebug>
#include <chrono>

#include "CSLOLTools.h"
#include "CSLOLToolsImpl.h"

CSLOLTools::CSLOLTools(QObject *parent) : QObject(parent) {
    qRegisterMetaType<CSLOLState>("CSLOLState");
    thread_ = new QThread();
    worker_ = new CSLOLToolsImpl();
    worker_->moveToThread(thread_);

    connect(worker_, &CSLOLToolsImpl::stateChanged, this, &CSLOLTools::setState);
    connect(worker_, &CSLOLToolsImpl::statusChanged, this, &CSLOLTools::setStatus);
    connect(worker_, &CSLOLToolsImpl::leaguePathChanged, this, &CSLOLTools::setLeaguePath);
    connect(worker_, &CSLOLToolsImpl::reportError, this, &CSLOLTools::reportError);

    connect(worker_, &CSLOLToolsImpl::blacklistChanged, this, &CSLOLTools::blacklistChanged);
    connect(worker_, &CSLOLToolsImpl::ignorebadChanged, this, &CSLOLTools::ignorebadChanged);
    connect(worker_, &CSLOLToolsImpl::initialized, this, &CSLOLTools::initialized);
    connect(worker_, &CSLOLToolsImpl::modDeleted, this, &CSLOLTools::modDeleted);
    connect(worker_, &CSLOLToolsImpl::installedMod, this, &CSLOLTools::installedMod);
    connect(worker_, &CSLOLToolsImpl::profileSaved, this, &CSLOLTools::profileSaved);
    connect(worker_, &CSLOLToolsImpl::profileLoaded, this, &CSLOLTools::profileLoaded);
    connect(worker_, &CSLOLToolsImpl::profileDeleted, this, &CSLOLTools::profileDeleted);
    connect(worker_, &CSLOLToolsImpl::modCreated, this, &CSLOLTools::modCreated);
    connect(worker_, &CSLOLToolsImpl::modEditStarted, this, &CSLOLTools::modEditStarted);
    connect(worker_, &CSLOLToolsImpl::modInfoChanged, this, &CSLOLTools::modInfoChanged);
    connect(worker_, &CSLOLToolsImpl::modWadsAdded, this, &CSLOLTools::modWadsAdded);
    connect(worker_, &CSLOLToolsImpl::modWadsRemoved, this, &CSLOLTools::modWadsRemoved);
    connect(worker_, &CSLOLToolsImpl::updatedMods, this, &CSLOLTools::updatedMods);

    connect(this, &CSLOLTools::changeLeaguePath, worker_, &CSLOLToolsImpl::changeLeaguePath);
    connect(this, &CSLOLTools::changeBlacklist, worker_, &CSLOLToolsImpl::changeBlacklist);
    connect(this, &CSLOLTools::changeIgnorebad, worker_, &CSLOLToolsImpl::changeIgnorebad);
    connect(this, &CSLOLTools::init, worker_, &CSLOLToolsImpl::init);
    connect(this, &CSLOLTools::deleteMod, worker_, &CSLOLToolsImpl::deleteMod);
    connect(this, &CSLOLTools::exportMod, worker_, &CSLOLToolsImpl::exportMod);
    connect(this, &CSLOLTools::installFantomeZip, worker_, &CSLOLToolsImpl::installFantomeZip);
    connect(this, &CSLOLTools::saveProfile, worker_, &CSLOLToolsImpl::saveProfile);
    connect(this, &CSLOLTools::loadProfile, worker_, &CSLOLToolsImpl::loadProfile);
    connect(this, &CSLOLTools::deleteProfile, worker_, &CSLOLToolsImpl::deleteProfile);
    connect(this, &CSLOLTools::stopProfile, worker_, &CSLOLToolsImpl::stopProfile);
    connect(this, &CSLOLTools::makeMod, worker_, &CSLOLToolsImpl::makeMod);
    connect(this, &CSLOLTools::startEditMod, worker_, &CSLOLToolsImpl::startEditMod);
    connect(this, &CSLOLTools::changeModInfo, worker_, &CSLOLToolsImpl::changeModInfo);
    connect(this, &CSLOLTools::addModWad, worker_, &CSLOLToolsImpl::addModWad);
    connect(this, &CSLOLTools::removeModWads, worker_, &CSLOLToolsImpl::removeModWads);
    connect(this, &CSLOLTools::refreshMods, worker_, &CSLOLToolsImpl::refreshMods);
    connect(this, &CSLOLTools::runDiag, worker_, &CSLOLToolsImpl::runDiag);

    connect(this, &CSLOLTools::destroyed, worker_, &CSLOLToolsImpl::deleteLater);
    connect(worker_, &CSLOLTools::destroyed, thread_, &QThread::deleteLater);

    thread_->start();
}

CSLOLTools::~CSLOLTools() {}

CSLOLToolsImpl::CSLOLState CSLOLTools::getState() { return state_; }

QString CSLOLTools::getStatus() { return status_; }

void CSLOLTools::setState(CSLOLState value) {
    if (state_ != value) {
        state_ = value;
        emit stateChanged(value);
    }
}

void CSLOLTools::setStatus(QString status) {
    if (status_ != status) {
        status_ = status;
        emit statusChanged(status);
    }
}

QString CSLOLTools::getLeaguePath() { return leaguePath_; }

void CSLOLTools::setLeaguePath(QString value) {
    if (leaguePath_ != value) {
        leaguePath_ = value;
        emit leaguePathChanged(value);
    }
}
