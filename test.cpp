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

#include <gtest/gtest.h>

#include "./property.h"
#include "./changerecorder.h"

#include "./test_internal/generictestdatastructure.h"

#include <sstream>

using namespace DataModelInterface;

//few static asserts
static_assert(std::is_same_v<StorageType<DataProxy<uint32_t>>::Type, StorageType<uint32_t>::Type>, "Error in StorageType implementation!");
static_assert(std::is_same_v<StorageType<DataProxy<std::string>>::Type, StorageType<std::string>::Type>, "Error in StorageType implementation!");

TEST(DataModelInterface, SetBuilding) {

    PropertySet* basicSet = new PropertySet();

    std::string testDataStr = "test";
    int32_t testDataInt = 42;

    std::string prop1Name = "prop1";
    std::string prop1Data = "toto";
    Property<std::string>* prop1 = new Property<std::string>(basicSet);
    prop1->set(prop1Data);

    bool prop1DeleteWatch = false;
    ClearNotifySlotWatcher prop1Watcher([&prop1DeleteWatch] () {
        prop1DeleteWatch = true;
    });
    prop1->connectClearWatcher(&prop1Watcher);

    basicSet->addBlock(prop1Name, prop1);

    DataStructureBlock* block = basicSet->block(prop1Name);

    ASSERT_TRUE(block->dataStructureKind() == DataStructureBlock::Property);
    ASSERT_EQ(block, static_cast<DataStructureBlock*>(prop1));

    GenericProperty* prop1Recovered = static_cast<GenericProperty*>(block);
    ASSERT_EQ(std::get<std::string>(prop1Recovered->data()), prop1Data);
    prop1Recovered->setData(testDataStr);
    ASSERT_EQ(prop1->get(), testDataStr);

    std::string subset1Name = "subset1";
    PropertySet* subset = new PropertySet(basicSet);

    bool subset1DeleteWatch = false;
    ClearNotifySlotWatcher subset1Watcher([&subset1DeleteWatch] () {
        subset1DeleteWatch = true;
    });
    subset->connectClearWatcher(&subset1Watcher);

    basicSet->addBlock(subset1Name, subset);

    std::string subprop1Name = "sprop1";
    std::string subprop1Data = "tata";
    Property<std::string>* subprop1 = new Property<std::string>(subset);
    subprop1->set(subprop1Data);

    bool subprop1DeleteWatch = false;
    ClearNotifySlotWatcher subprop1Watcher([&subprop1DeleteWatch] () {
        subprop1DeleteWatch = true;
    });
    subprop1->connectClearWatcher(&subprop1Watcher);

    subset->addBlock(subprop1Name, subprop1);

    block = basicSet->block({subset1Name, subprop1Name});

    ASSERT_TRUE(block->dataStructureKind() == DataStructureBlock::Property);
    ASSERT_EQ(block, static_cast<DataStructureBlock*>(subprop1));

    GenericProperty* subprop1Recovered = static_cast<GenericProperty*>(block);
    ASSERT_EQ(std::get<std::string>(subprop1Recovered->data()), subprop1Data);

    std::string strWatch;

    subprop1Recovered->connect<std::string const&>([&strWatch] (std::string const& data) {
        strWatch = data;
    });

    subprop1Recovered->setData(testDataStr);
    ASSERT_EQ(subprop1->get(), testDataStr);

    ASSERT_EQ(strWatch, testDataStr);

    std::string subprop2Name = "sprop2";
    int32_t subprop2Data = 69;
    Property<int32_t>* subprop2 = new Property<int32_t>(subset);
    subprop2->set(subprop2Data);

    bool subprop2DeleteWatch = false;
    ClearNotifySlotWatcher subprop2Watcher([&subprop2DeleteWatch] () {
        subprop2DeleteWatch = true;
    });
    subprop2->connectClearWatcher(&subprop2Watcher);

    subset->addBlock(subprop2Name, subprop2);

    block = basicSet->block({subset1Name, subprop2Name});

    ASSERT_TRUE(block->dataStructureKind() == DataStructureBlock::Property);
    ASSERT_EQ(block, static_cast<DataStructureBlock*>(subprop2));

    GenericProperty* subprop2Recovered = static_cast<GenericProperty*>(block);
    ASSERT_EQ(std::get<int32_t>(subprop2Recovered->data()), subprop2Data);

    delete basicSet;

    ASSERT_TRUE(prop1DeleteWatch);
    ASSERT_TRUE(subset1DeleteWatch);
    ASSERT_TRUE(subprop1DeleteWatch);
    ASSERT_TRUE(subprop2DeleteWatch);
}

