#ifndef LIBDATAMODEL_PROPERTY_H_GUARD
#define LIBDATAMODEL_PROPERTY_H_GUARD

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

#include <functional>
#include <variant>
#include <vector>
#include <forward_list>
#include <cstdint>
#include <string>
#include <sstream>
#include <map>

namespace DataModelInterface {

class PropertySet;
class GenericProperty;

using GenericDatum = std::variant<int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t,int64_t,uint64_t,float,double,std::string>;


namespace Internal {
template <class T, class U> struct contain_type;

template <class T, class... Ts>
struct contain_type<T, std::variant<Ts...>>
: std::bool_constant<(std::is_same_v<T, Ts> || ...)>
{ };

}

template<typename T>
std::string typeDescr() {
    if constexpr (std::is_same_v<T, int8_t>) {
        return "i8";
    }
    if constexpr (std::is_same_v<T, uint8_t>) {
        return "u8";
    }
    if constexpr (std::is_same_v<T, int16_t>) {
        return "i16";
    }
    if constexpr (std::is_same_v<T, uint16_t>) {
        return "u16";
    }
    if constexpr (std::is_same_v<T, int32_t>) {
        return "i32";
    }
    if constexpr (std::is_same_v<T, uint32_t>) {
        return "u32";
    }
    if constexpr (std::is_same_v<T, int64_t>) {
        return "i64";
    }
    if constexpr (std::is_same_v<T, uint64_t>) {
        return "u64";
    }
    if constexpr (std::is_same_v<T, float>) {
        return "f";
    }
    if constexpr (std::is_same_v<T, double>) {
        return "d";
    }
    if constexpr (std::is_same_v<T, std::string>) {
        return "str";
    }
    return "invalid";
}

template <typename T>
struct StorageType {
    using Type = T;
};

/*!
 * \brief the DataProxy class is meant to serve as a proxy, which allows to build DataModelInterfaces for arbitrary classes and data layout
 */
template<typename T>
class DataProxy{
public:
    using DataType = T;
    using Setter = std::function<void(DataType const&)>;
    using Getter = std::function<DataType()>;

    DataProxy(Getter const& getter, Setter const& setter);

    inline DataType get() const {
        return _getter();
    }

    inline void set(DataType const& data) {
        _setter(data);
    }

    inline operator T() { return _getter(); }
    inline DataProxy<T>& operator=(T const& data) {
        set(data);
        return *this;
    }
    inline DataProxy<T>& operator==(T const& data) {
        DataType d = get();
        return d == data;
    }
    inline DataProxy<T>& operator!=(T const& data) {
        DataType d = get();
        return d != data;
    }

protected:

    Setter _setter;
    Getter _getter;
};

template <typename T>
struct StorageType<DataProxy<T>> {
    using Type = typename StorageType<T>::Type;
};

/*!
 * \brief The ClearNotifySlotWatcher class is an helper class for a clear slot that is ran only once
 */
class ClearNotifySlotWatcher {
public:
    using ClearT = std::function<void()>;
    ClearNotifySlotWatcher() : _clearFunc() {

    }
    ClearNotifySlotWatcher(ClearT const& func) : _clearFunc(func) {

    }

    /*!
     * \brief clear run the clearing function, and set it to an invalid state so it will not run again in the future.
     */
    void clear() {
        if (_clearFunc) {
            _clearFunc(); //clear if need be
        }
        _clearFunc = ClearT();
    }
protected:
    ClearT _clearFunc;
};

/*!
 * \brief Notifiable is an interface a class needs to implement to be able to receive notifications
 */
class Notifiable {
public:
    ~Notifiable();
    inline void connectClearWatcher(ClearNotifySlotWatcher* slot) {
        _connections.push_front(slot);
    }
    inline void disconnectClearWatcher(ClearNotifySlotWatcher* slot) {
        _connections.remove(slot);
    }
protected:

    std::forward_list<ClearNotifySlotWatcher*> _connections;
};

class ChangeTracker {
public:
    ChangeTracker(ChangeTracker* parent = nullptr) :
    _parent(parent),
    _track(false),
    _hasChanged(false) {

    }

    virtual ~ChangeTracker();

    inline bool hasChanges() const {
        return _hasChanged;
    }

protected:

    void trackChanges(bool track = true) {
        _track = track;
    }

    void notifyChanges() {
        if (_track) {
            _hasChanged = true;
            if (_parent != nullptr) {
                _parent->notifyChanges();
            }
        }
    }

    void clearChangesMarker() {
        _hasChanged = false;
    }

private:
    ChangeTracker* _parent;
    bool _track;
    bool _hasChanged;
};

class DataStructureBlock : public Notifiable, public ChangeTracker {
public:

    using Url = std::vector<std::string>;

    enum Kind {
        Property = 0,
        Set = 1
    };

    inline DataStructureBlock(DataStructureBlock* parent = nullptr) :
        ChangeTracker(parent),
        _parent(parent)
    {

    }
    virtual ~DataStructureBlock();
    inline DataStructureBlock* getParent() {
        return _parent;
    }

