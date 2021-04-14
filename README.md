# kaseklis
A text file indexer mostly for developers working with large code
repositories. Simple, no dependencies, no command line options, has been
tested on Linux and Windows + Cygwin. 

Index cannot be updated, just re-created. To make things faster, indexes
can be nested, so more interesting folders can be indexed more 
frequently. When searching, all nested indexes are visited.

Also, project includes invariances.txt, which is an attempt to formalize
various features and refer them into the code.

### Building
```
cd src && make
```

### Installing
Put kaseklis binary on the PATH.

### Indexing
- cd to folder you want to index
```
kaseklis index
```

### Retrieving
```
kaseklis get word_I_am_looking_for
```