TEST(DataModelInterface, SetUrlEncodingDecoding) {
    PropertySet::Url url1{"test", "1", "prop"}; //simple
    PropertySet::Url url2{"42", "33", "27"}; //numbers only
    PropertySet::Url url3{"\\p1", "\\sp2/:32", "/ord8"}; //more complex
    PropertySet::Url url4{"/\\%::\\", "", "./&\\"}; //a bit crazy

    std::vector<PropertySet::Url> testData = {url1, url2, url3, url4};

    for (PropertySet::Url const& url : testData) {
        std::string encoded = PropertySet::urlEncode(url);
        ASSERT_FALSE(encoded.empty());

        PropertySet::Url decoded = PropertySet::urlDecode(encoded);

        ASSERT_EQ(decoded, url);
    }

    std::string encodedEmpty = PropertySet::urlEncode({});
    ASSERT_TRUE(encodedEmpty.empty());

    PropertySet::Url decodedEmpty = PropertySet::urlDecode("");
    ASSERT_TRUE(decodedEmpty.empty());
}

TEST(DataModelInterface, SignalsInSets) {

    std::unique_ptr<PropertySet> basicSet = std::make_unique<PropertySet>();

    std::string subsetName = "subset1";
    PropertySet* subSet = new PropertySet(basicSet.get());

    std::string prop1Name = "prop1";
    std::string prop1Data = "toto";
    Property<std::string>* prop1 = new Property<std::string>(basicSet.get());
    prop1->set(prop1Data);

    std::string subprop1Name = "subprop1";
    std::string subprop1Data = "tata";
    Property<std::string>* subprop1 = new Property<std::string>(subSet);
    prop1->set(prop1Data);

    int insertCount = 0;
    PropertySet::Url lastInsertUrl;
    DataStructureBlock const* lastInsertBlock;
    basicSet->connectPropertyInsertWatcher([&insertCount, &lastInsertUrl, &lastInsertBlock] (PropertySet::Url const& url, DataStructureBlock const* block) {
        insertCount++;
        lastInsertUrl = url;
        lastInsertBlock = block;
    });

    basicSet->addBlock(subsetName, subSet);

    ASSERT_EQ(insertCount, 1);
    ASSERT_EQ(lastInsertUrl, subSet->getUrl());
    ASSERT_EQ(lastInsertBlock, static_cast<DataStructureBlock*>(subSet));

    basicSet->addBlock(prop1Name, prop1);

    ASSERT_EQ(insertCount, 2);
    ASSERT_EQ(lastInsertUrl, prop1->getUrl());
    ASSERT_EQ(lastInsertBlock, static_cast<DataStructureBlock*>(prop1));

    subSet->addBlock(subprop1Name, subprop1);

    ASSERT_EQ(insertCount, 3);
    ASSERT_EQ(lastInsertUrl, subprop1->getUrl());
    ASSERT_EQ(lastInsertBlock, static_cast<DataStructureBlock*>(subprop1));

    int propCount = 0;
    PropertySet::Url lastPropUrl;
    GenericDatum lastPropBlockData;
    basicSet->connectChangeWatcher<GenericDatum>([&propCount, &lastPropUrl, &lastPropBlockData] (PropertySet::Url const& url, GenericDatum const& data) {
        propCount++;
        lastPropUrl = url;
        lastPropBlockData = data;
    });

    std::string testDataStr = "test";

    subprop1->setData(testDataStr);

    ASSERT_EQ(propCount, 1);
    ASSERT_EQ(lastPropUrl, subprop1->getUrl());
    ASSERT_EQ(lastPropBlockData, subprop1->data());

    prop1->setData(testDataStr);

    ASSERT_EQ(propCount, 2);
    ASSERT_EQ(lastPropUrl, prop1->getUrl());
    ASSERT_EQ(lastPropBlockData, prop1->data());

    int removeCount = 0;
    PropertySet::Url lastRemoveUrl;
    DataStructureBlock const* lastRemoveBlock;
    basicSet->connectPropertyClearWatcher([&removeCount, &lastRemoveUrl, &lastRemoveBlock] (PropertySet::Url const& url, DataStructureBlock const* block) {
        removeCount++;
        lastRemoveUrl = url;
        lastRemoveBlock = block;
    });

    auto urlSubset = subSet->getUrl();
    auto urlProp1 = prop1->getUrl();

    basicSet->clearBlock(subsetName);

    ASSERT_EQ(removeCount, 1);
    ASSERT_EQ(lastRemoveUrl, urlSubset);
    ASSERT_EQ(lastRemoveBlock, static_cast<DataStructureBlock*>(subSet));

    basicSet->clearBlock(prop1Name);

    ASSERT_EQ(removeCount, 2);
    ASSERT_EQ(lastRemoveUrl, urlProp1);
    ASSERT_EQ(lastRemoveBlock, static_cast<DataStructureBlock*>(prop1));

}