    virtual Kind dataStructureKind() const = 0;

    virtual std::string typeDescr() const = 0;

    virtual void commit();
    virtual bool hasUncommitedChanges();

    inline std::string const& id() {
        return _id;
    }

    inline Url getUrl() const {
        if (_id.empty()) {
            return {};
        }

        if (_parent == nullptr) {
            return {_id};
        }

        Url url = _parent->getUrl();
        url.push_back(_id);

        return url;
    }

protected:

    inline void setId(std::string const& id) {
        _id = id;
    }
    std::string _id;

private:
    DataStructureBlock* _parent;
    friend class GenericProperty;
    friend class PropertySet;
};

class GenericProperty : public DataStructureBlock {

public:
    template<typename DataT>
    using NotifySlot = std::function<void(DataT)>;

    using ConnectionId = uintptr_t;

    template<typename DataT>
    using ChangeNotifySlot = std::function<void(DataT, DataT)>;

    using GenericNotifySlot = ChangeNotifySlot<GenericDatum const&>;

    inline static std::string datumToString(GenericDatum const& datum) {
        return std::visit([] (auto d) {
            std::stringstream stream;
            stream << d;
            return stream.str();}, datum);

    }

protected:
    using SilentNotifySlot = std::function<void()>;

    struct NotifyData {
        inline NotifyData() {

        }
        inline NotifyData(GenericNotifySlot const& pSlot,
                          ClearNotifySlotWatcher::ClearT const& watcherFunc,
                          Notifiable* pTarget):
            slot(pSlot),
            watcher(watcherFunc),
            target(pTarget)
        {

        }
        GenericNotifySlot slot;
        ClearNotifySlotWatcher watcher;
        Notifiable* target;
    };

public:

    inline GenericProperty(DataStructureBlock* parent) :
        DataStructureBlock(parent)
    {

    }
    virtual ~GenericProperty();

    virtual GenericDatum data() const = 0;
    virtual void setData(GenericDatum const& data) = 0;

    virtual Kind dataStructureKind() const;

    std::string toJson() const;
    void fromJson(std::string const& jsonData);

    /*!
     * \brief connect add a slot that will be executed when the property change, with a given context
     * \param target the context object. When deleted, the slot will be removed.
     * \param slot the slot to execute when the property change
     * \return a connection id, which can be used later to refer to this specific slot.
     */
    template<typename DT>
    ConnectionId connect(Notifiable& target, NotifySlot<DT> const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _slots.emplace_front(
            [slot] (GenericDatum const& oldData, GenericDatum const& data) {
                std::visit([&slot] (auto d) {
                    using InT = std::remove_reference_t<decltype(d)>;
                    using OutT = std::remove_reference_t<DT>;
                    if constexpr (std::is_convertible_v<InT, OutT>) {
                        slot(d);
                    } else if constexpr (std::is_same_v<OutT, std::string>) {
                        std::stringstream stream;
                        stream << d;
                        slot(stream.str());
                    }

                }, data);
            },
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        NotifyData& node = _slots.front();
        node.watcher = ClearNotifySlotWatcher([&node] () -> void {
            node.slot = GenericNotifySlot(); //clear the slot when the watcher is notified that the target has been deleted.
        });
        target.connectClearWatcher(&(node.watcher));
        node.target = &target;
        return reinterpret_cast<ConnectionId>(&node);
    }

    /*!
     * \brief connect connect a slot without context
     * \param slot the slot to connect, will be executed when the data change.
     * \return
     */
    template<typename DT>
    ConnectionId connect(NotifySlot<DT> const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);;
        }

        _slots.emplace_front(
            [slot] (GenericDatum const& oldData, GenericDatum const& data) {
                std::visit([&slot] (auto d) {
                    using InT = std::remove_reference_t<decltype(d)>;
                    using OutT = std::remove_reference_t<DT>;
                    if constexpr (std::is_convertible_v<InT, OutT>) {
                        slot(d);
                    } else if constexpr (std::is_same_v<OutT, std::string>) {
                        std::stringstream stream;
                        stream << d;
                        slot(stream.str());
                    }

                }, data);
            },
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        NotifyData& node = _slots.front();
        node.target = nullptr;
        return reinterpret_cast<ConnectionId>(&node);
    }


