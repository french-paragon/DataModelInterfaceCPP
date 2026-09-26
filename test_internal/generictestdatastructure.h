#ifndef GENERICTESTDATASTRUCTURE_H
#define GENERICTESTDATASTRUCTURE_H

#include "../property.h"

namespace DataModelInterface {
namespace Internal {

class SubSubDataBlock {
public:

    static constexpr char TypeHint[] = "ssub";

    static constexpr char StringKey[] = "string";
    static constexpr char UIntKey[] = "int";

    SubSubDataBlock(std::string const& ref);
    ~SubSubDataBlock();

    PropertySet* getPropertySet();

    bool checkIsEquivalent(SubSubDataBlock* other);

    inline unsigned int getUIntProp() const {
        return _uint_prop;
    }

    void setUIntProp(unsigned int const& val);

    inline std::string getStringProp() const {
        return _string_prop;
    }

    void setStringProp(std::string const& val);
protected:

    PropertySet* _internalPropertySet;

    std::string _ref;
    std::string _string_prop;
    unsigned int _uint_prop;
};

class SubDataBlock {
public:
    SubDataBlock(std::string const& ref);
    virtual ~SubDataBlock();

    virtual PropertySet* getPropertySet() = 0;

    virtual bool checkIsEquivalent(SubDataBlock* other) = 0;

protected:

    PropertySet* _internalPropertySet;

    std::string _ref;
};

class SubDataBlock1 : public SubDataBlock {
public:

    static constexpr char TypeHint[] = "sub1";

    static constexpr char IntKey[] = "int";
    static constexpr char StringKey[] = "string";

    SubDataBlock1(std::string const& ref);

    PropertySet* getPropertySet() override;

    bool checkIsEquivalent(SubDataBlock* other) override;

    inline int getIntProp() const {
        return _int_prop;
    }

    void setIntProp(int const& val);

    inline std::string getStringProp() const {
        return _string_prop;
    }

    void setStringProp(std::string const& val);
protected:

    int _int_prop;
    std::string _string_prop;
};

class SubDataBlock2 : public SubDataBlock {
public:

    static constexpr char TypeHint[] = "sub2";

    static constexpr char BlocksKey[] = "blocks";
    static constexpr char LongKey[] = "long";

    SubDataBlock2(std::string const& ref);
    ~SubDataBlock2();

    PropertySet* getPropertySet() override;

    bool checkIsEquivalent(SubDataBlock* other) override;

    void insertDataBlock(std::string const& ref, SubSubDataBlock* block);
    void removeDataBlock(std::string const& ref);
    inline int nSubBlocks() const {
        return _subblocks.size();
    }
    SubSubDataBlock* getSubBlock(std::string const& ref);

    inline long getFloatProp() const {
        return _long_prop;
    }

    void setLongProp(long const& val);

protected:

    long _long_prop;
    std::map<std::string,SubSubDataBlock*> _subblocks;;
};

/*!
 * \brief The GenericTestDataStructure class shall serve as both test and example of how to use a data model interface on an arbitrary data structure
 */
class GenericTestDataStructure
{
public:

    static constexpr char TypeHint[] = "struct";

    static constexpr char BlocksKey[] = "blocks";
    static constexpr char IndividualBlockKey[] = "ind";

    GenericTestDataStructure();
    ~GenericTestDataStructure();

    PropertySet* getPropertySet();

    bool checkIsEquivalent(GenericTestDataStructure const& other);

    void insertDataBlock(std::string const& ref, SubDataBlock* block);
    void removeDataBlock(std::string const& ref);
    inline int nDataBlocks() const {
        return _sub_data_props.size();
    }
    SubDataBlock* getDataBlock(std::string const& ref);

    SubDataBlock2* individualBlock();

protected:

    PropertySet* _internalPropertySet;

    std::map<std::string, SubDataBlock*> _sub_data_props;
    SubDataBlock2* _ind_sub_data_prop;
};

} // namespace Internal
} // namespace DataModelInterface

#endif // GENERICTESTDATASTRUCTURE_H