TEST(DataModelInterface, PropertiesSetUnsyncedProxy) {

    PropertySet* basicSet = new PropertySet();

    std::string testDataStr = "test";

    std::string prop1Name = "prop1";
    std::string prop1Data = "toto";
    Property<std::string>* prop1 = new Property<std::string>(basicSet);
    prop1->set(prop1Data);

    basicSet->addBlock(prop1Name, prop1);

    PropertySet* proxy = buildUnsyncedProxyPropertySet(basicSet);

    std::vector<std::string> propsOriginal = basicSet->keys();
    std::vector<std::string> propsProxy = proxy->keys();

    ASSERT_EQ(propsProxy.size(), propsOriginal.size());
    ASSERT_EQ(propsProxy.size(), 1);
    ASSERT_EQ(propsOriginal[0], propsProxy[0]);

    DataStructureBlock* block = proxy->block(prop1Name);

    ASSERT_TRUE(block->dataStructureKind() == DataStructureBlock::Property);

    GenericProperty* prop1Proxy = static_cast<GenericProperty*>(block);
    ASSERT_EQ(std::get<std::string>(prop1Proxy->data()), prop1Data);
    prop1Proxy->setData(testDataStr);
    ASSERT_EQ(std::get<std::string>(prop1Proxy->data()), testDataStr);
    ASSERT_EQ(prop1->get(), prop1Data);

    proxy->commit();

    ASSERT_EQ(prop1->get(), testDataStr);

    delete basicSet;
    delete proxy;

}