    /*!
     * \brief connect add a slot that will be executed when the property change, with a given context
     * \param target the context object. When deleted, the slot will be removed.
     * \param slot the slot to execute when the property change
     * \return a connection id, which can be used later to refer to this specific slot.
     */
    template<typename DT>
    ConnectionId connect(Notifiable& target, ChangeNotifySlot<DT> const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _slots.emplace_front(
            [slot] (GenericDatum const& oldData, GenericDatum const& data) {
                std::visit([&slot, &oldData] (auto d) {
                    using InT = std::remove_reference_t<decltype(d)>;
                    using OutT = std::remove_reference_t<DT>;

                    OutT olddataVal = std::visit([] (auto d) -> OutT {
                        using InT = std::remove_reference_t<decltype(d)>;
                        using OutT = std::remove_reference_t<DT>;

                        if constexpr (std::is_convertible_v<InT, OutT>) {
                            return d;
                        } else if constexpr (std::is_same_v<OutT, std::string>) {
                            std::stringstream stream;
                            stream << d;
                            return stream.str();
                        }
                    }, oldData);

                    if constexpr (std::is_convertible_v<InT, OutT>) {
                        slot(olddataVal,d);
                    } else if constexpr (std::is_same_v<OutT, std::string>) {
                        std::stringstream stream;
                        stream << d;
                        slot(olddataVal, stream.str());
                    }

                }, data);
            },
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        NotifyData& node = _slots.front();
        node.watcher = ClearNotifySlotWatcher([&node] () -> void {
            node.slot = GenericNotifySlot(); //clear the slot when the watcher is notified that the target has been deleted.
        });
        target.connectClearWatcher(&(node.watcher));
        node.target = &target;
        return reinterpret_cast<ConnectionId>(&node);
    }

    /*!
     * \brief connect connect a slot without context
     * \param slot the slot to connect, will be executed when the data change.
     * \return
     */
    template<typename DT>
    ConnectionId connect(ChangeNotifySlot<DT> const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);;
        }

        _slots.emplace_front(
            [slot] (GenericDatum const& oldData, GenericDatum const& data) {
                std::visit([&slot, &oldData] (auto d) {
                    using InT = std::remove_reference_t<decltype(d)>;
                    using OutT = std::remove_reference_t<DT>;

                    OutT olddataVal = std::visit([] (auto d) -> OutT {
                        using InT = std::remove_reference_t<decltype(d)>;
                        using OutT = std::remove_reference_t<DT>;

                        if constexpr (std::is_convertible_v<InT, OutT>) {
                            return d;
                        } else if constexpr (std::is_same_v<OutT, std::string>) {
                            std::stringstream stream;
                            stream << d;
                            return stream.str();
                        }
                    }, oldData);

                    if constexpr (std::is_convertible_v<InT, OutT>) {
                        slot(olddataVal,d);
                    } else if constexpr (std::is_same_v<OutT, std::string>) {
                        std::stringstream stream;
                        stream << d;
                        slot(olddataVal, stream.str());
                    }

                }, data);
            },
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        NotifyData& node = _slots.front();
        node.target = nullptr;
        return reinterpret_cast<ConnectionId>(&node);
    }

    /*!
     * \brief disconnect all connections with a given target
     */
    void disconnect(Notifiable& target) {
        for (auto it = _slots.begin(); it != _slots.end(); ++it) {
            if (it->target == &target) {
                target.disconnectClearWatcher(&(it->watcher));
            }
        }
        _slots.remove_if([&target] (NotifyData const& data){return data.target == &target;});
    }

    /*!
     * \brief disconnect connection with a given id
     */
    void disconnect(ConnectionId id) {
        for (auto it = _slots.begin(); it != _slots.end(); ++it) {
            if (reinterpret_cast<ConnectionId>(&(*it)) == id) {
                if (it->target != nullptr) {
                    it->target->disconnectClearWatcher(&(it->watcher));
                }
                break;
            }
        }
        _slots.remove_if([id] (NotifyData const& data) -> bool {return reinterpret_cast<ConnectionId>(&data) == id;});
    }

    /*!
     * \brief gives you a Property that is just a proxy of the original property
     */
    GenericProperty getProxyProperty(ChangeTracker* changeTrackParent);
    /*!
     * \brief gives you a Property that sync with the original, but only when explicitly commited.
     */
    GenericProperty getUnsyncedProxyProperty(ChangeTracker* changeTrackParent);

protected:

    std::forward_list<NotifyData> _slots;

    void notify(GenericDatum const& oldData);

    bool _isStored;

};

template<typename T>
class Property : public GenericProperty {
public:

    static_assert(Internal::contain_type<T,GenericDatum>::value, "Data type for property is not supported by the library");

    using DataType = T;
    using DataStorageType = typename StorageType<DataType>::Type;

    Property(DataStructureBlock* parent) :
        GenericProperty(parent)
    {

    }

    virtual std::string typeDescr() const override {
        if constexpr (std::is_same_v<int8_t,DataStorageType>) {
            return "i8";
        }
        if constexpr (std::is_same_v<uint8_t,DataStorageType>) {
            return "u8";
        }
        if constexpr (std::is_same_v<int16_t,DataStorageType>) {
            return "i16";
        }
        if constexpr (std::is_same_v<uint16_t,DataStorageType>) {
            return "u16";
        }
        if constexpr (std::is_same_v<int32_t,DataStorageType>) {
            return "i32";
        }
        if constexpr (std::is_same_v<uint32_t,DataStorageType>) {
            return "u32";
        }
        if constexpr (std::is_same_v<int64_t,DataStorageType>) {
            return "i64";
        }
        if constexpr (std::is_same_v<uint64_t,DataStorageType>) {
            return "u64";
        }
        if constexpr (std::is_same_v<float,DataStorageType>) {
            return "f";
        }
        if constexpr (std::is_same_v<double,DataStorageType>) {
            return "d";
        }
        if constexpr (std::is_same_v<std::string,DataStorageType>) {
            return "str";
        }
        return "generic";
    }

