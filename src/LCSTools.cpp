#include "LCSTools.h"
#include "LCSToolsImpl.h"
#include <QDebug>
#include <chrono>

LCSTools::LCSTools(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<LCSState>("LCSState");
    thread_ = new QThread();
    worker_ = new LCSToolsImpl();
    worker_->moveToThread(thread_);

    connect(worker_, &LCSToolsImpl::stateChanged, this, &LCSTools::setState);
    connect(worker_, &LCSToolsImpl::statusChanged, this, &LCSTools::setStatus);
    connect(worker_, &LCSToolsImpl::leaguePathChanged, this, &LCSTools::setLeaguePath);
    connect(worker_, &LCSToolsImpl::reportWarning, this, &LCSTools::reportWarning);
    connect(worker_, &LCSToolsImpl::reportError, this, &LCSTools::reportError);

    connect(worker_, &LCSToolsImpl::progressStart, this, &LCSTools::progressStart);
    connect(worker_, &LCSToolsImpl::updateProgress, this, &LCSTools::updateProgress);
    connect(worker_, &LCSToolsImpl::progressEnd, this, &LCSTools::progressEnd);
    connect(worker_, &LCSToolsImpl::initialized, this, &LCSTools::initialized);
    connect(worker_, &LCSToolsImpl::modDeleted, this, &LCSTools::modDeleted);
    connect(worker_, &LCSToolsImpl::installedMod, this, &LCSTools::installedMod);
    connect(worker_, &LCSToolsImpl::profileSaved, this, &LCSTools::profileSaved);
    connect(worker_, &LCSToolsImpl::profileDeleted, this, &LCSTools::profileDeleted);
    connect(worker_, &LCSToolsImpl::updateProfileStatus, this, &LCSTools::updateProfileStatus);
    connect(worker_, &LCSToolsImpl::modEditStarted, this, &LCSTools::modEditStarted);
    connect(worker_, &LCSToolsImpl::modInfoChanged, this, &LCSTools::modInfoChanged);
    connect(worker_, &LCSToolsImpl::modImageChanged, this, &LCSTools::modImageChanged);
    connect(worker_, &LCSToolsImpl::modImageRemoved, this, &LCSTools::modImageRemoved);
    connect(worker_, &LCSToolsImpl::modWadsAdded, this, &LCSTools::modWadsAdded);
    connect(worker_, &LCSToolsImpl::modWadsRemoved, this, &LCSTools::modWadsRemoved);

    connect(this, &LCSTools::changeLeaguePath, worker_, &LCSToolsImpl::changeLeaguePath);
    connect(this, &LCSTools::init, worker_, &LCSToolsImpl::init);
    connect(this, &LCSTools::deleteMod, worker_, &LCSToolsImpl::deleteMod);
    connect(this, &LCSTools::exportMod, worker_, &LCSToolsImpl::exportMod);
    connect(this, &LCSTools::installFantomeZip, worker_, &LCSToolsImpl::installFantomeZip);
    connect(this, &LCSTools::saveProfile, worker_, &LCSToolsImpl::saveProfile);
    connect(this, &LCSTools::deleteProfile, worker_, &LCSToolsImpl::deleteProfile);
    connect(this, &LCSTools::runProfile, worker_, &LCSToolsImpl::runProfile);
    connect(this, &LCSTools::stopProfile, worker_, &LCSToolsImpl::stopProfile);
    connect(this, &LCSTools::makeMod, worker_, &LCSToolsImpl::makeMod);
    connect(this, &LCSTools::startEditMod, worker_, &LCSToolsImpl::startEditMod);
    connect(this, &LCSTools::changeModInfo, worker_, &LCSToolsImpl::changeModInfo);
    connect(this, &LCSTools::changeModImage, worker_, &LCSToolsImpl::changeModImage);
    connect(this, &LCSTools::removeModImage, worker_, &LCSToolsImpl::removeModImage);
    connect(this, &LCSTools::addModWads, worker_, &LCSToolsImpl::addModWads);
    connect(this, &LCSTools::removeModWads, worker_, &LCSToolsImpl::removeModWads);

    connect(this, &LCSTools::destroyed, thread_, &QThread::deleteLater);

    thread_->start();
}

LCSTools::~LCSTools(){
    thread_->exit();
}

LCSToolsImpl::LCSState LCSTools::getState() {
    return state_;
}

QString LCSTools::getStatus() {
    return status_;
}

void LCSTools::setState(LCSState value) {
    if (state_ != value) {
        state_ = value;
        emit stateChanged(value);
    }
}

void LCSTools::setStatus(QString status) {
    if (status_ != status) {
        status_ = status;
        emit statusChanged(status);
    }
}

QString LCSTools::getLeaguePath() {
    return leaguePath_;
}

void LCSTools::setLeaguePath(QString value) {
    if (leaguePath_ != value) {
        leaguePath_ = value;
        leaguePathChanged(value);
    }
}
