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
