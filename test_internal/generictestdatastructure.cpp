#include "generictestdatastructure.h"

#include <set>

namespace DataModelInterface {
namespace Internal {

SubSubDataBlock::SubSubDataBlock(std::string const& ref) : _ref(ref) {
    _internalPropertySet = nullptr;
}
SubSubDataBlock::~SubSubDataBlock() {
    //no deleting _internalPropertySet, it is assumed it will be done by the managing property set
}

PropertySet* SubSubDataBlock::getPropertySet() {

    if (_internalPropertySet == nullptr) {
        _internalPropertySet = new PropertySet();

        Property<unsigned int>* uintProp = new Property<unsigned int>(_internalPropertySet);

        uintProp->connect<unsigned int>([this] (unsigned int data) {
            setUIntProp(data);
        });

        _internalPropertySet->addBlock(UIntKey, uintProp);

        Property<std::string>* stringProp = new Property<std::string>(_internalPropertySet);

        stringProp->connect<std::string>([this] (std::string data) {
            setStringProp(data);
        });

        _internalPropertySet->addBlock(StringKey, stringProp);

    }

    return _internalPropertySet;

}

bool SubSubDataBlock::checkIsEquivalent(SubSubDataBlock* other) {

    if (other == nullptr) {
        return false;
    }

    if (other->_uint_prop != _uint_prop) {
        return false;
    }

    if (other->_string_prop != _string_prop) {
        return false;
    }

    return true;

}

void SubSubDataBlock::setUIntProp(unsigned int const& val) {

    if (val == _uint_prop) {
        return;
    }

    _uint_prop = val;

    if (_internalPropertySet != nullptr) {
        GenericProperty* uintProp = static_cast<GenericProperty*>(_internalPropertySet->block(UIntKey));

        uintProp->setData(_uint_prop);
    }

}

void SubSubDataBlock::setStringProp(std::string const& val) {

    if (val == _string_prop) {
        return;
    }

    _string_prop = val;

    if (_internalPropertySet != nullptr) {
        GenericProperty* stringProp = static_cast<GenericProperty*>(_internalPropertySet->block(StringKey));

        stringProp->setData(_string_prop);
    }

}

SubDataBlock::SubDataBlock(std::string const& ref) : _ref(ref) {
    _internalPropertySet = nullptr;
}
SubDataBlock::~SubDataBlock() {
    //no deleting _internalPropertySet, it is assumed it will be done by the managing property set
}

SubDataBlock1::SubDataBlock1(std::string const& ref) : SubDataBlock(ref) {

}

PropertySet* SubDataBlock1::getPropertySet() {

    if (_internalPropertySet == nullptr) {
        _internalPropertySet = new PropertySet();
        _internalPropertySet->setTypeInfosHook([] () {
            return SubDataBlock1::TypeHint;
        });

        Property<int>* intProp = new Property<int>(_internalPropertySet);

        intProp->connect<int>([this] (int data) {
            setIntProp(data);
        });

        _internalPropertySet->addBlock(IntKey, intProp);

        Property<std::string>* stringProp = new Property<std::string>(_internalPropertySet);

        stringProp->connect<std::string>([this] (std::string data) {
            setStringProp(data);
        });

        _internalPropertySet->addBlock(StringKey, stringProp);

    }

    return _internalPropertySet;
}

bool SubDataBlock1::checkIsEquivalent(SubDataBlock* p_other) {

    SubDataBlock1* other = dynamic_cast<SubDataBlock1*>(p_other);

    if (other == nullptr) {
        return false;
    }

    if (other->_int_prop != _int_prop) {
        return false;
    }

    if (other->_string_prop != _string_prop) {
        return false;
    }

    return true;

}

void SubDataBlock1::setIntProp(int const& val) {

    if (val == _int_prop) {
        return;
    }

    _int_prop = val;

    if (_internalPropertySet != nullptr) {
        GenericProperty* intProp = static_cast<GenericProperty*>(_internalPropertySet->block(IntKey));

        intProp->setData(_int_prop);
    }

}

void SubDataBlock1::setStringProp(std::string const& val) {

    if (val == _string_prop) {
        return;
    }

    _string_prop = val;

    if (_internalPropertySet != nullptr) {
        GenericProperty* stringProp = static_cast<GenericProperty*>(_internalPropertySet->block(StringKey));

        stringProp->setData(_string_prop);
    }
}

SubDataBlock2::SubDataBlock2(std::string const& ref) :
    SubDataBlock(ref) {

}
SubDataBlock2::~SubDataBlock2() {

    for (auto const& [key, block] : _subblocks) {
        if (block != nullptr) {
            delete block;
        }
    }

}

PropertySet* SubDataBlock2::getPropertySet() {

    if (_internalPropertySet == nullptr) {
        _internalPropertySet = new PropertySet();
        _internalPropertySet->setTypeInfosHook([] () {
            return SubDataBlock2::TypeHint;
        });

        Property<long>* longProp = new Property<long>(_internalPropertySet);

        longProp->connect<long>([this] (long data) {
            setLongProp(data);
        });

        _internalPropertySet->addBlock(LongKey, longProp);

        PropertySet* subBlocksSet = new PropertySet(_internalPropertySet);

        subBlocksSet->setSubSetConstructorHook([this] (std::string const& key, std::string const& typeInfos) -> PropertySet* {

            if (_subblocks.count(key) > 0) {
                SubSubDataBlock* block = _subblocks[key];
                return block->getPropertySet();
            }

            if (typeInfos == SubSubDataBlock::TypeHint or typeInfos.empty()) {
                SubSubDataBlock* block = new SubSubDataBlock(key);
                insertDataBlock(key, block);
                return block->getPropertySet();
            }

            return nullptr;

        });

        for (auto const& [key, block] : _subblocks) {
            constexpr bool manageBlock = false;
            subBlocksSet->addBlock(key, block->getPropertySet(), manageBlock);
        }

        subBlocksSet->connectPropertyClearWatcher([this] (PropertySet::Url const& url, DataStructureBlock const* block) {
            if (url.size() == 2) { //is a child property
                std::string key = url[1];
                if (_subblocks.count(key) > 0) {
                    SubSubDataBlock* block = _subblocks[key];
                    if (block != nullptr) {
                        delete block;
                    }
                    _subblocks.erase(key);
                }
            }
        });

        _internalPropertySet->addBlock(BlocksKey, subBlocksSet);

    }

    return _internalPropertySet;
}

bool SubDataBlock2::checkIsEquivalent(SubDataBlock* p_other) {
    SubDataBlock2* other = dynamic_cast<SubDataBlock2*>(p_other);

    if (other == nullptr) {
        return false;
    }

    if (other->_long_prop != _long_prop) {
        return false;
    }


    std::set<std::string> keys;

    for (auto const& [key, block] : _subblocks) {
        keys.insert(key);
    }

    std::set<std::string> other_keys;

    for (auto const& [key, block] : other->_subblocks) {
        other_keys.insert(key);
    }

    if (keys != other_keys) {
        return false;
    }

    for (auto const& [key, block] : _subblocks) {
        if (!block->checkIsEquivalent(other->_subblocks.at(key))) {
            return false;
        }
    }

    return true;
}

void SubDataBlock2::insertDataBlock(std::string const& ref, SubSubDataBlock* block) {
    if (_subblocks.count(ref) > 0) {
        return;
    }

    _subblocks[ref] = block;


    if (_internalPropertySet != nullptr) {
        PropertySet* subBlocksSet = static_cast<PropertySet*>(_internalPropertySet->block(BlocksKey));
        if (subBlocksSet != nullptr) {
            constexpr bool manageBlock = true;
            subBlocksSet->addBlock(ref, block->getPropertySet(), manageBlock);
        }
    }
}
void SubDataBlock2::removeDataBlock(std::string const& ref) {

    if (_internalPropertySet != nullptr) {
        PropertySet* subBlocksSet = static_cast<PropertySet*>(_internalPropertySet->block(BlocksKey));
        if (subBlocksSet != nullptr) {
            constexpr bool deleteBlock = false;
            subBlocksSet->clearBlock(ref, deleteBlock); //ensure the block is not deleted, as the model will be deleted by the destructor of the data structure
        }
    }

    if (_subblocks.count(ref) > 0) {
        SubSubDataBlock* block = _subblocks[ref];
        if (block != nullptr) {
            delete block;
        }
        _subblocks.erase(ref);
    }
}

SubSubDataBlock* SubDataBlock2::getSubBlock(std::string const& ref) {
    if (_subblocks.count(ref) > 0) {
        return _subblocks[ref];
    }
    return nullptr;
}

void SubDataBlock2::setLongProp(long const& val) {

    if (val == _long_prop) {
        return;
    }

    _long_prop = val;

    if (_internalPropertySet != nullptr) {
        GenericProperty* longProp = static_cast<GenericProperty*>(_internalPropertySet->block(LongKey));

        longProp->setData(_long_prop);
    }
}

GenericTestDataStructure::GenericTestDataStructure() {
    _ind_sub_data_prop = new SubDataBlock2(IndividualBlockKey);
    _internalPropertySet = nullptr;
}
GenericTestDataStructure::~GenericTestDataStructure() {
    if (_ind_sub_data_prop != nullptr) {
        delete _ind_sub_data_prop;
    }

    for (auto const& [key, block] : _sub_data_props) {
        if (block != nullptr) {
            delete block;
        }
    }

    if (_internalPropertySet != nullptr) {
        delete _internalPropertySet;
    }
}

PropertySet* GenericTestDataStructure::getPropertySet() {
    if (_internalPropertySet == nullptr) {
        _internalPropertySet = new PropertySet();

        PropertySet* subBlocksSet = new PropertySet(_internalPropertySet);

        subBlocksSet->setSubSetConstructorHook([this] (std::string const& key, std::string const& typeInfos) -> PropertySet* {

            if (_sub_data_props.count(key) > 0) {
                SubDataBlock* block = _sub_data_props[key];
                return block->getPropertySet();
            }

            if (typeInfos == SubDataBlock1::TypeHint) {
                SubDataBlock1* block = new SubDataBlock1(key);
                insertDataBlock(key, block);
                return block->getPropertySet();
            }

            if (typeInfos == SubDataBlock2::TypeHint) {
                SubDataBlock2* block = new SubDataBlock2(key);
                insertDataBlock(key, block);
                return block->getPropertySet();
            }

            return nullptr;

        });

        for (auto const& [key, block] : _sub_data_props) {
            constexpr bool manageBlock = true;
            subBlocksSet->addBlock(key, block->getPropertySet(), manageBlock);
        }

        subBlocksSet->connectPropertyClearWatcher([this] (PropertySet::Url const& url, DataStructureBlock const* block) {
            if (url.size() == 1) { //is a child property
                std::string key = block->id();
                if (_sub_data_props.count(key) > 0) {
                    SubDataBlock* block = _sub_data_props[key];
                    if (block != nullptr) {
                        delete block;
                    }
                    _sub_data_props.erase(key);
                }
            }
        });

        constexpr bool manageBlock = true;
        _internalPropertySet->addBlock(BlocksKey, subBlocksSet);
        _internalPropertySet->addBlock(IndividualBlockKey, _ind_sub_data_prop->getPropertySet(), manageBlock);
    }
    return _internalPropertySet;
}

bool GenericTestDataStructure::checkIsEquivalent(GenericTestDataStructure const& other) {

    if (!_ind_sub_data_prop->checkIsEquivalent(other._ind_sub_data_prop)) {
        return false;
    }

    std::set<std::string> keys;

    for (auto const& [key, block] : _sub_data_props) {
        keys.insert(key);
    }

    std::set<std::string> other_keys;

    for (auto const& [key, block] : other._sub_data_props) {
        other_keys.insert(key);
    }

    if (keys != other_keys) {
        return false;
    }

    for (auto const& [key, block] : _sub_data_props) {
        if (!block->checkIsEquivalent(other._sub_data_props.at(key))) {
            return false;
        }
    }

    return true;

}

void GenericTestDataStructure::insertDataBlock(std::string const& ref, SubDataBlock* block) {
    if (_sub_data_props.count(ref) > 0) {
        return;
    }

    _sub_data_props[ref] = block;


    if (_internalPropertySet != nullptr) {
        PropertySet* subBlocksSet = static_cast<PropertySet*>(_internalPropertySet->block(BlocksKey));
        if (subBlocksSet != nullptr) {
            constexpr bool manageBlock = false;
            subBlocksSet->addBlock(ref, block->getPropertySet(), manageBlock);
        }
    }
}
SubDataBlock* GenericTestDataStructure::getDataBlock(std::string const& ref) {
    if (_sub_data_props.count(ref) <= 0) {
        return nullptr;
    }
    return _sub_data_props[ref];
}
void GenericTestDataStructure::removeDataBlock(std::string const& ref) {

    if (_internalPropertySet != nullptr) {
        PropertySet* subBlocksSet = static_cast<PropertySet*>(_internalPropertySet->block(BlocksKey));
        if (subBlocksSet != nullptr) {
            constexpr bool deleteBlock = false;
            subBlocksSet->clearBlock(ref, deleteBlock); //ensure the block is not deleted, as the model will be deleted by the destructor of the data structure
        }
    }

    if (_sub_data_props.count(ref) > 0) {
        SubDataBlock* block = _sub_data_props[ref];
        if (block != nullptr) {
            delete block;
        }
        _sub_data_props.erase(ref);
    }
}

SubDataBlock2* GenericTestDataStructure::individualBlock() {
    return _ind_sub_data_prop;
}


} // namespace Internal
} // namespace DataModelInterface
