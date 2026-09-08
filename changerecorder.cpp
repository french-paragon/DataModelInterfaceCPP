/*
 *  This file is part of LibDataModelInterface, a library for structured dataset management in c++.
 *
 *  Copyright (C) 2026  Paragon<french.paragon@gmail.com>
 *
 *  LibDataModelInterface is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation,
 *  either version 3 of the License, or (at your option) any later version.
 *
 *  LibDataModelInterface is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with LibDataModelInterface. If not, see <https://www.gnu.org/licenses/>.
 */

#include "changerecorder.h"

namespace DataModelInterface {


ChangeRecorder::ChangeRecorder() :
    _target(nullptr),
    _maxChangesRecorded(0)
{

}

ChangeRecorder::ChangeRecorder(DataStructureBlock *target) :
    _target(target),
    _maxChangesRecorded(64),
    _clearWatcher([this] () {
        _target = nullptr;
    })
{
    if (target == nullptr) {
        return;
    }

    _target->connectClearWatcher(&_clearWatcher);

    DataStructureBlock::Kind kind = _target->dataStructureKind();

    if (kind == DataStructureBlock::Property) {

        GenericProperty* prop = static_cast<GenericProperty*>(_target);

        prop->connect<std::string>([this] (std::string const& oldDataRep, std::string const& newDataRep) {
            ChangeRecordInfo record;
            record.redo = {ChangeRecord{.url={}, .index="", .action=ChangeRecord::Set, .dataRep=newDataRep}};
            record.undo = {ChangeRecord{.url={}, .index="", .action=ChangeRecord::Set, .dataRep=oldDataRep}};
            addRecord(record);
        });

    } else if (kind == DataStructureBlock::Set) {

        PropertySet* set = static_cast<PropertySet*>(_target);

        set->connectChangeWatcher<std::string>([this] (PropertySet::Url const& url, std::string const& oldDataRep, std::string const& newDataRep) {
            ChangeRecordInfo record;
            record.redo = {ChangeRecord{.url=url, .index="", .action=ChangeRecord::Set, .dataRep=newDataRep}};
            record.undo = {ChangeRecord{.url=url, .index="", .action=ChangeRecord::Set, .dataRep=oldDataRep}};
            addRecord(record);
        });

        set->connectPropertyClearWatcher([this] (PropertySet::Url const& url, DataStructureBlock const* block) {
            ChangeRecordInfo record;

            std::string ref = url.back();
            PropertySet::Url parentUrl = {};

            if (url.size() > 1) {
                parentUrl.resize(url.size()-1);
                for (int i = 0; i < url.size()-1; i++) {
                    parentUrl[i] = url[i];
                }
            }

            record.redo = {ChangeRecord{.url=parentUrl, .index=ref, .action=ChangeRecord::Remove, .dataRep=""}};
            record.undo = blockCreationRecord(parentUrl, ref, block);
            addRecord(record);
        });

        set->connectPropertyInsertWatcher([this] (PropertySet::Url const& url, DataStructureBlock const* block) {
            ChangeRecordInfo record;

            std::string ref = url.back();
            PropertySet::Url parentUrl = {};

            if (url.size() > 1) {
                parentUrl.resize(url.size()-1);
                for (int i = 0; i < url.size()-1; i++) {
                    parentUrl[i] = url[i];
                }
            }

            record.redo = blockCreationRecord(parentUrl, ref, block);
            record.undo = {ChangeRecord{.url=parentUrl, .index=ref, .action=ChangeRecord::Remove, .dataRep=""}};
            addRecord(record);
        });

    }

}
ChangeRecorder::~ChangeRecorder() {
    for (CallBack* callBack : _callBacks) {
        delete callBack;
    }
}

uintptr_t ChangeRecorder::registerCallback(CallBack const& callback) {

    CallBack* callBackPtr = new CallBack(callback);
    _callBacks.push_back(callBackPtr);
    return reinterpret_cast<uintptr_t>(callBackPtr);
}
void ChangeRecorder::clearCallback(uintptr_t id) {
    CallBack* ptr = reinterpret_cast<CallBack*>(id);

    delete ptr;

    for (int i = 0; i < _callBacks.size(); i++) {
        if (_callBacks[i] == ptr) {
            _callBacks.erase(_callBacks.begin()+i);
            return;
        }
    }
}
void ChangeRecorder::addRecord(ChangeRecordInfo const& record) {

    while (_recordedChanges.size() >= _maxChangesRecorded) {
        _recordedChanges.pop_front();
    }

    _recordedChanges.push_back(record);

}

std::vector<ChangeRecord> ChangeRecorder::blockCreationRecord(PropertySet::Url const& url, std::string const& blockRef, DataStructureBlock const* block) {

    std::vector<ChangeRecord> ret;
    ret.push_back(ChangeRecord{.url=url, .index=blockRef, .action=ChangeRecord::Insert, .dataRep=block->typeDescr()});

    DataStructureBlock::Kind kind = block->dataStructureKind();

    PropertySet::Url sub_url = url;
    sub_url.push_back(blockRef);

    if (kind == DataStructureBlock::Property) {

        GenericProperty const* prop = static_cast<GenericProperty const*>(block);

        std::string strRep = GenericProperty::datumToString(prop->data());
        ret.push_back(ChangeRecord{.url=sub_url, .index="", .action=ChangeRecord::Set, .dataRep=strRep});

    } else {

        PropertySet const* set = static_cast<PropertySet const*>(block);

        std::vector<std::string> keys = set->keys();

        for (std::string const& key : keys) {
            DataStructureBlock const* subBlock = set->block(key);
            std::vector<ChangeRecord> keyChangeRecords = blockCreationRecord(sub_url, key, subBlock);

            for (ChangeRecord const& changeRecord : keyChangeRecords) {
                ret.push_back(changeRecord);
            }
        }

    }

    return ret;

}

} // namespace DataModelInterface
