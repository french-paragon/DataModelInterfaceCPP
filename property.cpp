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

#include "property.h"

#include <set>

namespace DataModelInterface {

Notifiable::~Notifiable() {
    for(ClearNotifySlotWatcher* slot : _connections) {
        slot->clear();
    }
}

ChangeTracker::~ChangeTracker() {

}

DataStructureBlock::~DataStructureBlock() {

}

void DataStructureBlock::commit() {
    return;
}
bool DataStructureBlock::hasUncommitedChanges() {
    return false;
}

GenericProperty::~GenericProperty() {

}

DataStructureBlock::Kind GenericProperty::dataStructureKind() const {
    return Kind::Property;
}

void GenericProperty::notify(const GenericDatum &oldData) {
    _slots.remove_if([] (NotifyData const& data) -> bool {return !bool(data.slot);});

    GenericDatum dat = data();

    for (NotifyData& slot : _slots) {
        if (slot.slot) {
            slot.slot(oldData, dat);
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

        p_set->notifyChanged({id()}, oldData, dat);
    }
}

PropertySet::~PropertySet() {
    for (auto&[key, val] : _data) {
        delete val;
    }
}

DataStructureBlock::Kind PropertySet::dataStructureKind() const {
    return Kind::Set;
}

std::string PropertySet::typeDescr() const {
    return "set";
}

void PropertySet::addBlock(std::string const& name, DataStructureBlock* property) {
    //TODO: check how to behave when assigning to an already existing property.
    if (_data.count(name) > 0) {
        constexpr bool deleteBlock = true;
        clearBlock(name, deleteBlock);
    }
    _data[name] = property;
    property->setId(name);
    notifyInserted(name);
    notifyChanges();
}
void PropertySet::clearBlock(std::string const& name, bool deleteBlock) {
    if (_data.count(name) > 0) {
        DataStructureBlock* block = _data[name];
        notifyClear(name);
        if (deleteBlock) {
            delete block;
        }
        _data.erase(name);
        notifyChanges();
    }
}
void PropertySet::clear(bool deleteBlock) {
    std::vector<std::string> props;
    props.reserve(_data.size());

    for (auto& [key, val] : _data) {
        props.push_back(key);
    }

    for (std::string const& prop : props) {
        clearBlock(prop, deleteBlock);
    }
}

void PropertySet::duplicateTo(PropertySet* other) {
    other->clear();

    std::vector<std::string> props;
    props.reserve(_data.size());

    //insert the blocks in order
    for (auto const& [k, v] : _data) {
        props.push_back(k);
    }

    for (std::string const& prop : props) {
        DataStructureBlock* b = block(prop);

        Kind k = b->dataStructureKind();

        switch (k) {
        case DataStructureBlock::Property: {
            GenericProperty* casted = static_cast<GenericProperty*>(b);
            GenericProperty* newProp = std::visit([other] (auto && data) -> GenericProperty* {
                using PropT = DataModelInterface::Property<std::remove_reference_t<decltype(data)>>;
                PropT* prop = new PropT(other);
                prop->setData(data);
                return prop;
            }, casted->data());
            other->addBlock(prop,newProp);
        }
            break;
        case DataStructureBlock::Set: {
            PropertySet* casted = static_cast<PropertySet*>(b);
            PropertySet* new_set = new PropertySet(other);
            casted->duplicateTo(new_set);
            other->addBlock(prop,new_set);
        }
            break;
        }
    }
}


bool PropertySet::isSimilarTo(PropertySet* other) {

    std::vector<std::string> self_keys = keys();
    std::vector<std::string> other_keys = other->keys();

    std::set<std::string> self_keys_set;
    std::set<std::string> other_keys_set;

    for (std::string const& key : self_keys) {
        self_keys_set.insert(key);
    }
    for (std::string const& key : other_keys) {
        other_keys_set.insert(key);
    }

    if (self_keys_set != other_keys_set) {
        return false;
    }

    for (std::string const& key : self_keys) {

        DataStructureBlock* selfPropBlock = block(key);
        DataStructureBlock* otherPropBlock = other->block(key);

        Kind selfPropKind = selfPropBlock->dataStructureKind();
        Kind otherPropKind = otherPropBlock->dataStructureKind();

        if (selfPropKind != otherPropKind) {
            return false;
        }

        if (selfPropKind == Kind::Property) {

            GenericProperty* selfProp = static_cast<GenericProperty*>(selfPropBlock);
            GenericProperty* otherProp = static_cast<GenericProperty*>(otherPropBlock);

            GenericDatum selfData = selfProp->data();
            GenericDatum otherData = otherProp->data();

            bool similar = std::visit([&otherData] (auto const& d1) {
                using T_D1 = std::remove_reference_t<decltype(d1)>;
                return std::visit([&d1] (auto const& d2) {
                    using T_D2 = std::remove_reference_t<decltype(d2)>;
                    if constexpr (std::is_convertible_v<T_D1, T_D2> or std::is_convertible_v<T_D2, T_D1>) {
                        return d1 == d2;
                    }
                    return false;
                }, otherData);
            }, selfData);

            if (!similar) {
                return false;
            }

        } else {
            PropertySet* selfPropSet = static_cast<PropertySet*>(selfPropBlock);
            PropertySet* otherPropSet = static_cast<PropertySet*>(otherPropBlock);

            bool similar = selfPropSet->isSimilarTo(otherPropSet);

            if (!similar) {
                return false;
            }
        }

    }

    return true;

}

class ProxyUnsyncedGenericProperty : public GenericProperty {
public:
    ProxyUnsyncedGenericProperty(GenericProperty* proxied, DataStructureBlock* parent = nullptr) :
        GenericProperty(parent),
        _proxied(proxied)
    {
        if (_proxied != nullptr) {
            _data = _proxied->data();
        }
        trackChanges(true);
    }

    virtual std::string typeDescr() const override {
        if (_proxied == nullptr) {
            return "invalid";
        }
        return _proxied->typeDescr();
    }

    virtual GenericDatum data() const override {
        return _data;
    }
    virtual void setData(GenericDatum const& data) override {
        if (_data != data) {
            GenericDatum oldDat = _data;
            _data = data;
            notify(oldDat);
            notifyChanges();
        }
    }

    virtual void commit() override {
        if (hasChanges()) {
            if (_proxied != nullptr) {
                _proxied->setData(_data);
            }
        }
        clearChangesMarker();
    }
    virtual bool hasUncommitedChanges() override {
        return hasChanges();
    }

protected:
    GenericProperty* _proxied;
    GenericDatum _data;
    friend class ProxyUnsyncedPropertySet;
};

class ProxyUnsyncedPropertySet : public PropertySet {
public:
    ProxyUnsyncedPropertySet(PropertySet* proxied, DataStructureBlock* parent = nullptr) :
        PropertySet(parent),
        _proxied(proxied)
    {
        trackChanges(true);
    }

    virtual ~ProxyUnsyncedPropertySet() {

    }

    virtual void commit() {

        std::vector<std::string> proxyKeys = _proxied->keys();

        for (std::string const& key : proxyKeys) {
            if (_data.count(key) <= 0) { //item is not present anymore
                _proxied->clearBlock(key);
            }
        }

        for (auto&[key, val] : _data) {
            if (_proxied->contains(key)) {
                DataStructureBlock* currentBlock = block(key);
                DataStructureBlock* remoteBlock = _proxied->block(key);

                if (currentBlock->dataStructureKind() == DataStructureBlock::Set) {
                    ProxyUnsyncedPropertySet* proxySubSet = dynamic_cast<ProxyUnsyncedPropertySet*>(currentBlock);

                    if (proxySubSet != nullptr) {
                        if (proxySubSet->_proxied == dynamic_cast<PropertySet*>(remoteBlock)) {
                            proxySubSet->commit();
                            continue;
                        } else {
                            //in theory should not happen, unless something strange occured.
                            proxySubSet->commit();
                            _proxied->addBlock(key, proxySubSet->_proxied);
                            continue;
                        }
                    }

                    PropertySet* newSubSet = dynamic_cast<PropertySet*>(currentBlock);

                    if (newSubSet != nullptr) {
                        _proxied->addBlock(key, newSubSet);
                        continue;
                    }

                    //block remain invalid, should not happen
                    _proxied->clearBlock(key);

                } else if (currentBlock->dataStructureKind() == Property) {
                    ProxyUnsyncedGenericProperty* proxyProp = dynamic_cast<ProxyUnsyncedGenericProperty*>(currentBlock);

                    if (proxyProp != nullptr) {
                        if (proxyProp->_proxied == dynamic_cast<GenericProperty*>(remoteBlock)) {
                            proxyProp->commit();
                            continue;
                        } else {
                            //in theory should not happen, unless something strange occured.
                            proxyProp->commit();
                            _proxied->addBlock(key, proxyProp->_proxied);
                            continue;
                        }
                    }

                    GenericProperty* newProp = dynamic_cast<GenericProperty*>(currentBlock);

                    if (newProp != nullptr) {
                        _proxied->addBlock(key, newProp);
                        continue;
                    }

                    //block remain invalid, should not happen
                    _proxied->clearBlock(key);

                }

            } else {
                DataStructureBlock* currentBlock = block(key);

                if (currentBlock->dataStructureKind() == DataStructureBlock::Set) {
                    ProxyUnsyncedPropertySet* proxySubSet = dynamic_cast<ProxyUnsyncedPropertySet*>(currentBlock);

                    if (proxySubSet != nullptr) {
                        proxySubSet->commit();
                        _proxied->addBlock(key, proxySubSet->_proxied);
                        continue;
                    }

                    PropertySet* newSubSet = dynamic_cast<PropertySet*>(currentBlock);

                    if (newSubSet != nullptr) {
                        _proxied->addBlock(key, newSubSet);
                        continue;
                    }

                    //block remain invalid, should not happen
                    _proxied->clearBlock(key);

                } else if (currentBlock->dataStructureKind() == Property) {
                    ProxyUnsyncedGenericProperty* proxyProp = dynamic_cast<ProxyUnsyncedGenericProperty*>(currentBlock);

                    if (proxyProp != nullptr) {
                        proxyProp->commit();
                        _proxied->addBlock(key, proxyProp->_proxied);
                        continue;
                    }

                    GenericProperty* newProp = dynamic_cast<GenericProperty*>(currentBlock);

                    if (newProp != nullptr) {
                        _proxied->addBlock(key, newProp);
                        continue;
                    }

                    //block remain invalid, should not happen
                    _proxied->clearBlock(key);

                }

            }
        }

        clearChangesMarker();
    }
    virtual bool hasUncommitedChanges() {
        return hasChanges();
    }

private:

    PropertySet* _proxied;

    friend PropertySet* buildUnsyncedProxyPropertySet(PropertySet* sourceSet, DataStructureBlock* parent);
};

PropertySet* buildUnsyncedProxyPropertySet(PropertySet* sourceSet, DataStructureBlock* parent) {

    ProxyUnsyncedPropertySet* proxy = new ProxyUnsyncedPropertySet(sourceSet);

    std::vector<std::string> keys = sourceSet->keys();

    for (std::string const& key : keys) {
        DataStructureBlock* block = sourceSet->block(key);

        if (block == nullptr) {
            continue;
        }

        if (block->dataStructureKind() == DataStructureBlock::Set) {
            proxy->_data[key] = buildUnsyncedProxyPropertySet(static_cast<PropertySet*>(block), proxy);
        } else if (block->dataStructureKind() == DataStructureBlock::Property) {
            proxy->_data[key] = buildUnsyncedProxyProperty(static_cast<GenericProperty*>(block), proxy);
        }
    }

    return proxy;

}
GenericProperty* buildUnsyncedProxyProperty(GenericProperty* sourceProperty, DataStructureBlock* parent) {

    ProxyUnsyncedGenericProperty* proxy = new ProxyUnsyncedGenericProperty(sourceProperty, parent);

    return proxy;
}

}
