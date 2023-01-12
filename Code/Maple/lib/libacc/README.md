libacc
-------------------------------------------------------------------------------
This project offers a header only acceleration structure library including
implementations for a BVH- and KD-Tree. Applications may include ray
intersection tests, closest surface point or nearest neighbor searches.

Requirements
-------------------------------------------------------------------------------
Compiler with C++11 support - parallel tree construction is implemented with
`std::thread` and `std::atomic`

License
-------------------------------------------------------------------------------
The software is licensed under the BSD 3-Clause license,
for more details see the LICENSE.txt file.
