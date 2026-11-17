#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

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
#include "./changerecorder.h"

namespace DataModelInterface {

/*!
 * \brief connectChangeWatcherToStream connect a change recorder such that every edit recorded in automatically published on a given stream
 * \param stream the stream the changes will be published to, the reference to this string shall remain valid as long as the change recorder exist, or the connection is active
 * \param changed_recorder the change recorder to listen to
 * \return the id of the connection, so it is possible to manually disconnect it later.
 */
template<typename StreamT>
uintptr_t connectChangeWatcherToStream(StreamT & stream, ChangeRecorder & changed_recorder) {

    return changed_recorder.registerCallback([&stream] (ChangeRecorder const* recorder,
                                                       ChangeRecorder::ChangeRecordInfo const& changeRecordInfos) {
        for (ChangeRecord const& changeRecord : changeRecordInfos.redo) {
            changeRecord.toStream(stream);
        }
    });
}

/*!
 * \brief pullRemoteChangesToPropertySet read changes from a stream and apply them on a given set
 * \param stream the stream to read the changes from
 * \param set the set to apply the changes onto
 * \return true if sucessfull, false otherwise.
 */
template<typename StreamT>
bool pullRemoteChangesToPropertySet(StreamT & stream, PropertySet* set) {

    std::vector<ChangeRecord> changes;
    changes.reserve(10); //have a reasonable reserve to start

    do {
        ChangeRecord cr = ChangeRecord::fromStream(stream);

        if (!cr.isValid()) {
            break;
        }

        changes.push_back(cr);
    } while (true);

    bool ok = true;

    for (int i = 0; i < changes.size(); i++) {

        bool isOverriden = false;

        for (int j = i+1; j < changes.size(); j++) {
            if (changes[j].doesOverride(changes[i])) {
                isOverriden = true;
                break;
            }
        }

        if (isOverriden) {
            continue;
        }

        ok = changes[i].apply(set) and ok; //need apply change first to avoid problem with short circuit
    }

    return ok;
}

} // namespace DataModelInterface

#endif // SYNCHRONIZATION_H
