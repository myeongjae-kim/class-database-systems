# Subject: Database Systems

This repository is a result of projects which I designed and implemented in Database Systems class at Hanyang University. I conducted five projects very well, but it was hard to solve last project. Last project is about logging and recovery. I learned the ARIES logging protocol, but implementing was difficult.

## Table of Contents

* [Project 1: SQL Practice](#project-1-sql-practice)
* [Project 2: Disk Based B\+Tree](#project-2-disk-based-btree)
* [Project 3: Buffer Management](#project-3-buffer-management)
* [Project 4: Join Table](#project-4-join-table)
* [Project 5: Transaction Logging &amp; Recovery](#project-5-transaction-logging--recovery)

## [Project 1: SQL Practice](#table-of-contents)

[Codes](https://github.com/hrzon/Class_DatabaseSystems/blob/master/project1.sql)

This project is solving thirty SQL problems.

## [Project 2: Disk Based B+Tree](#table-of-contents)

[Codes](https://github.com/hrzon/Class_DatabaseSystems/tree/master/project2)

Second project is building disk based B+Tree. Students are provided B+Tree which stores data at volatile memory. Building disk based B+Tree based on memory based B+Tree is the objective.

I developed the B+Tree using object-oriented pattern even though the language is C. It was a good experience because I implemented class in C language, but codes were generally longer than typical C codes.

Since every modification of B+Tree should be saved to disk, it is very slow. For speed up, buffer manager is needed. Next project is implementing buffer manager.

## [Project 3: Buffer Management](#table-of-contents)

[Wiki](https://github.com/hrzon/Class_DatabaseSystems/wiki/buffer_designdoc)

[Codes](https://github.com/hrzon/Class_DatabaseSystems/tree/master/project3)

Main memory is very fast, but disk is much slower than the memory. Key of speed up is reducing the number of disk I/O.

Every disk access should be made by buffer manager. The buffer manager stores some pages following caching policy. If a page is requested from index layer, buffer manager can provide a page without accessing disk when the buffer manager has the page.

Which page should be stored in the buffer manager? One of the most famous policies is LRU(Least Recently Used), and another famous one is MRU(Most Recently Used). In this project, LRU policy is used.

## [Project 4: Join Table](#table-of-contents)

[Wiki](https://github.com/hrzon/Class_DatabaseSystems/wiki/join_designdoc)

[Codes](https://github.com/hrzon/Class_DatabaseSystems/wiki/project4)

Join is a powerful tool for finding relationship between records. Many problem can be solved by join. There are several joins like Theta join, Equi join, and Natural join.

For the project, sort-merge join is selected which is a kind of natural join. Sort-merge is very fast when records are already sorted. In this project, only primary key will be used. It means that records are already sorted becuase it is B+Tree.

## [Project 5: Transaction Logging & Recovery](#table-of-contents)

[Wiki](https://github.com/hrzon/Class_DatabaseSystems/wiki/trx_designdoc)

[Codes](https://github.com/hrzon/Class_DatabaseSystems/tree/master/project5)

There are policies about writing pages to disk.

Steal: A dirty page can be written to disk even the page has a data of uncommitted transaction. (UNDO log is required)
No Steal: Only pages, which are not dirty, can be written to disk. (UNDO log is not required)

Force: Page should be written to disk at every update. (REDO log is not required)
No Force: Page is written only **when it needs to be**. (REDO log is required)

There are two cases of page-written situation in no force policy, and it is called Write-Ahead Logging(WAL).

1. About logs: If a data page tries to be written to disk, corresponding log pages should be written first.
2. About data: When a transaction tries to be finished, all logs pages should be written to disk first including "commit" log. 

We can make a table of policies.

|          | Steal                       | No Steal                          |
| -------- | --------------------------- | --------------------------------- |
| Force    | UNDO<br />No REDO           | No UNDO<br />No REDO<br />Slowest |
| No Force | UNDO<br />REDO<br />Fastest | No UNDO<br />REDO                 |

In this project, steal and no force policies will be used. Students implement undo and redo log. 

Recovery protocol is ARIES(*Algorithms for Recovery and Isolation Exploitting Semantics*). It guarantees finite recovering process even though crashes occur during recovering.