    inline DataStorageType get() const {
        return _data;
    }
    inline void set(DataStorageType const& data) {
        if (_data != data) {
            GenericDatum oldDat = _data;
            _data = data;
            notify(oldDat);
            notifyChanges();
        }

    }

    virtual GenericDatum data() const override {
        return get();
    }
    virtual void setData(GenericDatum const& data) override {

        std::visit([this] (auto d) {
            using InT = std::remove_reference_t<decltype(d)>;
            using OutT = std::remove_reference_t<DataStorageType>;
            if constexpr (std::is_convertible_v<InT, OutT>) {
                set(d);
            } else {
                //try some possible conversions
                if constexpr (std::is_convertible_v<InT, std::string>) {
                    if constexpr (std::is_integral_v<OutT>) {
                        if constexpr (sizeof(OutT) >= sizeof(long)) {
                            set(std::stoll(d));
                        } else if constexpr (sizeof(OutT) >= sizeof(int)) {
                            set(std::stol(d));
                        } else {
                            set(std::stoi(d));
                        }
                    } else if (std::is_floating_point_v<OutT>) {
                        if constexpr (sizeof(OutT) >= sizeof(long double)) {
                            set(std::stold(d));
                        } else if constexpr (sizeof(OutT) >= sizeof(double)) {
                            set(std::stod(d));
                        } else {
                            set(std::stof(d));
                        }
                    }
                } else if constexpr (std::is_convertible_v<std::string, OutT>) {
                    std::stringstream stream;
                    stream << d;
                    set(stream.str());
                }
            }

        }, data);

    }

protected:

    DataType _data;

};

struct ChangeRecord;

class PropertySet : public DataStructureBlock {

public:

    using NotifySlot = std::function<void(Url const&, DataStructureBlock const*)>;
    template<typename DT>
    using DataNotifySlot = std::function<void(Url const&, DT const& propData)>;
    template<typename DT>
    using DataChangeNotifySlot = std::function<void(Url const&, DT const& oldPropData, DT const& propData)>;


    using GenericChangeNotifySlot = DataChangeNotifySlot<GenericDatum const&>;

    using ConnectionId = uintptr_t;

protected:

    struct NotifyData {
        inline NotifyData() {

        }
        inline NotifyData(NotifySlot const& pSlot,
                          ClearNotifySlotWatcher::ClearT const& watcherFunc,
                          Notifiable* pTarget):
            slot(pSlot),
            watcher(watcherFunc),
            target(pTarget)
        {

        }
        NotifySlot slot;
        ClearNotifySlotWatcher watcher;
        Notifiable* target;
    };

    struct ChangeNotifyData {
        inline ChangeNotifyData() {

        }
        inline ChangeNotifyData(GenericChangeNotifySlot const& pSlot,
                          ClearNotifySlotWatcher::ClearT const& watcherFunc,
                          Notifiable* pTarget):
            slot(pSlot),
            watcher(watcherFunc),
            target(pTarget)
        {

        }
        GenericChangeNotifySlot slot;
        ClearNotifySlotWatcher watcher;
        Notifiable* target;
    };
public:

    PropertySet(DataStructureBlock* parent = nullptr):
        DataStructureBlock(parent)
    {

    }
    virtual ~PropertySet();

    virtual Kind dataStructureKind() const override;

    virtual std::string typeDescr() const override;

    inline std::vector<std::string> keys() const {
        std::vector<std::string> ret;
        ret.reserve(_data.size());

        for (auto&[key, val] : _data) {
            ret.push_back(key);
        }

        return ret;
    }

    inline bool contains(std::string const& key) const {
        return _data.count(key) > 0;
    }

    std::string toJson() const;
    void fromJson(std::string const& jsonData);

    inline DataStructureBlock* block(std::string const& name) {
        if (_data.count(name) > 0) {
            return _data[name];
        }
        return nullptr;
    }
    inline DataStructureBlock const* block(std::string const& name) const {
        if (_data.count(name) > 0) {
            return _data.at(name);
        }
        return nullptr;
    }

    inline DataStructureBlock* block(Url const& url) {
        if (url.size() <= 0) {
            return nullptr;
        }

        if (url.size() == 1) {
            return block(url[0]);
        }

        std::string head = url[0];
        std::string const* tail = &(url[1]);

        DataStructureBlock* subblock = block(head);
        if (subblock == nullptr) {
            return nullptr;
        }

        if(subblock->dataStructureKind() == Property) {
            return nullptr; //cannot go deeper
        }

        PropertySet* set = static_cast<PropertySet*>(subblock);

        return set->block(tail,url.size()-1);
    }
    inline DataStructureBlock const* subblock(Url const& url) const {
        if (url.size() <= 0) {
            return nullptr;
        }

        if (url.size() == 1) {
            return block(url[0]);
        }

        std::string head = url[0];
        std::string const* tail = &(url[1]);

        DataStructureBlock const* subblock = block(head);
        if (subblock == nullptr) {
            return nullptr;
        }

        if(subblock->dataStructureKind() == Property) {
            return nullptr; //cannot go deeper
        }

        PropertySet const* set = static_cast<PropertySet const*>(subblock);

        return set->block(tail,url.size()-1);

    }

