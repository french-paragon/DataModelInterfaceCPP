#ifndef LIBDATAMODEL_CHANGERECORDER_H_GUARD
#define LIBDATAMODEL_CHANGERECORDER_H_GUARD

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

#include "./property.h"

#include <deque>

namespace DataModelInterface {

/*!
 * \brief The ChangeRecorder class record a list of actions allowing to replay them on a different dataset, or implement undo/redo mechanism easily
 */
class ChangeRecorder
{
public:

    struct ChangeRecordInfo {
        std::vector<ChangeRecord> redo;
        std::vector<ChangeRecord> undo;
    };

    using CallBack = std::function<void(ChangeRecorder const*, ChangeRecordInfo const&)>;

    explicit ChangeRecorder();
    explicit ChangeRecorder(DataStructureBlock* target);
    ~ChangeRecorder();

    uintptr_t registerCallback(CallBack const& callback);
    void clearCallback(uintptr_t id);

    inline int maxChangesRecorded() const {
        return _maxChangesRecorded;
    }

    inline void setMaxChangesRecorded(int max) {
        if (max < 1) {
            return; //invalid
        }
        _maxChangesRecorded = max;
        while (_recordedChanges.size() > _maxChangesRecorded) {
            _recordedChanges.pop_front();
        }
    }

    using HistoryRecordIterator = std::deque<ChangeRecordInfo>::const_reverse_iterator;

    /*!
     * \brief changeIteratorBegin return an interator to the top of the change stack
     * \return an interator to the top of the change stack
     */
    inline HistoryRecordIterator historyIteratorBegin() const {
        return _recordedChanges.rbegin();
    }

    /*!
     * \brief changeIteratorBegin return an interator to after the bottom of the change stack
     * \return an interator to after the bottom of the change stack
     */
    inline HistoryRecordIterator historyIteratorEnd() const {
        return _recordedChanges.rend();
    }

    using ChangeRecordIterator = std::deque<ChangeRecordInfo>::const_iterator;

    /*!
     * \brief changeIteratorBegin return an interator to the top of the change stack
     * \return an interator to the top of the change stack
     */
    inline ChangeRecordIterator changeIteratorBegin() const {
        return _recordedChanges.begin();
    }

    /*!
     * \brief changeIteratorBegin return an interator to after the bottom of the change stack
     * \return an interator to after the bottom of the change stack
     */
    inline ChangeRecordIterator changeIteratorEnd() const {
        return _recordedChanges.end();
    }

    inline int nRecordedChanges() const {
        return _recordedChanges.size();
    }

protected:

    void addRecord(ChangeRecordInfo const& record);
    //record all the actions necessary to recreate the block in its current state
    std::vector<ChangeRecord> blockCreationRecord(PropertySet::Url const& url, std::string const& blockRef, DataStructureBlock const* block);

    std::vector<CallBack*> _callBacks;

    int _maxChangesRecorded;
    std::deque<ChangeRecordInfo> _recordedChanges;

    ClearNotifySlotWatcher _clearWatcher;
    DataStructureBlock* _target;
};

} // namespace DataModelInterface

#endif // LIBDATAMODEL_CHANGERECORDER_H_GUARD
