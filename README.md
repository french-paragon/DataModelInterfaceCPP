# Lib Data Model Interface

This library is meant to manage structured datasets (via hierachical key/values) in c++. It aims at offering the following functionalities:

- Support all important primitive types (integer and floating points numbers, and strings), and hierachical inclusion of datablocks.
- Callback mechanism to track and react to changes.
- Automatic undo/redo mechanism.
- Synchronization of datasets across arbitrary data streams.
- Depends only on the standard library.
- Mechanisms to expose arbitrary data classes as a Data model
- Mechanisms to accumulate changes before commiting them to a reference dataset later