TEST(DataModelInterface, SetChangesRecorder) {

    std::unique_ptr<PropertySet> initialReferenceSet = std::make_unique<PropertySet>();
    std::unique_ptr<PropertySet> modifiableSet = std::make_unique<PropertySet>();
    std::unique_ptr<PropertySet> finalReferenceSet = std::make_unique<PropertySet>();

    std::string prop1Name = "prop1";
    std::string prop1Data = "toto";
    Property<std::string>* prop1 = new Property<std::string>(initialReferenceSet.get());
    prop1->set(prop1Data);

    initialReferenceSet->addBlock(prop1Name, prop1);

    std::string subset1Name = "subset1";
    PropertySet* subset = new PropertySet(initialReferenceSet.get());
    initialReferenceSet->addBlock(subset1Name, subset);

    std::string subprop1Name = "subprop1";
    int subprop1Data = 42;
    Property<int>* subprop1 = new Property<int>(subset);
    subprop1->set(subprop1Data);

    subset->addBlock(subprop1Name, subprop1);

    initialReferenceSet->duplicateTo(modifiableSet.get());
    initialReferenceSet->duplicateTo(finalReferenceSet.get());

    ASSERT_TRUE(initialReferenceSet->isSimilarTo(modifiableSet.get()));
    ASSERT_TRUE(initialReferenceSet->isSimilarTo(finalReferenceSet.get()));

    ChangeRecorder changeRecorder(finalReferenceSet.get());
    changeRecorder.setMaxChangesRecorded(100); //plenty enough


    ChangeRecorder changeWatcher(initialReferenceSet.get()); //just there to ensure the initial reference does not change
    changeWatcher.setMaxChangesRecorded(100); //plenty enough

    int nChanges = 0;

    DataStructureBlock* prop1Block = finalReferenceSet->block(prop1Name);
    ASSERT_EQ(prop1Block->dataStructureKind(), DataStructureBlock::Kind::Property);
    GenericProperty* prop_1_access = static_cast<GenericProperty*>(prop1Block);
    std::string newProp1Data = "tutu";
    prop_1_access->setData(newProp1Data);
    nChanges++;

    ASSERT_EQ(changeRecorder.nRecordedChanges(), nChanges);
    ASSERT_EQ(changeWatcher.nRecordedChanges(), 0);

    finalReferenceSet->clearBlock(subset1Name);
    nChanges++;

    ASSERT_EQ(changeRecorder.nRecordedChanges(), nChanges);
    ASSERT_EQ(changeWatcher.nRecordedChanges(), 0);

    std::string subset2Name = "subset2";
    PropertySet* subset2 = new PropertySet(finalReferenceSet.get());
    finalReferenceSet->addBlock(subset2Name, subset2);
    nChanges++;

    ASSERT_EQ(changeRecorder.nRecordedChanges(), nChanges);
    ASSERT_EQ(changeWatcher.nRecordedChanges(), 0);

    std::string subprop2Name = "subprop2";
    double subprop2Data = 69;
    Property<double>* subprop2 = new Property<double>(subset2);
    subprop2->set(subprop2Data);

    subset2->addBlock(subprop2Name, subprop2);
    nChanges++;

    ASSERT_EQ(changeRecorder.nRecordedChanges(), nChanges);
    ASSERT_EQ(changeWatcher.nRecordedChanges(), 0);

    ASSERT_FALSE(initialReferenceSet->isSimilarTo(finalReferenceSet.get()));

    for (auto actionIt = changeRecorder.changeIteratorBegin(); actionIt != changeRecorder.changeIteratorEnd(); ++actionIt) {
        for (ChangeRecord const& record : actionIt->redo) {
            record.apply(modifiableSet.get());
        }
    }

    ASSERT_TRUE(finalReferenceSet->isSimilarTo(modifiableSet.get()));
    ASSERT_FALSE(initialReferenceSet->isSimilarTo(modifiableSet.get()));

    for (auto actionIt = changeRecorder.historyIteratorBegin(); actionIt != changeRecorder.historyIteratorEnd(); ++actionIt) {
        for (ChangeRecord const& record : actionIt->undo) {
            record.apply(modifiableSet.get());
        }
    }

    ASSERT_TRUE(initialReferenceSet->isSimilarTo(modifiableSet.get()));
    ASSERT_FALSE(finalReferenceSet->isSimilarTo(modifiableSet.get()));


}

TEST(DataModelInterface, ArbitraryDataTransfert) {

    Internal::GenericTestDataStructure source;
    Internal::GenericTestDataStructure target;

    ASSERT_TRUE(source.checkIsEquivalent(target));

    PropertySet* src_prop_set = source.getPropertySet();
    PropertySet* dst_prop_set = target.getPropertySet();

    ChangeRecorder changeRecorder(src_prop_set);
    changeRecorder.setMaxChangesRecorded(10); //buffer should be large enough

    ChangeRecorder::CallBack callBack = [dst_prop_set] (ChangeRecorder const* recorder,
                                                       ChangeRecorder::ChangeRecordInfo const& changeRecordInfos) {
        for (ChangeRecord const& changeRecord : changeRecordInfos.redo) {
            changeRecord.apply(dst_prop_set);
        }
    };

    changeRecorder.registerCallback(callBack);

    std::string keySubBlock1 = "test";
    Internal::SubDataBlock1* b1 = new Internal::SubDataBlock1(keySubBlock1);
    source.insertDataBlock(keySubBlock1, b1);

    b1->setIntProp(42);
    b1->setStringProp("test");

    Internal::SubDataBlock1* b1_inserted = dynamic_cast<Internal::SubDataBlock1*>(source.getDataBlock(keySubBlock1));

    ASSERT_EQ(source.nDataBlocks(), 1);
    ASSERT_EQ(b1, b1_inserted);
    ASSERT_EQ(b1_inserted->getIntProp(), 42);
    ASSERT_EQ(b1_inserted->getStringProp(), "test");

    ASSERT_TRUE(source.checkIsEquivalent(target));

    std::string keySubBlock2 = "test2";
    Internal::SubDataBlock2* b2 = new Internal::SubDataBlock2(keySubBlock2);
    source.insertDataBlock(keySubBlock2, b2);

    std::string keySubBlock3 = "test3";
    Internal::SubDataBlock2* b3 = new Internal::SubDataBlock2(keySubBlock3);
    source.insertDataBlock(keySubBlock3, b3);

    b2->setLongProp(69);
    b3->setLongProp(3327);

    std::string keySubSubBlock1 = "subtest1";
    Internal::SubSubDataBlock* sb1 = new Internal::SubSubDataBlock(keySubSubBlock1);
    b3->insertDataBlock(keySubSubBlock1, sb1);

    sb1->setStringProp("subtest");
    sb1->setUIntProp(101);

    ASSERT_EQ(source.nDataBlocks(), 3);

    Internal::SubDataBlock2* b2_inserted = dynamic_cast<Internal::SubDataBlock2*>(source.getDataBlock(keySubBlock2));
    Internal::SubDataBlock2* b3_inserted = dynamic_cast<Internal::SubDataBlock2*>(source.getDataBlock(keySubBlock3));

    ASSERT_EQ(b2, b2_inserted);
    ASSERT_EQ(b3, b3_inserted);

    Internal::SubSubDataBlock* sb1_inserted = b3_inserted->getSubBlock(keySubSubBlock1);

    ASSERT_EQ(sb1, sb1_inserted);

    ASSERT_EQ(b2_inserted->getFloatProp(), 69);
    ASSERT_EQ(b3_inserted->getFloatProp(), 3327);

    ASSERT_EQ(b2_inserted->nSubBlocks(), 0);
    ASSERT_EQ(b3_inserted->nSubBlocks(), 1);


    ASSERT_EQ(sb1->getUIntProp(), 101);
    ASSERT_EQ(sb1->getStringProp(), "subtest");


    ASSERT_TRUE(source.checkIsEquivalent(target));
}

