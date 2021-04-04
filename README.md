# kaseklis
A text file indexer mostly for developers working with large code
repositories. Simple, no dependencies, has been tested on Linux and 
Windows + Cygwin. 

Index cannot be updated, just re-created. To make things faster, indexes
can be nested, so more interesting folders can be indexed more 
frequently. When searching, all nested indexes are visited.

Also, project includes invariances.txt, which is an attempt to formalize
various features and refer them into the code.