    inline DataStructureBlock* block(int idx) {
        if (idx < _blocks.size() and idx >= 0) {
            return _blocks[idx];
        }
        return nullptr;
    }

    inline DataStructureBlock const* block(int idx) const {
        if (idx < _blocks.size() and idx >= 0) {
            return _blocks[idx];
        }
        return nullptr;
    }

    virtual void addBlock(std::string const& name, DataStructureBlock* block);
    virtual void clearBlock(std::string const& name, bool deleteBlock = true);

    void clear(bool deleteBlock = true);

    /*!
     * \brief duplicateTo duplicate the data into a different set
     * \param other the other set.
     */
    virtual void duplicateTo(PropertySet* other);
    /*!
     * \brief isSimilarTo check if two properties sets contain similar data
     * \param other the set to compare to
     * \return true if the sets are similar, false otherwise
     *
     * two sets are assumed to be simular if the have the same properties, and the data of these properties compare equal to one another
     */
    bool isSimilarTo(PropertySet* other);

    void apply(ChangeRecord const& action); //apply an action that has been recorded, this is meant mainly to implement undo/redo mechanism

    template <typename DT>
    inline ConnectionId connectChangeWatcher(Notifiable& target, DataNotifySlot<DT> const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _changeSlots.emplace_front(
            [slot] (Url const& url, GenericDatum const& oldData, GenericDatum const& data) {
                std::visit([&slot, &oldData, &url] (auto d) {
                    using InT = std::remove_reference_t<decltype(d)>;
                    using OutT = std::remove_reference_t<DT>;
                    if constexpr (std::is_convertible_v<InT, OutT>) {
                        slot(url, d);
                    } else if constexpr (std::is_same_v<OutT, std::string>) {
                        std::stringstream stream;
                        stream << d;
                        slot(url, stream.str());
                    }
                }, data);
            },
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        ChangeNotifyData& node = _changeSlots.front();
        node.watcher = ClearNotifySlotWatcher([&node] () -> void {
            node.slot = GenericChangeNotifySlot(); //clear the slot when the watcher is notified that the target has been deleted.
        });
        target.connectClearWatcher(&(node.watcher));
        node.target = &target;
        return reinterpret_cast<ConnectionId>(&node);
    }

    template <typename DT>
    inline ConnectionId connectChangeWatcher(DataNotifySlot<DT> const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _changeSlots.emplace_front(
            [slot] (Url const& url, GenericDatum const& oldData, GenericDatum const& data) {
                std::visit([&slot, &oldData, &url] (auto d) {
                    using InT = std::remove_reference_t<decltype(d)>;
                    using OutT = std::remove_reference_t<DT>;
                    if constexpr (std::is_convertible_v<InT, OutT>) {
                        slot(url, d);
                    } else if constexpr (std::is_same_v<OutT, std::string>) {
                        std::stringstream stream;
                        stream << d;
                        slot(url, stream.str());
                    }
                }, data);
            },
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        ChangeNotifyData& node = _changeSlots.front();
        node.target = nullptr;
        return reinterpret_cast<ConnectionId>(&node);
    }

    template <typename DT>
    inline ConnectionId connectChangeWatcher(Notifiable& target, DataChangeNotifySlot<DT> const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _changeSlots.emplace_front(
            [slot] (Url const& url, GenericDatum const& oldData, GenericDatum const& data) {
                std::visit([&slot, &oldData, &url] (auto d) {
                    using InT = std::remove_reference_t<decltype(d)>;
                    using OutT = std::remove_reference_t<DT>;

                    OutT olddataVal = std::visit([] (auto d) -> OutT {
                        using InT = std::remove_reference_t<decltype(d)>;
                        using OutT = std::remove_reference_t<DT>;

                        if constexpr (std::is_convertible_v<InT, OutT>) {
                            return d;
                        } else if constexpr (std::is_same_v<OutT, std::string>) {
                            std::stringstream stream;
                            stream << d;
                            return stream.str();
                        }
                    }, oldData);

                    if constexpr (std::is_convertible_v<InT, OutT>) {
                        slot(url, olddataVal,d);
                    } else if constexpr (std::is_same_v<OutT, std::string>) {
                        std::stringstream stream;
                        stream << d;
                        slot(url, olddataVal, stream.str());
                    }
                }, data);
            },
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        ChangeNotifyData& node = _changeSlots.front();
        node.watcher = ClearNotifySlotWatcher([&node] () -> void {
            node.slot = GenericChangeNotifySlot(); //clear the slot when the watcher is notified that the target has been deleted.
        });
        target.connectClearWatcher(&(node.watcher));
        node.target = &target;
        return reinterpret_cast<ConnectionId>(&node);
    }

    template <typename DT>
    inline ConnectionId connectChangeWatcher(DataChangeNotifySlot<DT> const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _changeSlots.emplace_front(
            [slot] (Url const& url, GenericDatum const& oldData, GenericDatum const& data) {
                std::visit([&slot, &oldData, &url] (auto d) {
                    using InT = std::remove_reference_t<decltype(d)>;
                    using OutT = std::remove_reference_t<DT>;

                    OutT olddataVal = std::visit([] (auto d) -> OutT {
                        using InT = std::remove_reference_t<decltype(d)>;
                        using OutT = std::remove_reference_t<DT>;

                        if constexpr (std::is_convertible_v<InT, OutT>) {
                            return d;
                        } else if constexpr (std::is_same_v<OutT, std::string>) {
                            std::stringstream stream;
                            stream << d;
                            return stream.str();
                        }
                    }, oldData);

                    if constexpr (std::is_convertible_v<InT, OutT>) {
                        slot(url, olddataVal,d);
                    } else if constexpr (std::is_same_v<OutT, std::string>) {
                        std::stringstream stream;
                        stream << d;
                        slot(url, olddataVal, stream.str());
                    }
                }, data);
            },
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        ChangeNotifyData& node = _changeSlots.front();
        node.target = nullptr;
        return reinterpret_cast<ConnectionId>(&node);
    }

    inline ConnectionId connectPropertyClearWatcher(Notifiable& target, NotifySlot const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _clearSlots.emplace_front(
            slot,
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        NotifyData& node = _clearSlots.front();
        node.watcher = ClearNotifySlotWatcher([&node] () -> void {
            node.slot = NotifySlot(); //clear the slot when the watcher is notified that the target has been deleted.
        });
        target.connectClearWatcher(&(node.watcher));
        node.target = &target;
        return reinterpret_cast<ConnectionId>(&node);
    }

    inline ConnectionId connectPropertyClearWatcher(NotifySlot const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _clearSlots.emplace_front(
            slot,
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        NotifyData& node = _clearSlots.front();
        node.target = nullptr;
        return reinterpret_cast<ConnectionId>(&node);
    }

    inline ConnectionId connectPropertyInsertWatcher(Notifiable& target, NotifySlot const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _insertSlots.emplace_front(
            slot,
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        NotifyData& node = _insertSlots.front();
        node.watcher = ClearNotifySlotWatcher([&node] () -> void {
            node.slot = NotifySlot(); //clear the slot when the watcher is notified that the target has been deleted.
        });
        target.connectClearWatcher(&(node.watcher));
        node.target = &target;
        return reinterpret_cast<ConnectionId>(&node);
    }

    inline ConnectionId connectPropertyInsertWatcher(NotifySlot const& slot) {

        if (!slot) {
            return reinterpret_cast<ConnectionId>(nullptr);
        }

        _insertSlots.emplace_front(
            slot,
            ClearNotifySlotWatcher::ClearT(),
            nullptr);
        NotifyData& node = _insertSlots.front();
        node.target = nullptr;
        return reinterpret_cast<ConnectionId>(&node);
    }

    /*!
     * encode a url to a form with no space that can be decoded back
     */
    static std::string urlEncode(Url const& url);
    static Url urlDecode(std::string const& str);
protected:

    DataStructureBlock* block(std::string const* url, int count) {
        if (count <= 0) {
            return nullptr;
        }

        if (count == 1) {
            return block(url[0]);
        }

        std::string head = url[0];
        std::string const* tail = url+1;

        DataStructureBlock* subblock = block(head);
        if (subblock == nullptr) {
            return nullptr;
        }

        if(subblock->dataStructureKind() == Property) {
            return nullptr; //cannot go deeper
        }

        PropertySet* set = static_cast<PropertySet*>(subblock);

        return set->block(tail,count-1);
    }

    DataStructureBlock const* block(std::string const* url, int count) const {
        if (count <= 0) {
            return nullptr;
        }

        if (count == 1) {
            return block(url[0]);
        }

        std::string head = url[0];
        std::string const* tail = url+1;

        DataStructureBlock const* subblock = block(head);
        if (subblock == nullptr) {
            return nullptr;
        }

        if(subblock->dataStructureKind() == Property) {
            return nullptr; //cannot go deeper
        }

        PropertySet const* set = static_cast<PropertySet const*>(subblock);

        return set->block(tail,count-1);
    }

    std::vector<DataStructureBlock*> _blocks;
    std::map<std::string, DataStructureBlock*> _data;

    std::forward_list<ChangeNotifyData> _changeSlots;
    std::forward_list<NotifyData> _clearSlots;
    std::forward_list<NotifyData> _insertSlots;

    inline void notifyChanged(Url const& itemChangedKey, GenericDatum const& oldData, GenericDatum const& data) {
        _changeSlots.remove_if([] (ChangeNotifyData const& data) -> bool {return !bool(data.slot);});

        DataStructureBlock const* dataBlock = block(itemChangedKey);

        for (ChangeNotifyData& slot : _changeSlots) {
            if (slot.slot) {
                slot.slot(itemChangedKey, oldData, data);
            }
        }

        DataStructureBlock* p = getParent();

        if (p == nullptr) {
            return;
        }

        Kind k = p->dataStructureKind();

        if (k == Set) {
            PropertySet* p_set = static_cast<PropertySet*>(p);

            if (p_set == nullptr) {
                return;
            }

            std::string currentId = id();

            DataStructureBlock* b = p_set->block(currentId);

            if (b != this) { //the block has not yet been inserted
                return;
            }

            Url extended(itemChangedKey.size()+1);
            extended[0] = currentId;
            for (int i = 0; i < itemChangedKey.size(); i++) {
                extended[i+1] = itemChangedKey[i];
            }


            p_set->notifyChanged(extended, oldData, data);
        }


    }

    inline void notifyClear(std::string const& itemAboutToBeClearedKey) {
        _clearSlots.remove_if([] (NotifyData const& data) -> bool {return !bool(data.slot);});

        DataStructureBlock const* dataBlock = block(itemAboutToBeClearedKey);

        for (NotifyData& slot : _clearSlots) {
            if (slot.slot) {
                slot.slot({itemAboutToBeClearedKey}, dataBlock);
            }
        }

        DataStructureBlock* p = getParent();

        if (p == nullptr) {
            return;
        }

        Kind k = p->dataStructureKind();

        if (k == Set) {
            PropertySet* p_set = static_cast<PropertySet*>(p);

            if (p_set == nullptr) {
                return;
            }

            p_set->notifyClear({id(), itemAboutToBeClearedKey});
        }
    }

    inline void notifyInserted(std::string const& itemInsertedKey) {
        _insertSlots.remove_if([] (NotifyData const& data) -> bool {return !bool(data.slot);});

        DataStructureBlock const* dataBlock = block(itemInsertedKey);

        for (NotifyData& slot : _insertSlots) {
            if (slot.slot) {
                slot.slot({itemInsertedKey}, dataBlock);
            }
        }

        DataStructureBlock* p = getParent();

        if (p == nullptr) {
            return;
        }

        Kind k = p->dataStructureKind();

        if (k == Set) {
            PropertySet* p_set = static_cast<PropertySet*>(p);

            if (p_set == nullptr) {
                return;
            }

            p_set->notifyInserted({id(), itemInsertedKey});
        }
    }

    inline void notifyClear(Url const& itemAboutToBeClearedKey) {
        _clearSlots.remove_if([] (NotifyData const& data) -> bool {return !bool(data.slot);});

        DataStructureBlock const* dataBlock = block(itemAboutToBeClearedKey);

        for (NotifyData& slot : _clearSlots) {
            if (slot.slot) {
                slot.slot(itemAboutToBeClearedKey, dataBlock);
            }
        }

        DataStructureBlock* p = getParent();

        if (p == nullptr) {
            return;
        }

        Kind k = p->dataStructureKind();

        if (k == Set) {
            PropertySet* p_set = static_cast<PropertySet*>(p);

            if (p_set == nullptr) {
                return;
            }

            Url extended(itemAboutToBeClearedKey.size()+1);
            extended[0] = id();
            for (int i = 0; i < itemAboutToBeClearedKey.size(); i++) {
                extended[i+1] = itemAboutToBeClearedKey[i];
            }

            p_set->notifyClear(extended);
        }
    }

    inline void notifyInserted(Url const& itemInsertedKey) {
        _insertSlots.remove_if([] (NotifyData const& data) -> bool {return !bool(data.slot);});

        DataStructureBlock const* dataBlock = block(itemInsertedKey);

        for (NotifyData& slot : _insertSlots) {
            if (slot.slot) {
                slot.slot(itemInsertedKey, dataBlock);
            }
        }

        DataStructureBlock* p = getParent();

        if (p == nullptr) {
            return;
        }

        Kind k = p->dataStructureKind();

        if (k == Set) {
            PropertySet* p_set = static_cast<PropertySet*>(p);

            if (p_set == nullptr) {
                return;
            }

            Url extended(itemInsertedKey.size()+1);
            extended[0] = id();
            for (int i = 0; i < itemInsertedKey.size(); i++) {
                extended[i+1] = itemInsertedKey[i];
            }

            p_set->notifyInserted(extended);
        }
    }

    friend class GenericProperty;

};

inline DataStructureBlock* buildTypedPropertyFromTypeDescr(DataStructureBlock* parent, std::string const& descr) {

    if (descr == "i8") {
        return new Property<int8_t>(parent);
    }
    if (descr == "u8") {
        return new Property<uint8_t>(parent);
    }
    if (descr == "i16") {
        return new Property<int16_t>(parent);
    }
    if (descr == "u16") {
        return new Property<uint16_t>(parent);
    }
    if (descr == "i32") {
        return new Property<int32_t>(parent);
    }
    if (descr == "u32") {
        return new Property<uint32_t>(parent);
    }
    if (descr == "i64") {
        return new Property<int64_t>(parent);
    }
    if (descr == "u64") {
        return new Property<uint64_t>(parent);
    }
    if (descr == "f") {
        return new Property<float>(parent);
    }
    if (descr == "d") {
        return new Property<double>(parent);
    }
    if (descr == "str") {
        return new Property<std::string>(parent);
    }
    if (descr == "set") {
        return new PropertySet(parent);
    }
    return nullptr;
}

struct ChangeRecord {
    enum Action {
        None = 0,
        Set = 1,
        Insert = 2,
        Remove = 3
    };
    DataStructureBlock::Url url; //the url the action took place at
    std::string index; //the element impacted (when editing a set or array)
    Action action; //the type of action
    std::string dataRep; //representation of the data

    static char actionToChar(Action act) {
        switch (act) {
        case None:
            return 'N';
        case Set:
            return 'S';
        case Insert:
            return 'I';
        case Remove:
            return 'R';
        }
        return 'N';
    }

    static Action charToAction(char act) {
        switch (act) {
        case 'N':
            return None;
        case 'S':
            return Set;
        case 'I':
            return Insert;
        case 'R':
            return Remove;
        }
        return None;
    }

    template<typename OutStreamT>
    OutStreamT& toStream(OutStreamT& out) const {
        if (!url.empty()) {
            out << PropertySet::urlEncode(url);
        }
        out << '\n';
        if (!index.empty()) {
            out << index;
        }
        out << '\n';
        out << actionToChar(action) << '\n';
        if (!dataRep.empty()) {
            out << dataRep;
        }

    }
    template<typename InStream>
    static ChangeRecord fromStream(InStream & in) {
        std::string urlStr;
        std::string index;
        char action;
        std::string dataRep;
        in >> urlStr;
        in >> index;
        in >> action;
        in >> dataRep;
        return ChangeRecord{PropertySet::urlDecode(urlStr), index, charToAction(action), dataRep};
    }

    bool apply(DataStructureBlock* block) const {

        if (block == nullptr) {
            return false;
        }

        switch (action) {
        case None:
            return true;
        case Set:
            return applySet(block);
        case Insert:
            return applyInsert(block);
        case Remove:
            return applyRemove(block);
        }

    }

protected:

    inline bool applySet(DataStructureBlock* block) const {

        DataStructureBlock* target = block;

        if (!url.empty()) {

            if (block->dataStructureKind() != DataStructureBlock::Set) {
                return false;
            }

            PropertySet* set = static_cast<PropertySet*>(block);

            target = set->block(url);
        }

        if (target == nullptr) {
            return false;
        }

        if (target->dataStructureKind() != DataStructureBlock::Property) {
            return false;
        }

        GenericProperty* prop = static_cast<GenericProperty*>(target);

        GenericDatum data = dataRep;

        prop->setData(data);
        return true;

    }

    inline bool applyInsert(DataStructureBlock* block) const {

        DataStructureBlock* target = block;

        if (!url.empty()) {

            if (block->dataStructureKind() != DataStructureBlock::Set) {
                return false;
            }

            PropertySet* set = static_cast<PropertySet*>(block);

            target = set->block(url);
        }

        if (target == nullptr) {
            return false;
        }

        if (target->dataStructureKind() != DataStructureBlock::Set) {
            return false;
        }

        PropertySet* set = static_cast<PropertySet*>(target);

        DataStructureBlock* b = set->block(index);

        if (b != nullptr) { //cannot insert a block if a property exist already
            return false;
        }

        DataStructureBlock* newBlock = buildTypedPropertyFromTypeDescr(set, dataRep);

        if (newBlock == nullptr) {
            return false;
        }

        set->addBlock(index, newBlock);
        return true;

    }

    inline bool applyRemove(DataStructureBlock* block) const {

        DataStructureBlock* target = block;

        if (!url.empty()) {

            if (block->dataStructureKind() != DataStructureBlock::Set) {
                return false;
            }

            PropertySet* set = static_cast<PropertySet*>(block);

            target = set->block(url);
        }

        if (target == nullptr) {
            return false;
        }

        if (target->dataStructureKind() != DataStructureBlock::Set) {
            return false;
        }

        PropertySet* set = static_cast<PropertySet*>(target);

        DataStructureBlock* b = set->block(index);

        if (b == nullptr) {
            return false;
        }

        constexpr bool deleteBlock = true;
        set->clearBlock(index, deleteBlock);
        return true;
    }

};


PropertySet* buildUnsyncedProxyPropertySet(PropertySet* sourceSet, DataStructureBlock *parent = nullptr);
GenericProperty* buildUnsyncedProxyProperty(GenericProperty* sourceProperty, DataStructureBlock *parent = nullptr);

}

#endif // LIBDATAMODEL_PROPERTY_H_GUARD