TEST(DataModelInterface, ArbitraryDataSync) {

    Internal::GenericTestDataStructure source1;
    Internal::GenericTestDataStructure source2;
    Internal::GenericTestDataStructure master;

    ASSERT_TRUE(source1.checkIsEquivalent(master));
    ASSERT_TRUE(source2.checkIsEquivalent(master));

    using StreamTransfertT = std::stringstream;

    StreamTransfertT test;

    //run short test
    ChangeRecord r1{.url = {"t"}, .index = "test", .action = ChangeRecord::Insert, .dataRep = "set"};
    ChangeRecord r2{.url = {"t2"}, .index = "foo", .action = ChangeRecord::Set, .dataRep = "42"};
    ChangeRecord r3{.url = {"r", "t"}, .index = "bar", .action = ChangeRecord::Remove, .dataRep = "string"};
    ChangeRecord r4{.url = {"url", "multi"}, .index = "foo", .action = ChangeRecord::Set, .dataRep = ""};
    ChangeRecord r5{.url = {"t2"}, .index = "foo", .action = ChangeRecord::Set, .dataRep = "string"};

    r1.toStream(test);
    r2.toStream(test);

    ChangeRecord p1 = ChangeRecord::fromStream(test);
    ChangeRecord p2 = ChangeRecord::fromStream(test);
    ChangeRecord pInv = ChangeRecord::fromStream(test);

    std::string str = test.str();
    int pos = test.tellg();

    ASSERT_EQ(pos, str.size());

    ASSERT_EQ(r1, p1);
    ASSERT_EQ(r2, p2);
    ASSERT_FALSE(pInv.isValid());

    test.str(""); //clear the string
    test.clear();//clear errors;

    r3.toStream(test);
    r3.toStream(test);
    r4.toStream(test);
    r5.toStream(test);

    p1 = ChangeRecord::fromStream(test);
    p2 = ChangeRecord::fromStream(test);
    ChangeRecord p3 = ChangeRecord::fromStream(test);
    ChangeRecord p4 = ChangeRecord::fromStream(test);
    pInv = ChangeRecord::fromStream(test);

    ASSERT_EQ(r3, p1);
    ASSERT_EQ(r3, p2);
    ASSERT_EQ(r4, p3);
    ASSERT_EQ(r5, p4);
    ASSERT_FALSE(pInv.isValid());

    test.str(""); //clear the string
    test.clear();//clear errors;

    StreamTransfertT s1_up; //source1 publish its changes, master read them
    StreamTransfertT s1_down; //master publish its changes there, and source1 read them

    StreamTransfertT s2_up; //source2 publish its changes, master read them
    StreamTransfertT s2_down; //master publish its changes there, and source2 read them


    PropertySet* src1_prop_set = source1.getPropertySet();
    PropertySet* src2_prop_set = source2.getPropertySet();
    PropertySet* dst_prop_set = master.getPropertySet();


    ChangeRecorder changeRecorderSource1(src1_prop_set);
    changeRecorderSource1.setMaxChangesRecorded(10); //buffer should be large enough

    ChangeRecorder changeRecorderSource2(src2_prop_set);
    changeRecorderSource2.setMaxChangesRecorded(10); //buffer should be large enough

    ChangeRecorder changeRecorderMaster(dst_prop_set);
    changeRecorderMaster.setMaxChangesRecorded(10); //buffer should be large enough

    ChangeRecorder::CallBack callBackCRS1 = [&s1_up] (ChangeRecorder const* recorder,
                                                 ChangeRecorder::ChangeRecordInfo const& changeRecordInfos) {
        for (ChangeRecord const& changeRecord : changeRecordInfos.redo) {
            changeRecord.toStream(s1_up);
        }
    };

    changeRecorderSource1.registerCallback(callBackCRS1);

    ChangeRecorder::CallBack callBackCRS2 = [&s2_up] (ChangeRecorder const* recorder,
                                                 ChangeRecorder::ChangeRecordInfo const& changeRecordInfos) {
        for (ChangeRecord const& changeRecord : changeRecordInfos.redo) {
            changeRecord.toStream(s2_up);
        }
    };

    changeRecorderSource2.registerCallback(callBackCRS2);

    ChangeRecorder::CallBack callBackMaster = [&s1_down, &s2_down] (ChangeRecorder const* recorder,
                                                     ChangeRecorder::ChangeRecordInfo const& changeRecordInfos) {
        for (ChangeRecord const& changeRecord : changeRecordInfos.redo) {
            changeRecord.toStream(s1_down);
            changeRecord.toStream(s2_down);
        }
    };

    changeRecorderMaster.registerCallback(callBackMaster);

    auto pullDataToMaster = [&s1_up, &s2_up, dst_prop_set] () {
        for (StreamTransfertT* streamPtr : {&s1_up, &s2_up}) {
            StreamTransfertT& stream = *streamPtr;

            std::vector<ChangeRecord> changes;
            changes.reserve(10); //have a reasonable reserve to start

            do {
                ChangeRecord cr = ChangeRecord::fromStream(stream);

                if (!cr.isValid()) {
                    break;
                }

                changes.push_back(cr);
            } while (true);

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

                changes[i].apply(dst_prop_set);
            }
        }
    };

    auto pullDataToSource1 = [&s1_down, src1_prop_set] () {

        std::vector<ChangeRecord> changes;
        changes.reserve(10); //have a reasonable reserve to start

        do {
            ChangeRecord cr = ChangeRecord::fromStream(s1_down);

            if (!cr.isValid()) {
                break;
            }

            changes.push_back(cr);
        } while (true);

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

            changes[i].apply(src1_prop_set);
        }
    };

    auto pullDataToSource2 = [&s2_down, src2_prop_set] () {

        std::vector<ChangeRecord> changes;
        changes.reserve(10); //have a reasonable reserve to start

        do {
            ChangeRecord cr = ChangeRecord::fromStream(s2_down);

            if (!cr.isValid()) {
                break;
            }

            changes.push_back(cr);
        } while (true);

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

            changes[i].apply(src2_prop_set);
        }
    };


    std::string keySubBlock1 = "test";
    Internal::SubDataBlock1* b1 = new Internal::SubDataBlock1(keySubBlock1);
    source1.insertDataBlock(keySubBlock1, b1);

    b1->setIntProp(42);
    b1->setStringProp("test");

    std::string keySubBlock2 = "test2";
    Internal::SubDataBlock2* b2 = new Internal::SubDataBlock2(keySubBlock2);
    source2.insertDataBlock(keySubBlock2, b2);

    std::string keySubBlock3 = "test3";
    Internal::SubDataBlock2* b3 = new Internal::SubDataBlock2(keySubBlock3);
    source2.insertDataBlock(keySubBlock3, b3);

    b2->setLongProp(69);
    b3->setLongProp(3327);

    ASSERT_FALSE(source1.checkIsEquivalent(source2));

    pullDataToMaster();
    pullDataToSource1();
    pullDataToSource2();
    pullDataToMaster();
    pullDataToSource1();
    pullDataToSource2();

    str = s1_up.str(); pos = s1_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s2_up.str(); pos = s2_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s1_up.str(); pos = s1_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s2_down.str(); pos = s2_down.tellg();
    ASSERT_EQ(pos, str.size());

    s1_up.clear();
    s1_up.str("");
    s2_up.clear();
    s2_up.str("");
    s1_down.clear();
    s1_down.str("");
    s2_down.clear();
    s2_down.str("");

    ASSERT_EQ(source1.nDataBlocks(), 3);
    ASSERT_EQ(source2.nDataBlocks(), 3);
    ASSERT_EQ(master.nDataBlocks(), 3);

    Internal::SubDataBlock1* b1_inserted = dynamic_cast<Internal::SubDataBlock1*>(source2.getDataBlock(keySubBlock1));
    Internal::SubDataBlock2* b2_inserted = dynamic_cast<Internal::SubDataBlock2*>(source1.getDataBlock(keySubBlock2));
    Internal::SubDataBlock2* b3_inserted = dynamic_cast<Internal::SubDataBlock2*>(source1.getDataBlock(keySubBlock3));

    ASSERT_TRUE(b1_inserted != nullptr);
    ASSERT_TRUE(b2_inserted != nullptr);
    ASSERT_TRUE(b3_inserted != nullptr);

    ASSERT_EQ(b1->getIntProp(), 42);
    ASSERT_EQ(b1->getStringProp(), "test");
    ASSERT_EQ(b2->getFloatProp(), 69);
    ASSERT_EQ(b3->getFloatProp(), 3327);

    ASSERT_EQ(b2_inserted->getFloatProp(), 69);
    ASSERT_EQ(b3_inserted->getFloatProp(), 3327);

    ASSERT_TRUE(source1.checkIsEquivalent(source2));


    std::string keySubSubBlock1 = "subtest1";
    Internal::SubSubDataBlock* sb1 = new Internal::SubSubDataBlock(keySubSubBlock1);
    b3_inserted->insertDataBlock(keySubSubBlock1, sb1);

    sb1->setStringProp("subtest");
    sb1->setUIntProp(101);

    pullDataToMaster();
    pullDataToSource1();
    pullDataToSource2();
    pullDataToMaster();
    pullDataToSource1();
    pullDataToSource2();

    str = s1_up.str(); pos = s1_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s2_up.str(); pos = s2_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s1_up.str(); pos = s1_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s2_down.str(); pos = s2_down.tellg();
    ASSERT_EQ(pos, str.size());

    s1_up.clear();
    s1_up.str("");
    s2_up.clear();
    s2_up.str("");
    s1_down.clear();
    s1_down.str("");
    s2_down.clear();
    s2_down.str("");

    ASSERT_EQ(b3->nSubBlocks(), 1);
    ASSERT_EQ(b3_inserted->nSubBlocks(), 1);

    Internal::SubSubDataBlock* sb1_inserted = b3->getSubBlock(keySubSubBlock1);

    ASSERT_TRUE(sb1_inserted != nullptr);

    ASSERT_EQ(sb1_inserted->getUIntProp(), 101);
    ASSERT_EQ(sb1_inserted->getStringProp(), "subtest");

    ASSERT_TRUE(source1.checkIsEquivalent(source2));

    source2.removeDataBlock(keySubBlock1);
    source1.removeDataBlock(keySubBlock2);

    pullDataToMaster();
    pullDataToSource1();
    pullDataToSource2();
    pullDataToMaster();
    pullDataToSource1();
    pullDataToSource2();

    str = s1_up.str(); pos = s1_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s2_up.str(); pos = s2_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s1_up.str(); pos = s1_up.tellg();
    ASSERT_EQ(pos, str.size());

    str = s2_down.str(); pos = s2_down.tellg();
    ASSERT_EQ(pos, str.size());

    s1_up.clear();
    s1_up.str("");
    s2_up.clear();
    s2_up.str("");
    s1_down.clear();
    s1_down.str("");
    s2_down.clear();
    s2_down.str("");

    ASSERT_EQ(source1.nDataBlocks(), 1);
    ASSERT_EQ(source2.nDataBlocks(), 1);
    ASSERT_EQ(master.nDataBlocks(), 1);

    ASSERT_TRUE(source1.checkIsEquivalent(source2));

}
